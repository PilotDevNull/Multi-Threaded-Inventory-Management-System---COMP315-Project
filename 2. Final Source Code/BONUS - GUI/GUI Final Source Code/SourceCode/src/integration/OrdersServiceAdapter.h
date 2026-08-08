#ifndef ORDERSSERVICEADAPTER_H
#define ORDERSSERVICEADAPTER_H

#include <QPair>
#include <QString>
#include <QVector>

#include "OrdersBatchResultDto.h"

//Forward declare the manual order entry struct used by AppController
struct ManualOrderEntry {
    int runtimeId = 0;
    int warehouseId = 0;
    QVector<QPair<int,int>> items;
};

// Runs the protected backend's batch order pipeline and converts the final
// outcome into immutable DTOs for the GUI.
class OrdersServiceAdapter
{
public:
    OrdersBatchResultDto processDefaultWarehouses(
        int workerCount = kDefaultWorkerCount,
        const QVector<ManualOrderEntry> &manualOrders = {}) const;

private:
    QString resolveProductsCsvPath() const;
    QString resolveWarehouseOrdersPath(int warehouseId) const;

    static constexpr int kWarehouseCount = 5;
    static constexpr int kDefaultWorkerCount = 4;
    static constexpr int kMaxWorkerCount = 32;
};

#endif // ORDERSSERVICEADAPTER_H
