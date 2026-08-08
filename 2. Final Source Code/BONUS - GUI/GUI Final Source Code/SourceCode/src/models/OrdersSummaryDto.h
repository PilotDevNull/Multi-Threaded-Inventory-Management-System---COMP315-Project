#ifndef ORDERSSUMMARYDTO_H
#define ORDERSSUMMARYDTO_H

#include <QVector>

struct OrdersWarehouseCountDto {
    int warehouseId = 0;
    int totalOrders = 0;
    int successCount = 0;
    int partialCount = 0;
    int failedCount = 0;
};

struct OrdersSummaryDto {
    int totalOrders = 0;
    int successCount = 0;
    int partialCount = 0;
    int failedCount = 0;
    QVector<OrdersWarehouseCountDto> perWarehouseCounts;
};

#endif // ORDERSSUMMARYDTO_H
