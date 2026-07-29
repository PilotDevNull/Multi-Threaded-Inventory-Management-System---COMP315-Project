#include "orderspage.h"
#include "ui_orderspage.h"
#include <QFileDialog>
#include <QDir>

#include <QFrame>
#include <QHeaderView>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSpinBox>
#include <QStyle>
#include <QTableWidget>
#include <QVBoxLayout>

#include "uihelpers.h"

namespace {

struct WarehouseCard {
    QFrame *frame;
    QLabel *title;
    QLineEdit *file;
    QPushButton *browse;
    QPushButton *start;
    QPushButton *stop;
    QLabel *status;
    QLabel *success;
    QLabel *failed;
    QString accent;
    QString titleText;
};

QString eventRole(const QString &event)
{
    const QString lowered = event.toLower();
    if (lowered.contains("reject") || lowered.contains("error") || lowered.contains("fail")) {
        return QStringLiteral("fail");
    }
    if (lowered.contains("complete") || lowered.contains("success") || lowered.contains("ready")) {
        return QStringLiteral("success");
    }
    if (lowered.contains("process") || lowered.contains("created") || lowered.contains("start") ||
        lowered.contains("running")) {
        return QStringLiteral("running");
    }
    if (lowered.contains("valid")) {
        return QStringLiteral("success");
    }
    return QStringLiteral("idle");
}

void populateConsoleRow(QTableWidget *table,
                        int row,
                        const QString &time,
                        const QString &warehouse,
                        const QString &eventTitle,
                        const QString &eventSubtitle,
                        const QString &stateText)
{
    UiHelpers::setTableText(table, row, 0, time);
    UiHelpers::setTableChip(table, row, 1, warehouse, warehouse.toLower());
    UiHelpers::setTableText(table, row, 2, eventTitle + "\n" + eventSubtitle);
    UiHelpers::setTableBadge(table, row, 3, stateText, eventRole(eventTitle));
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

void makePageScrollable(QWidget *page,
                        QWidget *fixedBottomWidget,
                        const QString &scrollAreaName,
                        const QString &scrollContentName,
                        const QMargins &margins,
                        int spacing)
{
    auto *outerLayout = qobject_cast<QVBoxLayout *>(page ? page->layout() : nullptr);
    if (!outerLayout || !fixedBottomWidget) {
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
    scrollArea->setObjectName(scrollAreaName);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setAlignment(Qt::AlignTop);
    scrollArea->viewport()->setObjectName(scrollAreaName + QStringLiteral("Viewport"));
    scrollArea->viewport()->setAttribute(Qt::WA_StyledBackground, false);

    auto *scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName(scrollContentName);
    scrollContent->setAttribute(Qt::WA_StyledBackground, false);
    scrollContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

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
        page ? page->findChild<QWidget *>(QStringLiteral("ordersPageScrollContent"))
             : nullptr;
    auto *scrollArea =
        page ? page->findChild<QScrollArea *>(QStringLiteral("ordersPageScrollArea"))
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

OrdersPage::OrdersPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OrdersPage)
{
    ui->setupUi(this);
    setupUiDefaults();
}

OrdersPage::~OrdersPage()
{
    delete ui;
}

void OrdersPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    refreshScrollableContentSize(this);
}

void OrdersPage::setupUiDefaults()
{
    makePageScrollable(this,
                       ui->bottomStatusFrame,
                       QStringLiteral("ordersPageScrollArea"),
                       QStringLiteral("ordersPageScrollContent"),
                       QMargins(30, 20, 30, 24),
                       16);
    UiHelpers::setPageLayout(this, QMargins(0, 0, 0, 0), 0);
    UiHelpers::setProperty(ui->ordersTitleLabel, "textRole", "pageTitle");

    // --- Surfaces ---
    UiHelpers::setSurface(ui->warehousesGroupBox, "card");
    UiHelpers::setSurface(ui->consoleGroupBox, "card");
    UiHelpers::setSurface(ui->manualOrderGroupBox, "card");
    UiHelpers::setSurface(ui->bottomStatusFrame, "soft");
    ui->warehousesGroupBox->setMinimumWidth(364);
    ui->centerColumnFrame->setMinimumWidth(620);

    // --- Shadows ---
    UiHelpers::applyCardShadow(ui->warehousesGroupBox, 34, 8, QColor(24, 39, 62, 22));
    UiHelpers::applyCardShadow(ui->consoleGroupBox,    34, 8, QColor(24, 39, 62, 24));
    UiHelpers::applyCardShadow(ui->manualOrderGroupBox,32, 8, QColor(24, 39, 62, 20));

    // --- Layout tuning ---
    // Warehouses: title is now an explicit QLabel inside the layout,
    // so we no longer need the 28 px top margin that was reserved for the
    // QGroupBox external title.  Use a small uniform inner margin instead.
    UiHelpers::tuneLayout(ui->warehousesGroupBox, QMargins(14, 12, 14, 12), 10);

    // Console: same reasoning – explicit header label in toolbar row
    UiHelpers::tuneLayout(ui->consoleGroupBox, QMargins(16, 12, 16, 16), 14);

    // Manual entry: same reasoning
    UiHelpers::tuneLayout(ui->manualOrderGroupBox, QMargins(16, 12, 16, 16), 14);

    UiHelpers::tuneLayout(ui->bottomStatusFrame, QMargins(30, 12, 30, 12), 12);

    // --- Card-level title labels (now explicit QLabels inside each card) ---
    UiHelpers::setProperty(ui->warehousesTitleLabel,  "textRole", "cardTitle");
    UiHelpers::setProperty(ui->consoleTitleLabel,     "textRole", "cardTitle");
    UiHelpers::setProperty(ui->manualEntryTitleLabel, "textRole", "cardTitle");

    // --- Other text roles ---
    UiHelpers::setProperty(ui->manualWarehouseLabel,      "textRole", "labelTitle");
    UiHelpers::setProperty(ui->manualHelperLabel,         "textRole", "muted");
    UiHelpers::setProperty(ui->activeThreadsLabelTitle,   "textRole", "labelTitle");
    UiHelpers::setProperty(ui->queueStatusLabelTitle,     "textRole", "labelTitle");
    UiHelpers::setProperty(ui->mutexStateLabelTitle,      "textRole", "labelTitle");
    ui->bottomStatusFrame->setAttribute(Qt::WA_StyledBackground, true);

    // --- Thread config panel buttons ---
    UiHelpers::setButtonRole(ui->submitManualOrderButton, "success");
    UiHelpers::setButtonRole(ui->clearManualOrderButton,  "secondary");

    // --- Scroll area ---
    ui->scrollArea->setFrameShape(QFrame::NoFrame);
    ui->scrollAreaWidgetContents->setAttribute(Qt::WA_StyledBackground, false);
    ui->warehousesTitleLabel->setContentsMargins(6, 0, 0, 0);

    // --- Warehouse cards ---
    const WarehouseCard cards[] = {
        {ui->w1Frame, ui->w1TitleLabel, ui->warehouse1FileLineEdit,
         ui->warehouse1BrowseButton, ui->warehouse1StartButton, ui->warehouse1StopButton,
         ui->warehouse1StatusLabel, ui->warehouse1SuccessLabel, ui->warehouse1FailedLabel,
         "w1", "W1"},
        {ui->w2Frame, ui->w2TitleLabel, ui->warehouse2FileLineEdit,
         ui->warehouse2BrowseButton, ui->warehouse2StartButton, ui->warehouse2StopButton,
         ui->warehouse2StatusLabel, ui->warehouse2SuccessLabel, ui->warehouse2FailedLabel,
         "w2", "W2"},
        {ui->w3Frame, ui->w3TitleLabel, ui->warehouse3FileLineEdit,
         ui->warehouse3BrowseButton, ui->warehouse3StartButton, ui->warehouse3StopButton,
         ui->warehouse3StatusLabel, ui->warehouse3SuccessLabel, ui->warehouse3FailedLabel,
         "w3", "W3"},
        {ui->w4Frame, ui->w4TitleLabel, ui->warehouse4FileLineEdit,
         ui->warehouse4BrowseButton, ui->warehouse4StartButton, ui->warehouse4StopButton,
         ui->warehouse4StatusLabel, ui->warehouse4SuccessLabel, ui->warehouse4FailedLabel,
         "w4", "W4"},
        {ui->w5Frame, ui->w5TitleLabel, ui->warehouse5FileLineEdit,
         ui->warehouse5BrowseButton, ui->warehouse5StartButton, ui->warehouse5StopButton,
         ui->warehouse5StatusLabel, ui->warehouse5SuccessLabel, ui->warehouse5FailedLabel,
         "w5", "W5"},
    };

    const QStringList states        = {"Running", "Idle", "Idle", "Idle", "Idle"};
    const QStringList sourceFiles   = {"final_project_v2/Orders/Warehouse1_Orders.csv",
                                       "final_project_v2/Orders/Warehouse2_Orders.csv",
                                       "final_project_v2/Orders/Warehouse3_Orders.csv",
                                       "final_project_v2/Orders/Warehouse4_Orders.csv",
                                       "final_project_v2/Orders/Warehouse5_Orders.csv"};

    for (int i = 0; i < 5; ++i) {
        const WarehouseCard &card = cards[i];
        UiHelpers::setSurface(card.frame, "warehouse", card.accent);
        UiHelpers::tuneLayout(card.frame, QMargins(14, 14, 14, 14), 10);
        UiHelpers::applyCardShadow(card.frame, 22, 6, QColor(24, 39, 62, 16));
        card.title->setText(card.titleText);
        card.title->setContentsMargins(4, 0, 0, 0);
        UiHelpers::setProperty(card.title,   "textRole", "sectionTitle");
        UiHelpers::setButtonRole(card.browse, "secondary");
        UiHelpers::setButtonRole(card.start,  "success");
        UiHelpers::setButtonRole(card.stop,   "danger");
        card.file->setText(sourceFiles[i]);
        card.browse->setEnabled(true);
        card.start->hide();
        card.stop->hide();
        card.status->setText(states[i]);
        UiHelpers::setBadgeText(card.status, QStringLiteral("Ready"), QStringLiteral("idle"));
        UiHelpers::setBadgeText(card.success, QStringLiteral("S 0"), QStringLiteral("success"));
        UiHelpers::setBadgeText(card.failed, QStringLiteral("P/F 0/0"), QStringLiteral("idle"));
        card.status->setMinimumWidth(82);
        card.success->setMinimumWidth(86);
        card.failed->setMinimumWidth(86);
        // Wire browse button
        QLineEdit *fileEdit = card.file;
        connect(card.browse, &QPushButton::clicked, this, [this, fileEdit]() {
            const QString p = QFileDialog::getOpenFileName(
                this, tr("Select Order CSV"), QString(), tr("CSV Files (*.csv);;All Files (*)"));
            if (!p.isEmpty()) fileEdit->setText(QDir::toNativeSeparators(p));
        });
    }

    // --- Order Processing Console table ---
    UiHelpers::setupReadOnlyTable(ui->orderConsoleTableWidget, 58);
    ui->orderConsoleTableWidget->setWordWrap(true);
    ui->orderConsoleTableWidget->setColumnCount(4);
    ui->orderConsoleTableWidget->setHorizontalHeaderLabels({"Time","Warehouse","Event","Status"});
    ui->orderConsoleTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->orderConsoleTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->orderConsoleTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->orderConsoleTableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->orderConsoleTableWidget->setColumnWidth(2, 258);
    ui->orderConsoleTableWidget->setColumnWidth(3, 152);
    ui->orderConsoleTableWidget->setRowCount(0);
    ui->consoleSearchLineEdit->setEnabled(true);
    ui->consoleSearchLineEdit->setPlaceholderText(QStringLiteral("Search console events…"));
    connect(ui->consoleSearchLineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int row = 0; row < ui->orderConsoleTableWidget->rowCount(); ++row) {
            bool show = text.isEmpty();
            if (!show) {
                for (int col = 0; col < ui->orderConsoleTableWidget->columnCount(); ++col) {
                    auto *item = ui->orderConsoleTableWidget->item(row, col);
                    if (item && item->text().contains(text, Qt::CaseInsensitive)) { show = true; break; }
                }
            }
            ui->orderConsoleTableWidget->setRowHidden(row, !show);
        }
    });

    // --- Thread configuration dropdown (mirrors CLI options) ---
    ui->manualWarehouseComboBox->clear();
    ui->manualWarehouseComboBox->addItem(QStringLiteral("1  - Single-threaded"),  1);
    ui->manualWarehouseComboBox->addItem(QStringLiteral("2  - Light parallel"),   2);
    ui->manualWarehouseComboBox->addItem(QStringLiteral("4  - Balanced (default)"), 4);
    ui->manualWarehouseComboBox->addItem(QStringLiteral("8  - Heavy parallel"),   8);
    ui->manualWarehouseComboBox->addItem(QStringLiteral("16 - High throughput"),  16);
    ui->manualWarehouseComboBox->addItem(QStringLiteral("32 - Max throughput"),   32);
    ui->manualWarehouseComboBox->setCurrentIndex(2); // default: 4 threads
    ui->manualWarehouseComboBox->setEnabled(true);
    ui->submitManualOrderButton->setEnabled(true);
    ui->clearManualOrderButton->setEnabled(true);

    //Manual order form labels + styling
    ui->manualProductIdLabel->show();
    ui->manualQuantityLabel->show();
    UiHelpers::setProperty(ui->manualOrderFormTitle, "textRole", "sectionTitle");
    UiHelpers::setProperty(ui->manualProductIdLabel, "textRole", "labelTitle");
    UiHelpers::setProperty(ui->manualQuantityLabel,  "textRole", "labelTitle");
    UiHelpers::setProperty(ui->manualOrderWarehouseLabel, "textRole", "labelTitle");
    UiHelpers::setProperty(ui->manualOrderQueueLabel, "textRole", "muted");
    UiHelpers::setButtonRole(ui->addManualOrderButton, "secondary");

    //Populate the warehouse dropdown for manual orders (1-5)
    ui->manualOrderWarehouseCombo->clear();
    for (int i = 1; i <= 5; ++i) {
        ui->manualOrderWarehouseCombo->addItem(
            QStringLiteral("Warehouse %1").arg(i), i);
    }

    //Wire the Queue Order button to emit the signal
    connect(ui->addManualOrderButton, &QPushButton::clicked, this, [this]() {
        const int productId = ui->manualProductIdSpinBox->value();
        const int quantity  = ui->manualQuantitySpinBox->value();
        const int warehouseId = ui->manualOrderWarehouseCombo->currentData().toInt();

        QVector<QPair<int,int>> items;
        items.append({productId, quantity});
        emit manualOrderRequested(warehouseId, items);
    });

    ui->manualHelperLabel->setText(
        QStringLiteral("Select thread count and press Start All. Queue manual orders below before starting."));

    // --- Bottom status strip ---
    ui->activeThreadsLabelTitle->setText(QStringLiteral("Total Orders"));
    ui->queueStatusLabelTitle->setText(QStringLiteral("Batch Status"));
    ui->mutexStateLabelTitle->setText(QStringLiteral("Results"));
    clearOrdersSummary();

    // --- Splitter proportions ---
    UiHelpers::softenSplitter(ui->mainSplitter);
    ui->mainSplitter->setMinimumHeight(0);
    ui->mainSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->mainSplitter->setSizes({360, 580, 260});
    ui->mainSplitter->setStretchFactor(0, 4);
    ui->mainSplitter->setStretchFactor(1, 6);
    ui->mainSplitter->setStretchFactor(2, 3);
    // Outer layout: stretch the scroll area (index 0) to fill
    if (auto *lay = qobject_cast<QVBoxLayout*>(layout())) {
        lay->setStretch(0, 1); // scroll area fills
        lay->setStretch(1, 0); // bottom status bar fixed
    }

    refreshScrollableContentSize(this);
    QMetaObject::invokeMethod(this, [this]() { refreshScrollableContentSize(this); },
                              Qt::QueuedConnection);
}

void OrdersPage::setWarehouseSummary(int warehouseId,
                                     const QString &statusText,
                                     const QString &statusRole,
                                     const QString &successText,
                                     const QString &successRole,
                                     const QString &failedText,
                                     const QString &failedRole)
{
    QLabel *statusLabel = nullptr;
    QLabel *successLabel = nullptr;
    QLabel *failedLabel = nullptr;

    switch (warehouseId) {
    case 1:
        statusLabel = ui->warehouse1StatusLabel;
        successLabel = ui->warehouse1SuccessLabel;
        failedLabel = ui->warehouse1FailedLabel;
        break;
    case 2:
        statusLabel = ui->warehouse2StatusLabel;
        successLabel = ui->warehouse2SuccessLabel;
        failedLabel = ui->warehouse2FailedLabel;
        break;
    case 3:
        statusLabel = ui->warehouse3StatusLabel;
        successLabel = ui->warehouse3SuccessLabel;
        failedLabel = ui->warehouse3FailedLabel;
        break;
    case 4:
        statusLabel = ui->warehouse4StatusLabel;
        successLabel = ui->warehouse4SuccessLabel;
        failedLabel = ui->warehouse4FailedLabel;
        break;
    case 5:
        statusLabel = ui->warehouse5StatusLabel;
        successLabel = ui->warehouse5SuccessLabel;
        failedLabel = ui->warehouse5FailedLabel;
        break;
    default:
        return;
    }

    UiHelpers::setBadgeText(statusLabel, statusText, statusRole);
    UiHelpers::setBadgeText(successLabel, successText, successRole);
    UiHelpers::setBadgeText(failedLabel, failedText, failedRole);
}

void OrdersPage::setBatchRunning(bool running)
{
    if (running) {
        setBatchStatus(QStringLiteral("Processing"), QStringLiteral("running"));
        for (int warehouseId = 1; warehouseId <= 5; ++warehouseId) {
            setWarehouseSummary(warehouseId,
                                QStringLiteral("Batch running"),
                                QStringLiteral("running"),
                                QStringLiteral("S 0"),
                                QStringLiteral("success"),
                                QStringLiteral("P/F 0/0"),
                                QStringLiteral("idle"));
        }
        return;
    }

    if (ui->queueStatusLabel->text() == QStringLiteral("Processing")) {
        setBatchStatus(QStringLiteral("Ready"), QStringLiteral("idle"));
    }
}

void OrdersPage::setBatchStatus(const QString &text, const QString &badgeRole)
{
    UiHelpers::setBadgeText(ui->queueStatusLabel, text, badgeRole);
}

void OrdersPage::setOrdersSummary(const OrdersSummaryDto &summary)
{
    UiHelpers::setBadgeText(ui->activeThreadsLabel,
                            QString::number(summary.totalOrders),
                            summary.totalOrders > 0 ? QStringLiteral("success")
                                                    : QStringLiteral("idle"));
    UiHelpers::setBadgeText(ui->queueStatusLabel, QStringLiteral("Completed"), QStringLiteral("success"));

    const QString resultsText = QStringLiteral("S %1 | P %2 | F %3")
                                    .arg(summary.successCount)
                                    .arg(summary.partialCount)
                                    .arg(summary.failedCount);
    QString resultsRole = QStringLiteral("success");
    if (summary.failedCount > 0 && summary.successCount == 0 && summary.partialCount == 0) {
        resultsRole = QStringLiteral("fail");
    } else if (summary.failedCount > 0 || summary.partialCount > 0) {
        resultsRole = QStringLiteral("warning");
    }
    UiHelpers::setBadgeText(ui->mutexStateLabel, resultsText, resultsRole);

    for (int warehouseId = 1; warehouseId <= 5; ++warehouseId) {
        OrdersWarehouseCountDto counts;
        counts.warehouseId = warehouseId;

        for (const OrdersWarehouseCountDto &entry : summary.perWarehouseCounts) {
            if (entry.warehouseId == warehouseId) {
                counts = entry;
                break;
            }
        }

        const QString statusText =
            counts.totalOrders > 0 ? QStringLiteral("Completed") : QStringLiteral("No orders");
        const QString statusRole =
            counts.totalOrders > 0 ? QStringLiteral("success") : QStringLiteral("idle");
        const QString failedText =
            QStringLiteral("P/F %1/%2").arg(counts.partialCount).arg(counts.failedCount);
        const QString failedRole =
            counts.failedCount > 0
                ? QStringLiteral("fail")
                : (counts.partialCount > 0 ? QStringLiteral("warning") : QStringLiteral("idle"));

        setWarehouseSummary(warehouseId,
                            statusText,
                            statusRole,
                            QStringLiteral("S %1").arg(counts.successCount),
                            QStringLiteral("success"),
                            failedText,
                            failedRole);
    }
}

void OrdersPage::clearOrdersSummary()
{
    UiHelpers::setBadgeText(ui->activeThreadsLabel, QStringLiteral("0"), QStringLiteral("idle"));
    UiHelpers::setBadgeText(ui->queueStatusLabel, QStringLiteral("Ready"), QStringLiteral("idle"));
    UiHelpers::setBadgeText(ui->mutexStateLabel, QStringLiteral("S 0 | P 0 | F 0"), QStringLiteral("idle"));

    for (int warehouseId = 1; warehouseId <= 5; ++warehouseId) {
        setWarehouseSummary(warehouseId,
                            QStringLiteral("Ready"),
                            QStringLiteral("idle"),
                            QStringLiteral("S 0"),
                            QStringLiteral("success"),
                            QStringLiteral("P/F 0/0"),
                            QStringLiteral("idle"));
    }
}

void OrdersPage::updateQueueCount(int count)
{
    ui->manualOrderQueueLabel->setText(
        QStringLiteral("%1 order%2 queued")
            .arg(count)
            .arg(count == 1 ? "" : "s"));
}

void OrdersPage::appendConsoleEvent(const QString &time,
                                    const QString &warehouse,
                                    const QString &event,
                                    const QString &message,
                                    const QString &state)
{
    const int row = ui->orderConsoleTableWidget->rowCount();
    ui->orderConsoleTableWidget->insertRow(row);
    const QString stateText = state.isEmpty() ? event : state;
    populateConsoleRow(ui->orderConsoleTableWidget, row, time, warehouse, event, message, stateText);
    ui->orderConsoleTableWidget->scrollToBottom();
}
