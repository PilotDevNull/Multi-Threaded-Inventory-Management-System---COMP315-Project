#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QFutureWatcher>
#include <QString>

#include "AnalyticsSnapshotDto.h"
#include "InventoryServiceAdapter.h"
#include "InventorySnapshotDto.h"
#include "LogsSnapshotDto.h"
#include "OrdersBatchResultDto.h"
#include "OrdersServiceAdapter.h"
#include "OrdersSummaryDto.h"

// Keeps the GUI pointed at immutable DTO snapshots rather than backend objects.
class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);

    bool refreshInventorySnapshot();
    bool startOrdersBatch(int workerCount = 4);

    //Add a manual order to the pool so it gets processed in the next batch.
    //Returns the runtime order ID assigned by the backend, or -1 on failure.
    int addManualOrder(int warehouseId,
                       const QVector<QPair<int,int>> &items);
    int pendingManualOrders() const;

    const InventorySnapshotDto   &inventorySnapshot() const;
    const OrdersSummaryDto       &ordersSummary() const;
    const LogsSnapshotDto        &logsSnapshot() const;
    const AnalyticsSnapshotDto   &analyticsSnapshot() const;

    QString lastInventoryError() const;
    QString lastOrdersError() const;

    bool isOrdersBatchRunning() const;
    bool hasOrdersSummary() const;
    bool hasLogsSnapshot() const;
    bool hasAnalyticsSnapshot() const;

signals:
    void ordersBatchStarted();
    void ordersBatchFinished();
    void ordersBatchFailed(const QString &errorMessage);

private slots:
    void handleOrdersBatchFinished();

private:
    InventoryServiceAdapter m_inventoryService;
    InventorySnapshotDto m_inventorySnapshot;
    QString m_lastInventoryError;

    OrdersServiceAdapter m_ordersService;
    OrdersSummaryDto     m_ordersSummary;
    LogsSnapshotDto      m_logsSnapshot;
    AnalyticsSnapshotDto m_analyticsSnapshot;
    QString              m_lastOrdersError;
    bool                 m_hasOrdersSummary    = false;
    bool                 m_hasLogsSnapshot     = false;
    bool                 m_hasAnalyticsSnapshot = false;
    QFutureWatcher<OrdersBatchResultDto> m_ordersWatcher;

    //Manual orders queued by the user before a batch runs.
    //Stored as (warehouseId, items) pairs. Flushed into the OrderPool
    //at the start of each batch so they get processed alongside CSV orders.
    QVector<ManualOrderEntry> m_pendingManualOrders;
};

#endif // APPCONTROLLER_H
