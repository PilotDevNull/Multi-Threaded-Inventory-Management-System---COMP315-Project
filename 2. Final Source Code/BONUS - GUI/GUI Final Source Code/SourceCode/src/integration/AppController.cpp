#include "AppController.h"

#include <QtConcurrent/QtConcurrentRun>

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    connect(&m_ordersWatcher,
            &QFutureWatcher<OrdersBatchResultDto>::finished,
            this,
            &AppController::handleOrdersBatchFinished);
}

bool AppController::refreshInventorySnapshot()
{
    const InventorySnapshotDto snapshot = m_inventoryService.loadInventorySnapshot();
    if (!snapshot.loadError.isEmpty()) {
        m_lastInventoryError = snapshot.loadError;
        return false;
    }

    m_inventorySnapshot = snapshot;
    m_lastInventoryError.clear();
    return true;
}

bool AppController::startOrdersBatch(int workerCount)
{
  if (m_ordersWatcher.isRunning()) {
    return false;
  }

    m_ordersSummary      = OrdersSummaryDto{};
    m_logsSnapshot       = LogsSnapshotDto{};
    m_analyticsSnapshot  = AnalyticsSnapshotDto{};
    m_lastOrdersError.clear();
    m_hasOrdersSummary    = false;
    m_hasLogsSnapshot     = false;
    m_hasAnalyticsSnapshot = false;

  //Grab any manual orders the user queued and clear the pending list
  const QVector<ManualOrderEntry> manualOrders = m_pendingManualOrders;
  m_pendingManualOrders.clear();

  const OrdersServiceAdapter ordersService = m_ordersService;
  m_ordersWatcher.setFuture(QtConcurrent::run([ordersService, workerCount, manualOrders]() {
        return ordersService.processDefaultWarehouses(workerCount, manualOrders);
    }));
  emit ordersBatchStarted();
  return true;
}

int AppController::addManualOrder(int warehouseId,
                                  const QVector<QPair<int,int>> &items)
{
    if (items.isEmpty() || warehouseId < 1 || warehouseId > 5) {
        return -1;
    }
    //We use a simple static counter here just for the GUI-side ID.
    //The real backend Order ID gets assigned when Order(warehouseId) runs.
    static int guiOrderCounter = 9000;
    ManualOrderEntry entry;
    entry.runtimeId = ++guiOrderCounter;
    entry.warehouseId = warehouseId;
    entry.items = items;
    m_pendingManualOrders.append(entry);
    return entry.runtimeId;
}

int AppController::pendingManualOrders() const
{
    return m_pendingManualOrders.size();
}

const InventorySnapshotDto &AppController::inventorySnapshot() const
{
    return m_inventorySnapshot;
}

const OrdersSummaryDto &AppController::ordersSummary() const
{
    return m_ordersSummary;
}

QString AppController::lastInventoryError() const
{
    return m_lastInventoryError;
}

QString AppController::lastOrdersError() const
{
    return m_lastOrdersError;
}

bool AppController::isOrdersBatchRunning() const
{
    return m_ordersWatcher.isRunning();
}

bool AppController::hasOrdersSummary() const
{
    return m_hasOrdersSummary;
}

const LogsSnapshotDto &AppController::logsSnapshot() const
{
    return m_logsSnapshot;
}

bool AppController::hasLogsSnapshot() const
{
    return m_hasLogsSnapshot;
}

const AnalyticsSnapshotDto &AppController::analyticsSnapshot() const
{
    return m_analyticsSnapshot;
}

bool AppController::hasAnalyticsSnapshot() const
{
    return m_hasAnalyticsSnapshot;
}

void AppController::handleOrdersBatchFinished()
{
    const OrdersBatchResultDto result = m_ordersWatcher.result();
    if (!result.isValid()) {
        m_ordersSummary      = OrdersSummaryDto{};
        m_logsSnapshot       = LogsSnapshotDto{};
        m_analyticsSnapshot  = AnalyticsSnapshotDto{};
        m_lastOrdersError     = result.errorMessage;
        m_hasOrdersSummary    = false;
        m_hasLogsSnapshot     = false;
        m_hasAnalyticsSnapshot = false;
        emit ordersBatchFailed(m_lastOrdersError);
        return;
    }

    m_ordersSummary  = result.summary;
    m_logsSnapshot   = result.logsSnapshot;
    m_lastOrdersError.clear();
    m_hasOrdersSummary = true;
    m_hasLogsSnapshot  = true;

    m_analyticsSnapshot = result.analyticsSnapshot;
    m_hasAnalyticsSnapshot = true;

    emit ordersBatchFinished();
}
