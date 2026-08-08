#include "logspage.h"
#include "ui_logspage.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>

#include <QAbstractScrollArea>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QHeaderView>
#include <QLocale>
#include <QMetaObject>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QWheelEvent>
#include <QVBoxLayout>

#include "uihelpers.h"

namespace {

QString statusRole(const QString &status)
{
    const QString lowered = status.toLower();
    if (lowered.contains("success")) {
        return QStringLiteral("success");
    }
    if (lowered.contains("partial")) {
        return QStringLiteral("warning");
    }
    if (lowered.contains("fail")) {
        return QStringLiteral("fail");
    }
    return QStringLiteral("idle");
}

QString typeRole(const QString &type)
{
    if (type == QLatin1String("Taxable")) {
        return QStringLiteral("taxed");
    }
    if (type == QLatin1String("Discounted")) {
        return QStringLiteral("discounted");
    }
    if (type == QLatin1String("Standard")) {
        return QStringLiteral("standard");
    }
    return QStringLiteral("idle");
}

QString displayText(const QString &value, const QString &fallback = QStringLiteral("--"))
{
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? fallback : trimmed;
}

QString formatCurrency(double value)
{
    return QStringLiteral("R %1").arg(QLocale().toString(value, 'f', 2));
}

QString itemStatusText(const ProcessedAnalyticsRecordDto &record)
{
    const QString status = displayText(record.itemStatus, QString());
    if (!status.isEmpty()) {
        return status;
    }
    return record.itemSucceeded ? QStringLiteral("SUCCESS") : QStringLiteral("FAILED");
}

QString warehouseText(int warehouseId)
{
    return warehouseId > 0 ? QStringLiteral("W%1").arg(warehouseId) : QStringLiteral("--");
}

// Columns: Order | Product | Type | Category | Qty | Warehouse | Item Status | Order Status | Final Price
void populateAnalyticsRecordRow(QTableWidget *table,
                                int row,
                                const ProcessedAnalyticsRecordDto &record)
{
    const QString productType = displayText(record.productType);
    const QString itemStatus = itemStatusText(record);
    const QString orderStatus = displayText(record.orderStatus);
    const QString warehouse = warehouseText(record.warehouseId);

    UiHelpers::setTableText(table, row, 0, QString::number(record.orderId),
                            Qt::AlignRight | Qt::AlignVCenter);
    UiHelpers::setTableText(table, row, 1, QString::number(record.productId),
                            Qt::AlignRight | Qt::AlignVCenter);
    UiHelpers::setTableChipAligned(table, row, 2, productType, typeRole(productType),
                                   Qt::AlignLeft | Qt::AlignVCenter);
    UiHelpers::setTableText(table, row, 3, displayText(record.category));
    UiHelpers::setTableText(table, row, 4, QString::number(record.quantity),
                            Qt::AlignRight | Qt::AlignVCenter);
    UiHelpers::setTableChip(table, row, 5, warehouse, warehouse.toLower());
    UiHelpers::setTableBadgeAligned(table, row, 6, itemStatus, statusRole(itemStatus),
                                    Qt::AlignLeft | Qt::AlignVCenter);
    UiHelpers::setTableBadgeAligned(table, row, 7, orderStatus, statusRole(orderStatus),
                                    Qt::AlignLeft | Qt::AlignVCenter);
    UiHelpers::setTableText(table, row, 8, formatCurrency(record.finalPrice),
                            Qt::AlignRight | Qt::AlignVCenter);
}

void populateDetailField(QTableWidget *table,
                         int row,
                         const QString &field,
                         const QString &value)
{
    UiHelpers::setTableText(table, row, 0, field);
    UiHelpers::setTableText(table, row, 1, value);
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
        page ? page->findChild<QScrollArea *>(QStringLiteral("logsPageScrollArea"))
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

void makePageScrollable(QWidget *page, const QMargins &margins, int spacing)
{
    auto *outerLayout = qobject_cast<QVBoxLayout *>(page ? page->layout() : nullptr);
    if (!outerLayout || outerLayout->count() == 0) {
        return;
    }

    QLayoutItem *fixedBottomItem = nullptr;
    if (outerLayout->count() > 0) {
        fixedBottomItem = outerLayout->takeAt(outerLayout->count() - 1);
    }

    auto *scrollArea = new QScrollArea(page);
    scrollArea->setObjectName(QStringLiteral("logsPageScrollArea"));
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->viewport()->setObjectName(QStringLiteral("logsPageScrollViewport"));
    scrollArea->viewport()->setAttribute(Qt::WA_StyledBackground, false);

    auto *scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName(QStringLiteral("logsPageScrollContent"));
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
        page ? page->findChild<QWidget *>(QStringLiteral("logsPageScrollContent"))
             : nullptr;
    auto *scrollArea =
        page ? page->findChild<QScrollArea *>(QStringLiteral("logsPageScrollArea"))
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

LogsPage::LogsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LogsPage)
{
    ui->setupUi(this);
    setupUiDefaults();
}

LogsPage::~LogsPage()
{
    delete ui;
}

bool LogsPage::eventFilter(QObject *watched, QEvent *event)
{
    if (event && event->type() == QEvent::Wheel) {
        auto *wheelEvent = static_cast<QWheelEvent *>(event);
        auto *scrollArea = scrollAreaForObject(watched);
        if ((scrollArea == ui->logsTableWidget || scrollArea == ui->detailItemsTableWidget)
            && !canScrollVertically(scrollArea, verticalWheelDelta(wheelEvent))
            && forwardWheelToPageScrollArea(this, wheelEvent)) {
            wheelEvent->accept();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void LogsPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Splitter fills available space automatically — no scroll refresh needed
}

void LogsPage::setupUiDefaults()
{
    // No scroll wrapper — splitter fills available space
    UiHelpers::setPageLayout(this, QMargins(28, 20, 28, 0), 10);
    UiHelpers::setProperty(ui->logsTitleLabel, "textRole", "pageTitle");

    UiHelpers::setSurface(ui->filterFrame, "toolbar");
    UiHelpers::setSurface(ui->logsTableGroupBox, "card");
    UiHelpers::setSurface(ui->logDetailsGroupBox, "card");
    UiHelpers::setSurface(ui->actionStripFrame, "soft");
    UiHelpers::applyCardShadow(ui->filterFrame, 22, 6, QColor(24, 39, 62, 18));
    UiHelpers::applyCardShadow(ui->logsTableGroupBox, 34, 8, QColor(24, 39, 62, 24));
    UiHelpers::applyCardShadow(ui->logDetailsGroupBox, 34, 8, QColor(24, 39, 62, 22));

    UiHelpers::tuneLayout(ui->filterFrame, QMargins(14, 12, 14, 12), 12);
    UiHelpers::tuneLayout(ui->logsTableGroupBox, QMargins(16, 12, 16, 16), 14);
    UiHelpers::tuneLayout(ui->logDetailsGroupBox, QMargins(16, 12, 16, 16), 12);
    UiHelpers::tuneLayout(ui->actionStripFrame, QMargins(30, 12, 30, 12), 12);

    // ---- Labels -------------------------------------------------------
    ui->systemLogsTitleLabel->setText(tr("Processed Item Records"));
    UiHelpers::setProperty(ui->systemLogsTitleLabel, "textRole", "cardTitle");

    ui->logDetailsTitleLabel->setText(tr("Record Details"));
    UiHelpers::setProperty(ui->logDetailsTitleLabel, "textRole", "cardTitle");

    UiHelpers::setProperty(ui->totalLogEntriesTitle, "textRole", "muted");
    UiHelpers::setProperty(ui->failedOrdersTitle, "textRole", "muted");
    UiHelpers::setProperty(ui->logExportStatusLabel, "textRole", "muted");
    UiHelpers::setProperty(ui->selectedOrderIdLabel, "textRole", "sectionTitle");
    ui->totalLogEntriesTitle->setText(tr("processed orders"));
    ui->failedOrdersTitle->setText(tr("failed orders"));

    // ---- Filter bar (disabled — filtering not implemented) ------------
    ui->warehouseFilterComboBox->addItems({"Warehouse", "W1", "W2", "W3", "W4", "W5"});
    ui->statusFilterComboBox->addItems({"Item Status: All", "Success", "Failed"});
    ui->logSearchLineEdit->setPlaceholderText(tr("Search captured records"));

    ui->warehouseFilterComboBox->setEnabled(true);
    ui->statusFilterComboBox->setEnabled(true);
    ui->logSearchLineEdit->setEnabled(true);
    ui->fromDateLineEdit->setEnabled(false);
    ui->toDateLineEdit->setEnabled(false);
    ui->fromDateLineEdit->hide();
    ui->toDateLineEdit->hide();

    // ---- Pagination (disabled — single-page batch result) -------------
    UiHelpers::setButtonRole(ui->logsPrevPageButton, "page");
    UiHelpers::setButtonRole(ui->logsPage1Button, "pageActive");
    UiHelpers::setButtonRole(ui->logsPage2Button, "page");
    UiHelpers::setButtonRole(ui->logsPage3Button, "page");
    UiHelpers::setButtonRole(ui->logsNextPageButton, "page");
    ui->logsPrevPageButton->setEnabled(false);
    ui->logsPage1Button->setEnabled(false);
    ui->logsPage2Button->setEnabled(false);
    ui->logsPage3Button->setEnabled(false);
    ui->logsNextPageButton->setEnabled(false);

    // ---- Action strip (disabled — export not implemented) -------------
    UiHelpers::setButtonRole(ui->exportSelectedCsvButton, "secondary");
    UiHelpers::setButtonRole(ui->exportAllCsvButton, "success");
    UiHelpers::setButtonRole(ui->openLogFolderButton, "secondary");
    ui->exportSelectedCsvButton->setEnabled(true);
    ui->exportAllCsvButton->setEnabled(true);
    ui->openLogFolderButton->setEnabled(false);
    ui->actionStripFrame->setAttribute(Qt::WA_StyledBackground, true);
    ui->logExportStatusLabel->setText(tr("Run an orders batch to populate this view."));

    // Wire Export All CSV button
    connect(ui->exportAllCsvButton, &QPushButton::clicked, this, [this]() {
        exportAllCsv();
    });
    connect(ui->exportSelectedCsvButton, &QPushButton::clicked, this, [this]() {
        exportAllCsv();
    });

    // Wire filter bar
    auto applyFilter = [this]() {
        const int whIdx = ui->warehouseFilterComboBox->currentIndex();
        const int statusIdx = ui->statusFilterComboBox->currentIndex();
        const QString search = ui->logSearchLineEdit->text().trimmed().toLower();
        for (int row = 0; row < ui->logsTableWidget->rowCount(); ++row) {
            bool show = true;
            if (whIdx > 0) {
                const QString wh = ui->warehouseFilterComboBox->itemText(whIdx);
                const auto *item = ui->logsTableWidget->item(row, 5);
                if (!item || !item->text().contains(wh)) show = false;
            }
            if (show && statusIdx > 0) {
                const QString st = ui->statusFilterComboBox->itemText(statusIdx).toUpper();
                const auto *item = ui->logsTableWidget->item(row, 6);
                if (!item || !item->text().toUpper().contains(st)) show = false;
            }
            if (show && !search.isEmpty()) {
                bool found = false;
                for (int col = 0; col < ui->logsTableWidget->columnCount(); ++col) {
                    const auto *item = ui->logsTableWidget->item(row, col);
                    if (item && item->text().toLower().contains(search)) { found = true; break; }
                }
                if (!found) show = false;
            }
            ui->logsTableWidget->setRowHidden(row, !show);
        }
    };
    connect(ui->warehouseFilterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [applyFilter](int){ applyFilter(); });
    connect(ui->statusFilterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [applyFilter](int){ applyFilter(); });
    connect(ui->logSearchLineEdit, &QLineEdit::textChanged, this, [applyFilter](const QString&){ applyFilter(); });

    // ---- Main table: captured item/order records from v2 analytics --------
    UiHelpers::setupReadOnlyTable(ui->logsTableWidget, 48);
    ui->logsTableWidget->setMinimumHeight(200);
    ui->logsTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->logsTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->logsTableWidget->installEventFilter(this);
    ui->logsTableWidget->viewport()->installEventFilter(this);
    ui->logsTableWidget->setColumnCount(9);
    ui->logsTableWidget->setHorizontalHeaderLabels({
        tr("Order"), tr("Product"), tr("Type"), tr("Category"), tr("Qty"),
        tr("Warehouse"), tr("Item Status"), tr("Order Status"), tr("Final Price")
    });
    ui->logsTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->logsTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->logsTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->logsTableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->logsTableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui->logsTableWidget->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    ui->logsTableWidget->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    ui->logsTableWidget->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Fixed);
    ui->logsTableWidget->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    ui->logsTableWidget->setColumnWidth(2, 118);
    ui->logsTableWidget->setColumnWidth(6, 124);
    ui->logsTableWidget->setColumnWidth(7, 124);
    ui->logsTableWidget->setRowCount(0);  // empty until a batch completes

    // Populate detail panel when a row is selected.
    connect(ui->logsTableWidget, &QTableWidget::currentItemChanged,
            this, [this](QTableWidgetItem *current, QTableWidgetItem *) {
                if (current) {
                    updateDetailPanel(current->row());
                } else {
                    clearDetailPanel();
                }
            });

    // ---- Counters (start at zero) ------------------------------------
    ui->totalLogEntriesLabel->setText(QStringLiteral("0"));
    ui->failedOrdersCountLabel->setText(QStringLiteral("0"));
    UiHelpers::setBadge(ui->totalLogEntriesLabel, "info");
    UiHelpers::setBadge(ui->failedOrdersCountLabel, "fail");

    // ---- Detail panel ------------------------------------------------
    // Field/value table. Timestamps and failure reasons are not fabricated.
    UiHelpers::setupReadOnlyTable(ui->detailItemsTableWidget, 40, false);
    ui->detailItemsTableWidget->setMinimumHeight(100);
    ui->detailItemsTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->detailItemsTableWidget->installEventFilter(this);
    ui->detailItemsTableWidget->viewport()->installEventFilter(this);
    ui->detailItemsTableWidget->setColumnCount(2);
    ui->detailItemsTableWidget->setHorizontalHeaderLabels({tr("Field"), tr("Value")});
    ui->detailItemsTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->detailItemsTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->detailItemsTableWidget->setRowCount(0);

    // Wallet / pricing rows are not shown (no real pricing data available).
    ui->detailsTotalsFrame->hide();

    // Processing history timeline is not shown (no per-step data available).
    ui->detailsHistoryLabel->hide();
    ui->logDetailsTextEdit->hide();

    clearDetailPanel();

    UiHelpers::setProperty(ui->selectedWarehouseLabel, "role", "detailMetaBadge");
    UiHelpers::setProperty(ui->selectedStatusLabel, "role", "detailMetaBadge");
    ui->selectedWarehouseLabel->setMinimumWidth(58);
    ui->selectedStatusLabel->setMinimumWidth(70);

    UiHelpers::softenSplitter(ui->mainSplitter);
    ui->mainSplitter->setMinimumHeight(0);
    ui->mainSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->mainSplitter->setSizes({1000, 380});
    ui->mainSplitter->setStretchFactor(0, 3);
    ui->mainSplitter->setStretchFactor(1, 1);
    // Give the splitter all remaining vertical space in the page layout
    if (auto *lay = qobject_cast<QVBoxLayout*>(layout())) {
        // layout order: title(0), filterFrame(1), mainSplitter(2), actionStripFrame(3)
        lay->setStretch(0, 0);
        lay->setStretch(1, 0);
        lay->setStretch(2, 1);
        lay->setStretch(3, 0);
    }

}


// ---------------------------------------------------------------------------
// setLogsSnapshot — primary data-injection point
// ---------------------------------------------------------------------------
void LogsPage::setLogsSnapshot(const LogsSnapshotDto &snapshot)
{
    m_snapshot = snapshot;

    // Populate main table with captured backend analytics records.
    const QSignalBlocker blocker(ui->logsTableWidget);
    Q_UNUSED(blocker);
    const int count = snapshot.analyticsRecords.size();
    ui->logsTableWidget->setRowCount(count);
    for (int i = 0; i < count; ++i) {
        populateAnalyticsRecordRow(ui->logsTableWidget, i, snapshot.analyticsRecords[i]);
    }

    // Update counters.
    ui->totalLogEntriesLabel->setText(QString::number(snapshot.totalCount));
    UiHelpers::setBadge(ui->totalLogEntriesLabel, "info");
    ui->failedOrdersCountLabel->setText(QString::number(snapshot.failedCount));
    UiHelpers::setBadge(ui->failedOrdersCountLabel, "fail");

    // Status line.
    if (count > 0) {
        ui->logExportStatusLabel->setText(
            tr("Last batch: %1 captured item records from %2 processed orders; %3 failed orders.")
                .arg(count)
                .arg(snapshot.totalCount)
                .arg(snapshot.failedCount));
    } else if (snapshot.totalCount > 0) {
        ui->logExportStatusLabel->setText(
            tr("Last batch processed %1 orders, but no analytics item records were captured.")
                .arg(snapshot.totalCount));
    } else {
        ui->logExportStatusLabel->setText(tr("Run an orders batch to populate this view."));

    // Wire Export All CSV button
    connect(ui->exportAllCsvButton, &QPushButton::clicked, this, [this]() {
        exportAllCsv();
    });
    connect(ui->exportSelectedCsvButton, &QPushButton::clicked, this, [this]() {
        exportAllCsv();
    });

    // Wire filter bar
    auto applyFilter = [this]() {
        const int whIdx = ui->warehouseFilterComboBox->currentIndex();
        const int statusIdx = ui->statusFilterComboBox->currentIndex();
        const QString search = ui->logSearchLineEdit->text().trimmed().toLower();
        for (int row = 0; row < ui->logsTableWidget->rowCount(); ++row) {
            bool show = true;
            if (whIdx > 0) {
                const QString wh = ui->warehouseFilterComboBox->itemText(whIdx);
                const auto *item = ui->logsTableWidget->item(row, 5);
                if (!item || !item->text().contains(wh)) show = false;
            }
            if (show && statusIdx > 0) {
                const QString st = ui->statusFilterComboBox->itemText(statusIdx).toUpper();
                const auto *item = ui->logsTableWidget->item(row, 6);
                if (!item || !item->text().toUpper().contains(st)) show = false;
            }
            if (show && !search.isEmpty()) {
                bool found = false;
                for (int col = 0; col < ui->logsTableWidget->columnCount(); ++col) {
                    const auto *item = ui->logsTableWidget->item(row, col);
                    if (item && item->text().toLower().contains(search)) { found = true; break; }
                }
                if (!found) show = false;
            }
            ui->logsTableWidget->setRowHidden(row, !show);
        }
    };
    connect(ui->warehouseFilterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [applyFilter](int){ applyFilter(); });
    connect(ui->statusFilterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [applyFilter](int){ applyFilter(); });
    connect(ui->logSearchLineEdit, &QLineEdit::textChanged, this, [applyFilter](const QString&){ applyFilter(); });
    }

    // Clear the detail panel until the user selects a row.
    clearDetailPanel();

}

// ---------------------------------------------------------------------------
// updateDetailPanel — called on row selection
// ---------------------------------------------------------------------------
void LogsPage::updateDetailPanel(int row)
{
    if (row < 0 || row >= m_snapshot.analyticsRecords.size()) {
        clearDetailPanel();
        return;
    }

    const ProcessedAnalyticsRecordDto &record = m_snapshot.analyticsRecords[row];
    const QString productType = displayText(record.productType);
    const QString category = displayText(record.category);
    const QString itemStatus = itemStatusText(record);
    const QString orderStatus = displayText(record.orderStatus);
    const QString warehouse = warehouseText(record.warehouseId);

    ui->selectedOrderIdLabel->setText(
        QStringLiteral("Order #%1 / Product #%2").arg(record.orderId).arg(record.productId));

    ui->selectedWarehouseLabel->setText(warehouse);
    UiHelpers::setBadge(ui->selectedWarehouseLabel, warehouse.toLower());

    ui->selectedStatusLabel->setText(itemStatus);
    UiHelpers::setBadge(ui->selectedStatusLabel, statusRole(itemStatus));

    ui->detailItemsTableWidget->setRowCount(9);
    populateDetailField(ui->detailItemsTableWidget, 0, tr("Order ID"),
                        QString::number(record.orderId));
    populateDetailField(ui->detailItemsTableWidget, 1, tr("Product ID"),
                        QString::number(record.productId));
    UiHelpers::setTableText(ui->detailItemsTableWidget, 2, 0, tr("Product Type"));
    UiHelpers::setTableChipAligned(ui->detailItemsTableWidget, 2, 1, productType,
                                   typeRole(productType), Qt::AlignLeft | Qt::AlignVCenter);
    populateDetailField(ui->detailItemsTableWidget, 3, tr("Category"), category);
    populateDetailField(ui->detailItemsTableWidget, 4, tr("Quantity"),
                        QString::number(record.quantity));
    UiHelpers::setTableText(ui->detailItemsTableWidget, 5, 0, tr("Warehouse"));
    UiHelpers::setTableChipAligned(ui->detailItemsTableWidget, 5, 1, warehouse,
                                   warehouse.toLower(), Qt::AlignLeft | Qt::AlignVCenter);
    UiHelpers::setTableText(ui->detailItemsTableWidget, 6, 0, tr("Item Status"));
    UiHelpers::setTableBadgeAligned(ui->detailItemsTableWidget, 6, 1, itemStatus,
                                    statusRole(itemStatus), Qt::AlignLeft | Qt::AlignVCenter);
    UiHelpers::setTableText(ui->detailItemsTableWidget, 7, 0, tr("Order Status"));
    UiHelpers::setTableBadgeAligned(ui->detailItemsTableWidget, 7, 1, orderStatus,
                                    statusRole(orderStatus), Qt::AlignLeft | Qt::AlignVCenter);
    populateDetailField(ui->detailItemsTableWidget, 8, tr("Final Price"),
                        formatCurrency(record.finalPrice));
}

// ---------------------------------------------------------------------------
// clearDetailPanel — resets right panel to "no selection" state
// ---------------------------------------------------------------------------
void LogsPage::clearDetailPanel()
{
    ui->selectedOrderIdLabel->setText(QStringLiteral("--"));
    ui->selectedWarehouseLabel->setText(QStringLiteral("--"));
    UiHelpers::setBadge(ui->selectedWarehouseLabel, "idle");
    ui->selectedStatusLabel->setText(QStringLiteral("--"));
    UiHelpers::setBadge(ui->selectedStatusLabel, "idle");
    ui->detailItemsTableWidget->setRowCount(0);
}

// ---------------------------------------------------------------------------
// setLogs — legacy helper (6-column raw string path; unused in normal flow)
// ---------------------------------------------------------------------------
void LogsPage::setLogs(const std::vector<QStringList> &logs)
{
    // Legacy path writes the available raw fields into the item-record table.
    ui->logsTableWidget->setRowCount(static_cast<int>(logs.size()));
    for (int i = 0; i < static_cast<int>(logs.size()); ++i) {
        const QStringList &row = logs[static_cast<size_t>(i)];
        if (row.size() < 4) {
            continue;
        }
        UiHelpers::setTableText(ui->logsTableWidget, i, 0, row[0],
                                Qt::AlignRight | Qt::AlignVCenter);
        UiHelpers::setTableText(ui->logsTableWidget, i, 1, row[3]);
        UiHelpers::setTableChipAligned(ui->logsTableWidget, i, 2, QStringLiteral("--"),
                                       QStringLiteral("idle"), Qt::AlignLeft | Qt::AlignVCenter);
        UiHelpers::setTableText(ui->logsTableWidget, i, 3, QStringLiteral("--"));
        UiHelpers::setTableText(ui->logsTableWidget, i, 4, QStringLiteral("--"),
                                Qt::AlignRight | Qt::AlignVCenter);
        UiHelpers::setTableChip(ui->logsTableWidget, i, 5, row[1], row[1].toLower());
        UiHelpers::setTableBadgeAligned(ui->logsTableWidget, i, 6, row[2], statusRole(row[2]),
                                        Qt::AlignLeft | Qt::AlignVCenter);
        UiHelpers::setTableBadgeAligned(ui->logsTableWidget, i, 7, row[2], statusRole(row[2]),
                                        Qt::AlignLeft | Qt::AlignVCenter);
        UiHelpers::setTableText(ui->logsTableWidget, i, 8, QStringLiteral("--"),
                                Qt::AlignRight | Qt::AlignVCenter);
    }
}

void LogsPage::exportAllCsv()
{
    const int total = ui->logsTableWidget->rowCount();
    if (total == 0) {
        QMessageBox::information(this, tr("No Data"), tr("Run an orders batch first to generate log records."));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export All Logs"),
        QString("Analytics/Logs_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        tr("CSV Files (*.csv)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Failed"), tr("Cannot write to '%1'.").arg(path));
        return;
    }
    QTextStream out(&file);
    // Header row
    QStringList headers;
    for (int col = 0; col < ui->logsTableWidget->columnCount(); ++col)
        headers << ui->logsTableWidget->horizontalHeaderItem(col)->text();
    out << headers.join(',') << "\r\n";

    // Data rows (only visible rows)
    for (int row = 0; row < total; ++row) {
        if (ui->logsTableWidget->isRowHidden(row)) continue;
        QStringList cols;
        for (int col = 0; col < ui->logsTableWidget->columnCount(); ++col) {
            const auto *item = ui->logsTableWidget->item(row, col);
            QString val = item ? item->text() : QString();
            val.replace(',', ';');
            cols << val;
        }
        out << cols.join(',') << "\r\n";
    }
    QMessageBox::information(this, tr("Export Complete"),
        tr("Logs exported to '%1'.").arg(path));
}
