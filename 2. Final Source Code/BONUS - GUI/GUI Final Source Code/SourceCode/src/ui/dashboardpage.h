#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <QString>
#include <QWidget>

#include "AnalyticsSnapshotDto.h"

namespace Ui {
class DashboardPage;
}

class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);
    ~DashboardPage();

    void setInventoryMetrics(int products, int stock, int lowStock);
    void setAnalyticsSnapshot(const AnalyticsSnapshotDto &snapshot);
    void setOrdersBatchRunning();
    void setOrdersBatchFailed(const QString &errorMessage);

private:
    Ui::DashboardPage *ui;
    void setupUiDefaults();
    void setRevenueUnavailable(const QString &reason);
    void setRevenueMetrics(double revenue, double lostRevenue, bool incompleteRecords);
    void setWarehouseRow(int index,
                         const QString &statusText,
                         const QString &statusRole,
                         int processed,
                         int success,
                         int partialOrFailed);
    void resetWarehouseRows(const QString &statusText, const QString &statusRole);
    void setActivityFeedUnavailable(const QString &message,
                                    const QString &badgeText,
                                    const QString &badgeRole);
};

#endif // DASHBOARDPAGE_H
