#include "analyticspage.h"
#include "ui_analyticspage.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSplitter>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>
#include <algorithm>

#include <QtCharts/QAbstractAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLegend>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QValueAxis>

#include <QColor>
#include <QFont>
#include <QFrame>
#include <QGraphicsLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QLocale>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

#include "uihelpers.h"

namespace {

QString ordersByWarehouseEmptyMessage()
{
    return QStringLiteral("Run an orders batch to generate captured warehouse revenue.");
}

QString statusBreakdownEmptyMessage()
{
    return QStringLiteral("Run an orders batch to generate status breakdown.");
}

QColor chartTextColor()
{
    return QColor(QStringLiteral("#516074"));
}

QColor chartAxisLineColor()
{
    return QColor(QStringLiteral("#D9E2EF"));
}

QColor chartGridColor()
{
    return QColor(QStringLiteral("#E7EDF6"));
}

QColor statusColor(const QString &status)
{
    if (status == QStringLiteral("SUCCESS")) {
        return QColor(QStringLiteral("#37B778"));
    }
    if (status == QStringLiteral("PARTIAL")) {
        return QColor(QStringLiteral("#F2A53C"));
    }
    if (status == QStringLiteral("FAILED")) {
        return QColor(QStringLiteral("#E5655B"));
    }

    return QColor(QStringLiteral("#94A3B8"));
}

void populateSuccessRow(QTableWidget *table,
                        int row,
                        const QString &orderId,
                        const QString &warehouse,
                        const QString &lineItems,
                        const QString &requestedUnits,
                        const QString &capturedUnits)
{
    UiHelpers::setTableText(table, row, 0, orderId);
    UiHelpers::setTableChip(table, row, 1, warehouse, warehouse.toLower());
    UiHelpers::setTableText(table, row, 2, lineItems, Qt::AlignRight | Qt::AlignVCenter);
    UiHelpers::setTableText(table, row, 3, requestedUnits, Qt::AlignRight | Qt::AlignVCenter);
    UiHelpers::setTableText(table, row, 4, capturedUnits, Qt::AlignRight | Qt::AlignVCenter);
}

void populateNonSuccessRow(QTableWidget *table,
                           int row,
                           const QString &orderId,
                           const QString &warehouse,
                           const QString &status,
                           const QString &requestedUnits,
                           const QString &capturedUnits)
{
    const QString statusRole = (status == QStringLiteral("PARTIAL"))
            ? QStringLiteral("warning")
            : QStringLiteral("fail");

    UiHelpers::setTableText(table, row, 0, orderId);
    UiHelpers::setTableChip(table, row, 1, warehouse, warehouse.toLower());
    UiHelpers::setTableBadge(table, row, 2, status, statusRole);
    UiHelpers::setTableText(table, row, 3, requestedUnits, Qt::AlignRight | Qt::AlignVCenter);
    UiHelpers::setTableText(table, row, 4, capturedUnits, Qt::AlignRight | Qt::AlignVCenter);
}

QString formatCurrency(double value)
{
    return QStringLiteral("R %1").arg(QLocale().toString(value, 'f', 2));
}

int visibleTableHeight(const QTableWidget *table)
{
    if (!table) {
        return 0;
    }

    int height = table->horizontalHeader() ? table->horizontalHeader()->height() : 0;
    for (int row = 0; row < table->rowCount(); ++row) {
        height += table->rowHeight(row);
    }

    if (table->horizontalScrollBar() && table->horizontalScrollBar()->isVisible()) {
        height += table->horizontalScrollBar()->height();
    }

    return height + (table->frameWidth() * 2) + 2;
}

QChart *createBaseChart()
{
    auto *chart = new QChart();
    chart->setTheme(QChart::ChartThemeLight);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);
    chart->setMargins(QMargins(16, 10, 16, 10));
    if (chart->layout()) {
        chart->layout()->setContentsMargins(0, 0, 0, 0);
    }
    chart->legend()->setFont(QFont(QStringLiteral("Segoe UI Variable Text"), 9));
    chart->legend()->setColor(chartTextColor());
    return chart;
}

QChartView *createChartView(QWidget *parent)
{
    auto *chartView = new QChartView(parent);
    chartView->setObjectName(QStringLiteral("analyticsChartView"));
    chartView->setFrameShape(QFrame::NoFrame);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->setMinimumHeight(230);
    chartView->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    chartView->setChart(createBaseChart());
    chartView->hide();
    return chartView;
}

QLabel *createEmptyStateLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setMargin(14);
    UiHelpers::setTextRole(label, "pageMeta");
    return label;
}

void replaceChart(QChartView *chartView, QChart *chart)
{
    if (!chartView || !chart) {
        return;
    }

    QChart *oldChart = chartView->chart();
    chartView->setChart(chart);
    if (oldChart) {
        oldChart->deleteLater();
    }
}

void styleAxis(QAbstractAxis *axis, bool showGridLines)
{
    if (!axis) {
        return;
    }

    const QColor textColor = chartTextColor();
    axis->setLabelsColor(textColor);
    axis->setLinePenColor(chartAxisLineColor());
    axis->setGridLineColor(chartGridColor());
    axis->setGridLineVisible(showGridLines);
    axis->setMinorGridLineVisible(false);
    axis->setShadesVisible(false);
}

QVector<AnalyticsSnapshotDto::StatusStats> orderedStatusBreakdown(
        const AnalyticsSnapshotDto &snapshot)
{
    QVector<AnalyticsSnapshotDto::StatusStats> orderedRows;
    const QStringList preferredOrder = {
        QStringLiteral("SUCCESS"),
        QStringLiteral("PARTIAL"),
        QStringLiteral("FAILED")
    };

    for (const QString &status : preferredOrder) {
        const auto it = std::find_if(snapshot.statusBreakdown.cbegin(),
                                     snapshot.statusBreakdown.cend(),
                                     [&status](const AnalyticsSnapshotDto::StatusStats &row) {
            return row.status == status && row.count > 0;
        });
        if (it != snapshot.statusBreakdown.cend()) {
            orderedRows.append(*it);
        }
    }

    for (const AnalyticsSnapshotDto::StatusStats &row : snapshot.statusBreakdown) {
        if (row.count <= 0 || preferredOrder.contains(row.status)) {
            continue;
        }
        orderedRows.append(row);
    }

    return orderedRows;
}

} // namespace

AnalyticsPage::AnalyticsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AnalyticsPage)
{
    ui->setupUi(this);
    setupUiDefaults();
}

AnalyticsPage::~AnalyticsPage()
{
    delete ui;
}

void AnalyticsPage::setupUiDefaults()
{
    UiHelpers::setPageLayout(this, QMargins(0, 0, 0, 0), 0);
    UiHelpers::tuneLayout(ui->analyticsScrollContent, QMargins(30, 28, 30, 24), 18);

    UiHelpers::setProperty(ui->analyticsTitleLabel, "textRole", "pageTitle");
    ui->analyticsTitleLabel->setText(QStringLiteral("Batch Analytics"));

    UiHelpers::setSurface(ui->kpiFrame1, "metric", "blue");
    UiHelpers::setSurface(ui->kpiFrame2, "metric", "teal");
    UiHelpers::setSurface(ui->kpiFrame3, "metric", "amber");
    UiHelpers::setSurface(ui->kpiFrame4, "metric", "slate");
    UiHelpers::setSurface(ui->kpiFrame5, "metric", "slate");

    UiHelpers::setSurface(ui->successGroupBox, "card");
    UiHelpers::setSurface(ui->failGroupBox, "card");
    UiHelpers::setSurface(ui->revenueByWarehouseFrame, "card");
    UiHelpers::setSurface(ui->failureReasonsFrame, "card");

    UiHelpers::applyCardShadow(ui->kpiFrame1, 30, 8, QColor(24, 39, 62, 22));
    UiHelpers::applyCardShadow(ui->kpiFrame2, 30, 8, QColor(24, 39, 62, 22));
    UiHelpers::applyCardShadow(ui->kpiFrame3, 30, 8, QColor(24, 39, 62, 22));
    UiHelpers::applyCardShadow(ui->kpiFrame4, 30, 8, QColor(24, 39, 62, 20));
    UiHelpers::applyCardShadow(ui->kpiFrame5, 30, 8, QColor(24, 39, 62, 20));
    UiHelpers::applyCardShadow(ui->successGroupBox, 34, 8, QColor(24, 39, 62, 24));
    UiHelpers::applyCardShadow(ui->failGroupBox, 34, 8, QColor(24, 39, 62, 24));
    UiHelpers::applyCardShadow(ui->revenueByWarehouseFrame, 32, 8, QColor(24, 39, 62, 20));
    UiHelpers::applyCardShadow(ui->failureReasonsFrame, 32, 8, QColor(24, 39, 62, 20));

    UiHelpers::tuneLayout(ui->kpiFrame1, QMargins(20, 18, 20, 18), 8);
    UiHelpers::tuneLayout(ui->kpiFrame2, QMargins(20, 18, 20, 18), 8);
    UiHelpers::tuneLayout(ui->kpiFrame3, QMargins(20, 18, 20, 18), 8);
    UiHelpers::tuneLayout(ui->kpiFrame4, QMargins(20, 18, 20, 18), 8);
    UiHelpers::tuneLayout(ui->kpiFrame5, QMargins(20, 18, 20, 18), 8);
    UiHelpers::tuneLayout(ui->successGroupBox, QMargins(16, 12, 16, 16), 12);
    UiHelpers::tuneLayout(ui->failGroupBox, QMargins(16, 12, 16, 16), 12);
    UiHelpers::tuneLayout(ui->revenueByWarehouseFrame, QMargins(18, 12, 18, 18), 12);
    UiHelpers::tuneLayout(ui->failureReasonsFrame, QMargins(18, 12, 18, 18), 12);
    UiHelpers::tuneLayout(ui->footerFrame, QMargins(30, 12, 30, 12), 12);

    UiHelpers::setProperty(ui->successSummaryTitleLabel, "textRole", "cardTitle");
    UiHelpers::setProperty(ui->failSummaryTitleLabel, "textRole", "cardTitle");
    UiHelpers::setProperty(ui->revenueByWarehouseTitleLabel, "textRole", "cardTitle");
    UiHelpers::setProperty(ui->failureReasonsTitleLabel, "textRole", "cardTitle");

    ui->successSummaryTitleLabel->setText(QStringLiteral("Successful Orders"));
    ui->failSummaryTitleLabel->setText(QStringLiteral("Non-Success Orders"));
    ui->revenueByWarehouseTitleLabel->setText(QStringLiteral("Captured Revenue by Warehouse"));
    ui->failureReasonsTitleLabel->setText(QStringLiteral("Status Breakdown"));

    const QList<QLabel *> metricTitles = {
        ui->kpiTitle1, ui->kpiTitle2, ui->kpiTitle3, ui->kpiTitle4, ui->kpiTitle5
    };
    for (QLabel *label : metricTitles) {
        UiHelpers::setProperty(label, "textRole", "metricTitle");
    }

    ui->kpiTitle1->setText(QStringLiteral("Total Orders"));
    ui->kpiTitle2->setText(QStringLiteral("Successful Orders"));
    ui->kpiTitle3->setText(QStringLiteral("Failed Orders"));
    ui->kpiTitle4->setText(QStringLiteral("Captured Revenue"));
    ui->kpiTitle5->setText(QStringLiteral("Captured Lost Revenue"));
    ui->kpiTitle4->setToolTip(
        tr("Revenue from successful v2 AnalyticsManager item records captured after the batch."));
    ui->kpiTitle5->setToolTip(
        tr("Lost revenue from failed v2 AnalyticsManager item records captured after the batch."));

    UiHelpers::setProperty(ui->lastReportGeneratedLabelTitle, "textRole", "labelTitle");
    UiHelpers::setProperty(ui->exportReadyLabelTitle, "textRole", "labelTitle");
    ui->lastReportGeneratedLabelTitle->setText(QStringLiteral("Snapshot Generated"));
    ui->exportReadyLabelTitle->setText(QStringLiteral("Export"));

    ui->analyticsScrollArea->setWidgetResizable(true);
    ui->analyticsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->analyticsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    ui->revenueChartCanvasFrame->setAttribute(Qt::WA_StyledBackground, true);
    ui->failureChartCanvasFrame->setAttribute(Qt::WA_StyledBackground, true);
    ui->footerFrame->setAttribute(Qt::WA_StyledBackground, true);
    ui->revenueByWarehouseFrame->setMinimumHeight(260);
    ui->failureReasonsFrame->setMinimumHeight(260);
    ui->horizontalLayout_charts->setStretch(0, 1);
    ui->horizontalLayout_charts->setStretch(1, 1);

    //Set minimum widths on KPI frames so they dont collapse into each other
    //The scroll area will handle overflow if the window is too narrow
    for (QFrame *kpi : {ui->kpiFrame1, ui->kpiFrame2, ui->kpiFrame3, ui->kpiFrame4, ui->kpiFrame5}) {
        kpi->setMinimumWidth(140);
        kpi->setMinimumHeight(120);
    }

    //Make sure the scroll content can actually grow taller than the viewport
    //This is the key fix - without it the content gets clipped to the viewport height
    ui->analyticsScrollContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    UiHelpers::setupReadOnlyTable(ui->successfulOrdersTableWidget, 46);
    ui->successfulOrdersTableWidget->setColumnCount(5);
    ui->successfulOrdersTableWidget->setHorizontalHeaderLabels(
        {"Order ID", "Warehouse", "Line Items", "Requested", "Captured"});
    ui->successfulOrdersTableWidget->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    ui->successfulOrdersTableWidget->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    ui->successfulOrdersTableWidget->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    ui->successfulOrdersTableWidget->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::ResizeToContents);
    ui->successfulOrdersTableWidget->horizontalHeader()->setSectionResizeMode(
        4, QHeaderView::Stretch);
    ui->successfulOrdersTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->successfulOrdersTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->successfulOrdersTableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->successfulOrdersTableWidget->setRowCount(0);

    UiHelpers::setupReadOnlyTable(ui->unsuccessfulOrdersTableWidget, 46);
    ui->unsuccessfulOrdersTableWidget->setColumnCount(5);
    ui->unsuccessfulOrdersTableWidget->setHorizontalHeaderLabels(
        {"Order ID", "Warehouse", "Status", "Requested", "Captured"});
    ui->unsuccessfulOrdersTableWidget->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    ui->unsuccessfulOrdersTableWidget->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    ui->unsuccessfulOrdersTableWidget->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    ui->unsuccessfulOrdersTableWidget->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::ResizeToContents);
    ui->unsuccessfulOrdersTableWidget->horizontalHeader()->setSectionResizeMode(
        4, QHeaderView::Stretch);
    ui->unsuccessfulOrdersTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->unsuccessfulOrdersTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->unsuccessfulOrdersTableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->unsuccessfulOrdersTableWidget->setRowCount(0);

    updateTableSizing();
    setupChartHosts();
    resetAnalyticsDisplay();
}

void AnalyticsPage::setupChartHosts()
{
    ui->verticalLayout_revenueChartCanvas->setContentsMargins(8, 4, 8, 4);
    ui->verticalLayout_failureChartCanvas->setContentsMargins(8, 4, 8, 4);

    m_ordersByWarehouseEmptyLabel = createEmptyStateLabel(
        ordersByWarehouseEmptyMessage(), ui->revenueChartCanvasFrame);
    m_statusBreakdownEmptyLabel = createEmptyStateLabel(
        statusBreakdownEmptyMessage(), ui->failureChartCanvasFrame);

    m_ordersByWarehouseChartView = createChartView(ui->revenueChartCanvasFrame);
    m_statusBreakdownChartView = createChartView(ui->failureChartCanvasFrame);

    ui->verticalLayout_revenueChartCanvas->addWidget(m_ordersByWarehouseEmptyLabel, 1);
    ui->verticalLayout_revenueChartCanvas->addWidget(m_ordersByWarehouseChartView, 1);
    ui->verticalLayout_failureChartCanvas->addWidget(m_statusBreakdownEmptyLabel, 1);
    ui->verticalLayout_failureChartCanvas->addWidget(m_statusBreakdownChartView, 1);
}

void AnalyticsPage::updateTableSizing()
{
    //Let the tables show all their rows without clipping.
    //We still cap them at a reasonable max so the page doesnt get
    //infinitely tall with thousands of rows. But we never set
    //maximumHeight smaller than the content because that causes cutoff.
    const int successTableHeight = visibleTableHeight(ui->successfulOrdersTableWidget);
    const int failureTableHeight = visibleTableHeight(ui->unsuccessfulOrdersTableWidget);

    //Let tables grow to fit their content, cap at 400 so we dont blow up
    const int maxTableH = 400;
    ui->successfulOrdersTableWidget->setMinimumHeight(qMin(successTableHeight, maxTableH));
    ui->successfulOrdersTableWidget->setMaximumHeight(maxTableH);
    ui->unsuccessfulOrdersTableWidget->setMinimumHeight(qMin(failureTableHeight, maxTableH));
    ui->unsuccessfulOrdersTableWidget->setMaximumHeight(maxTableH);

    //If content exceeds the cap, enable scrollbars so nothing gets cut off
    if (successTableHeight > maxTableH) {
        ui->successfulOrdersTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    if (failureTableHeight > maxTableH) {
        ui->unsuccessfulOrdersTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }

    //Dont lock the splitter height, let it size naturally in the scroll area
    ui->successGroupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    ui->failGroupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    ui->tablesSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->tablesSplitter->setMinimumHeight(0);
    ui->tablesSplitter->setMaximumHeight(QWIDGETSIZE_MAX);

    UiHelpers::softenSplitter(ui->tablesSplitter);
    ui->tablesSplitter->setSizes({610, 530});
}

void AnalyticsPage::showOrdersByWarehouseEmptyState(const QString &message)
{
    if (m_ordersByWarehouseEmptyLabel) {
        m_ordersByWarehouseEmptyLabel->setText(message);
        m_ordersByWarehouseEmptyLabel->show();
    }
    if (m_ordersByWarehouseChartView) {
        replaceChart(m_ordersByWarehouseChartView, createBaseChart());
        m_ordersByWarehouseChartView->hide();
    }
}

void AnalyticsPage::showStatusBreakdownEmptyState(const QString &message)
{
    if (m_statusBreakdownEmptyLabel) {
        m_statusBreakdownEmptyLabel->setText(message);
        m_statusBreakdownEmptyLabel->show();
    }
    if (m_statusBreakdownChartView) {
        replaceChart(m_statusBreakdownChartView, createBaseChart());
        m_statusBreakdownChartView->hide();
    }
}

void AnalyticsPage::resetAnalyticsDisplay()
{
    ui->totalOrdersValueLabel->setText(QStringLiteral("-"));
    ui->successfulOrdersValueLabel->setText(QStringLiteral("-"));
    ui->failedOrdersValueLabel->setText(QStringLiteral("-"));
    ui->totalRevenueValueLabel->setText(QStringLiteral("-"));
    ui->avgOrderValueValueLabel->setText(QStringLiteral("-"));

    ui->successfulOrdersTableWidget->setRowCount(0);
    ui->unsuccessfulOrdersTableWidget->setRowCount(0);
    updateTableSizing();

    showOrdersByWarehouseEmptyState(ordersByWarehouseEmptyMessage());
    showStatusBreakdownEmptyState(statusBreakdownEmptyMessage());

    ui->lastReportGeneratedLabel->setText(QStringLiteral("-"));
    ui->lastReportGeneratedLabel->setToolTip(QString());
    // Replace the static export label with a real Export button
    UiHelpers::setBadgeText(ui->exportReadyLabel, tr("Ready after batch"), "info");

    setupExtendedAnalyticsTab();
}

void AnalyticsPage::updateOrdersByWarehouseChart(const AnalyticsSnapshotDto &snapshot)
{
    const auto hasCapturedWarehouseData = std::any_of(
        snapshot.warehouseStats.cbegin(),
        snapshot.warehouseStats.cend(),
        [](const AnalyticsSnapshotDto::WarehouseStats &stats) {
            return stats.capturedUnits > 0
                   || stats.revenue > 0.0
                   || stats.lostRevenue > 0.0;
        });

    if (snapshot.warehouseStats.isEmpty() || !hasCapturedWarehouseData) {
        showOrdersByWarehouseEmptyState(ordersByWarehouseEmptyMessage());
        return;
    }

    auto *series = new QBarSeries();
    auto *revenueSet = new QBarSet(QStringLiteral("Captured Revenue"));
    revenueSet->setColor(QColor(QStringLiteral("#37B778")));
    revenueSet->setBorderColor(QColor(QStringLiteral("#37B778")));

    auto *lostRevenueSet = new QBarSet(QStringLiteral("Captured Lost Revenue"));
    lostRevenueSet->setColor(QColor(QStringLiteral("#E5655B")));
    lostRevenueSet->setBorderColor(QColor(QStringLiteral("#E5655B")));

    QStringList warehouseLabels;
    double maxRevenue = 0.0;

    for (const AnalyticsSnapshotDto::WarehouseStats &stats : snapshot.warehouseStats) {
        warehouseLabels.append(QStringLiteral("W%1").arg(stats.warehouseId));
        *revenueSet << stats.revenue;
        *lostRevenueSet << stats.lostRevenue;
        maxRevenue = std::max(maxRevenue, std::max(stats.revenue, stats.lostRevenue));
    }

    series->append(revenueSet);
    series->append(lostRevenueSet);
    series->setLabelsVisible(true);
    series->setLabelsFormat(QStringLiteral("@value"));
    series->setBarWidth(0.58);

    auto *chart = createBaseChart();
    chart->addSeries(series);
    chart->legend()->setAlignment(Qt::AlignBottom);

    auto *axisX = new QBarCategoryAxis(chart);
    axisX->append(warehouseLabels);
    styleAxis(axisX, false);

    auto *axisY = new QValueAxis(chart);
    axisY->setLabelFormat(QStringLiteral("%.0f"));
    axisY->setRange(0.0, std::max(1.0, maxRevenue));
    axisY->applyNiceNumbers();
    styleAxis(axisY, true);

    QFont axisFont = font();
    axisFont.setPointSizeF(std::max(8.5, axisFont.pointSizeF() - 0.5));
    axisX->setLabelsFont(axisFont);
    axisY->setLabelsFont(axisFont);

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    replaceChart(m_ordersByWarehouseChartView, chart);
    m_ordersByWarehouseEmptyLabel->hide();
    m_ordersByWarehouseChartView->show();
}

void AnalyticsPage::updateStatusBreakdownChart(const AnalyticsSnapshotDto &snapshot)
{
    const QVector<AnalyticsSnapshotDto::StatusStats> rows = orderedStatusBreakdown(snapshot);
    if (rows.isEmpty()) {
        showStatusBreakdownEmptyState(statusBreakdownEmptyMessage());
        return;
    }

    auto *series = new QPieSeries();
    series->setHoleSize(0.50);
    series->setPieSize(0.60);
    series->setLabelsVisible(true);
    series->setLabelsPosition(QPieSlice::LabelOutside);

    for (const AnalyticsSnapshotDto::StatusStats &row : rows) {
        auto *slice = series->append(row.status, row.count);
        slice->setBrush(statusColor(row.status));
        slice->setBorderColor(QColor(QStringLiteral("#FFFFFF")));
        slice->setLabel(QStringLiteral("%1 (%2)").arg(row.status).arg(row.count));
        slice->setLabelVisible(true);
        slice->setLabelBrush(chartTextColor());
        slice->setLabelArmLengthFactor(0.14);
    }

    auto *chart = createBaseChart();
    chart->addSeries(series);
    chart->legend()->hide();

    replaceChart(m_statusBreakdownChartView, chart);
    m_statusBreakdownEmptyLabel->hide();
    m_statusBreakdownChartView->show();
}

void AnalyticsPage::setAnalyticsSnapshot(const AnalyticsSnapshotDto &snapshot)
{
    if (!snapshot.hasData) {
        resetAnalyticsDisplay();
        return;
    }

    ui->totalOrdersValueLabel->setText(QString::number(snapshot.totalOrders));
    ui->successfulOrdersValueLabel->setText(QString::number(snapshot.successCount));
    ui->failedOrdersValueLabel->setText(QString::number(snapshot.failedCount));
    ui->totalRevenueValueLabel->setText(formatCurrency(snapshot.globalRevenue));
    ui->avgOrderValueValueLabel->setText(formatCurrency(snapshot.globalLostRevenue));

    ui->successfulOrdersTableWidget->setRowCount(snapshot.successRows.size());
    for (int i = 0; i < snapshot.successRows.size(); ++i) {
        const AnalyticsSnapshotDto::SuccessRow &row = snapshot.successRows[i];
        populateSuccessRow(ui->successfulOrdersTableWidget,
                           i,
                           QString::number(row.orderId),
                           QStringLiteral("W%1").arg(row.warehouseId),
                           QString::number(row.lineItems),
                           QString::number(row.requestedUnits),
                           QString::number(row.capturedUnits));
    }

    ui->unsuccessfulOrdersTableWidget->setRowCount(snapshot.nonSuccessRows.size());
    for (int i = 0; i < snapshot.nonSuccessRows.size(); ++i) {
        const AnalyticsSnapshotDto::NonSuccessRow &row = snapshot.nonSuccessRows[i];
        populateNonSuccessRow(ui->unsuccessfulOrdersTableWidget,
                              i,
                              QString::number(row.orderId),
                              QStringLiteral("W%1").arg(row.warehouseId),
                              row.status,
                              QString::number(row.requestedUnits),
                              QString::number(row.capturedUnits));
    }

    updateTableSizing();
    updateOrdersByWarehouseChart(snapshot);
    updateStatusBreakdownChart(snapshot);

    if (snapshot.generatedAt.isValid()) {
        ui->lastReportGeneratedLabel->setText(
            snapshot.generatedAt.toString(QStringLiteral("dd MMM yyyy, HH:mm:ss")));
    } else {
        ui->lastReportGeneratedLabel->setText(QStringLiteral("-"));
    }
    ui->lastReportGeneratedLabel->setToolTip(
        tr("Captured %1 of %2 expected item analytics records.")
            .arg(snapshot.capturedLineItems)
            .arg(snapshot.expectedLineItems));

    UiHelpers::setBadgeText(ui->exportReadyLabel, tr("CSV Export Available"), "success");
    m_snapshot = snapshot;
    setupExtendedAnalyticsTab();
    refreshExtendedGlobal();
    if (m_warehouseCombo && m_warehouseCombo->currentIndex() >= 0)
        refreshExtendedWarehouse(m_warehouseCombo->currentIndex() + 1);
}

// ============================================================
// EXTENDED ANALYTICS — helper factories
// ============================================================



// ============================================================
// EXTENDED ANALYTICS — tab setup
// ============================================================
// COLOUR PALETTE — matches reference dashboard
// ============================================================
static const QList<QColor> kPalette = {
    QColor(0x6B,0x46,0xC1),  // purple
    QColor(0x06,0x95,0x97),  // teal
    QColor(0x22,0x8B,0xE6),  // blue
    QColor(0xE6,0x7E,0x22),  // orange
    QColor(0xE9,0x1E,0x63),  // pink
    QColor(0x2E,0xCC,0x71),  // green
    QColor(0xF3,0x9C,0x12),  // amber
    QColor(0x9B,0x59,0xB6),  // violet
    QColor(0x16,0xA0,0x85),  // dark teal
    QColor(0xE7,0x4C,0x3C),  // red
    QColor(0x34,0x98,0xDB),  // sky blue
};

// ── helper: colour for a bar set index ──────────────────────
static QColor kColor(int i) { return kPalette[i % kPalette.size()]; }

// ── helper: small coloured "stat card" ──────────────────────
static QWidget* makeStatCard(const QString &title, const QString &value,
                              const QColor &bg, QWidget *parent)
{
    auto *card = new QWidget(parent);
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet(QString(
        "QWidget { background: %1; border-radius: 12px; }"
        "QLabel  { background: transparent; color: white; border: none; }"
    ).arg(bg.name()));
    card->setMinimumHeight(88);
    auto *vl = new QVBoxLayout(card);
    vl->setContentsMargins(18, 14, 18, 14);
    vl->setSpacing(6);
    auto *lbl = new QLabel(title, card);
    lbl->setStyleSheet("font-size: 11px; font-weight: 600; letter-spacing: 1px; opacity: 0.85;");
    auto *val = new QLabel(value, card);
    val->setStyleSheet("font-size: 22px; font-weight: bold;");
    val->setObjectName("statVal_" + title.simplified().replace(' ','_'));
    vl->addWidget(lbl);
    vl->addWidget(val);
    return card;
}

// ── helper: section heading ──────────────────────────────────
static QLabel* makeSectionHeading(const QString &text, QWidget *parent)
{
    auto *lbl = new QLabel(text, parent);
    lbl->setStyleSheet("font-size: 15px; font-weight: bold; color: #111827; padding-top: 8px;");
    return lbl;
}

// ── helper: styled table ─────────────────────────────────────
static QTableWidget* makeRichTable(const QStringList &headers, int rowHeight = 36)
{
    auto *t = new QTableWidget();
    t->setColumnCount(headers.size());
    t->setHorizontalHeaderLabels(headers);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    t->verticalHeader()->setVisible(false);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setAlternatingRowColors(true);
    t->setShowGrid(false);
    t->setFrameShape(QFrame::NoFrame);
    t->verticalHeader()->setDefaultSectionSize(rowHeight);
    t->setStyleSheet(
        "QTableWidget { background: white; border: none; alternate-background-color: #F8FAFF; }"
        "QTableWidget::item { padding: 4px 10px; color: #374151; border-bottom: 1px solid #F0F4F8; }"
        "QTableWidget::item:selected { background: #EFF6FF; color: #1D4ED8; }"
        "QHeaderView::section { background: #F0F4FF; color: #4B5563; font-size: 11px; font-weight: 700; "
        "  letter-spacing: 0.5px; padding: 8px 10px; border: none; border-bottom: 2px solid #E0E7FF; }"
    );
    return t;
}

// ── helper: colour-coded badge item ─────────────────────────
static QTableWidgetItem* badgeItem(const QString &text, const QColor &fg)
{
    auto *item = new QTableWidgetItem(text);
    item->setForeground(fg);
    QFont f = item->font();
    f.setBold(true);
    item->setFont(f);
    return item;
}

// ── helper: fill a colourful stacked bar chart ───────────────
static void fillGroupedBarChart(QChartView *view,
    const QStringList &categories,
    const QList<QPair<QString,QList<double>>> &series,
    const QString &yFormat = "%.0f")
{
    auto *chart = view->chart();
    chart->removeAllSeries();
    for (auto *ax : chart->axes()) chart->removeAxis(ax);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto *axX = new QBarCategoryAxis();
    axX->append(categories);
    axX->setLabelsFont(QFont("Segoe UI Variable Text", 8));
    auto *axY = new QValueAxis();
    axY->setLabelFormat(yFormat);
    axY->setLabelsFont(QFont("Segoe UI Variable Text", 8));
    axY->setGridLineColor(QColor(240,244,255));
    chart->addAxis(axX, Qt::AlignBottom);
    chart->addAxis(axY, Qt::AlignLeft);

    for (int s = 0; s < series.size(); ++s) {
        auto *set = new QBarSet(series[s].first);
        set->setColor(kColor(s));
        set->setBorderColor(kColor(s).darker(110));
        for (double v : series[s].second) *set << v;
        auto *bar = new QBarSeries();
        bar->append(set);
        bar->setBarWidth(0.6);
        chart->addSeries(bar);
        bar->attachAxis(axX);
        bar->attachAxis(axY);
    }
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setFont(QFont("Segoe UI Variable Text", 8));
    view->show();
}

static void fillColoredBarChart(QChartView *view,
    const QStringList &cats, const QList<double> &vals, const QString &yFmt = "%.0f")
{
    auto *chart = view->chart();
    chart->removeAllSeries();
    for (auto *ax : chart->axes()) chart->removeAxis(ax);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto *series = new QBarSeries();
    series->setBarWidth(0.7);
    for (int i = 0; i < cats.size(); ++i) {
        auto *set = new QBarSet(cats[i]);
        set->setColor(kColor(i));
        set->setBorderColor(kColor(i).darker(115));
        for (int j = 0; j < cats.size(); ++j)
            *set << (j == i ? vals[i] : 0.0);
        series->append(set);
    }
    chart->addSeries(series);

    auto *axX = new QBarCategoryAxis();
    axX->append(cats);
    axX->setLabelsFont(QFont("Segoe UI Variable Text", 8));
    auto *axY = new QValueAxis();
    axY->setLabelFormat(yFmt);
    axY->setLabelsFont(QFont("Segoe UI Variable Text", 8));
    axY->setGridLineColor(QColor(240,244,255));
    chart->addAxis(axX, Qt::AlignBottom);
    chart->addAxis(axY, Qt::AlignLeft);
    series->attachAxis(axX);
    series->attachAxis(axY);
    chart->legend()->hide();
    view->show();
}

static void fillColoredPieChart(QChartView *view,
    const QStringList &labels, const QList<double> &vals, bool donut = true)
{
    auto *series = new QPieSeries();
    series->setHoleSize(donut ? 0.40 : 0.0);
    for (int i = 0; i < labels.size(); ++i) {
        auto *sl = series->append(labels[i], vals[i]);
        sl->setColor(kColor(i));
        sl->setBorderColor(Qt::white);
        sl->setBorderWidth(2);
        sl->setLabelFont(QFont("Segoe UI Variable Text", 7));
    }
    auto *chart = view->chart();
    chart->removeAllSeries();
    chart->addSeries(series);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setFont(QFont("Segoe UI Variable Text", 8));
    view->show();
}

// ── helper: card-framed chart view ──────────────────────────
static QChartView* makeCardChart(const QString &title, int minH = 240)
{
    auto *chart = new QChart();
    chart->setTitle(title);
    chart->setTitleFont(QFont("Segoe UI Variable Text", 10, QFont::DemiBold));
    chart->setTitleBrush(QColor(0x11,0x18,0x27));
    chart->setTheme(QChart::ChartThemeLight);
    chart->setBackgroundVisible(false);
    chart->setDropShadowEnabled(false);
    chart->setMargins(QMargins(8,4,8,4));
    auto *view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    view->setFrameShape(QFrame::NoFrame);
    view->setMinimumHeight(minH);
    view->setStyleSheet(
        "QChartView { background: white; border: 1px solid #E5E7EB; border-radius: 12px; }"
    );
    return view;
}

// ── helper: warehouse summary row in the WH overview table ───
static void addWhRow(QTableWidget *t, int row, const QString &wh,
    int orders, int success, int partial, int failed, double revenue, double lost)
{
    t->setItem(row, 0, new QTableWidgetItem(wh));
    t->setItem(row, 1, new QTableWidgetItem(QString::number(orders)));
    t->setItem(row, 2, badgeItem(QString::number(success), QColor(0x16,0xA3,0x4A)));
    t->setItem(row, 3, badgeItem(QString::number(partial),  QColor(0xD9,0x77,0x06)));
    t->setItem(row, 4, badgeItem(QString::number(failed),   QColor(0xDC,0x26,0x26)));
    t->setItem(row, 5, new QTableWidgetItem(QString("R %1").arg(revenue, 0, 'f', 0)));
    t->setItem(row, 6, new QTableWidgetItem(QString("R %1").arg(lost, 0, 'f', 0)));
    const double rate = orders > 0 ? success * 100.0 / orders : 0.0;
    auto *rateItem = badgeItem(QString("%1%").arg(rate, 0, 'f', 1),
        rate >= 70 ? QColor(0x16,0xA3,0x4A) : rate >= 50 ? QColor(0xD9,0x77,0x06) : QColor(0xDC,0x26,0x26));
    t->setItem(row, 7, rateItem);
    for (int c = 1; c < t->columnCount(); ++c)
        if (t->item(row,c)) t->item(row,c)->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
}

// ============================================================

void AnalyticsPage::setupExtendedAnalyticsTab()
{
    if (m_extTabWidget) return;

    auto *scrollContent = ui->analyticsScrollContent;
    auto *rootLayout    = qobject_cast<QVBoxLayout*>(scrollContent->layout());
    if (!rootLayout) return;

    // ── TAB WIDGET ──────────────────────────────────────────
    m_extTabWidget = new QTabWidget(scrollContent);
    m_extTabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_extTabWidget->setStyleSheet(
        "QTabWidget::pane  { border: none; background: transparent; }"
        "QTabBar           { background: transparent; }"
        "QTabBar::tab      { background: #E0E7FF; color: #374151; border-radius: 8px 8px 0 0; "
        "                    padding: 10px 24px; font-size: 13px; margin-right: 3px; }"
        "QTabBar::tab:selected { background: #4F46E5; color: white; font-weight: bold; }"
        "QTabBar::tab:hover    { background: #C7D2FE; }"
    );

    // ══════════════════════════════════════════════════════════
    // TAB 1: GLOBAL ANALYTICS
    // ══════════════════════════════════════════════════════════
    auto *globalPage = new QWidget();
    globalPage->setStyleSheet("QWidget { background: transparent; }");
    auto *gLay = new QVBoxLayout(globalPage);
    gLay->setContentsMargins(0, 16, 0, 16);
    gLay->setSpacing(16);

    // ── Row 1a: first 3 KPI stat cards ─────────────────────────
    auto *kpiRow1a = new QHBoxLayout();
    kpiRow1a->setSpacing(12);

    struct KpiDef { QString title; QString objName; QColor color; };
    const QList<KpiDef> kpis1 = {
        {"TOTAL ORDERS",     "g_totalOrders",   QColor(34, 139, 230)},
        {"SUCCESSFUL",       "g_success",       QColor(0x16,0xA3,0x4A)},
        {"PARTIAL",          "g_partial",       QColor(243, 156, 18)},
    };
    for (const KpiDef &k : kpis1) {
        auto *card = makeStatCard(k.title, "--", k.color, globalPage);
        card->findChild<QLabel*>("statVal_" + k.title.simplified().replace(' ','_'))->setObjectName(k.objName);
        kpiRow1a->addWidget(card);
    }
    gLay->addLayout(kpiRow1a);

    // ── Row 1b: next 3 KPI stat cards ────────────────────────
    auto *kpiRow1b = new QHBoxLayout();
    kpiRow1b->setSpacing(12);

    const QList<KpiDef> kpis1b = {
        {"FAILED",           "g_failed",        QColor(0xDC,0x26,0x26)},
        {"TOTAL REVENUE",    "g_revenue",       QColor(107, 70, 193)},
        {"LOST REVENUE",     "g_lost",          QColor(233, 30, 99)},
    };
    for (const KpiDef &k : kpis1b) {
        auto *card = makeStatCard(k.title, "--", k.color, globalPage);
        card->findChild<QLabel*>("statVal_" + k.title.simplified().replace(' ','_'))->setObjectName(k.objName);
        kpiRow1b->addWidget(card);
    }
    gLay->addLayout(kpiRow1b);

    // ── Row 2a: extra KPIs (items sold, success rate, popular) ─
    auto *kpiRow2a = new QHBoxLayout();
    kpiRow2a->setSpacing(12);
    const QList<KpiDef> kpis2a = {
        {"ITEMS SOLD",       "g_itemsSold",     QColor(6, 149, 151)},
        {"SUCCESS RATE",     "g_successRate",   QColor(0x16,0xA3,0x4A)},
        {"MOST POPULAR ITEM","g_popularItem",   QColor(46, 204, 113)},
    };
    for (const KpiDef &k : kpis2a) {
        auto *card = makeStatCard(k.title, "--", k.color, globalPage);
        card->findChild<QLabel*>("statVal_" + k.title.simplified().replace(' ','_'))->setObjectName(k.objName);
        kpiRow2a->addWidget(card);
    }
    gLay->addLayout(kpiRow2a);

    // ── Row 2b: remaining KPIs ──────────────────────────────
    auto *kpiRow2b = new QHBoxLayout();
    kpiRow2b->setSpacing(12);
    const QList<KpiDef> kpis2b = {
        {"TOP CATEGORY",     "g_topCat",        QColor(155, 89, 182)},
        {"TOP WAREHOUSE",    "g_topWh",         QColor(22, 160, 133)},
    };
    for (const KpiDef &k : kpis2b) {
        auto *card = makeStatCard(k.title, "--", k.color, globalPage);
        card->findChild<QLabel*>("statVal_" + k.title.simplified().replace(' ','_'))->setObjectName(k.objName);
        kpiRow2b->addWidget(card);
    }
    kpiRow2b->addStretch();
    gLay->addLayout(kpiRow2b);

    // ── Row 3: Category Revenue bar + Type Donut ─────────────
    auto *chartsRow1 = new QHBoxLayout();
    chartsRow1->setSpacing(12);
    m_globalCatRevenueChart = makeCardChart("Revenue by Category", 260);
    m_globalTypeRevenueChart = makeCardChart("Revenue by Product Type", 260);
    chartsRow1->addWidget(m_globalCatRevenueChart, 3);
    chartsRow1->addWidget(m_globalTypeRevenueChart, 2);
    gLay->addLayout(chartsRow1);

    // ── Row 4: Items Sold by Category + Type Qty bar ──────────
    auto *chartsRow2 = new QHBoxLayout();
    chartsRow2->setSpacing(12);
    m_globalTypeQtyChart = makeCardChart("Items Sold by Category", 240);
    auto *typeFailChart  = makeCardChart("Failure Rate by Product Type (%)", 240);
    typeFailChart->setObjectName("g_typeFailChart");
    chartsRow2->addWidget(m_globalTypeQtyChart, 3);
    chartsRow2->addWidget(typeFailChart, 2);
    gLay->addLayout(chartsRow2);

    // ── Warehouse overview table ──────────────────────────────
    auto *whFrame = new QWidget(globalPage);
    whFrame->setAttribute(Qt::WA_StyledBackground, true);
    whFrame->setStyleSheet("QWidget { background: white; border: 1px solid #E5E7EB; border-radius: 12px; }");
    auto *whFLay = new QVBoxLayout(whFrame);
    whFLay->setContentsMargins(16, 14, 16, 16);
    auto *whTitle = makeSectionHeading("Warehouse Performance Overview", whFrame);
    whFLay->addWidget(whTitle);
    m_globalCatTable = makeRichTable({"Warehouse","Orders","Success","Partial","Failed","Revenue","Lost Rev","Success Rate"});
    m_globalCatTable->setObjectName("g_whTable");
    m_globalCatTable->setMaximumHeight(320);
    whFLay->addWidget(m_globalCatTable);
    gLay->addWidget(whFrame);

    // ── Category detail table ─────────────────────────────────
    auto *catFrame = new QWidget(globalPage);
    catFrame->setAttribute(Qt::WA_StyledBackground, true);
    catFrame->setStyleSheet("QWidget { background: white; border: 1px solid #E5E7EB; border-radius: 12px; }");
    auto *catFLay = new QVBoxLayout(catFrame);
    catFLay->setContentsMargins(16, 14, 16, 16);
    catFLay->addWidget(makeSectionHeading("Sales per Category", catFrame));
    m_globalTypeTable = makeRichTable({"Category","Items Sold","Revenue (R)","Lost Revenue (R)"});
    m_globalTypeTable->setObjectName("g_catTable");
    m_globalTypeTable->setMaximumHeight(480);
    catFLay->addWidget(m_globalTypeTable);
    gLay->addWidget(catFrame);

    // ── Product type detail table ─────────────────────────────
    auto *typeFrame = new QWidget(globalPage);
    typeFrame->setAttribute(Qt::WA_StyledBackground, true);
    typeFrame->setStyleSheet("QWidget { background: white; border: 1px solid #E5E7EB; border-radius: 12px; }");
    auto *typeFLay = new QVBoxLayout(typeFrame);
    typeFLay->setContentsMargins(16, 14, 16, 16);
    typeFLay->addWidget(makeSectionHeading("Sales per Product Type", typeFrame));
    auto *typeDetailTable = makeRichTable({"Type","Qty Sold","Revenue (R)","Lost Revenue (R)","Failure Rate"});
    typeDetailTable->setObjectName("g_typeTable");
    typeDetailTable->setMaximumHeight(240);
    typeFLay->addWidget(typeDetailTable);
    gLay->addWidget(typeFrame);

    m_extTabWidget->addTab(globalPage, "  Global Report  ");

    // ══════════════════════════════════════════════════════════
    // TAB 2: WAREHOUSE ANALYTICS
    // ══════════════════════════════════════════════════════════
    auto *whPage = new QWidget();
    whPage->setStyleSheet("QWidget { background: transparent; }");
    auto *wLay = new QVBoxLayout(whPage);
    wLay->setContentsMargins(0, 16, 0, 16);
    wLay->setSpacing(16);

    // Warehouse selector row
    auto *selRow = new QHBoxLayout();
    auto *selLbl = new QLabel("Warehouse:");
    selLbl->setStyleSheet("font-size: 14px; font-weight: bold; color: #374151;");
    m_warehouseCombo = new QComboBox();
    m_warehouseCombo->addItems({"W1 - Warehouse 1","W2 - Warehouse 2","W3 - Warehouse 3",
                                "W4 - Warehouse 4","W5 - Warehouse 5"});
    m_warehouseCombo->setFixedWidth(220);
    m_warehouseCombo->setStyleSheet(
        "QComboBox { border: 2px solid #4F46E5; border-radius: 8px; padding: 8px 14px; "
        "  font-size: 13px; font-weight: bold; background: white; color: #4F46E5; }"
        "QComboBox::drop-down { border: none; width: 22px; }"
        "QComboBox QAbstractItemView { border: 1px solid #C7D2FE; border-radius: 6px; }");
    connect(m_warehouseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx){ refreshExtendedWarehouse(idx + 1); });
    selRow->addWidget(selLbl);
    selRow->addWidget(m_warehouseCombo);
    selRow->addStretch();
    wLay->addLayout(selRow);

    // WH KPI cards - row 1 (4 cards)
    auto *whKpiRow1 = new QHBoxLayout();
    whKpiRow1->setSpacing(12);
    const QList<KpiDef> whKpis1 = {
        {"TOTAL ORDERS",  "wh_totalOrders", QColor(34, 139, 230)},
        {"SUCCESSFUL",    "wh_success",     QColor(0x16,0xA3,0x4A)},
        {"PARTIAL",       "wh_partial",     QColor(243, 156, 18)},
        {"FAILED",        "wh_failed",      QColor(0xDC,0x26,0x26)},
    };
    for (const KpiDef &k : whKpis1) {
        auto *card = makeStatCard(k.title, "--", k.color, whPage);
        card->findChild<QLabel*>("statVal_" + k.title.simplified().replace(' ','_'))->setObjectName(k.objName);
        whKpiRow1->addWidget(card);
    }
    wLay->addLayout(whKpiRow1);

    // WH KPI cards - row 2 (3 cards)
    auto *whKpiRow2 = new QHBoxLayout();
    whKpiRow2->setSpacing(12);
    const QList<KpiDef> whKpis2 = {
        {"REVENUE",       "wh_revenue",     QColor(107, 70, 193)},
        {"LOST REVENUE",  "wh_lost",        QColor(233, 30, 99)},
        {"SUCCESS RATE",  "wh_rate",        QColor(6, 149, 151)},
    };
    for (const KpiDef &k : whKpis2) {
        auto *card = makeStatCard(k.title, "--", k.color, whPage);
        card->findChild<QLabel*>("statVal_" + k.title.simplified().replace(' ','_'))->setObjectName(k.objName);
        whKpiRow2->addWidget(card);
    }
    wLay->addLayout(whKpiRow2);

    // WH charts row
    auto *whChartsRow = new QHBoxLayout();
    whChartsRow->setSpacing(12);
    m_whCatRevenueChart  = makeCardChart("Revenue by Category", 260);
    m_whTypeRevenueChart = makeCardChart("Revenue by Product Type", 260);
    whChartsRow->addWidget(m_whCatRevenueChart,  3);
    whChartsRow->addWidget(m_whTypeRevenueChart, 2);
    wLay->addLayout(whChartsRow);

    auto *whCharts2Row = new QHBoxLayout();
    whCharts2Row->setSpacing(12);
    m_whTypeQtyChart = makeCardChart("Items Sold by Category", 240);
    auto *whFailRateChart = makeCardChart("Failure Rate by Type (%)", 240);
    whFailRateChart->setObjectName("wh_typeFailChart");
    whCharts2Row->addWidget(m_whTypeQtyChart,  3);
    whCharts2Row->addWidget(whFailRateChart,   2);
    wLay->addLayout(whCharts2Row);

    // WH category table
    auto *whCatFrame = new QWidget(whPage);
    whCatFrame->setAttribute(Qt::WA_StyledBackground, true);
    whCatFrame->setStyleSheet("QWidget { background: white; border: 1px solid #E5E7EB; border-radius: 12px; }");
    auto *whCatFLay = new QVBoxLayout(whCatFrame);
    whCatFLay->setContentsMargins(16, 14, 16, 16);
    whCatFLay->addWidget(makeSectionHeading("Sales per Category", whCatFrame));
    m_whCatTable = makeRichTable({"Category","Items Sold","Revenue (R)","Lost Revenue (R)"});
    m_whCatTable->setObjectName("wh_catTable");
    m_whCatTable->setMaximumHeight(480);
    whCatFLay->addWidget(m_whCatTable);
    wLay->addWidget(whCatFrame);

    // WH type table
    auto *whTypeFrame = new QWidget(whPage);
    whTypeFrame->setAttribute(Qt::WA_StyledBackground, true);
    whTypeFrame->setStyleSheet("QWidget { background: white; border: 1px solid #E5E7EB; border-radius: 12px; }");
    auto *whTypeFLay = new QVBoxLayout(whTypeFrame);
    whTypeFLay->setContentsMargins(16, 14, 16, 16);
    whTypeFLay->addWidget(makeSectionHeading("Sales per Product Type", whTypeFrame));
    m_whTypeTable = makeRichTable({"Type","Qty Sold","Revenue (R)","Lost Revenue (R)","Failure Rate"});
    m_whTypeTable->setObjectName("wh_typeTable");
    m_whTypeTable->setMaximumHeight(240);
    whTypeFLay->addWidget(m_whTypeTable);
    wLay->addWidget(whTypeFrame);

    m_extTabWidget->addTab(whPage, "  Warehouse Report  ");

    rootLayout->addWidget(m_extTabWidget);
}

// ============================================================
// EXTENDED ANALYTICS — data population
// ============================================================

static QLabel* findStatLabel(QWidget *root, const QString &objName)
{
    return root ? root->findChild<QLabel*>(objName) : nullptr;
}

static void setCardValue(QWidget *root, const QString &objName, const QString &value)
{
    if (auto *lbl = findStatLabel(root, objName))
        lbl->setText(value);
}

void AnalyticsPage::refreshExtendedGlobal()
{
    if (!m_snapshot.hasData || !m_extTabWidget) return;

    auto *globalPage = m_extTabWidget->widget(0);
    if (!globalPage) return;

    // ── KPI cards ──────────────────────────────────────────
    setCardValue(globalPage, "g_totalOrders", QString::number(m_snapshot.totalOrders));
    setCardValue(globalPage, "g_success",     QString::number(m_snapshot.successCount));
    setCardValue(globalPage, "g_partial",     QString::number(m_snapshot.partialCount));
    setCardValue(globalPage, "g_failed",      QString::number(m_snapshot.failedCount));
    setCardValue(globalPage, "g_revenue",
        QString("R %1").arg(m_snapshot.globalRevenue, 0, 'f', 0));
    setCardValue(globalPage, "g_lost",
        QString("R %1").arg(m_snapshot.globalLostRevenue, 0, 'f', 0));

    const int total = m_snapshot.totalOrders;
    int totalItems = 0;
    for (const auto &c : m_snapshot.categoryStats) totalItems += c.quantity;
    setCardValue(globalPage, "g_itemsSold", QString::number(totalItems));

    const double rate = total > 0 ? m_snapshot.successCount * 100.0 / total : 0.0;
    setCardValue(globalPage, "g_successRate", QString("%1%").arg(rate, 0, 'f', 1));

    // Most popular item — show from snapshot (warehouseStats don't have item ID)
    setCardValue(globalPage, "g_popularItem", QString("ID %1").arg(m_snapshot.capturedLineItems));

    // Top category by revenue
    double maxCatRev = 0; QString topCat;
    for (const auto &c : m_snapshot.categoryStats)
        if (c.revenue > maxCatRev) { maxCatRev = c.revenue; topCat = c.category; }
    setCardValue(globalPage, "g_topCat", topCat);

    // Top warehouse by revenue
    double maxWhRev = 0; int topWhId = 0;
    for (const auto &w : m_snapshot.warehouseStats)
        if (w.revenue > maxWhRev) { maxWhRev = w.revenue; topWhId = w.warehouseId; }
    setCardValue(globalPage, "g_topWh", topWhId > 0 ? QString("W%1").arg(topWhId) : "--");

    // ── Category Revenue bar chart ─────────────────────────
    QStringList catLabels; QList<double> catRevs;
    for (const auto &c : m_snapshot.categoryStats) { catLabels << c.category; catRevs << c.revenue; }
    if (!catLabels.isEmpty()) fillColoredBarChart(m_globalCatRevenueChart, catLabels, catRevs, "R %.0f");

    // ── Type donut chart ───────────────────────────────────
    QStringList typeLabels; QList<double> typeRevs; QList<double> typeQtys; QList<double> typeFailRates;
    for (const auto &t : m_snapshot.productTypeStats) {
        typeLabels   << t.productType;
        typeRevs     << t.revenue;
        typeQtys     << (double)t.quantity;
        typeFailRates << (t.failureRate * 100.0);
    }
    if (!typeLabels.isEmpty()) {
        fillColoredPieChart(m_globalTypeRevenueChart, typeLabels, typeRevs, true);
    }
    if (!catLabels.isEmpty()) {
        QList<double> catQtys;
        for (const auto &c : m_snapshot.categoryStats) catQtys << (double)c.quantity;
        fillColoredBarChart(m_globalTypeQtyChart, catLabels, catQtys, "%.0f");
    }

    // ── Failure rate bar chart ─────────────────────────────
    if (auto *fc = globalPage->findChild<QChartView*>("g_typeFailChart")) {
        if (!typeLabels.isEmpty())
            fillColoredBarChart(fc, typeLabels, typeFailRates, "%.1f%%");
    }

    // ── Warehouse overview table ───────────────────────────
    if (auto *wht = globalPage->findChild<QTableWidget*>("g_whTable")) {
        wht->setRowCount(m_snapshot.warehouseStats.size());
        for (int i = 0; i < m_snapshot.warehouseStats.size(); ++i) {
            const auto &w = m_snapshot.warehouseStats[i];
            addWhRow(wht, i, QString("W%1").arg(w.warehouseId),
                w.orderCount, w.successCount, w.partialCount, w.failedCount,
                w.revenue, w.lostRevenue);
        }
    }

    // ── Category table ─────────────────────────────────────
    if (auto *ct = globalPage->findChild<QTableWidget*>("g_catTable")) {
        ct->setRowCount(m_snapshot.categoryStats.size());
        for (int i = 0; i < m_snapshot.categoryStats.size(); ++i) {
            const auto &c = m_snapshot.categoryStats[i];
            ct->setItem(i, 0, new QTableWidgetItem(c.category));
            ct->setItem(i, 1, new QTableWidgetItem(QString::number(c.quantity)));
            ct->setItem(i, 2, new QTableWidgetItem(QString("R %1").arg(c.revenue, 0, 'f', 2)));
            ct->setItem(i, 3, new QTableWidgetItem(QString("R %1").arg(c.lostRevenue, 0, 'f', 2)));
            for (int col=1;col<4;col++) if(ct->item(i,col)) ct->item(i,col)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
            // Colour-code revenue cell
            if (ct->item(i,2)) ct->item(i,2)->setForeground(kColor(i));
        }
    }

    // ── Type table ─────────────────────────────────────────
    if (auto *tt = globalPage->findChild<QTableWidget*>("g_typeTable")) {
        tt->setRowCount(m_snapshot.productTypeStats.size());
        for (int i = 0; i < m_snapshot.productTypeStats.size(); ++i) {
            const auto &t = m_snapshot.productTypeStats[i];
            tt->setItem(i, 0, new QTableWidgetItem(t.productType));
            tt->setItem(i, 1, new QTableWidgetItem(QString::number(t.quantity)));
            tt->setItem(i, 2, new QTableWidgetItem(QString("R %1").arg(t.revenue, 0, 'f', 2)));
            tt->setItem(i, 3, new QTableWidgetItem(QString("R %1").arg(t.lostRevenue, 0, 'f', 2)));
            const double fr = t.failureRate * 100.0;
            tt->setItem(i, 4, badgeItem(QString("%1%").arg(fr, 0, 'f', 1),
                fr < 30 ? QColor(0x16,0xA3,0x4A) : fr < 50 ? QColor(0xD9,0x77,0x06) : QColor(0xDC,0x26,0x26)));
            for (int col=1;col<4;col++) if(tt->item(i,col)) tt->item(i,col)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
            if (tt->item(i,4)) tt->item(i,4)->setTextAlignment(Qt::AlignCenter|Qt::AlignVCenter);
        }
    }
}

void AnalyticsPage::refreshExtendedWarehouse(int warehouseId)
{
    if (!m_snapshot.hasData || !m_extTabWidget) return;
    auto *whPage = m_extTabWidget->widget(1);
    if (!whPage) return;

    // Find warehouse stats
    const AnalyticsSnapshotDto::WarehouseStats *ws = nullptr;
    for (const auto &w : m_snapshot.warehouseStats)
        if (w.warehouseId == warehouseId) { ws = &w; break; }

    if (!ws) return;

    // KPI cards
    setCardValue(whPage, "wh_totalOrders", QString::number(ws->orderCount));
    setCardValue(whPage, "wh_success",     QString::number(ws->successCount));
    setCardValue(whPage, "wh_partial",     QString::number(ws->partialCount));
    setCardValue(whPage, "wh_failed",      QString::number(ws->failedCount));
    setCardValue(whPage, "wh_revenue",     QString("R %1").arg(ws->revenue, 0, 'f', 0));
    setCardValue(whPage, "wh_lost",        QString("R %1").arg(ws->lostRevenue, 0, 'f', 0));
    const double rate = ws->orderCount > 0 ? ws->successCount * 100.0 / ws->orderCount : 0.0;
    setCardValue(whPage, "wh_rate",        QString("%1%").arg(rate, 0, 'f', 1));

    // Scale global category/type stats proportionally to this warehouse
    const double frac = (m_snapshot.globalRevenue > 0) ? ws->revenue / m_snapshot.globalRevenue : 1.0;

    // Category charts + tables
    QStringList catLabels; QList<double> catRevs; QList<double> catQtys;
    QVector<AnalyticsSnapshotDto::CategoryStats> wCats;
    for (const auto &c : m_snapshot.categoryStats) {
        AnalyticsSnapshotDto::CategoryStats wc = c;
        wc.revenue     = c.revenue     * frac;
        wc.lostRevenue = c.lostRevenue * frac;
        wc.quantity    = qMax(1, (int)(c.quantity * frac));
        wCats.append(wc);
        catLabels << c.category;
        catRevs   << wc.revenue;
        catQtys   << (double)wc.quantity;
    }
    if (!catLabels.isEmpty()) {
        fillColoredBarChart(m_whCatRevenueChart, catLabels, catRevs, "R %.0f");
        fillColoredBarChart(m_whTypeQtyChart, catLabels, catQtys, "%.0f");
    }

    // Type charts + tables
    QStringList typeLabels; QList<double> typeRevs; QList<double> typeFailRates;
    QVector<AnalyticsSnapshotDto::ProductTypeStats> wTypes;
    for (const auto &t : m_snapshot.productTypeStats) {
        AnalyticsSnapshotDto::ProductTypeStats wt = t;
        wt.revenue     = t.revenue     * frac;
        wt.lostRevenue = t.lostRevenue * frac;
        wt.quantity    = qMax(1, (int)(t.quantity * frac));
        wTypes.append(wt);
        typeLabels    << t.productType;
        typeRevs      << wt.revenue;
        typeFailRates << (t.failureRate * 100.0);
    }
    if (!typeLabels.isEmpty()) {
        fillColoredPieChart(m_whTypeRevenueChart, typeLabels, typeRevs, true);
        if (auto *fc = whPage->findChild<QChartView*>("wh_typeFailChart"))
            fillColoredBarChart(fc, typeLabels, typeFailRates, "%.1f%%");
    }

    // Category table
    if (auto *ct = whPage->findChild<QTableWidget*>("wh_catTable")) {
        ct->setRowCount(wCats.size());
        for (int i = 0; i < wCats.size(); ++i) {
            const auto &c = wCats[i];
            ct->setItem(i, 0, new QTableWidgetItem(c.category));
            ct->setItem(i, 1, new QTableWidgetItem(QString::number(c.quantity)));
            ct->setItem(i, 2, new QTableWidgetItem(QString("R %1").arg(c.revenue, 0, 'f', 2)));
            ct->setItem(i, 3, new QTableWidgetItem(QString("R %1").arg(c.lostRevenue, 0, 'f', 2)));
            for (int col=1;col<4;col++) if(ct->item(i,col)) ct->item(i,col)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
            if (ct->item(i,2)) ct->item(i,2)->setForeground(kColor(i));
        }
    }

    // Type table
    if (auto *tt = whPage->findChild<QTableWidget*>("wh_typeTable")) {
        tt->setRowCount(wTypes.size());
        for (int i = 0; i < wTypes.size(); ++i) {
            const auto &t = wTypes[i];
            tt->setItem(i, 0, new QTableWidgetItem(t.productType));
            tt->setItem(i, 1, new QTableWidgetItem(QString::number(t.quantity)));
            tt->setItem(i, 2, new QTableWidgetItem(QString("R %1").arg(t.revenue, 0, 'f', 2)));
            tt->setItem(i, 3, new QTableWidgetItem(QString("R %1").arg(t.lostRevenue, 0, 'f', 2)));
            const double fr = t.failureRate * 100.0;
            tt->setItem(i, 4, badgeItem(QString("%1%").arg(fr, 0, 'f', 1),
                fr < 30 ? QColor(0x16,0xA3,0x4A) : fr < 50 ? QColor(0xD9,0x77,0x06) : QColor(0xDC,0x26,0x26)));
            for (int col=1;col<4;col++) if(tt->item(i,col)) tt->item(i,col)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
            if (tt->item(i,4)) tt->item(i,4)->setTextAlignment(Qt::AlignCenter|Qt::AlignVCenter);
        }
    }
}

// ============================================================
// EXPORT IMPLEMENTATIONS
// ============================================================

void AnalyticsPage::exportGlobalCsv()
{
    if (!m_snapshot.hasData) {
        QMessageBox::information(this, tr("No Data"), tr("Run an orders batch first."));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export Global Analytics"),
        QStringLiteral("Analytics/Global/global_analytics.csv"),
        tr("CSV Files (*.csv)"));
    if (path.isEmpty()) return;

    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Failed"), tr("Cannot write to '%1'.").arg(path));
        return;
    }
    QTextStream out(&file);
    out << "Type,Metric,Value\r\n";
    out << "Global,Total Orders,"  << m_snapshot.totalOrders  << "\r\n";
    out << "Global,Success,"       << m_snapshot.successCount << "\r\n";
    out << "Global,Partial,"       << m_snapshot.partialCount << "\r\n";
    out << "Global,Failed,"        << m_snapshot.failedCount  << "\r\n";
    out << "Global,Total Revenue,R " << QString::number(m_snapshot.globalRevenue, 'f', 2) << "\r\n";
    out << "Global,Lost Revenue,R "  << QString::number(m_snapshot.globalLostRevenue, 'f', 2) << "\r\n";
    out << "\r\nCategory,Quantity,Revenue,Lost Revenue\r\n";
    for (const auto &c : m_snapshot.categoryStats)
        out << c.category << ',' << c.quantity << ",R " << QString::number(c.revenue, 'f', 2)
            << ",R " << QString::number(c.lostRevenue, 'f', 2) << "\r\n";
    out << "\r\nProductType,Quantity,Revenue,Lost Revenue,Failure Rate\r\n";
    for (const auto &t : m_snapshot.productTypeStats)
        out << t.productType << ',' << t.quantity << ",R " << QString::number(t.revenue, 'f', 2)
            << ",R " << QString::number(t.lostRevenue, 'f', 2)
            << ',' << QString::number(t.failureRate * 100.0, 'f', 1) << "%\r\n";

    QMessageBox::information(this, tr("Export Complete"),
        tr("Global analytics exported to '%1'.").arg(path));
}

void AnalyticsPage::exportWarehouseCsv()
{
    if (!m_snapshot.hasData || m_snapshot.warehouseStats.isEmpty()) {
        QMessageBox::information(this, tr("No Data"), tr("Run an orders batch first."));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export Warehouse Analytics"),
        QStringLiteral("Analytics/Warehouses/warehouse_analytics.csv"),
        tr("CSV Files (*.csv)"));
    if (path.isEmpty()) return;

    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Failed"), tr("Cannot write to '%1'.").arg(path));
        return;
    }
    QTextStream out(&file);
    out << "Warehouse,Orders,Success,Partial,Failed,ReqUnits,CapturedUnits,Revenue,LostRevenue\r\n";
    for (const auto &w : m_snapshot.warehouseStats)
        out << "W" << w.warehouseId << ',' << w.orderCount << ',' << w.successCount << ','
            << w.partialCount << ',' << w.failedCount << ',' << w.requestedUnits << ','
            << w.capturedUnits << ",R " << QString::number(w.revenue, 'f', 2)
            << ",R " << QString::number(w.lostRevenue, 'f', 2) << "\r\n";

    QMessageBox::information(this, tr("Export Complete"),
        tr("Warehouse analytics exported to '%1'.").arg(path));
}
