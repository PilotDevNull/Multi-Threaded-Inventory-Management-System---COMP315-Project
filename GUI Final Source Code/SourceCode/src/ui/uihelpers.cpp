#include "uihelpers.h"

#include <algorithm>

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QHash>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QRegularExpression>
#include <QSizePolicy>
#include <QSplitter>
#include <QStyle>
#include <QTableWidget>
#include <QVector>
#include <QWidget>

namespace {

constexpr int kProjectLookupDepth = 6;
constexpr int kMaxFuzzyProductIconDistance = 2;
constexpr int kMinSharedFuzzyPrefix = 4;

QString canonicalExistingPath(const QString &candidatePath)
{
    const QFileInfo info(candidatePath);
    if (!info.exists()) {
        return {};
    }

    if (info.isDir()) {
        const QString canonicalPath = QDir(candidatePath).canonicalPath();
        return canonicalPath.isEmpty() ? info.absoluteFilePath() : canonicalPath;
    }

    const QString canonicalPath = info.canonicalFilePath();
    return canonicalPath.isEmpty() ? info.absoluteFilePath() : canonicalPath;
}

QString resolveProjectPath(const QString &relativePath)
{
    QDir searchDir(QCoreApplication::applicationDirPath());
    for (int depth = 0; depth < kProjectLookupDepth; ++depth) {
        const QString candidatePath = canonicalExistingPath(searchDir.filePath(relativePath));
        if (!candidatePath.isEmpty()) {
            return candidatePath;
        }

        if (!searchDir.cdUp()) {
            break;
        }
    }

#ifdef NEXUS_SOURCE_DIR
    const QString sourceCandidate =
        canonicalExistingPath(QDir(QStringLiteral(NEXUS_SOURCE_DIR)).filePath(relativePath));
    if (!sourceCandidate.isEmpty()) {
        return sourceCandidate;
    }
#endif

    return {};
}

QString compactSnakeCaseKey(QString value)
{
    value.remove(QLatin1Char('_'));
    return value;
}

QStringList productIconStemCandidates(const QString &productName)
{
    QStringList candidates;
    const auto appendCandidate = [&candidates](const QString &candidate) {
        if (!candidate.isEmpty() && !candidates.contains(candidate)) {
            candidates.append(candidate);
        }
    };

    const QString snakeCaseName = UiHelpers::productNameToSnakeCase(productName);
    appendCandidate(snakeCaseName);

    static const QRegularExpression kContainsDigit(QStringLiteral("\\d"));
    QStringList parts = snakeCaseName.split(QLatin1Char('_'), Qt::SkipEmptyParts);
    while (!parts.isEmpty() && parts.constLast().contains(kContainsDigit)) {
        parts.removeLast();
        appendCandidate(parts.join(QLatin1Char('_')));
    }

    return candidates;
}

int boundedEditDistance(const QString &lhs, const QString &rhs, int maxDistance)
{
    if (lhs == rhs) {
        return 0;
    }

    if (std::abs(lhs.size() - rhs.size()) > maxDistance) {
        return maxDistance + 1;
    }

    QVector<int> previous(rhs.size() + 1);
    QVector<int> current(rhs.size() + 1);

    for (int column = 0; column <= rhs.size(); ++column) {
        previous[column] = column;
    }

    for (int row = 1; row <= lhs.size(); ++row) {
        current[0] = row;
        int rowMinimum = current[0];

        for (int column = 1; column <= rhs.size(); ++column) {
            const int substitutionCost = lhs.at(row - 1) == rhs.at(column - 1) ? 0 : 1;
            current[column] = std::min({previous[column] + 1,
                                        current[column - 1] + 1,
                                        previous[column - 1] + substitutionCost});
            rowMinimum = std::min(rowMinimum, current[column]);
        }

        if (rowMinimum > maxDistance) {
            return maxDistance + 1;
        }

        previous = current;
    }

    return previous[rhs.size()];
}

int sharedPrefixLength(const QString &lhs, const QString &rhs)
{
    const int limit = std::min(lhs.size(), rhs.size());
    int prefixLength = 0;
    while (prefixLength < limit && lhs.at(prefixLength) == rhs.at(prefixLength)) {
        ++prefixLength;
    }

    return prefixLength;
}

struct ProductIconEntry {
    QString stem;
    QString compactStem;
    QString path;
};

struct ProductIconIndex {
    QList<ProductIconEntry> entries;
    QHash<QString, QString> exactMatches;
};

ProductIconIndex buildProductIconIndex()
{
    ProductIconIndex index;

    const QString iconDirectoryPath = resolveProjectPath(QStringLiteral("assets/icons/products"));
    if (iconDirectoryPath.isEmpty()) {
        return index;
    }

    const QDir iconDirectory(iconDirectoryPath);
    const QStringList iconFiles =
        iconDirectory.entryList({QStringLiteral("*.png")}, QDir::Files, QDir::Name);

    for (const QString &iconFile : iconFiles) {
        const QString path = canonicalExistingPath(iconDirectory.filePath(iconFile));
        if (path.isEmpty()) {
            continue;
        }

        const QString stem = QFileInfo(iconFile).completeBaseName();
        index.entries.append({stem, compactSnakeCaseKey(stem), path});
        index.exactMatches.insert(stem, path);
    }

    return index;
}

const ProductIconIndex &productIconIndex()
{
    static const ProductIconIndex index = buildProductIconIndex();
    return index;
}

QString matchProductIconPath(const QString &candidateStem)
{
    const ProductIconIndex &index = productIconIndex();
    if (candidateStem.isEmpty() || index.entries.isEmpty()) {
        return {};
    }

    const auto exactMatch = index.exactMatches.constFind(candidateStem);
    if (exactMatch != index.exactMatches.constEnd()) {
        return exactMatch.value();
    }

    const QString compactCandidate = compactSnakeCaseKey(candidateStem);
    QString compactMatchPath;
    int compactMatchCount = 0;
    for (const ProductIconEntry &entry : index.entries) {
        if (entry.compactStem == compactCandidate) {
            compactMatchPath = entry.path;
            ++compactMatchCount;
            if (compactMatchCount > 1) {
                break;
            }
        }
    }

    if (compactMatchCount == 1) {
        return compactMatchPath;
    }

    int bestDistance = kMaxFuzzyProductIconDistance + 1;
    QString bestPath;
    bool ambiguous = false;
    for (const ProductIconEntry &entry : index.entries) {
        const int compactCandidateSize = static_cast<int>(compactCandidate.size());
        const int entryCompactSize = static_cast<int>(entry.compactStem.size());
        const int requiredSharedPrefix =
            std::min(kMinSharedFuzzyPrefix, std::min(compactCandidateSize, entryCompactSize));
        if (sharedPrefixLength(compactCandidate, entry.compactStem) < requiredSharedPrefix) {
            continue;
        }

        const int distance =
            boundedEditDistance(compactCandidate, entry.compactStem, kMaxFuzzyProductIconDistance);
        if (distance > kMaxFuzzyProductIconDistance) {
            continue;
        }

        if (distance < bestDistance) {
            bestDistance = distance;
            bestPath = entry.path;
            ambiguous = false;
            continue;
        }

        if (distance == bestDistance && entry.path != bestPath) {
            ambiguous = true;
        }
    }

    return (!ambiguous && bestDistance <= kMaxFuzzyProductIconDistance) ? bestPath : QString();
}

QFont semiboldFont()
{
    QFont font = QApplication::font();
    font.setWeight(QFont::DemiBold);
    return font;
}

QWidget *makePillWidget(const QString &text,
                       const QString &role,
                       QTableWidget *table,
                       Qt::Alignment alignment = Qt::AlignCenter)
{
    auto *container = new QWidget(table);
    container->setObjectName(QStringLiteral("tablePillContainer"));
    container->setAttribute(Qt::WA_StyledBackground, false);

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(6);
    layout->setAlignment(alignment);

    auto *dot = new QFrame(container);
    dot->setObjectName(QStringLiteral("badgeDot"));
    dot->setProperty("badgeRole", role);
    dot->setFixedSize(8, 8);

    auto *label = new QLabel(text, container);
    label->setAlignment(Qt::AlignCenter);
    label->setFont(semiboldFont());
    label->setProperty("badgeRole", role);
    label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    label->setAttribute(Qt::WA_StyledBackground, true);

    layout->addWidget(dot);
    layout->addWidget(label);
    return container;
}

QTableWidgetItem *makeItem(const QString &text,
                           Qt::Alignment alignment,
                           const QColor &foreground = QColor())
{
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(alignment);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    if (foreground.isValid()) {
        item->setForeground(foreground);
    }
    return item;
}

} // namespace

namespace UiHelpers {

void polish(QWidget *widget)
{
    if (!widget) {
        return;
    }

    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

void repolishDeep(QWidget *widget)
{
    if (!widget) {
        return;
    }

    polish(widget);

    const auto children = widget->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children) {
        repolishDeep(child);
    }
}

void setProperty(QWidget *widget, const char *name, const QVariant &value)
{
    if (!widget) {
        return;
    }

    widget->setProperty(name, value);
    polish(widget);
}

void setTextRole(QWidget *widget, const QString &role)
{
    setProperty(widget, "textRole", role);
}

void setSurface(QWidget *widget, const QString &surface, const QString &accent)
{
    if (!widget) {
        return;
    }

    widget->setProperty("surface", surface);
    widget->setProperty("accent", accent);
    polish(widget);
}

void setButtonRole(QPushButton *button, const QString &role)
{
    if (!button) {
        return;
    }

    button->setProperty("buttonRole", role);
    polish(button);
}

void setBadge(QLabel *label, const QString &role)
{
    setBadgeText(label, label ? label->text() : QString(), role);
}

void setBadgeText(QLabel *label, const QString &text, const QString &role)
{
    if (!label) {
        return;
    }

    label->setText(text);
    label->setAlignment(Qt::AlignCenter);
    label->setFont(semiboldFont());
    label->setProperty("badgeRole", role);
    polish(label);
}

QLabel *makeBadge(const QString &text, const QString &role, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    setBadge(label, role);
    return label;
}

QLabel *makeIcon(const QString &text, const QString &color, int fontSize, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("iconLabel"));
    label->setAlignment(Qt::AlignCenter);
    label->setFixedSize(fontSize + 6, fontSize + 6);
    label->setStyleSheet(
        QStringLiteral("color: %1; font-size: %2px; background: transparent; border: none; padding: 0;")
            .arg(color)
            .arg(fontSize));
    return label;
}

void setStandardIcon(QAbstractButton *button, QStyle::StandardPixmap standardPixmap, const QSize &size)
{
    if (!button) {
        return;
    }

    button->setIcon(button->style()->standardIcon(standardPixmap));
    button->setIconSize(size);
}

void tuneLayout(QWidget *widget, const QMargins &margins, int spacing)
{
    if (!widget || !widget->layout()) {
        return;
    }

    widget->layout()->setContentsMargins(margins);
    if (spacing >= 0) {
        widget->layout()->setSpacing(spacing);
    }
}

void setPageLayout(QWidget *page, const QMargins &margins, int spacing)
{
    if (!page || !page->layout()) {
        return;
    }

    page->setAttribute(Qt::WA_StyledBackground, true);
    page->layout()->setContentsMargins(margins);
    page->layout()->setSpacing(spacing);
}

void applyCardShadow(QWidget *widget, int blurRadius, int verticalOffset, const QColor &color)
{
    if (!widget) {
        return;
    }

    //QGraphicsDropShadowEffect causes ghost artifacts on resize/unfullscreen
    //on Windows (and sometimes Linux too). The shadow gets painted at the old
    //geometry and doesnt clear until a full repaint cycle catches up.
    //Instead of a real shadow effect we just set a subtle border via stylesheet
    //to give cards some visual definition without any repaint issues.
    Q_UNUSED(blurRadius);
    Q_UNUSED(verticalOffset);
    Q_UNUSED(color);

    widget->setAttribute(Qt::WA_StyledBackground, true);

    //Remove any existing graphics effect that might have been set previously
    if (widget->graphicsEffect()) {
        widget->setGraphicsEffect(nullptr);
    }
}

QString productNameToSnakeCase(const QString &productName)
{
    QString snakeCaseName = productName.trimmed().toLower();
    snakeCaseName.replace(QRegularExpression(QStringLiteral("[\\s\\-]+")), QStringLiteral("_"));
    snakeCaseName.remove(QRegularExpression(QStringLiteral("[^a-z0-9_]+")));
    snakeCaseName.replace(QRegularExpression(QStringLiteral("_+")), QStringLiteral("_"));
    snakeCaseName.remove(QRegularExpression(QStringLiteral("^_+|_+$")));
    return snakeCaseName;
}

QString resolveProductIconPath(const QString &productName)
{
    const QString cacheKey = productNameToSnakeCase(productName);
    if (cacheKey.isEmpty()) {
        return {};
    }

    static QHash<QString, QString> pathCache;
    const auto cachedPath = pathCache.constFind(cacheKey);
    if (cachedPath != pathCache.constEnd()) {
        return cachedPath.value();
    }

    QString resolvedPath;
    const QStringList candidates = productIconStemCandidates(productName);
    for (const QString &candidate : candidates) {
        resolvedPath = matchProductIconPath(candidate);
        if (!resolvedPath.isEmpty()) {
            break;
        }
    }

    pathCache.insert(cacheKey, resolvedPath);
    return resolvedPath;
}

QIcon productIconForName(const QString &productName)
{
    const QString cacheKey = productNameToSnakeCase(productName);
    if (cacheKey.isEmpty()) {
        return {};
    }

    static QHash<QString, QIcon> iconCache;
    const auto cachedIcon = iconCache.constFind(cacheKey);
    if (cachedIcon != iconCache.constEnd()) {
        return cachedIcon.value();
    }

    const QString iconPath = resolveProductIconPath(productName);
    const QIcon icon = iconPath.isEmpty() ? QIcon() : QIcon(iconPath);
    iconCache.insert(cacheKey, icon);
    return icon;
}

void setupTable(QTableWidget *table, int rowHeight, bool alternating)
{
    if (!table) {
        return;
    }

    table->setAttribute(Qt::WA_StyledBackground, true);
    table->setAlternatingRowColors(alternating);
    table->setShowGrid(false);
    table->setFrameShape(QFrame::NoFrame);
    table->setFocusPolicy(Qt::NoFocus);
    table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setWordWrap(false);
    table->setMouseTracking(true);
    table->setIconSize(QSize(18, 18));

    table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(rowHeight);
    table->horizontalHeader()->setHighlightSections(false);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->horizontalHeader()->setMinimumSectionSize(42);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setFixedHeight(40);
}

void setupReadOnlyTable(QTableWidget *table, int rowHeight, bool alternating)
{
    setupTable(table, rowHeight, alternating);
    if (!table) {
        return;
    }

    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void setTableText(QTableWidget *table,
                  int row,
                  int column,
                  const QString &text,
                  Qt::Alignment alignment,
                  const QColor &foreground)
{
    if (!table) {
        return;
    }

    table->setItem(row, column, makeItem(text, alignment, foreground));
}

void setTableBadge(QTableWidget *table, int row, int column, const QString &text, const QString &role)
{
    setTableBadgeAligned(table, row, column, text, role, Qt::AlignCenter);
}

void setTableBadgeAligned(QTableWidget *table,
                          int row,
                          int column,
                          const QString &text,
                          const QString &role,
                          Qt::Alignment alignment)
{
    if (!table) {
        return;
    }

    auto *item = makeItem(QString(), alignment);
    item->setData(Qt::UserRole, text);
    table->setItem(row, column, item);
    table->setCellWidget(row, column, makePillWidget(text, role, table, alignment));
}

void setTableChip(QTableWidget *table, int row, int column, const QString &text, const QString &role)
{
    setTableChipAligned(table, row, column, text, role, Qt::AlignCenter);
}

void setTableChipAligned(QTableWidget *table,
                         int row,
                         int column,
                         const QString &text,
                         const QString &role,
                         Qt::Alignment alignment)
{
    setTableBadgeAligned(table, row, column, text, role, alignment);
}

void softenSplitter(QSplitter *splitter)
{
    if (!splitter) {
        return;
    }

    splitter->setHandleWidth(10);
    splitter->setChildrenCollapsible(false);
}

} // namespace UiHelpers
