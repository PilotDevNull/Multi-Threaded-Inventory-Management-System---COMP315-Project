#ifndef ANALYTICSPAGE_H
#define ANALYTICSPAGE_H

#include <QtCharts/QChartView>
#include <QWidget>
#include <QComboBox>
#include <QTabWidget>
#include <QLabel>
#include <QTableWidget>

#include "AnalyticsSnapshotDto.h"

namespace Ui {
class AnalyticsPage;
}

class AnalyticsPage : public QWidget
{
    Q_OBJECT

public:
    explicit AnalyticsPage(QWidget *parent = nullptr);
    ~AnalyticsPage();

    void setAnalyticsSnapshot(const AnalyticsSnapshotDto &snapshot);
    void exportGlobalCsv();
    void exportWarehouseCsv();

private:
    Ui::AnalyticsPage *ui;

    // Charts (original batch view)
    QChartView *m_ordersByWarehouseChartView   = nullptr;
    QChartView *m_statusBreakdownChartView      = nullptr;
    QLabel     *m_ordersByWarehouseEmptyLabel   = nullptr;
    QLabel     *m_statusBreakdownEmptyLabel     = nullptr;

    // Extended analytics tab widget injected into the page
    QTabWidget    *m_extTabWidget       = nullptr;
    QComboBox     *m_warehouseCombo     = nullptr;

    // Global extended charts
    QChartView    *m_globalCatRevenueChart   = nullptr;
    QChartView    *m_globalTypeRevenueChart  = nullptr;
    QChartView    *m_globalTypeQtyChart      = nullptr;

    // Warehouse extended charts
    QChartView    *m_whCatRevenueChart   = nullptr;
    QChartView    *m_whTypeRevenueChart  = nullptr;
    QChartView    *m_whTypeQtyChart      = nullptr;

    // Tables
    QTableWidget  *m_globalCatTable  = nullptr;
    QTableWidget  *m_globalTypeTable = nullptr;
    QTableWidget  *m_whCatTable      = nullptr;
    QTableWidget  *m_whTypeTable     = nullptr;

    // Cached snapshot
    AnalyticsSnapshotDto m_snapshot;

    void setupUiDefaults();
    void setupChartHosts();
    void setupExtendedAnalyticsTab();
    void updateOrdersByWarehouseChart(const AnalyticsSnapshotDto &snapshot);
    void updateStatusBreakdownChart(const AnalyticsSnapshotDto &snapshot);
    void showOrdersByWarehouseEmptyState(const QString &message);
    void showStatusBreakdownEmptyState(const QString &message);
    void resetAnalyticsDisplay();
    void updateTableSizing();
    void refreshExtendedGlobal();
    void refreshExtendedWarehouse(int warehouseId);
};

#endif // ANALYTICSPAGE_H
