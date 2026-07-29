#include "dashboardpage.h"
#include "ui_dashboardpage.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QPixmap>
#include <QSizePolicy>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>

#include "uihelpers.h"

namespace {

constexpr int kLowStockThreshold = 100;

QString formatMetricNumber(int value)
{
    return QLocale().toString(value);
}

QString formatCurrency(double value)
{
    return QStringLiteral("R %1").arg(QLocale().toString(value, 'f', 2));
}

QString batchStatusRole(int partialCount, int failedCount)
{
    if (failedCount > 0) {
        return QStringLiteral("fail");
    }
    if (partialCount > 0) {
        return QStringLiteral("warning");
    }
    return QStringLiteral("success");
}

QString batchStatusText(int partialCount, int failedCount)
{
    if (failedCount > 0 && partialCount > 0) {
        return QStringLiteral("Partial / Failed");
    }
    if (failedCount > 0) {
        return QStringLiteral("Failed");
    }
    if (partialCount > 0) {
        return QStringLiteral("Partial");
    }
    return QStringLiteral("Complete");
}

QString metricCardStyleSheet(const QString &theme)
{
    QString background;
    QString borderColor;
    QString labelColor;
    QString valueColor;

    if (theme == QStringLiteral("blue")) {
        background = QStringLiteral("qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #3C8BFF, stop:0.42 #256FE8, stop:1 #1855C9)");
        borderColor = QStringLiteral("rgba(255, 255, 255, 0.20)");
        labelColor = QStringLiteral("rgba(255, 255, 255, 0.82)");
        valueColor = QStringLiteral("#FFFFFF");
    } else if (theme == QStringLiteral("teal")) {
        background = QStringLiteral("qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #3FD2C0, stop:0.44 #1FAF9A, stop:1 #15867A)");
        borderColor = QStringLiteral("rgba(255, 255, 255, 0.18)");
        labelColor = QStringLiteral("rgba(255, 255, 255, 0.82)");
        valueColor = QStringLiteral("#FFFFFF");
    } else if (theme == QStringLiteral("amber")) {
        background = QStringLiteral("qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #FFCB73, stop:0.46 #F2A43A, stop:1 #D88318)");
        borderColor = QStringLiteral("rgba(255, 255, 255, 0.16)");
        labelColor = QStringLiteral("rgba(255, 255, 255, 0.80)");
        valueColor = QStringLiteral("#FFFFFF");
    } else {
        background = QStringLiteral("qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #F8FBFF, stop:0.52 #EEF3F9, stop:1 #E3E9F2)");
        borderColor = QStringLiteral("#D7E0EB");
        labelColor = QStringLiteral("#728097");
        valueColor = QStringLiteral("#122033");
    }

    return QStringLiteral(
               "QFrame {"
               "background: %1;"
               "border: 1px solid %2;"
               "border-radius: 20px;"
               "}"
               "QWidget#metricPatternWidget { background: transparent; }"
               "QLabel { background: transparent; border: none; }"
               "QLabel[role=\"kpiLabel\"] { color: %3; }"
               "QLabel[role=\"kpiValue\"] { color: %4; }")
        .arg(background, borderColor, labelColor, valueColor);
}

struct MetricCardConfig {
    QFrame *card;
    QLabel *titleLabel;
    QLabel *valueLabel;
    QString theme;
    QString patternResource;
};

struct AlphaContentStats {
    QRect bounds;
    QPointF centroid;
    qreal coverage = 0.0;
};

int alphaThresholdForTheme(const QString &theme)
{
    return theme == QStringLiteral("revenue") ? 10 : 20;
}

QColor overlayTintForTheme(const QString &theme)
{
    if (theme == QStringLiteral("blue")) {
        return QColor(QStringLiteral("#D7E7FF"));
    }
    if (theme == QStringLiteral("teal")) {
        return QColor(QStringLiteral("#D7FAF2"));
    }
    if (theme == QStringLiteral("amber")) {
        return QColor(QStringLiteral("#FFE7C1"));
    }
    if (theme == QStringLiteral("revenue")) {
        return QColor(QStringLiteral("#9AA8BA"));
    }
    return QColor(Qt::white);
}

QColor directPatternTintForTheme(const QString &theme)
{
    if (theme == QStringLiteral("blue")) {
        return QColor(QStringLiteral("#6EA9FF"));
    }
    if (theme == QStringLiteral("teal")) {
        return QColor(QStringLiteral("#56D9C8"));
    }
    if (theme == QStringLiteral("amber")) {
        return QColor(QStringLiteral("#FFC86D"));
    }
    return QColor(QStringLiteral("#C7D2E0"));
}

int directPatternTintAlphaForTheme(const QString &theme)
{
    if (theme == QStringLiteral("revenue")) {
        return 18;
    }
    return 28;
}

qreal alphaGammaForTheme(const QString &theme)
{
    return theme == QStringLiteral("revenue") ? 0.64 : 0.72;
}

qreal alphaBoostForTheme(const QString &theme)
{
    return theme == QStringLiteral("revenue") ? 2.35 : 1.85;
}

int maxOverlayAlphaForTheme(const QString &theme)
{
    return theme == QStringLiteral("revenue") ? 110 : 92;
}

AlphaContentStats analyzeAlphaContent(const QImage &image, int threshold)
{
    AlphaContentStats stats;
    if (image.isNull()) {
        return stats;
    }

    int minX = image.width();
    int minY = image.height();
    int maxX = -1;
    int maxY = -1;
    int coveredPixels = 0;
    qreal sumX = 0.0;
    qreal sumY = 0.0;
    qreal sumAlpha = 0.0;

    for (int y = 0; y < image.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const int alpha = qAlpha(line[x]);
            if (alpha < threshold) {
                continue;
            }

            minX = qMin(minX, x);
            minY = qMin(minY, y);
            maxX = qMax(maxX, x);
            maxY = qMax(maxY, y);
            ++coveredPixels;
            sumX += x * alpha;
            sumY += y * alpha;
            sumAlpha += alpha;
        }
    }

    if (maxX < minX || maxY < minY || sumAlpha <= 0.0) {
        return stats;
    }

    stats.bounds = QRect(QPoint(minX, minY), QPoint(maxX, maxY));
    stats.centroid = QPointF(sumX / sumAlpha, sumY / sumAlpha);
    stats.coverage = qreal(coveredPixels) / qreal(image.width() * image.height());
    return stats;
}

QRect expandedRect(const QRect &rect, const QSize &imageSize, int padX, int padY)
{
    return rect.adjusted(-padX, -padY, padX, padY)
        .intersected(QRect(QPoint(0, 0), imageSize));
}

QRect focusRectForTheme(const QImage &image, const QString &theme)
{
    const AlphaContentStats stats = analyzeAlphaContent(image, alphaThresholdForTheme(theme));
    if (stats.bounds.isEmpty()) {
        return QRect(QPoint(0, 0), image.size());
    }

    QRect focusRect = stats.bounds;
    const qreal preferredAspect = theme == QStringLiteral("revenue") ? 1.34 : 1.26;
    const qreal contentAspect = focusRect.width() / qreal(qMax(1, focusRect.height()));

    if (contentAspect > preferredAspect) {
        const int targetWidth = qMax(1, qRound(focusRect.height() * preferredAspect));
        const int maxLeft = focusRect.right() - targetWidth + 1;
        const int centeredLeft = qRound(stats.centroid.x()) - (targetWidth / 2);
        const int left = qBound(focusRect.left(), centeredLeft, maxLeft);
        focusRect.setLeft(left);
        focusRect.setWidth(targetWidth);
    } else if (contentAspect < preferredAspect) {
        const int targetHeight = qMax(1, qRound(focusRect.width() / preferredAspect));
        const int maxTop = focusRect.bottom() - targetHeight + 1;
        const int centeredTop = qRound(stats.centroid.y()) - (targetHeight / 2);
        const int top = qBound(focusRect.top(), centeredTop, maxTop);
        focusRect.setTop(top);
        focusRect.setHeight(targetHeight);
    }

    const int padX = qMax(12, qRound(focusRect.width() * 0.08));
    const int padY = qMax(12, qRound(focusRect.height() * 0.10));
    return expandedRect(focusRect, image.size(), padX, padY);
}

bool shouldUseTintedOverlay(const QImage &image, const QString &theme)
{
    const AlphaContentStats stats = analyzeAlphaContent(image, alphaThresholdForTheme(theme));
    if (stats.bounds.isEmpty()) {
        return false;
    }

    // Sparse line-art assets disappear on pale cards unless they are recolored at runtime.
    return stats.coverage < 0.04;
}

QPixmap makeDirectPatternPixmap(const QImage &image, const QString &theme)
{
    if (image.isNull()) {
        return {};
    }

    QImage directPattern = image.copy(focusRectForTheme(image, theme)).convertToFormat(QImage::Format_ARGB32);
    if (directPattern.isNull()) {
        return {};
    }

    QColor tint = directPatternTintForTheme(theme);
    tint.setAlpha(directPatternTintAlphaForTheme(theme));

    QPainter painter(&directPattern);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter.fillRect(directPattern.rect(), tint);
    painter.end();

    return QPixmap::fromImage(directPattern);
}

QPixmap makeTintedPatternPixmap(const QImage &image, const QString &theme)
{
    if (image.isNull()) {
        return {};
    }

    const QRect focusRect = focusRectForTheme(image, theme);
    const QImage source = image.copy(focusRect).convertToFormat(QImage::Format_ARGB32);
    if (source.isNull()) {
        return {};
    }

    const QColor tint = overlayTintForTheme(theme);
    const qreal gamma = alphaGammaForTheme(theme);
    const qreal boost = alphaBoostForTheme(theme);
    const int maxAlpha = maxOverlayAlphaForTheme(theme);

    QImage overlay(source.size(), QImage::Format_ARGB32);
    overlay.fill(Qt::transparent);

    for (int y = 0; y < source.height(); ++y) {
        const QRgb *sourceLine = reinterpret_cast<const QRgb *>(source.constScanLine(y));
        QRgb *overlayLine = reinterpret_cast<QRgb *>(overlay.scanLine(y));
        for (int x = 0; x < source.width(); ++x) {
            const int alpha = qAlpha(sourceLine[x]);
            if (alpha == 0) {
                overlayLine[x] = qRgba(0, 0, 0, 0);
                continue;
            }

            const qreal normalized = alpha / 255.0;
            const int boostedAlpha = qMin(maxAlpha, qRound(std::pow(normalized, gamma) * 255.0 * boost));
            overlayLine[x] = qRgba(tint.red(), tint.green(), tint.blue(), boostedAlpha);
        }
    }

    return QPixmap::fromImage(overlay);
}

struct MetricPatternAsset {
    QPixmap pixmap;
    qreal opacity = 1.0;
};

MetricPatternAsset loadMetricPatternAsset(const QString &patternResource, const QString &theme)
{
    const QImage image(patternResource);
    if (image.isNull()) {
        return {};
    }

    if (shouldUseTintedOverlay(image, theme)) {
        return {makeTintedPatternPixmap(image, theme), 1.0};
    }

    return {makeDirectPatternPixmap(image, theme), 0.96};
}

class MetricPatternWidget : public QWidget
{
public:
    explicit MetricPatternWidget(const QString &theme,
                                 const QString &patternResource,
                                 QWidget *parent = nullptr)
        : QWidget(parent)
        , m_patternAsset(loadMetricPatternAsset(patternResource, theme))
    {
        setObjectName(QStringLiteral("metricPatternWidget"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setFixedSize(112, 88);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const QRectF bounds = rect().adjusted(0, 6, -2, -6);
        if (bounds.isEmpty()) {
            return;
        }

        if (!m_patternAsset.pixmap.isNull()) {
            const QSize targetSize = m_patternAsset.pixmap.size().scaled(bounds.size().toSize(),
                                                                         Qt::KeepAspectRatio);
            QRect targetRect(QPoint(), targetSize);
            targetRect.moveRight(static_cast<int>(bounds.right()));
            targetRect.moveTop(qRound(bounds.center().y() - (targetRect.height() / 2.0)));

            painter.setOpacity(m_patternAsset.opacity);
            painter.drawPixmap(targetRect,
                               m_patternAsset.pixmap.scaled(targetSize,
                                                            Qt::KeepAspectRatio,
                                                            Qt::SmoothTransformation));
            return;
        }
    }

private:
    MetricPatternAsset m_patternAsset;
};

QWidget *makeFeedWidget(const QString &time,
                        const QString &warehouse,
                        const QString &message,
                        const QString &badgeText,
                        const QString &badgeRole,
                        QWidget *parent)
{
    auto *container = new QWidget(parent);
    container->setObjectName(QStringLiteral("feedRowWidget"));
    container->setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(6, 12, 6, 12);
    layout->setSpacing(12);

    auto *timeLabel = new QLabel(time, container);
    UiHelpers::setTextRole(timeLabel, "timelineMeta");

    auto *warehouseLabel = UiHelpers::makeBadge(warehouse, warehouse.toLower(), container);

    auto *messageLabel = new QLabel(message, container);
    messageLabel->setWordWrap(true);
    UiHelpers::setTextRole(messageLabel, "feedText");

    auto *badgeLabel = UiHelpers::makeBadge(badgeText, badgeRole, container);

    layout->addWidget(timeLabel);
    layout->addWidget(warehouseLabel);
    layout->addWidget(messageLabel, 1);
    layout->addWidget(badgeLabel);
    return container;
}

void setFeedRow(QTableWidget *table,
                int row,
                const QString &time,
                const QString &warehouse,
                const QString &message,
                const QString &badgeText,
                const QString &badgeRole)
{
    auto *item = new QTableWidgetItem;
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    table->setItem(row, 0, item);
    table->setCellWidget(row, 0, makeFeedWidget(time, warehouse, message, badgeText, badgeRole, table));
}

QHBoxLayout *addCardHeader(QGroupBox *box, const QString &title)
{
    box->setTitle(QString());

    auto *vbox = qobject_cast<QVBoxLayout *>(box->layout());

    auto *header = new QHBoxLayout;
    header->setContentsMargins(0, 0, 0, 4);
    header->setSpacing(8);

    auto *titleLabel = new QLabel(title, box);
    UiHelpers::setProperty(titleLabel, "role", "cardTitle");

    header->addWidget(titleLabel);
    header->addStretch();

    vbox->insertLayout(0, header);
    return header;
}

QFrame *makeSnapshotSeparator(QWidget *parent)
{
    auto *sep = new QFrame(parent);
    sep->setObjectName(QStringLiteral("snapshotSep"));
    sep->setFrameShape(QFrame::NoFrame);
    sep->setAttribute(Qt::WA_StyledBackground, true);
    sep->setFixedSize(1, 20);
    return sep;
}

} // namespace

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DashboardPage)
{
    ui->setupUi(this);
    setupUiDefaults();
}

DashboardPage::~DashboardPage()
{
    delete ui;
}

void DashboardPage::setupUiDefaults()
{
    UiHelpers::setPageLayout(this, QMargins(30, 28, 30, 28), 20);

    //Set a minimum height on the whole page so the content doesnt collapse
    //to nothing when the window gets small. The main window has a minimum
    //size of 1320x860 so this shouldnt be an issue in practice but it
    //prevents the activity feed from disappearing on smaller monitors.
    setMinimumHeight(600);

    UiHelpers::setProperty(ui->dashboardTitleLabel, "textRole", "pageTitle");
    ui->kpiTitle3->setText(tr("Low Stock (<= %1)").arg(kLowStockThreshold));
    ui->kpiTitle4->setText(tr("Captured Revenue"));
    ui->kpiTitle3->setToolTip(tr("Products at or below %1 units in the backend snapshot.").arg(kLowStockThreshold));
    ui->kpiTitle4->setToolTip(
        tr("Revenue from captured final_project_v2 analytics records after a completed batch."));
    ui->todayRevenueValueLabel->setToolTip(
        tr("Unavailable until a completed orders batch produces analytics records."));

    const QList<MetricCardConfig> metricCardConfigs = {
        {ui->kpiFrame1, ui->kpiTitle1, ui->totalProductsValueLabel,
         QStringLiteral("blue"), QStringLiteral(":/dashboard/metrics/total-products.png")},
        {ui->kpiFrame2, ui->kpiTitle2, ui->totalStockUnitsValueLabel,
         QStringLiteral("teal"), QStringLiteral(":/dashboard/metrics/total-stock-units.png")},
        {ui->kpiFrame3, ui->kpiTitle3, ui->lowStockItemsValueLabel,
         QStringLiteral("amber"), QStringLiteral(":/dashboard/metrics/low-stock-items.png")},
        {ui->kpiFrame4, ui->kpiTitle4, ui->todayRevenueValueLabel,
         QStringLiteral("revenue"), QStringLiteral(":/dashboard/metrics/today-revenue.png")}
    };

    // --- KPI cards (unchanged) ---
    const QList<QWidget *> metricCards = {ui->kpiFrame1, ui->kpiFrame2, ui->kpiFrame3, ui->kpiFrame4};
    for (QWidget *card : metricCards) {
        UiHelpers::setProperty(card, "role", "kpiCard");
        UiHelpers::tuneLayout(card, QMargins(16, 16, 16, 16), 6);
        UiHelpers::applyCardShadow(card, 18, 4, QColor(15, 23, 42, 14));
    }

    UiHelpers::setSurface(ui->kpiFrame1, "metric", "blue");
    UiHelpers::setSurface(ui->kpiFrame2, "metric", "teal");
    UiHelpers::setSurface(ui->kpiFrame3, "metric", "amber");
    UiHelpers::setSurface(ui->kpiFrame4, "metric", "slate");
    UiHelpers::setProperty(ui->kpiFrame1, "kpiTone", "blue");
    UiHelpers::setProperty(ui->kpiFrame2, "kpiTone", "indigo");
    UiHelpers::setProperty(ui->kpiFrame3, "kpiTone", "orange");
    UiHelpers::setProperty(ui->kpiFrame4, "kpiTone", "green");

    // --- Card surfaces, shadows, layout tuning, roles ---
    UiHelpers::setSurface(ui->warehouseStatusGroupBox, "card");
    UiHelpers::setSurface(ui->liveFeedGroupBox, "card");
    UiHelpers::setSurface(ui->healthGroupBox, "card");
    UiHelpers::applyCardShadow(ui->warehouseStatusGroupBox, 18, 4, QColor(15, 23, 42, 12));
    UiHelpers::applyCardShadow(ui->liveFeedGroupBox, 12, 3, QColor(15, 23, 42, 8));
    UiHelpers::applyCardShadow(ui->healthGroupBox, 14, 3, QColor(15, 23, 42, 9));

    UiHelpers::tuneLayout(ui->warehouseStatusGroupBox, QMargins(18, 14, 18, 18), 14);
    UiHelpers::tuneLayout(ui->liveFeedGroupBox, QMargins(18, 14, 18, 14), 10);
    UiHelpers::tuneLayout(ui->healthGroupBox, QMargins(18, 14, 18, 18), 14);

    UiHelpers::setProperty(ui->warehouseStatusGroupBox, "role", "dashboardCard");
    UiHelpers::setProperty(ui->liveFeedGroupBox, "role", "dashboardCard");
    UiHelpers::setProperty(ui->healthGroupBox, "role", "dashboardCard");
    
    auto *leftColumnFrame = new QFrame(ui->mainSplitter);
    leftColumnFrame->setObjectName(QStringLiteral("dashboardPrimaryColumnFrame"));
    leftColumnFrame->setFrameShape(QFrame::NoFrame);
    leftColumnFrame->setAttribute(Qt::WA_StyledBackground, true);
    leftColumnFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    auto *leftColumnLayout = new QVBoxLayout(leftColumnFrame);
    leftColumnLayout->setContentsMargins(0, 0, 0, 0);
    leftColumnLayout->setSpacing(20);

    if (auto *bottomLayout = ui->horizontalLayout_bottom) {
        bottomLayout->removeWidget(ui->healthGroupBox);
        bottomLayout->removeWidget(ui->snapshotGroupBox);
    }
    if (auto *rightColumnLayout = qobject_cast<QVBoxLayout *>(ui->dashboardSideColumnFrame->layout())) {
        rightColumnLayout->removeWidget(ui->runtimeStatusGroupBox);
        rightColumnLayout->setStretch(0, 1);
    }

    ui->mainSplitter->insertWidget(0, leftColumnFrame);
    leftColumnLayout->addWidget(ui->warehouseStatusGroupBox);
    leftColumnLayout->addWidget(ui->healthGroupBox);

    if (auto *pageLayout = qobject_cast<QVBoxLayout *>(layout())) {
        pageLayout->removeItem(ui->horizontalLayout_bottom);
    }

    ui->runtimeStatusGroupBox->hide();
    ui->runtimeStatusGroupBox->deleteLater();
    ui->snapshotGroupBox->hide();
    ui->snapshotGroupBox->deleteLater();

    ui->dashboardSideColumnFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    ui->liveFeedGroupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    ui->activityFeedTableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // ================================================================
    // Change Group 1 — Card headers with ellipsis menu buttons
    // ================================================================
    addCardHeader(ui->warehouseStatusGroupBox, QStringLiteral("Warehouse Batch Status"));

    // ================================================================
    // Change Group 2 — Dropdown filter combo for warehouse card
    // ================================================================
    addCardHeader(ui->liveFeedGroupBox, QStringLiteral("Completed Batch Activity"));
    addCardHeader(ui->healthGroupBox, QStringLiteral("Batch Signals"));

    // --- KPI labels ---
    const QList<QLabel *> metricTitles = {ui->kpiTitle1, ui->kpiTitle2, ui->kpiTitle3, ui->kpiTitle4};
    for (QLabel *label : metricTitles) {
        UiHelpers::setProperty(label, "textRole", "metricTitle");
        UiHelpers::setProperty(label, "role", "kpiLabel");
    }

    const QList<QLabel *> metricValues = {
        ui->totalProductsValueLabel,
        ui->totalStockUnitsValueLabel,
        ui->lowStockItemsValueLabel,
        ui->todayRevenueValueLabel
    };
    for (QLabel *label : metricValues) {
        UiHelpers::setProperty(label, "role", "kpiValue");
    }
    UiHelpers::setProperty(ui->todayRevenueValueLabel, "metricState", "placeholder");

    for (const MetricCardConfig &config : metricCardConfigs) {
        if (auto *cardLayout = qobject_cast<QVBoxLayout *>(config.card->layout())) {
            config.titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            cardLayout->removeWidget(config.titleLabel);

            auto *headerRow = new QHBoxLayout;
            headerRow->setContentsMargins(0, 0, 0, 0);
            headerRow->setSpacing(8);
            headerRow->addWidget(config.titleLabel, 0, Qt::AlignTop);
            headerRow->addStretch(1);
            headerRow->addWidget(new MetricPatternWidget(config.theme, config.patternResource, config.card),
                                 0, Qt::AlignTop | Qt::AlignRight);
            cardLayout->insertLayout(0, headerRow);
        }

        config.card->setStyleSheet(metricCardStyleSheet(config.theme));
        UiHelpers::repolishDeep(config.card);
    }

    // --- Warehouse table headers ---
    ui->warehouseHeaderFileLabel->setText(QStringLiteral("Orders CSV"));
    ui->warehouseHeaderFailLabel->setText(QStringLiteral("Partial/Fail"));
    const QList<QLabel *> headerLabels = {
        ui->warehouseHeaderNameLabel,
        ui->warehouseHeaderFileLabel,
        ui->warehouseHeaderStatusLabel,
        ui->warehouseHeaderProcessedLabel,
        ui->warehouseHeaderSuccessLabel,
        ui->warehouseHeaderFailLabel
    };
    for (QLabel *label : headerLabels) {
        UiHelpers::setProperty(label, "textRole", "tableHeader");
        UiHelpers::setProperty(label, "role", "dashboardTableHeader");
    }

    // ================================================================
    // Change Group 4 — File labels styled as chips
    // ================================================================
    const QList<QLabel *> fileLabels = {
        ui->warehouse1FileLabel,
        ui->warehouse2FileLabel,
        ui->warehouse3FileLabel,
        ui->warehouse4FileLabel,
        ui->warehouse5FileLabel
    };
    for (int i = 0; i < fileLabels.size(); ++i) {
        fileLabels[i]->setText(QStringLiteral("Warehouse%1_Orders.csv").arg(i + 1));
        UiHelpers::setProperty(fileLabels[i], "role", "fileChip");
    }

    // --- Health section labels ---
    ui->healthHelperLabel->setVisible(false); // Change Group 6: hide helper text
    ui->mutexHealthTitle->setText(QStringLiteral("Catalog"));
    ui->queueHealthTitle->setText(QStringLiteral("Orders Batch"));
    ui->threadHealthTitle->setText(QStringLiteral("Analytics Records"));
    ui->exportHealthTitle->setText(QStringLiteral("Lost Revenue"));
    UiHelpers::setProperty(ui->mutexHealthTitle, "textRole", "labelTitle");
    UiHelpers::setProperty(ui->queueHealthTitle, "textRole", "labelTitle");
    UiHelpers::setProperty(ui->threadHealthTitle, "textRole", "labelTitle");
    UiHelpers::setProperty(ui->exportHealthTitle, "textRole", "labelTitle");

    // --- KPI values ---
    ui->totalProductsValueLabel->setText(QStringLiteral("--"));
    ui->totalStockUnitsValueLabel->setText(QStringLiteral("--"));
    ui->lowStockItemsValueLabel->setText(QStringLiteral("--"));
    ui->todayRevenueValueLabel->setText(QStringLiteral("Unavailable"));

    // --- Activity feed ---
    UiHelpers::setupReadOnlyTable(ui->activityFeedTableWidget, 60, false);
    UiHelpers::setProperty(ui->activityFeedTableWidget, "role", "activityStream");
    ui->activityFeedTableWidget->viewport()->setObjectName(QStringLiteral("activityFeedViewport"));
    ui->activityFeedTableWidget->viewport()->setAttribute(Qt::WA_StyledBackground, true);
    ui->activityFeedTableWidget->viewport()->setAutoFillBackground(true);
    UiHelpers::polish(ui->activityFeedTableWidget->viewport());
    ui->activityFeedTableWidget->setColumnCount(1);
    ui->activityFeedTableWidget->setRowCount(1);
    ui->activityFeedTableWidget->horizontalHeader()->hide();
    ui->activityFeedTableWidget->verticalHeader()->hide();
    ui->activityFeedTableWidget->setShowGrid(false);
    ui->activityFeedTableWidget->setFocusPolicy(Qt::NoFocus);
    ui->activityFeedTableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->activityFeedTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    setActivityFeedUnavailable(QStringLiteral("No completed batch activity"),
                               QStringLiteral("Unavailable"),
                               QStringLiteral("idle"));

    // --- Splitter ---
    UiHelpers::softenSplitter(ui->mainSplitter);
    ui->mainSplitter->setSizes({600, 380});
    ui->mainSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->mainSplitter->setMinimumHeight(400);
    ui->mainSplitter->setStretchFactor(0, 7);
    ui->mainSplitter->setStretchFactor(1, 4);
    ui->gridLayout_wh->setHorizontalSpacing(12);
    ui->gridLayout_wh->setVerticalSpacing(12);

    // --- W1-W5 name badges ---
    UiHelpers::setBadge(ui->w1Label, "w1");
    UiHelpers::setBadge(ui->w2Label, "w2");
    UiHelpers::setBadge(ui->w3Label, "w3");
    UiHelpers::setBadge(ui->w4Label, "w4");
    UiHelpers::setBadge(ui->w5Label, "w5");

    // ================================================================
    // Change Group 3 — Status column with checkmark prefix
    // ================================================================
    UiHelpers::setBadgeText(ui->warehouse1StatusLabel, QStringLiteral("Not run"), QStringLiteral("idle"));
    UiHelpers::setBadgeText(ui->warehouse2StatusLabel, QStringLiteral("Not run"), QStringLiteral("idle"));
    UiHelpers::setBadgeText(ui->warehouse3StatusLabel, QStringLiteral("Not run"), QStringLiteral("idle"));
    UiHelpers::setBadgeText(ui->warehouse4StatusLabel, QStringLiteral("Not run"), QStringLiteral("idle"));
    UiHelpers::setBadgeText(ui->warehouse5StatusLabel, QStringLiteral("Not run"), QStringLiteral("idle"));

    // ================================================================
    // Change Group 4 — Processed column as progress pills
    // ================================================================
    const QList<QLabel *> processedLabels = {
        ui->warehouse1ProcessedLabel, ui->warehouse2ProcessedLabel,
        ui->warehouse3ProcessedLabel, ui->warehouse4ProcessedLabel,
        ui->warehouse5ProcessedLabel
    };
    const QStringList processedValues = {
        QStringLiteral("0"), QStringLiteral("0"), QStringLiteral("0"),
        QStringLiteral("0"), QStringLiteral("0")
    };
    for (int i = 0; i < processedLabels.size(); ++i) {
        processedLabels[i]->setText(processedValues[i]);
        processedLabels[i]->setAlignment(Qt::AlignCenter);
        UiHelpers::setProperty(processedLabels[i], "role", "processedPill");
    }
    UiHelpers::setProperty(ui->warehouse1ProcessedLabel, "processedActive", "false");

    // Success column: green dot + number
    const QList<QLabel *> successLabels = {
        ui->warehouse1SuccessLabel, ui->warehouse2SuccessLabel,
        ui->warehouse3SuccessLabel, ui->warehouse4SuccessLabel,
        ui->warehouse5SuccessLabel
    };
    const QStringList successValues = {
        QStringLiteral("0"), QStringLiteral("0"), QStringLiteral("0"),
        QStringLiteral("0"), QStringLiteral("0")
    };
    for (int i = 0; i < successLabels.size(); ++i) {
        successLabels[i]->setText(QStringLiteral("\u25CF  ") + successValues[i]);
        successLabels[i]->setAlignment(Qt::AlignCenter);
        UiHelpers::setProperty(successLabels[i], "role", "successCell");
    }

    // Fail column: red dot + number
    const QList<QLabel *> failLabels = {
        ui->warehouse1FailLabel, ui->warehouse2FailLabel,
        ui->warehouse3FailLabel, ui->warehouse4FailLabel,
        ui->warehouse5FailLabel
    };
    const QStringList failValues = {
        QStringLiteral("0"), QStringLiteral("0"), QStringLiteral("0"),
        QStringLiteral("0"), QStringLiteral("0")
    };
    for (int i = 0; i < failLabels.size(); ++i) {
        failLabels[i]->setText(QStringLiteral("\u25CF  ") + failValues[i]);
        failLabels[i]->setAlignment(Qt::AlignCenter);
        UiHelpers::setProperty(failLabels[i], "role", "failCell");
    }

    resetWarehouseRows(QStringLiteral("Not run"), QStringLiteral("idle"));

    // ================================================================
    // Change Group 3 — Runtime status with green circle prefix
    // ================================================================
    // ================================================================
    // Change Group 3 — Health section with green dot indicators
    // ================================================================
    for (auto *rowLayout : {ui->horizontalLayout_healthRow1, ui->horizontalLayout_healthRow2,
                            ui->horizontalLayout_healthRow3, ui->horizontalLayout_healthRow4}) {
        rowLayout->insertStretch(1);
    }

    UiHelpers::setBadgeText(ui->mutexHealthLabel, QStringLiteral("Pending"), QStringLiteral("idle"));
    UiHelpers::setBadgeText(ui->queueHealthLabel, QStringLiteral("Not run"), QStringLiteral("idle"));
    UiHelpers::setBadgeText(ui->threadHealthLabel, QStringLiteral("Unavailable"), QStringLiteral("idle"));
    UiHelpers::setBadgeText(ui->exportHealthLabel, QStringLiteral("Unavailable"), QStringLiteral("idle"));

    // ================================================================
    // Change Group 5 — Snapshot: horizontal compact layout
    // ================================================================
    // ================================================================
    // Change Group 6 — Alternating row tinting for warehouse grid
    // ================================================================
}

void DashboardPage::setInventoryMetrics(int products, int stock, int lowStock)
{
    ui->totalProductsValueLabel->setText(formatMetricNumber(products));
    ui->totalStockUnitsValueLabel->setText(formatMetricNumber(stock));
    ui->lowStockItemsValueLabel->setText(formatMetricNumber(lowStock));
    UiHelpers::setBadgeText(ui->mutexHealthLabel, QStringLiteral("Loaded"), QStringLiteral("success"));
    ui->mutexHealthLabel->setToolTip(tr("%1 products and %2 stock units loaded from the v2 catalog.")
                                         .arg(formatMetricNumber(products),
                                              formatMetricNumber(stock)));
}

void DashboardPage::setRevenueUnavailable(const QString &reason)
{
    ui->kpiTitle4->setText(tr("Captured Revenue"));
    UiHelpers::setProperty(ui->todayRevenueValueLabel, "metricState", "placeholder");
    ui->todayRevenueValueLabel->setText(QStringLiteral("Unavailable"));
    ui->todayRevenueValueLabel->setToolTip(reason);
    ui->kpiTitle4->setToolTip(reason);
    UiHelpers::setBadgeText(ui->exportHealthLabel, QStringLiteral("Unavailable"), QStringLiteral("idle"));
    ui->exportHealthLabel->setToolTip(reason);
}

void DashboardPage::setRevenueMetrics(double revenue, double lostRevenue, bool incompleteRecords)
{
    ui->kpiTitle4->setText(tr("Captured Revenue"));
    UiHelpers::setProperty(ui->todayRevenueValueLabel, "metricState", "live");
    ui->todayRevenueValueLabel->setText(formatCurrency(revenue));

    QString tooltip = tr("Captured revenue: %1\nCaptured lost revenue: %2")
                          .arg(formatCurrency(revenue),
                               formatCurrency(lostRevenue));
    if (incompleteRecords) {
        tooltip.append(tr("\nCaptured analytics records are fewer than expected line items."));
    }
    ui->kpiTitle4->setToolTip(tooltip);
    ui->todayRevenueValueLabel->setToolTip(tooltip);

    UiHelpers::setBadgeText(ui->exportHealthLabel,
                            formatCurrency(lostRevenue),
                            lostRevenue > 0.0 ? QStringLiteral("warning") : QStringLiteral("success"));
    ui->exportHealthLabel->setToolTip(tooltip);
}

void DashboardPage::setWarehouseRow(int index,
                                    const QString &statusText,
                                    const QString &statusRole,
                                    int processed,
                                    int success,
                                    int partialOrFailed)
{
    const QList<QLabel *> statusLabels = {
        ui->warehouse1StatusLabel, ui->warehouse2StatusLabel, ui->warehouse3StatusLabel,
        ui->warehouse4StatusLabel, ui->warehouse5StatusLabel
    };
    const QList<QLabel *> processedLabels = {
        ui->warehouse1ProcessedLabel, ui->warehouse2ProcessedLabel, ui->warehouse3ProcessedLabel,
        ui->warehouse4ProcessedLabel, ui->warehouse5ProcessedLabel
    };
    const QList<QLabel *> successLabels = {
        ui->warehouse1SuccessLabel, ui->warehouse2SuccessLabel, ui->warehouse3SuccessLabel,
        ui->warehouse4SuccessLabel, ui->warehouse5SuccessLabel
    };
    const QList<QLabel *> failLabels = {
        ui->warehouse1FailLabel, ui->warehouse2FailLabel, ui->warehouse3FailLabel,
        ui->warehouse4FailLabel, ui->warehouse5FailLabel
    };

    if (index < 0 || index >= statusLabels.size()) {
        return;
    }

    UiHelpers::setBadgeText(statusLabels[index], statusText, statusRole);
    processedLabels[index]->setText(formatMetricNumber(processed));
    UiHelpers::setProperty(processedLabels[index],
                           "processedActive",
                           processed > 0 ? QStringLiteral("true") : QStringLiteral("false"));
    successLabels[index]->setText(QStringLiteral("\u25CF  %1").arg(formatMetricNumber(success)));
    failLabels[index]->setText(QStringLiteral("\u25CF  %1").arg(formatMetricNumber(partialOrFailed)));
}

void DashboardPage::resetWarehouseRows(const QString &statusText, const QString &statusRole)
{
    for (int i = 0; i < 5; ++i) {
        setWarehouseRow(i, statusText, statusRole, 0, 0, 0);
    }
}

void DashboardPage::setActivityFeedUnavailable(const QString &message,
                                               const QString &badgeText,
                                               const QString &badgeRole)
{
    ui->activityFeedTableWidget->setRowCount(0);
    ui->activityFeedTableWidget->setRowCount(1);
    setFeedRow(ui->activityFeedTableWidget,
               0,
               QStringLiteral("Batch"),
               QStringLiteral("ALL"),
               message,
               badgeText,
               badgeRole);
}

void DashboardPage::setOrdersBatchRunning()
{
    setRevenueUnavailable(tr("Unavailable while the current orders batch is still running."));
    resetWarehouseRows(QStringLiteral("Running"), QStringLiteral("running"));
    setActivityFeedUnavailable(QStringLiteral("Orders batch is processing"),
                               QStringLiteral("Running"),
                               QStringLiteral("running"));
    UiHelpers::setBadgeText(ui->queueHealthLabel, QStringLiteral("Running"), QStringLiteral("running"));
    UiHelpers::setBadgeText(ui->threadHealthLabel, QStringLiteral("Pending"), QStringLiteral("idle"));
}

void DashboardPage::setOrdersBatchFailed(const QString &errorMessage)
{
    setRevenueUnavailable(tr("Unavailable because the latest orders batch failed."));
    resetWarehouseRows(QStringLiteral("Failed"), QStringLiteral("fail"));
    setActivityFeedUnavailable(errorMessage.trimmed().isEmpty()
                                   ? QStringLiteral("Orders batch failed")
                                   : errorMessage.trimmed(),
                               QStringLiteral("Failed"),
                               QStringLiteral("fail"));
    UiHelpers::setBadgeText(ui->queueHealthLabel, QStringLiteral("Failed"), QStringLiteral("fail"));
    UiHelpers::setBadgeText(ui->threadHealthLabel, QStringLiteral("Unavailable"), QStringLiteral("idle"));
}

void DashboardPage::setAnalyticsSnapshot(const AnalyticsSnapshotDto &snapshot)
{
    if (!snapshot.hasData) {
        setRevenueUnavailable(tr("Unavailable until a completed orders batch produces analytics records."));
        resetWarehouseRows(QStringLiteral("Not run"), QStringLiteral("idle"));
        setActivityFeedUnavailable(QStringLiteral("No completed batch activity"),
                                   QStringLiteral("Unavailable"),
                                   QStringLiteral("idle"));
        UiHelpers::setBadgeText(ui->queueHealthLabel, QStringLiteral("Not run"), QStringLiteral("idle"));
        UiHelpers::setBadgeText(ui->threadHealthLabel, QStringLiteral("Unavailable"), QStringLiteral("idle"));
        return;
    }

    if (snapshot.hasCapturedAnalytics) {
        setRevenueMetrics(snapshot.globalRevenue,
                          snapshot.globalLostRevenue,
                          snapshot.hasIncompleteItemAnalytics);
        UiHelpers::setBadgeText(ui->threadHealthLabel,
                                QStringLiteral("%1 / %2")
                                    .arg(formatMetricNumber(snapshot.capturedLineItems),
                                         formatMetricNumber(snapshot.expectedLineItems)),
                                snapshot.hasIncompleteItemAnalytics
                                    ? QStringLiteral("warning")
                                    : QStringLiteral("success"));
    } else {
        setRevenueUnavailable(tr("Unavailable because the completed batch did not produce analytics records."));
        UiHelpers::setBadgeText(ui->threadHealthLabel, QStringLiteral("Unavailable"), QStringLiteral("idle"));
    }

    const QString overallRole = batchStatusRole(snapshot.partialCount, snapshot.failedCount);
    UiHelpers::setBadgeText(ui->queueHealthLabel,
                            batchStatusText(snapshot.partialCount, snapshot.failedCount),
                            overallRole);
    ui->queueHealthLabel->setToolTip(
        tr("%1 orders: %2 success, %3 partial, %4 failed.")
            .arg(formatMetricNumber(snapshot.totalOrders),
                 formatMetricNumber(snapshot.successCount),
                 formatMetricNumber(snapshot.partialCount),
                 formatMetricNumber(snapshot.failedCount)));

    resetWarehouseRows(QStringLiteral("No orders"), QStringLiteral("idle"));
    for (const AnalyticsSnapshotDto::WarehouseStats &stats : snapshot.warehouseStats) {
        const int index = stats.warehouseId - 1;
        const int partialOrFailed = stats.partialCount + stats.failedCount;
        const QString role = stats.orderCount > 0
                                 ? batchStatusRole(stats.partialCount, stats.failedCount)
                                 : QStringLiteral("idle");
        const QString text = stats.orderCount > 0
                                 ? batchStatusText(stats.partialCount, stats.failedCount)
                                 : QStringLiteral("No orders");
        setWarehouseRow(index,
                        text,
                        role,
                        stats.orderCount,
                        stats.successCount,
                        partialOrFailed);
    }

    const int warehouseRows = qMin(4, snapshot.warehouseStats.size());
    ui->activityFeedTableWidget->setRowCount(0);
    ui->activityFeedTableWidget->setRowCount(1 + warehouseRows);
    setFeedRow(ui->activityFeedTableWidget,
               0,
               QStringLiteral("Batch"),
               QStringLiteral("ALL"),
               tr("%1 orders: %2 success, %3 partial, %4 failed")
                   .arg(formatMetricNumber(snapshot.totalOrders),
                        formatMetricNumber(snapshot.successCount),
                        formatMetricNumber(snapshot.partialCount),
                        formatMetricNumber(snapshot.failedCount)),
               batchStatusText(snapshot.partialCount, snapshot.failedCount),
               overallRole);

    for (int i = 0; i < warehouseRows; ++i) {
        const AnalyticsSnapshotDto::WarehouseStats &stats = snapshot.warehouseStats[i];
        const bool hasOrders = stats.orderCount > 0;
        const QString role = hasOrders
                                 ? batchStatusRole(stats.partialCount, stats.failedCount)
                                 : QStringLiteral("idle");
        const QString badgeText = hasOrders
                                      ? batchStatusText(stats.partialCount, stats.failedCount)
                                      : QStringLiteral("No orders");
        QString message = tr("%1 orders, %2 success, %3 partial/failed")
                              .arg(formatMetricNumber(stats.orderCount),
                                   formatMetricNumber(stats.successCount),
                                   formatMetricNumber(stats.partialCount + stats.failedCount));
        if (snapshot.hasCapturedAnalytics) {
            message.append(tr(", %1 captured units").arg(formatMetricNumber(stats.capturedUnits)));
        }
        setFeedRow(ui->activityFeedTableWidget,
                   i + 1,
                   QStringLiteral("Batch"),
                   QStringLiteral("W%1").arg(stats.warehouseId),
                   message,
                   badgeText,
                   role);
    }
}
