#ifndef LOGSSNAPSHOTDTO_H
#define LOGSSNAPSHOTDTO_H

#include <QString>
#include <QVector>

// Single line item inside a processed order (product + requested quantity).
struct ProcessedOrderItemDto {
    int productId = 0;
    int quantity  = 0;
};

// One order as it came out of the backend worker (status set, cart read-only).
// runtimeOrderId  : the auto-incremented Order::orderID assigned by the backend.
// warehouseId     : which Warehouse CSV this order originated from (1-5).
// status          : "SUCCESS", "PARTIAL", or "FAILED".
// items           : the cart contents (productId -> quantity pairs).
struct ProcessedOrderDto {
    int runtimeOrderId = 0;
    int warehouseId    = 0;
    QString status;
    QVector<ProcessedOrderItemDto> items;
};

// Backend analytics record captured after v2 workers complete.
struct ProcessedAnalyticsRecordDto {
    int orderId = 0;
    int productId = 0;
    QString productType;
    QString category;
    int quantity = 0;
    int warehouseId = 0;
    bool itemSucceeded = false;
    QString itemStatus;
    QString orderStatus;
    double finalPrice = 0.0;
};

// Immutable snapshot of the last completed batch, ready for the Logs page.
// Counters mirror OrdersSummaryDto but are kept here so LogsPage only
// depends on this one header, not on the Orders-specific DTO hierarchy.
struct LogsSnapshotDto {
    QVector<ProcessedOrderDto> orders;
    QVector<ProcessedAnalyticsRecordDto> analyticsRecords;
    int totalCount   = 0;
    int successCount = 0;
    int partialCount = 0;
    int failedCount  = 0;
};

#endif // LOGSSNAPSHOTDTO_H
