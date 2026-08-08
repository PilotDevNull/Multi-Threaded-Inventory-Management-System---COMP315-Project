#include "inventorypage.h"
#include "ui_inventorypage.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QCheckBox>
#include <QRegularExpression>
#include <QInputDialog>
#include <algorithm>

#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFrame>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLayout>
#include <QLabel>
#include <QLocale>
#include <QMetaObject>
#include <QResizeEvent>
#include <QShowEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWheelEvent>
#include <QtGlobal>
#include <QVBoxLayout>

#include "uihelpers.h"

namespace {

constexpr int kLowStockThreshold = 100;
constexpr int kBasePriceRole = Qt::UserRole;
constexpr int kFinalPriceRole = Qt::UserRole + 1;

QString formatCurrency(double value)
{
    return QStringLiteral("R %1").arg(QLocale().toString(value, 'f', 2));
}

QString formatWholeNumber(int value)
{
    return QLocale().toString(value);
}

QTableWidgetItem *makeReadOnlyTableItem(const QString &text,
                                        Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter,
                                        const QIcon &icon = QIcon())
{
    auto *item = icon.isNull() ? new QTableWidgetItem(text) : new QTableWidgetItem(icon, text);
    item->setTextAlignment(alignment);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

QString typeRole(const QString &type)
{
    if (type == QLatin1String("Taxable")) {
        return QStringLiteral("taxed");
    }
    if (type == QLatin1String("Discounted")) {
        return QStringLiteral("discounted");
    }
    return QStringLiteral("standard");
}

QString editorTypeForBackendType(const QString &type)
{
    if (type == QLatin1String("Taxable") || type == QLatin1String("Discounted")) {
        return type;
    }

    return QStringLiteral("Standard");
}

QString stockBadgeRole(int quantity)
{
    if (quantity <= kLowStockThreshold) {
        return QStringLiteral("warning");
    }
    if (quantity <= 340) {
        return QStringLiteral("info");
    }
    return QStringLiteral("success");
}

QString stockTone(int quantity)
{
    if (quantity <= kLowStockThreshold) {
        return QStringLiteral("low");
    }
    if (quantity <= 340) {
        return QStringLiteral("medium");
    }
    return QStringLiteral("high");
}

QString inventoryRowBand(int row)
{
    return (row % 2 == 0) ? QStringLiteral("odd") : QStringLiteral("even");
}

QWidget *makeProductCellWidget(const QString &productId,
                               const QString &rowBand,
                               QTableWidget *table)
{
    auto *container = new QWidget(table);
    container->setObjectName(QStringLiteral("productCellWidget"));
    container->setAttribute(Qt::WA_StyledBackground, true);
    container->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    container->setProperty("rowBand", rowBand);
    container->setProperty("selected", false);

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(0);

    auto *idLabel = new QLabel(productId, container);
    idLabel->setObjectName(QStringLiteral("productCellIdLabel"));
    idLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    layout->addWidget(idLabel);
    layout->addStretch();
    return container;
}

QWidget *makeQuantityWidget(int quantity,
                            int maximum,
                            const QString &rowBand,
                            QTableWidget *table)
{
    auto *container = new QWidget(table);
    container->setObjectName(QStringLiteral("stockCell"));
    container->setAttribute(Qt::WA_StyledBackground, true);
    container->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    container->setProperty("rowBand", rowBand);
    container->setProperty("selected", false);

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(10);

    auto *valueLabel = new QLabel(formatWholeNumber(quantity), container);
    valueLabel->setObjectName(QStringLiteral("stockValueLabel"));
    valueLabel->setProperty("stockTone", stockTone(quantity));
    valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    valueLabel->setMinimumWidth(34);
    valueLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    auto *track = new QFrame(container);
    track->setObjectName(QStringLiteral("stockBarTrack"));
    track->setFixedHeight(10);
    track->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    auto *trackLayout = new QHBoxLayout(track);
    trackLayout->setContentsMargins(0, 0, 0, 0);
    trackLayout->setSpacing(0);

    auto *fill = new QFrame(track);
    fill->setObjectName(QStringLiteral("stockBarFill"));
    fill->setProperty("stockTone", stockTone(quantity));
    fill->setMinimumHeight(10);
    fill->setMaximumHeight(10);
    fill->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    const int percent = qBound(10, static_cast<int>((static_cast<double>(quantity) / maximum) * 100.0), 100);
    trackLayout->addWidget(fill, percent);
    trackLayout->addStretch(100 - percent);

    layout->addWidget(valueLabel, 0, Qt::AlignVCenter);
    layout->addWidget(track, 1, Qt::AlignVCenter);
    return container;
}

void populateInventoryRow(QTableWidget *table,
                          int row,
                          const QString &productId,
                          const QString &name,
                          double basePrice,
                          double finalPrice,
                          int quantity,
                          const QString &type,
                          const QString &category,
                          int maximumQuantity)
{
    const QString rowBand = inventoryRowBand(row);
    auto *productItem = makeReadOnlyTableItem(productId);
    productItem->setData(Qt::UserRole, productId);
    table->setItem(row, 0, productItem);
    table->setCellWidget(row, 0, makeProductCellWidget(productId, rowBand, table));

    const QIcon productIcon = UiHelpers::productIconForName(name);
    table->setItem(row,
                   1,
                   makeReadOnlyTableItem(name, Qt::AlignLeft | Qt::AlignVCenter, productIcon));
    UiHelpers::setTableText(table, row, 2, formatCurrency(basePrice), Qt::AlignRight | Qt::AlignVCenter);
    if (QTableWidgetItem *priceItem = table->item(row, 2)) {
        priceItem->setData(kBasePriceRole, basePrice);
        priceItem->setData(kFinalPriceRole, finalPrice);
    }

    auto *item = new QTableWidgetItem;
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setData(Qt::UserRole, quantity);
    table->setItem(row, 3, item);
    table->setCellWidget(row, 3, makeQuantityWidget(quantity, maximumQuantity, rowBand, table));

    UiHelpers::setTableChip(table, row, 4, type, typeRole(type));
    UiHelpers::setTableText(table, row, 5, category);
}

void populateImportPreviewRow(QTableWidget *table,
                              int row,
                              const QString &productId,
                              const QString &name,
                              double basePrice,
                              int quantity,
                              const QString &type,
                              const QString &category)
{
    UiHelpers::setTableText(table, row, 0, productId);

    const QIcon productIcon = UiHelpers::productIconForName(name);
    table->setItem(row,
                   1,
                   makeReadOnlyTableItem(name, Qt::AlignLeft | Qt::AlignVCenter, productIcon));
    UiHelpers::setTableText(table, row, 2, formatCurrency(basePrice), Qt::AlignRight | Qt::AlignVCenter);
    UiHelpers::setTableText(table, row, 3, formatWholeNumber(quantity),
                            Qt::AlignLeft | Qt::AlignVCenter);
    UiHelpers::setTableChip(table, row, 4, type, typeRole(type));
    UiHelpers::setTableText(table, row, 5, category);
}

int verticalWheelDelta(const QWheelEvent *event)
{
    if (!event) {
        return 0;
    }

    const QPoint angleDelta = event->angleDelta();
    if (angleDelta.y() != 0 && qAbs(angleDelta.y()) >= qAbs(angleDelta.x())) {
        return angleDelta.y();
    }

    const QPoint pixelDelta = event->pixelDelta();
    if (pixelDelta.y() != 0 && qAbs(pixelDelta.y()) >= qAbs(pixelDelta.x())) {
        return pixelDelta.y();
    }

    return 0;
}

QAbstractScrollArea *scrollAreaForObject(QObject *watched)
{
    if (auto *scrollArea = qobject_cast<QAbstractScrollArea *>(watched)) {
        return scrollArea;
    }

    auto *widget = qobject_cast<QWidget *>(watched);
    return widget ? qobject_cast<QAbstractScrollArea *>(widget->parentWidget()) : nullptr;
}

bool canScrollVertically(QAbstractScrollArea *scrollArea, int deltaY)
{
    if (!scrollArea || deltaY == 0) {
        return false;
    }

    QScrollBar *scrollBar = scrollArea->verticalScrollBar();
    if (!scrollBar || !scrollBar->isEnabled() || scrollBar->maximum() <= scrollBar->minimum()) {
        return false;
    }

    if (deltaY > 0) {
        return scrollBar->value() > scrollBar->minimum();
    }

    return scrollBar->value() < scrollBar->maximum();
}

QPointF wheelGlobalPosition(const QWheelEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->globalPosition();
#else
    return event->globalPosF();
#endif
}

void appendTransferredItem(QVBoxLayout *targetLayout, QLayoutItem *item)
{
    if (!targetLayout || !item) {
        delete item;
        return;
    }

    if (QWidget *widget = item->widget()) {
        targetLayout->addWidget(widget);
        delete item;
        return;
    }

    if (QLayout *layout = item->layout()) {
        targetLayout->addLayout(layout);
        delete item;
        return;
    }

    if (QSpacerItem *spacer = item->spacerItem()) {
        targetLayout->addSpacerItem(spacer);
        return;
    }

    delete item;
}

bool forwardWheelToPageScrollArea(QWidget *page, QWheelEvent *event)
{
    auto *scrollArea =
        page ? page->findChild<QScrollArea *>(QStringLiteral("inventoryPageScrollArea"))
             : nullptr;
    if (!scrollArea) {
        return false;
    }

    const int deltaY = verticalWheelDelta(event);
    if (!canScrollVertically(scrollArea, deltaY)) {
        return false;
    }

    QScrollBar *pageScrollBar = scrollArea->verticalScrollBar();
    const int beforeValue = pageScrollBar ? pageScrollBar->value() : 0;
    const QPointF globalPos = wheelGlobalPosition(event);
    const QPointF viewportPos(scrollArea->viewport()->mapFromGlobal(globalPos.toPoint()));
    QWheelEvent forwardedEvent(viewportPos,
                               globalPos,
                               event->pixelDelta(),
                               event->angleDelta(),
                               event->buttons(),
                               event->modifiers(),
                               event->phase(),
                               event->inverted(),
                               event->source());

    QCoreApplication::sendEvent(scrollArea->viewport(), &forwardedEvent);

    return forwardedEvent.isAccepted()
           || (pageScrollBar && pageScrollBar->value() != beforeValue);
}

void refreshWidgetStyle(QWidget *widget)
{
    if (!widget) {
        return;
    }

    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

void setRowSelectionState(QTableWidget *table, int row, bool selected)
{
    if (!table || row < 0 || row >= table->rowCount()) {
        return;
    }

    for (const int column : {0, 3}) {
        if (QWidget *cellWidget = table->cellWidget(row, column)) {
            cellWidget->setProperty("selected", selected);
            refreshWidgetStyle(cellWidget);
        }
    }
}

QFrame *wrapEditorSection(QVBoxLayout *editorLayout,
                          QWidget *parent,
                          const QString &frameName,
                          const QList<QWidget *> &widgets,
                          int spacing = 8)
{
    if (!editorLayout || !parent || widgets.isEmpty()) {
        return nullptr;
    }

    int insertIndex = -1;
    for (QWidget *widget : widgets) {
        if (!widget) {
            continue;
        }

        const int widgetIndex = editorLayout->indexOf(widget);
        if (widgetIndex >= 0 && (insertIndex < 0 || widgetIndex < insertIndex)) {
            insertIndex = widgetIndex;
        }
    }

    if (insertIndex < 0) {
        return nullptr;
    }

    auto *sectionFrame = new QFrame(parent);
    sectionFrame->setObjectName(frameName);
    sectionFrame->setFrameShape(QFrame::NoFrame);
    sectionFrame->setAttribute(Qt::WA_StyledBackground, false);

    auto *sectionLayout = new QVBoxLayout(sectionFrame);
    sectionLayout->setContentsMargins(0, 0, 0, 0);
    sectionLayout->setSpacing(spacing);

    for (QWidget *widget : widgets) {
        if (!widget) {
            continue;
        }

        editorLayout->removeWidget(widget);
        sectionLayout->addWidget(widget);
    }

    editorLayout->insertWidget(insertIndex, sectionFrame);
    return sectionFrame;
}

void makePageScrollable(QWidget *page,
                        QWidget *fixedBottomWidget,
                        const QMargins &margins,
                        int spacing)
{
    auto *outerLayout = qobject_cast<QVBoxLayout *>(page ? page->layout() : nullptr);
    if (!outerLayout || outerLayout->count() == 0) {
        return;
    }

    QLayoutItem *fixedBottomItem = nullptr;
    for (int index = 0; index < outerLayout->count(); ++index) {
        QLayoutItem *item = outerLayout->itemAt(index);
        if (item && item->widget() == fixedBottomWidget) {
            fixedBottomItem = outerLayout->takeAt(index);
            break;
        }
    }

    auto *scrollArea = new QScrollArea(page);
    scrollArea->setObjectName(QStringLiteral("inventoryPageScrollArea"));
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->viewport()->setObjectName(QStringLiteral("inventoryPageScrollViewport"));
    scrollArea->viewport()->setAttribute(Qt::WA_StyledBackground, false);

    auto *scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName(QStringLiteral("inventoryPageScrollContent"));
    scrollContent->setAttribute(Qt::WA_StyledBackground, false);
    scrollContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(margins);
    scrollLayout->setSpacing(spacing);

    while (outerLayout->count() > 0) {
        appendTransferredItem(scrollLayout, outerLayout->takeAt(0));
    }

    scrollArea->setWidget(scrollContent);

    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    outerLayout->addWidget(scrollArea);
    if (fixedBottomItem) {
        appendTransferredItem(outerLayout, fixedBottomItem);
    }
}

void refreshScrollableContentSize(QWidget *page)
{
    auto *scrollContent =
        page ? page->findChild<QWidget *>(QStringLiteral("inventoryPageScrollContent"))
             : nullptr;
    auto *scrollArea =
        page ? page->findChild<QScrollArea *>(QStringLiteral("inventoryPageScrollArea"))
             : nullptr;
    if (!scrollContent || !scrollContent->layout() || !scrollArea) {
        return;
    }

    scrollContent->layout()->activate();
    scrollContent->layout()->invalidate();
    scrollContent->updateGeometry();
    scrollArea->updateGeometry();
}

} // namespace

InventoryPage::InventoryPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::InventoryPage)
{
    ui->setupUi(this);
    setupUiDefaults();
}

InventoryPage::~InventoryPage()
{
    delete ui;
}

void InventoryPage::applyReadOnlyMode()
{
    const QString writeDisabledTooltip =
        tr("Write-back to final_project_v2 remains disabled in this protected integration slice.");
    const QString browseDisabledTooltip =
        tr("CSV browse, preview, and import stay disabled while the page reflects the protected backend snapshot.");
    const QString filterDisabledTooltip =
        tr("Search, sort, and low-stock filters are not wired yet for the protected backend snapshot.");

    ui->inventorySearchLineEdit->setEnabled(false);
    ui->sortComboBox->setEnabled(false);
    ui->filterButton->setEnabled(false);
    ui->lowStockOnlyCheckBox->setEnabled(false);
    ui->addProductButton->setEnabled(false);
    ui->updateProductButton->setEnabled(false);
    ui->removeProductButton->setEnabled(false);
    ui->browseCsvButton->setEnabled(false);
    ui->previewCsvButton->setEnabled(false);
    ui->importCsvButton->setEnabled(false);
    ui->inventoryPagePrevButton->hide();
    ui->inventoryPage1Button->hide();
    ui->inventoryPage2Button->hide();
    ui->inventoryPage3Button->hide();
    ui->inventoryPageNextButton->hide();

    ui->productIdSpinBox->setReadOnly(true);
    ui->productNameLineEdit->setReadOnly(true);
    ui->productPriceDoubleSpinBox->setReadOnly(true);
    ui->productQuantitySpinBox->setReadOnly(true);
    ui->discountThresholdSpinBox->setReadOnly(true);
    ui->productTypeComboBox->setEnabled(false);
    ui->taxRateComboBox->setEnabled(false);
    ui->typeBaseRadioButton->setEnabled(false);
    ui->typeTaxedRadioButton->setEnabled(false);
    ui->typeDiscountedRadioButton->setEnabled(false);

    ui->inventorySearchLineEdit->setToolTip(filterDisabledTooltip);
    ui->sortComboBox->setToolTip(filterDisabledTooltip);
    ui->filterButton->setToolTip(filterDisabledTooltip);
    ui->lowStockOnlyCheckBox->setToolTip(filterDisabledTooltip);
    ui->addProductButton->setToolTip(writeDisabledTooltip);
    ui->updateProductButton->setToolTip(writeDisabledTooltip);
    ui->removeProductButton->setToolTip(writeDisabledTooltip);
    ui->browseCsvButton->setToolTip(browseDisabledTooltip);
    ui->previewCsvButton->setToolTip(browseDisabledTooltip);
    ui->importCsvButton->setToolTip(browseDisabledTooltip);
}

bool InventoryPage::eventFilter(QObject *watched, QEvent *event)
{
    if (event && event->type() == QEvent::Wheel) {
        auto *wheelEvent = static_cast<QWheelEvent *>(event);
        auto *scrollArea = scrollAreaForObject(watched);
        if ((scrollArea == ui->inventoryTableWidget || scrollArea == ui->importPreviewTableWidget)
            && !canScrollVertically(scrollArea, verticalWheelDelta(wheelEvent))
            && forwardWheelToPageScrollArea(this, wheelEvent)) {
            wheelEvent->accept();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void InventoryPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    refreshScrollableContentSize(this);
}

void InventoryPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshScrollableContentSize(this);
    QMetaObject::invokeMethod(this, [this]() { refreshScrollableContentSize(this); },
                              Qt::QueuedConnection);
}

void InventoryPage::setupUiDefaults()
{
    m_backendReadOnlyMode = false;

    makePageScrollable(this, ui->statusFrame, QMargins(28, 24, 28, 24), 18);
    UiHelpers::setPageLayout(this, QMargins(0, 0, 0, 0), 0);
    UiHelpers::setProperty(ui->inventoryTitleLabel, "textRole", "pageTitle");
    ui->inventoryTitleLabel->setText(tr("Inventory Management"));
    ui->inventoryTableTitleLabel->setText(tr("Product Catalogue"));
    ui->productEditorTitleLabel->setText(tr("Selected Product"));
    ui->importTitleLabel->setText(tr("Source Catalogue (CSV)"));
    ui->importSummaryTitleLabel->setText(tr("Import Preview"));
    ui->productPriceLabel->setText(tr("Base Price (R)"));
    ui->productTypeLabel->setText(tr("Backend Type"));
    ui->taxRateLabel->setText(tr("Rate Detail"));
    ui->discountThresholdLabel->setText(tr("Discount Detail"));
    ui->mutexStatusLabelTitle->setText(tr("Access Mode"));

    ui->controlsFrame->setAttribute(Qt::WA_StyledBackground, true);
    UiHelpers::setSurface(ui->inventoryTableGroupBox, "card");
    UiHelpers::setSurface(ui->editorGroupBox, "card");
    UiHelpers::setSurface(ui->importSummaryGroupBox, "card");
    UiHelpers::setSurface(ui->editorNotesFrame, "soft");
    UiHelpers::setSurface(ui->importGroupBox, "card");
    UiHelpers::setSurface(ui->statusFrame, "card");
    UiHelpers::applyCardShadow(ui->inventoryTableGroupBox, 26, 8, QColor(24, 39, 62, 20));
    UiHelpers::applyCardShadow(ui->editorGroupBox, 24, 8, QColor(24, 39, 62, 18));
    UiHelpers::applyCardShadow(ui->editorNotesFrame, 18, 5, QColor(24, 39, 62, 12));
    UiHelpers::applyCardShadow(ui->importGroupBox, 24, 8, QColor(24, 39, 62, 18));
    UiHelpers::applyCardShadow(ui->importSummaryGroupBox, 22, 8, QColor(24, 39, 62, 16));

    UiHelpers::tuneLayout(ui->controlsFrame, QMargins(0, 0, 0, 0), 14);
    UiHelpers::tuneLayout(ui->inventoryTableGroupBox, QMargins(18, 16, 18, 18), 14);
    UiHelpers::tuneLayout(ui->editorGroupBox, QMargins(18, 18, 18, 24), 12);
    UiHelpers::tuneLayout(ui->editorNotesFrame, QMargins(12, 10, 12, 10), 8);
    UiHelpers::tuneLayout(ui->importGroupBox, QMargins(18, 16, 18, 16), 12);
    UiHelpers::tuneLayout(ui->importSummaryGroupBox, QMargins(16, 16, 16, 16), 12);
    UiHelpers::tuneLayout(ui->statusFrame, QMargins(30, 12, 30, 12), 10);
    UiHelpers::tuneLayout(ui->bottomSectionFrame, QMargins(0, 0, 0, 0), 18);

    UiHelpers::setProperty(ui->inventoryTableTitleLabel, "textRole", "cardTitle");
    UiHelpers::setProperty(ui->productEditorTitleLabel, "textRole", "cardTitle");
    UiHelpers::setProperty(ui->importTitleLabel, "textRole", "cardTitle");
    UiHelpers::setProperty(ui->importSummaryTitleLabel, "textRole", "cardTitle");

    const QList<QLabel *> formLabels = {
        ui->sortLabel,
        ui->productIdLabel,
        ui->productNameLabel,
        ui->productPriceLabel,
        ui->productQuantityLabel,
        ui->productTypeLabel,
        ui->taxRateLabel,
        ui->discountThresholdLabel,
        ui->discountThresholdUnitsLabel,
        ui->inventoryStatusLabelTitle,
        ui->mutexStatusLabelTitle
    };
    for (QLabel *label : formLabels) {
        UiHelpers::setProperty(label, "textRole", "labelTitle");
    }
    UiHelpers::setProperty(ui->discountThresholdUnitsLabel, "textRole", "pageMeta");

    UiHelpers::setProperty(ui->inventoryTableSummaryLabel, "textRole", "muted");
    UiHelpers::setProperty(ui->inventorySelectionHintLabel, "textRole", "muted");
    UiHelpers::setProperty(ui->editorHelperLabel, "textRole", "muted");
    UiHelpers::setProperty(ui->inventoryFooterLabel, "textRole", "muted");
    UiHelpers::setProperty(ui->importHelperLabel, "textRole", "muted");
    UiHelpers::setProperty(ui->importSummaryRowsLabel, "textRole", "muted");
    UiHelpers::setProperty(ui->importSummaryFileLabel, "textRole", "muted");
    UiHelpers::setProperty(ui->importSummaryHintLabel, "textRole", "muted");

    ui->inventoryTableSummaryLabel->setWordWrap(true);
    ui->inventorySelectionHintLabel->setWordWrap(true);
    ui->inventoryFooterLabel->setWordWrap(true);
    ui->importSummaryRowsLabel->setWordWrap(true);

    ui->inventorySearchLineEdit->setPlaceholderText(tr("Search by name, ID or category…"));
    ui->inventorySearchLineEdit->setClearButtonEnabled(false);
    ui->sortComboBox->addItems({"ProductID", "Name", "Quantity", "Price", "Type", "Category"});
    ui->productTypeComboBox->addItems({"Standard", "Taxable", "Discounted"});
    ui->taxRateComboBox->addItem(tr("Not surfaced in snapshot"));
    ui->productPriceDoubleSpinBox->setPrefix("R ");
    ui->typeBaseRadioButton->setText(tr("Standard"));
    ui->typeTaxedRadioButton->setText(tr("Taxable"));
    ui->discountThresholdUnitsLabel->hide();

    for (QAbstractSpinBox *field : {static_cast<QAbstractSpinBox *>(ui->productIdSpinBox),
                                    static_cast<QAbstractSpinBox *>(ui->productPriceDoubleSpinBox),
                                    static_cast<QAbstractSpinBox *>(ui->productQuantitySpinBox),
                                    static_cast<QAbstractSpinBox *>(ui->discountThresholdSpinBox)}) {
        field->setButtonSymbols(QAbstractSpinBox::NoButtons);
        field->setMinimumHeight(40);
    }

    ui->productNameLineEdit->setMinimumHeight(40);
    ui->taxRateComboBox->setMinimumHeight(40);
    ui->csvPathLineEdit->setMinimumHeight(42);
    ui->browseCsvButton->setMinimumHeight(42);
    ui->previewCsvButton->setMinimumHeight(42);
    ui->importCsvButton->setMinimumHeight(42);
    ui->inventorySearchLineEdit->setMinimumHeight(42);
    ui->filterButton->setMinimumHeight(42);
    ui->sortComboBox->setMinimumHeight(42);
    ui->inventoryTableWidget->setMinimumHeight(200);
    ui->inventoryTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->importPreviewTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    ui->productPriceFieldFrame->setAttribute(Qt::WA_StyledBackground, false);
    ui->productQuantityFieldFrame->setAttribute(Qt::WA_StyledBackground, false);
    ui->editorPriceQuantityFrame->setAttribute(Qt::WA_StyledBackground, false);
    ui->productTypeButtonsFrame->setAttribute(Qt::WA_StyledBackground, false);
    ui->discountThresholdFrame->setAttribute(Qt::WA_StyledBackground, false);
    ui->bottomSectionFrame->setAttribute(Qt::WA_StyledBackground, false);
    QFrame *productTypeSectionFrame =
        wrapEditorSection(ui->verticalLayout_editor,
                          ui->editorGroupBox,
                          QStringLiteral("productTypeSectionFrame"),
                          {ui->productTypeLabel, ui->productTypeButtonsFrame, ui->productTypeComboBox});
    QFrame *discountSectionFrame =
        wrapEditorSection(ui->verticalLayout_editor,
                          ui->editorGroupBox,
                          QStringLiteral("discountSectionFrame"),
                          {ui->discountThresholdLabel, ui->discountThresholdFrame});
    QFrame *taxRateSectionFrame =
        wrapEditorSection(ui->verticalLayout_editor,
                          ui->editorGroupBox,
                          QStringLiteral("taxRateSectionFrame"),
                          {ui->taxRateLabel, ui->taxRateComboBox});
    ui->verticalLayout_editor->setSpacing(12);
    ui->horizontalLayout_priceQuantity->setContentsMargins(0, 4, 0, 8);
    ui->horizontalLayout_priceQuantity->setSpacing(14);
    ui->horizontalLayout_typeButtons->setContentsMargins(0, 2, 0, 4);
    ui->horizontalLayout_discountThreshold->setContentsMargins(0, 4, 0, 4);
    ui->editorGroupBox->setMinimumHeight(0);
    ui->productPriceFieldFrame->setMinimumHeight(112);
    ui->productQuantityFieldFrame->setMinimumHeight(112);
    ui->editorPriceQuantityFrame->setMinimumHeight(134);
    ui->productTypeButtonsFrame->setMinimumHeight(42);
    ui->discountThresholdFrame->setMinimumHeight(82);
    ui->mainSplitter->setMinimumHeight(0);
    if (productTypeSectionFrame) {
        if (auto *layout = qobject_cast<QVBoxLayout *>(productTypeSectionFrame->layout())) {
            layout->setContentsMargins(0, 6, 0, 0);
            layout->setSpacing(10);
        }
        const int productTypeIndex = ui->verticalLayout_editor->indexOf(productTypeSectionFrame);
        if (productTypeIndex >= 0) {
            ui->verticalLayout_editor->insertSpacing(productTypeIndex, 14);
        }
        productTypeSectionFrame->setMinimumHeight(92);
    }
    if (discountSectionFrame) {
        if (auto *layout = qobject_cast<QVBoxLayout *>(discountSectionFrame->layout())) {
            layout->setSpacing(10);
        }
        discountSectionFrame->setMinimumHeight(118);
    }
    if (taxRateSectionFrame) {
        if (auto *layout = qobject_cast<QVBoxLayout *>(taxRateSectionFrame->layout())) {
            layout->setSpacing(10);
        }
        taxRateSectionFrame->setMinimumHeight(90);
    }
    ui->verticalLayout_tableCard->insertSpacing(3, 24);

    UiHelpers::setButtonRole(ui->filterButton, "secondary");
    UiHelpers::setButtonRole(ui->addProductButton, "primary");
    UiHelpers::setButtonRole(ui->updateProductButton, "secondary");
    UiHelpers::setButtonRole(ui->removeProductButton, "secondary");
    UiHelpers::setButtonRole(ui->browseCsvButton, "secondary");
    UiHelpers::setButtonRole(ui->previewCsvButton, "secondary");
    UiHelpers::setButtonRole(ui->importCsvButton, "success");
    UiHelpers::setButtonRole(ui->inventoryPagePrevButton, "page");
    UiHelpers::setButtonRole(ui->inventoryPage1Button, "pageActive");
    UiHelpers::setButtonRole(ui->inventoryPage2Button, "page");
    UiHelpers::setButtonRole(ui->inventoryPage3Button, "page");
    UiHelpers::setButtonRole(ui->inventoryPageNextButton, "page");

    ui->filterButton->setIcon(QIcon());
    ui->removeProductButton->setIcon(QIcon());
    ui->browseCsvButton->setIcon(QIcon());
    ui->previewCsvButton->setIcon(QIcon());
    ui->importCsvButton->setIcon(QIcon());

    UiHelpers::setupReadOnlyTable(ui->inventoryTableWidget, 56);
    ui->inventoryTableWidget->setIconSize(QSize(20, 20));
    ui->inventoryTableWidget->viewport()->setObjectName(QStringLiteral("inventoryTableViewport"));
    ui->inventoryTableWidget->viewport()->setAttribute(Qt::WA_StyledBackground, true);
    ui->inventoryTableWidget->installEventFilter(this);
    ui->inventoryTableWidget->viewport()->installEventFilter(this);
    ui->inventoryTableWidget->setColumnCount(6);
    ui->inventoryTableWidget->setHorizontalHeaderLabels({"ProductID", "Name", "Base Price", "Quantity", "Type", "Category"});
    ui->inventoryTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->inventoryTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->inventoryTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->inventoryTableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->inventoryTableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->inventoryTableWidget->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    ui->inventoryTableWidget->setColumnWidth(0, 92);
    ui->inventoryTableWidget->setColumnWidth(3, 190);
    ui->inventoryTableWidget->setColumnWidth(4, 140);
    ui->inventoryTableWidget->setRowCount(0);

    UiHelpers::setupReadOnlyTable(ui->importPreviewTableWidget, 42, false);
    ui->importPreviewTableWidget->setIconSize(QSize(18, 18));
    // Re-apply height cap after setupReadOnlyTable so default-section-size recalculation
    // cannot override the intended fixed-height constraint from the .ui file.
    ui->importPreviewTableWidget->setMinimumHeight(240);
    ui->importPreviewTableWidget->setMaximumHeight(240);
    ui->importPreviewTableWidget->viewport()->setObjectName(QStringLiteral("importPreviewViewport"));
    ui->importPreviewTableWidget->viewport()->setAttribute(Qt::WA_StyledBackground, true);
    ui->importPreviewTableWidget->installEventFilter(this);
    ui->importPreviewTableWidget->viewport()->installEventFilter(this);
    ui->importPreviewTableWidget->setColumnCount(6);
    ui->importPreviewTableWidget->setHorizontalHeaderLabels({"ProductID", "Name", "Base Price", "Quantity", "Type", "Category"});
    ui->importPreviewTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->importPreviewTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->importPreviewTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->importPreviewTableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->importPreviewTableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui->importPreviewTableWidget->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    ui->importPreviewTableWidget->setRowCount(0);

    ui->inventoryTableSummaryLabel->setText(
        tr("Waiting for the protected backend catalogue to load from final_project_v2/Catalog/Products.csv."));
    ui->inventorySelectionHintLabel->setText(
        tr("Select a backend row to inspect immutable base price, final price, category, and stock."));
    ui->inventoryFooterLabel->setText(tr("No backend products loaded"));
    ui->csvPathLineEdit->clear();
    ui->csvPathLineEdit->setPlaceholderText(
        tr("final_project_v2%1Catalog%1Products.csv").arg(QDir::separator()));
    ui->editorHelperLabel->setText(
        tr("Select a backend row to inspect immutable base price, final price, category, and stock values."));
    ui->editorHelperLabel->show();
    ui->importHelperLabel->setText(
        tr("The preview below reflects the protected backend catalogue, including category and product type. Import remains disabled in this slice."));
    ui->importSummaryRowsLabel->setText(tr("No preview rows loaded"));
    ui->importSummaryFileLabel->setText(tr("Waiting for backend catalogue path"));
    UiHelpers::setBadgeText(ui->importSummaryStatusLabel, tr("Awaiting snapshot"), "idle");
    UiHelpers::setBadgeText(ui->editorStockAlertLabel, tr("Stock detail unavailable"), "idle");
    UiHelpers::setBadgeText(ui->editorSyncLabel, tr("Final price unavailable"), "idle");
    ui->importSummaryHintLabel->setText(
        tr("Tax, discount, and category data are owned by the v2 backend. This page shows immutable snapshot values only."));

    ui->inventoryStatusLabel->setText(tr("Waiting for backend snapshot"));
    ui->mutexStatusLabel->setText(tr("Read-only snapshot"));
    UiHelpers::setBadge(ui->inventoryStatusLabel, "idle");
    UiHelpers::setBadge(ui->mutexStatusLabel, "idle");
    ui->statusFrame->setAttribute(Qt::WA_StyledBackground, true);
    ui->statusFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ui->statusFrame->setMinimumHeight(56);
    ui->statusFrame->setMaximumHeight(56);

    UiHelpers::softenSplitter(ui->mainSplitter);
    ui->horizontalLayout_bottomSection->setStretch(0, 1);
    ui->horizontalLayout_bottomSection->setStretch(1, 0);
    ui->mainSplitter->setSizes({700, 380});

    const auto syncTypeControls = [this]() {
        const QString currentType = ui->productTypeComboBox->currentText();
        const bool isStandard = currentType == QLatin1String("Standard");
        const bool isTaxable = currentType == QLatin1String("Taxable");
        const bool isDiscounted = currentType == QLatin1String("Discounted");

        ui->typeBaseRadioButton->setChecked(isStandard);
        ui->typeTaxedRadioButton->setChecked(isTaxable);
        ui->typeDiscountedRadioButton->setChecked(isDiscounted);
        ui->taxRateComboBox->setEnabled(false);
        ui->discountThresholdSpinBox->setEnabled(false);

        if (m_backendReadOnlyMode) {
            ui->productTypeComboBox->setEnabled(false);
            ui->typeBaseRadioButton->setEnabled(false);
            ui->typeTaxedRadioButton->setEnabled(false);
            ui->typeDiscountedRadioButton->setEnabled(false);
        }
    };

    connect(ui->typeBaseRadioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->productTypeComboBox->setCurrentText("Standard");
        }
    });
    connect(ui->typeTaxedRadioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->productTypeComboBox->setCurrentText("Taxable");
        }
    });
    connect(ui->typeDiscountedRadioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->productTypeComboBox->setCurrentText("Discounted");
        }
    });
    connect(ui->productTypeComboBox, &QComboBox::currentTextChanged, this,
            [syncTypeControls](const QString &) { syncTypeControls(); });

    ui->productIdSpinBox->setSpecialValueText(QString());
    ui->productQuantitySpinBox->setSpecialValueText(QString());
    ui->productPriceDoubleSpinBox->setSpecialValueText(QString());
    ui->discountThresholdSpinBox->setSpecialValueText(QStringLiteral("n/a"));

    const auto setEditorSelectionState = [this](bool selected) {
        ui->productIdSpinBox->setProperty("rowSelected", selected);
        ui->productQuantitySpinBox->setProperty("rowSelected", selected);
        refreshWidgetStyle(ui->productIdSpinBox);
        refreshWidgetStyle(ui->productQuantitySpinBox);
    };

    const auto clearEditor = [this, syncTypeControls, setEditorSelectionState]() {
        const QSignalBlocker blockerId(ui->productIdSpinBox);
        const QSignalBlocker blockerName(ui->productNameLineEdit);
        const QSignalBlocker blockerPrice(ui->productPriceDoubleSpinBox);
        const QSignalBlocker blockerQuantity(ui->productQuantitySpinBox);
        const QSignalBlocker blockerType(ui->productTypeComboBox);
        const QSignalBlocker blockerThreshold(ui->discountThresholdSpinBox);
        const QSignalBlocker blockerTax(ui->taxRateComboBox);

        ui->productIdSpinBox->setValue(ui->productIdSpinBox->minimum());
        ui->productIdSpinBox->clear();
        ui->productNameLineEdit->clear();
        ui->productPriceDoubleSpinBox->setValue(ui->productPriceDoubleSpinBox->minimum());
        ui->productPriceDoubleSpinBox->clear();
        ui->productQuantitySpinBox->setValue(ui->productQuantitySpinBox->minimum());
        ui->productQuantitySpinBox->clear();
        ui->productTypeComboBox->setCurrentText(QStringLiteral("Standard"));
        ui->discountThresholdSpinBox->setValue(ui->discountThresholdSpinBox->minimum());
        ui->taxRateComboBox->setCurrentIndex(0);

        syncTypeControls();
        setEditorSelectionState(false);
        if (m_backendReadOnlyMode) {
            ui->editorHelperLabel->setText(
                QStringLiteral("Select a backend row to inspect immutable base price, final price, category, and stock values."));
        } else {
            ui->editorHelperLabel->setText(QStringLiteral("Select a product row to load it into the editor."));
        }
        ui->editorHelperLabel->show();
        UiHelpers::setBadgeText(ui->editorStockAlertLabel, tr("Stock detail unavailable"), "idle");
        UiHelpers::setBadgeText(ui->editorSyncLabel, tr("Final price unavailable"), "idle");
        ui->updateProductButton->setEnabled(false);
        ui->removeProductButton->setEnabled(false);
    };

    const auto syncEditorFromSelection = [this, syncTypeControls, setEditorSelectionState]() {
        QTableWidgetItem *currentItem = ui->inventoryTableWidget->currentItem();
        const int currentRow = currentItem ? currentItem->row() : -1;

        for (int row = 0; row < ui->inventoryTableWidget->rowCount(); ++row) {
            setRowSelectionState(ui->inventoryTableWidget, row, row == currentRow);
        }

        if (currentRow < 0) {
            return;
        }

        const QSignalBlocker blockerId(ui->productIdSpinBox);
        const QSignalBlocker blockerName(ui->productNameLineEdit);
        const QSignalBlocker blockerPrice(ui->productPriceDoubleSpinBox);
        const QSignalBlocker blockerQuantity(ui->productQuantitySpinBox);
        const QSignalBlocker blockerType(ui->productTypeComboBox);
        const QSignalBlocker blockerThreshold(ui->discountThresholdSpinBox);
        const QSignalBlocker blockerTax(ui->taxRateComboBox);

        const QString productId = ui->inventoryTableWidget->item(currentRow, 0)
                                      ? ui->inventoryTableWidget->item(currentRow, 0)->text()
                                      : QString();
        const QString name = ui->inventoryTableWidget->item(currentRow, 1)
                                 ? ui->inventoryTableWidget->item(currentRow, 1)->text()
                                 : QString();
        const double basePrice = ui->inventoryTableWidget->item(currentRow, 2)
                                     ? ui->inventoryTableWidget->item(currentRow, 2)->data(kBasePriceRole).toDouble()
                                     : 0.0;
        const double finalPrice = ui->inventoryTableWidget->item(currentRow, 2)
                                      ? ui->inventoryTableWidget->item(currentRow, 2)->data(kFinalPriceRole).toDouble()
                                      : 0.0;
        const int quantity = ui->inventoryTableWidget->item(currentRow, 3)
                                 ? ui->inventoryTableWidget->item(currentRow, 3)->data(Qt::UserRole).toInt()
                                 : 0;
        const QString type = ui->inventoryTableWidget->item(currentRow, 4)
                                 ? ui->inventoryTableWidget->item(currentRow, 4)->data(Qt::UserRole).toString()
                                 : QStringLiteral("Standard");
        const QString category = ui->inventoryTableWidget->item(currentRow, 5)
                                     ? ui->inventoryTableWidget->item(currentRow, 5)->text()
                                     : QString();

        ui->productIdSpinBox->setValue(productId.toInt());
        ui->productNameLineEdit->setText(name);
        ui->productPriceDoubleSpinBox->setValue(basePrice);
        ui->productQuantitySpinBox->setValue(quantity);
        ui->productTypeComboBox->setCurrentText(editorTypeForBackendType(type));
        ui->discountThresholdSpinBox->setValue(ui->discountThresholdSpinBox->minimum());
        ui->taxRateComboBox->setCurrentIndex(0);

        syncTypeControls();
        setEditorSelectionState(true);
        if (m_backendReadOnlyMode) {
            const QString categoryClause = category.isEmpty()
                ? QString()
                : QStringLiteral(" Category: %1.").arg(category);
            ui->editorHelperLabel->setText(
                QStringLiteral("Viewing a protected backend row.%1 Base price is shown in the editor, final price reflects backend pricing rules, and detailed tax or discount rates are not surfaced in this slice.").arg(categoryClause));
        } else {
            ui->editorHelperLabel->setText(
                QStringLiteral("Editing the selected catalogue row. Use Update Product to apply changes."));
        }
        ui->editorHelperLabel->show();
        UiHelpers::setBadgeText(ui->editorStockAlertLabel,
                                tr("Stock %1 units").arg(formatWholeNumber(quantity)),
                                stockBadgeRole(quantity));
        UiHelpers::setBadgeText(ui->editorSyncLabel,
                                tr("Final price %1").arg(formatCurrency(finalPrice)),
                                typeRole(type));
        ui->updateProductButton->setEnabled(true);
        ui->removeProductButton->setEnabled(true);
    };

    connect(ui->inventoryTableWidget, &QTableWidget::itemSelectionChanged, this,
            [this, syncEditorFromSelection, clearEditor]() {
                if (ui->inventoryTableWidget->selectedItems().isEmpty()) {
                    clearEditor();
                    for (int row = 0; row < ui->inventoryTableWidget->rowCount(); ++row) {
                        setRowSelectionState(ui->inventoryTableWidget, row, false);
                    }
                    return;
                }

                syncEditorFromSelection();
            });

    connect(ui->updateProductButton, &QPushButton::clicked, this, [this, syncEditorFromSelection]() {

        QTableWidgetItem *currentItem = ui->inventoryTableWidget->currentItem();
        if (!currentItem) {
            return;
        }

        const int row = currentItem->row();
        const QString existingCategory = ui->inventoryTableWidget->item(row, 5)
            ? ui->inventoryTableWidget->item(row, 5)->text() : QString();
        populateInventoryRow(ui->inventoryTableWidget,
                             row,
                             QString::number(ui->productIdSpinBox->value()),
                             ui->productNameLineEdit->text(),
                             ui->productPriceDoubleSpinBox->value(),
                             ui->productPriceDoubleSpinBox->value(),
                             ui->productQuantitySpinBox->value(),
                             ui->productTypeComboBox->currentText(),
                             existingCategory,
                             ui->productQuantitySpinBox->value());
        // Sync back to m_products
        const int pid = ui->productIdSpinBox->value();
        for (ProductDto &p : m_products) {
            if (p.id == pid) {
                p.name      = ui->productNameLineEdit->text();
                p.basePrice  = ui->productPriceDoubleSpinBox->value();
                p.finalPrice = p.basePrice;
                p.quantity  = ui->productQuantitySpinBox->value();
                p.type      = ui->productTypeComboBox->currentText();
                p.category  = existingCategory;
                break;
            }
        }
        ui->inventoryTableWidget->selectRow(row);
        ui->inventoryTableWidget->setCurrentCell(row, 0);
        syncEditorFromSelection();
    });

    connect(ui->removeProductButton, &QPushButton::clicked, this, [this, clearEditor]() {

        QTableWidgetItem *currentItem = ui->inventoryTableWidget->currentItem();
        if (!currentItem) {
            return;
        }

        const int removedId = ui->productIdSpinBox->value();
        ui->inventoryTableWidget->removeRow(currentItem->row());
        m_products.erase(std::remove_if(m_products.begin(), m_products.end(),
            [removedId](const ProductDto &p){ return p.id == removedId; }), m_products.end());
        clearEditor();
        ui->inventoryFooterLabel->setText(
            tr("%1 products").arg(formatWholeNumber(ui->inventoryTableWidget->rowCount())));
    });

    // Enable all interactive controls
    ui->inventorySearchLineEdit->setEnabled(true);
    ui->sortComboBox->setEnabled(true);
    ui->filterButton->setEnabled(true);
    ui->lowStockOnlyCheckBox->setEnabled(true);
    ui->addProductButton->setEnabled(true);
    ui->browseCsvButton->setEnabled(true);
    ui->previewCsvButton->setEnabled(true);
    ui->importCsvButton->setEnabled(true);
    ui->productNameLineEdit->setReadOnly(false);
    ui->productPriceDoubleSpinBox->setReadOnly(false);
    ui->productQuantitySpinBox->setReadOnly(false);

    // Wire search
    connect(ui->inventorySearchLineEdit, &QLineEdit::textChanged, this, [this]() {
        filterAndSortTable();
    });
    connect(ui->filterButton, &QPushButton::clicked, this, [this]() {
        filterAndSortTable();
    });
    connect(ui->sortComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        filterAndSortTable();
    });
    connect(ui->lowStockOnlyCheckBox, &QCheckBox::toggled, this, [this](bool) {
        filterAndSortTable();
    });

    // Wire Browse CSV
    connect(ui->browseCsvButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Open Products CSV"), QString(), tr("CSV Files (*.csv);;All Files (*)"));
        if (!path.isEmpty()) {
            ui->csvPathLineEdit->setText(QDir::toNativeSeparators(path));
        }
    });

    // Wire Preview CSV
    connect(ui->previewCsvButton, &QPushButton::clicked, this, [this]() {
        previewCsvFile(ui->csvPathLineEdit->text());
    });

    // Wire Import CSV
    connect(ui->importCsvButton, &QPushButton::clicked, this, [this]() {
        importCsvFile(ui->csvPathLineEdit->text());
    });

    // Wire Add Product button
    connect(ui->addProductButton, &QPushButton::clicked, this, [this]() {
        addNewProduct();
    });

    clearEditor();

    refreshScrollableContentSize(this);
    QMetaObject::invokeMethod(this, [this]() { refreshScrollableContentSize(this); },
                              Qt::QueuedConnection);
}

void InventoryPage::setInventoryData(const QVector<ProductDto> &products, const QString &sourcePath)
{
    m_backendReadOnlyMode = false;
    m_products = products;
    if (!products.isEmpty()) m_nextId = products.last().id + 1;

    int maximumQuantity = 1;
    for (const ProductDto &product : products) {
        maximumQuantity = qMax(maximumQuantity, product.quantity);
    }

    ui->inventoryTableWidget->clearSelection();
    ui->inventoryTableWidget->setRowCount(products.size());

    for (int i = 0; i < products.size(); ++i) {
        const ProductDto &product = products[i];
        populateInventoryRow(ui->inventoryTableWidget,
                             i,
                             QString::number(product.id),
                             product.name,
                             product.basePrice,
                             product.finalPrice,
                             product.quantity,
                             product.type,
                             product.category,
                             maximumQuantity);
    }

    const int previewCount = qMin(3, products.size());
    ui->importPreviewTableWidget->setRowCount(previewCount);
    for (int i = 0; i < previewCount; ++i) {
        const ProductDto &product = products[i];
        populateImportPreviewRow(ui->importPreviewTableWidget,
                                 i,
                                 QString::number(product.id),
                                 product.name,
                                 product.basePrice,
                                 product.quantity,
                                 product.type,
                                 product.category);
    }

    const QFileInfo sourceInfo(sourcePath);
    const QString fileName =
        sourceInfo.fileName().isEmpty() ? QStringLiteral("Products.csv") : sourceInfo.fileName();
    ui->csvPathLineEdit->setText(QDir::toNativeSeparators(sourcePath));
    ui->inventoryTableSummaryLabel->setText(
        tr("%1 products loaded from %2").arg(formatWholeNumber(products.size())).arg(fileName));
    ui->inventorySelectionHintLabel->setText(
        products.isEmpty() ? tr("No products available.")
            : tr("Select a row to edit. Use Add Product to create new entries."));
    ui->inventoryFooterLabel->setText(
        products.isEmpty() ? tr("No products loaded")
            : tr("%1 products").arg(formatWholeNumber(products.size())));
    ui->importHelperLabel->setText(tr("Browse and select a CSV file, preview it, then press Import Data to load."));
    ui->importSummaryRowsLabel->setText(tr("%1 preview rows shown").arg(formatWholeNumber(previewCount)));
    ui->importSummaryFileLabel->setText(QDir::toNativeSeparators(sourcePath));
    UiHelpers::setBadgeText(ui->importSummaryStatusLabel, tr("Loaded"), "success");
    UiHelpers::setBadgeText(ui->editorStockAlertLabel, tr("Select a row"), "idle");
    UiHelpers::setBadgeText(ui->editorSyncLabel, tr("Select a row"), "idle");
    ui->inventoryStatusLabel->setText(
        products.isEmpty() ? tr("No products loaded") : tr("Loaded %1").arg(fileName));
    ui->mutexStatusLabel->setText(tr("Read/Write"));
    UiHelpers::setBadge(ui->inventoryStatusLabel, products.isEmpty() ? "warning" : "success");
    UiHelpers::setBadge(ui->mutexStatusLabel, "info");
    ui->editorHelperLabel->setText(QStringLiteral("Select a product row to edit it."));
    ui->editorHelperLabel->show();
    ui->importSummaryHintLabel->setText(tr("Preview reflects the selected CSV. Press Import Data to replace the current catalogue."));

    if (ui->inventoryTableWidget->rowCount() > 0) {
        ui->inventoryTableWidget->setCurrentCell(0, 0);
    }

    refreshScrollableContentSize(this);
}

// ============================================================
// IMPLEMENTED FEATURES
// ============================================================

static QVector<ProductDto> parseCsvProducts(const QString &path, bool &ok)
{
    QVector<ProductDto> products;
    ok = false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return products;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        // Format: id,name,category,price,quantity,type,rate,discount
        QStringList f = line.split(',');
        if (f.size() < 5) continue;
        ProductDto p;
        p.id       = f[0].trimmed().toInt();
        p.name     = f[1].trimmed();
        p.category = f.size() > 2 ? f[2].trimmed() : QStringLiteral("General");
        p.basePrice  = f.size() > 3 ? f[3].trimmed().toDouble() : 0.0;
        p.finalPrice = p.basePrice;
        p.quantity = f.size() > 4 ? f[4].trimmed().toInt() : 0;
        // Type field: T=Taxable, D=Discounted, S/default=Standard
        if (f.size() > 5) {
            const QString t = f[5].trimmed().toUpper();
            if (t == "T" || t == "TAXABLE")
                p.type = QStringLiteral("Taxable");
            else if (t == "D" || t == "DISCOUNTED")
                p.type = QStringLiteral("Discounted");
            else
                p.type = QStringLiteral("Standard");
        } else {
            p.type = QStringLiteral("Standard");
        }
        // Apply rate to final price
        if (f.size() > 6) {
            double rate = f[6].trimmed().toDouble();
            if (p.type == QStringLiteral("Taxable"))
                p.finalPrice = p.basePrice * (1.0 + rate);
            else if (p.type == QStringLiteral("Discounted"))
                p.finalPrice = p.basePrice * (1.0 - rate);
        }
        products.append(p);
    }
    ok = !products.isEmpty();
    return products;
}

void InventoryPage::filterAndSortTable()
{
    const QString query  = ui->inventorySearchLineEdit->text().trimmed().toLower();
    const QString sortBy = ui->sortComboBox->currentText();
    const bool lowOnly   = ui->lowStockOnlyCheckBox->isChecked();

    // Filter
    QVector<ProductDto> filtered;
    for (const ProductDto &p : m_products) {
        if (lowOnly && p.quantity > 100) continue;
        if (!query.isEmpty()) {
            const bool match = QString::number(p.id).contains(query)
                || p.name.toLower().contains(query)
                || p.category.toLower().contains(query)
                || p.type.toLower().contains(query);
            if (!match) continue;
        }
        filtered.append(p);
    }

    // Sort
    if (sortBy == QStringLiteral("Name"))
        std::sort(filtered.begin(), filtered.end(), [](const ProductDto &a, const ProductDto &b){ return a.name < b.name; });
    else if (sortBy == QStringLiteral("Quantity"))
        std::sort(filtered.begin(), filtered.end(), [](const ProductDto &a, const ProductDto &b){ return a.quantity < b.quantity; });
    else if (sortBy == QStringLiteral("Price"))
        std::sort(filtered.begin(), filtered.end(), [](const ProductDto &a, const ProductDto &b){ return a.basePrice < b.basePrice; });
    else if (sortBy == QStringLiteral("Type"))
        std::sort(filtered.begin(), filtered.end(), [](const ProductDto &a, const ProductDto &b){ return a.type < b.type; });
    else if (sortBy == QStringLiteral("Category"))
        std::sort(filtered.begin(), filtered.end(), [](const ProductDto &a, const ProductDto &b){ return a.category < b.category; });
    // else ProductID - already natural order

    // Repopulate table
    int maxQty = 1;
    for (const ProductDto &p : filtered) maxQty = qMax(maxQty, p.quantity);

    const QSignalBlocker blocker(ui->inventoryTableWidget);
    ui->inventoryTableWidget->clearSelection();
    ui->inventoryTableWidget->setRowCount(filtered.size());
    for (int i = 0; i < filtered.size(); ++i)
        populateInventoryRow(ui->inventoryTableWidget, i,
            QString::number(filtered[i].id), filtered[i].name,
            filtered[i].basePrice, filtered[i].finalPrice,
            filtered[i].quantity, filtered[i].type,
            filtered[i].category, maxQty);

    ui->inventoryFooterLabel->setText(tr("%1 products").arg(filtered.size()));
}

void InventoryPage::previewCsvFile(const QString &path)
{
    if (path.trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("No File"), tr("Please select a CSV file first."));
        return;
    }
    bool ok = false;
    const QVector<ProductDto> previewed = parseCsvProducts(path, ok);
    if (!ok) {
        QMessageBox::warning(this, tr("Preview Failed"),
            tr("Could not read '%1'.\nCheck that it is a valid Products CSV.").arg(path));
        return;
    }
    const int show = qMin(previewed.size(), 50);
    ui->importPreviewTableWidget->setRowCount(show);
    for (int i = 0; i < show; ++i)
        populateImportPreviewRow(ui->importPreviewTableWidget, i,
            QString::number(previewed[i].id), previewed[i].name,
            previewed[i].basePrice, previewed[i].quantity,
            previewed[i].type, previewed[i].category);
    ui->importSummaryRowsLabel->setText(tr("%1 rows (showing %2)").arg(previewed.size()).arg(show));
    UiHelpers::setBadgeText(ui->importSummaryStatusLabel, tr("Preview Ready"), "info");
    ui->importSummaryFileLabel->setText(QFileInfo(path).fileName());
}

void InventoryPage::importCsvFile(const QString &path)
{
    if (path.trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("No File"), tr("Please browse and select a CSV file first."));
        return;
    }
    bool ok = false;
    const QVector<ProductDto> imported = parseCsvProducts(path, ok);
    if (!ok) {
        QMessageBox::warning(this, tr("Import Failed"),
            tr("Could not import '%1'.").arg(path));
        return;
    }
    auto reply = QMessageBox::question(this, tr("Import Data"),
        tr("Replace the current catalogue with %1 products from '%2'?")
            .arg(imported.size()).arg(QFileInfo(path).fileName()),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (reply != QMessageBox::Yes) return;

    setInventoryData(imported, path);
}

void InventoryPage::addNewProduct()
{
    bool ok;
    const QString name = QInputDialog::getText(this, tr("Add Product"),
        tr("Product name:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    ProductDto p;
    p.id        = m_nextId++;
    p.name      = name.trimmed();
    p.category  = QStringLiteral("General");
    p.type      = QStringLiteral("Standard");
    p.basePrice  = 0.0;
    p.finalPrice = 0.0;
    p.quantity  = 0;
    m_products.append(p);

    const int row = ui->inventoryTableWidget->rowCount();
    ui->inventoryTableWidget->setRowCount(row + 1);
    populateInventoryRow(ui->inventoryTableWidget, row,
        QString::number(p.id), p.name, p.basePrice, p.finalPrice,
        p.quantity, p.type, p.category, 1);

    ui->inventoryTableWidget->setCurrentCell(row, 0);
    ui->inventoryTableWidget->scrollToBottom();
    ui->inventoryFooterLabel->setText(tr("%1 products").arg(m_products.size()));
}

void InventoryPage::exportCatalogueCsv()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export Catalogue"), QStringLiteral("Catalog/Updated_Products.csv"),
        tr("CSV Files (*.csv)"));
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Failed"), tr("Cannot write to '%1'.").arg(path));
        return;
    }
    QTextStream out(&file);
    for (const ProductDto &p : m_products) {
        // id,name,category,price,quantity,type,rate,discount
        QString typeCode = QStringLiteral("S");
        if (p.type == QStringLiteral("Taxable"))     typeCode = QStringLiteral("T");
        if (p.type == QStringLiteral("Discounted"))  typeCode = QStringLiteral("D");
        out << p.id << ',' << p.name << ',' << p.category << ','
            << p.basePrice << ',' << p.quantity << ',' << typeCode << "\r\n";
    }
    QMessageBox::information(this, tr("Export Complete"),
        tr("Exported %1 products to '%2'.").arg(m_products.size()).arg(path));
}
