#include "AnalyticsAggregator.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <QDateTime>
#include <QMap>

#include "AnalyticsManager.h"
#include "AnalyticsRecord.h"

namespace {

constexpr int kPreviewRowLimit = 5;

struct CategoryAccumulator {
    int quantity = 0;
    double revenue = 0.0;
    double lostRevenue = 0.0;
};

struct ProductTypeAccumulator {
    int quantity = 0;
    double revenue = 0.0;
    double lostRevenue = 0.0;
    double failureRate = 0.0;
};

QString toQString(const std::string &value)
{
    return QString::fromStdString(value).trimmed();
}

QString nonEmptyOrUnknown(QString value)
{
    value = value.trimmed();
    return value.isEmpty() ? QStringLiteral("UNKNOWN") : value;
}

QString normalizedStatus(QString status)
{
    status = status.trimmed().toUpper();
    return status.isEmpty() ? QStringLiteral("UNKNOWN") : status;
}

int requestedUnitsForOrder(const ProcessedOrderDto &order)
{
    int requestedUnits = 0;
    for (const ProcessedOrderItemDto &item : order.items) {
        requestedUnits += item.quantity;
    }
    return requestedUnits;
}

void seedSummaryTotals(AnalyticsSnapshotDto &snapshot, const OrdersSummaryDto &summary)
{
    snapshot.totalOrders = summary.totalOrders;
    snapshot.successCount = summary.successCount;
    snapshot.partialCount = summary.partialCount;
    snapshot.failedCount = summary.failedCount;
}

void addStatusBreakdown(AnalyticsSnapshotDto &snapshot)
{
    const QVector<AnalyticsSnapshotDto::StatusStats> rows = {
        {QStringLiteral("SUCCESS"), snapshot.successCount},
        {QStringLiteral("PARTIAL"), snapshot.partialCount},
        {QStringLiteral("FAILED"), snapshot.failedCount}
    };

    for (const AnalyticsSnapshotDto::StatusStats &row : rows) {
        if (row.count > 0) {
            snapshot.statusBreakdown.append(row);
        }
    }
}

QMap<int, int> capturedUnitsByOrder(const std::vector<AnalyticsRecord> &records)
{
    QMap<int, int> capturedUnits;
    for (const AnalyticsRecord &record : records) {
        capturedUnits[record.getOrderID()] += record.getQuantity();
    }
    return capturedUnits;
}

void addOrderDerivedFields(AnalyticsSnapshotDto &snapshot,
                           const OrdersSummaryDto &summary,
                           const LogsSnapshotDto &logs,
                           QMap<int, AnalyticsSnapshotDto::WarehouseStats> &warehouseMap,
                           const QMap<int, int> &capturedOrderUnits)
{
    for (const OrdersWarehouseCountDto &counts : summary.perWarehouseCounts) {
        AnalyticsSnapshotDto::WarehouseStats &stats = warehouseMap[counts.warehouseId];
        stats.warehouseId = counts.warehouseId;
        stats.orderCount = counts.totalOrders;
        stats.successCount = counts.successCount;
        stats.partialCount = counts.partialCount;
        stats.failedCount = counts.failedCount;
    }

    for (const ProcessedOrderDto &order : logs.orders) {
        const int requestedUnits = requestedUnitsForOrder(order);
        const int capturedUnits = capturedOrderUnits.value(order.runtimeOrderId);

        snapshot.totalRequestedUnits += requestedUnits;
        snapshot.expectedLineItems += order.items.size();

        AnalyticsSnapshotDto::WarehouseStats &stats = warehouseMap[order.warehouseId];
        stats.warehouseId = order.warehouseId;
        if (stats.orderCount == 0) {
            ++stats.orderCount;
            const QString status = normalizedStatus(order.status);
            if (status == QLatin1String("SUCCESS")) {
                ++stats.successCount;
            } else if (status == QLatin1String("PARTIAL")) {
                ++stats.partialCount;
            } else if (status == QLatin1String("FAILED")) {
                ++stats.failedCount;
            }
        }
        stats.requestedUnits += requestedUnits;

        const QString status = normalizedStatus(order.status);
        if (status == QLatin1String("SUCCESS") && snapshot.successRows.size() < kPreviewRowLimit) {
            AnalyticsSnapshotDto::SuccessRow row;
            row.orderId = order.runtimeOrderId;
            row.warehouseId = order.warehouseId;
            row.lineItems = static_cast<int>(order.items.size());
            row.requestedUnits = requestedUnits;
            row.capturedUnits = capturedUnits;
            snapshot.successRows.append(row);
        } else if (status != QLatin1String("SUCCESS")
                   && snapshot.nonSuccessRows.size() < kPreviewRowLimit) {
            AnalyticsSnapshotDto::NonSuccessRow row;
            row.orderId = order.runtimeOrderId;
            row.warehouseId = order.warehouseId;
            row.requestedUnits = requestedUnits;
            row.capturedUnits = capturedUnits;
            row.status = status;
            snapshot.nonSuccessRows.append(row);
        }
    }
}

void addRecordDerivedFields(AnalyticsSnapshotDto &snapshot,
                            const std::vector<AnalyticsRecord> &records,
                            QMap<int, AnalyticsSnapshotDto::WarehouseStats> &warehouseMap,
                            QMap<QString, CategoryAccumulator> &categoryMap,
                            QMap<QString, ProductTypeAccumulator> &typeMap)
{
    snapshot.capturedLineItems = static_cast<int>(records.size());
    snapshot.hasCapturedAnalytics = !records.empty();

    for (const AnalyticsRecord &record : records) {
        const int quantity = record.getQuantity();
        const double value = record.getFinalPrice() * quantity;
        const bool itemSucceeded = record.getStatus();
        const QString category = nonEmptyOrUnknown(toQString(record.getCategory()));
        const QString productType = nonEmptyOrUnknown(toQString(record.getProductType()));

        AnalyticsSnapshotDto::WarehouseStats &warehouseStats =
            warehouseMap[record.getWarehouseID()];
        warehouseStats.warehouseId = record.getWarehouseID();
        warehouseStats.capturedUnits += quantity;
        if (itemSucceeded) {
            warehouseStats.processedUnits += quantity;
            warehouseStats.revenue += value;
            snapshot.totalProcessedUnits += quantity;
            snapshot.globalRevenue += value;
        } else {
            warehouseStats.lostUnits += quantity;
            warehouseStats.lostRevenue += value;
            snapshot.totalLostUnits += quantity;
            snapshot.globalLostRevenue += value;
        }

        snapshot.totalCapturedUnits += quantity;

        CategoryAccumulator &categoryStats = categoryMap[category];
        categoryStats.quantity += quantity;
        if (itemSucceeded) {
            categoryStats.revenue += value;
        } else {
            categoryStats.lostRevenue += value;
        }

        ProductTypeAccumulator &typeStats = typeMap[productType];
        typeStats.quantity += quantity;
        if (itemSucceeded) {
            typeStats.revenue += value;
        } else {
            typeStats.lostRevenue += value;
        }
    }
}

template <typename Value>
void mergeStringMapKeys(QMap<QString, Value> &target, const std::map<std::string, Value> &source)
{
    for (const auto &[key, value] : source) {
        target[nonEmptyOrUnknown(toQString(key))] = value;
    }
}

void applyBackendManagerMetrics(AnalyticsSnapshotDto &snapshot,
                                const AnalyticsManager *analyticsManager,
                                QMap<int, AnalyticsSnapshotDto::WarehouseStats> &warehouseMap,
                                QMap<QString, CategoryAccumulator> &categoryMap,
                                QMap<QString, ProductTypeAccumulator> &typeMap)
{
    if (!analyticsManager) {
        return;
    }

    snapshot.globalRevenue = analyticsManager->getGlobalRevenue();
    snapshot.globalLostRevenue = analyticsManager->getGlobalLostRevenue();
    snapshot.totalCapturedUnits = analyticsManager->getGlobalTotalItems();

    for (auto it = warehouseMap.begin(); it != warehouseMap.end(); ++it) {
        AnalyticsSnapshotDto::WarehouseStats &stats = it.value();
        stats.revenue = analyticsManager->getWarehouseRevenue(stats.warehouseId);
        stats.lostRevenue = analyticsManager->getWarehouseLostRevenue(stats.warehouseId);
        stats.capturedUnits = analyticsManager->getWarehouseTotalItems(stats.warehouseId);
    }

    QMap<QString, int> categoryQuantities;
    mergeStringMapKeys(categoryQuantities, analyticsManager->getGlobalItemsSoldPerCategory());
    for (auto it = categoryQuantities.cbegin(); it != categoryQuantities.cend(); ++it) {
        categoryMap[it.key()].quantity = it.value();
    }

    QMap<QString, int> typeQuantities;
    QMap<QString, double> typeRevenue;
    QMap<QString, double> typeFailureRate;
    mergeStringMapKeys(typeQuantities, analyticsManager->getGlobalQuantityByType());
    mergeStringMapKeys(typeRevenue, analyticsManager->getGlobalRevenueByType());
    mergeStringMapKeys(typeFailureRate, analyticsManager->getGlobalFailureRateByType());

    for (auto it = typeQuantities.cbegin(); it != typeQuantities.cend(); ++it) {
        typeMap[it.key()].quantity = it.value();
    }
    for (auto it = typeRevenue.cbegin(); it != typeRevenue.cend(); ++it) {
        typeMap[it.key()].revenue = it.value();
    }
    for (auto it = typeFailureRate.cbegin(); it != typeFailureRate.cend(); ++it) {
        typeMap[it.key()].failureRate = it.value();
    }
}

void appendFlattenedMetrics(AnalyticsSnapshotDto &snapshot,
                            const QMap<int, AnalyticsSnapshotDto::WarehouseStats> &warehouseMap,
                            const QMap<QString, CategoryAccumulator> &categoryMap,
                            const QMap<QString, ProductTypeAccumulator> &typeMap)
{
    for (const AnalyticsSnapshotDto::WarehouseStats &stats : warehouseMap) {
        if (stats.warehouseId > 0) {
            snapshot.warehouseStats.append(stats);
        }
    }

    for (auto it = categoryMap.cbegin(); it != categoryMap.cend(); ++it) {
        AnalyticsSnapshotDto::CategoryStats row;
        row.category = it.key();
        row.quantity = it.value().quantity;
        row.revenue = it.value().revenue;
        row.lostRevenue = it.value().lostRevenue;
        snapshot.categoryStats.append(row);
    }

    for (auto it = typeMap.cbegin(); it != typeMap.cend(); ++it) {
        AnalyticsSnapshotDto::ProductTypeStats row;
        row.productType = it.key();
        row.quantity = it.value().quantity;
        row.revenue = it.value().revenue;
        row.lostRevenue = it.value().lostRevenue;
        row.failureRate = it.value().failureRate;
        snapshot.productTypeStats.append(row);
    }
}

AnalyticsSnapshotDto buildAnalyticsInternal(const OrdersSummaryDto &summary,
                                            const LogsSnapshotDto &logs,
                                            const AnalyticsManager *analyticsManager,
                                            const std::vector<AnalyticsRecord> &records)
{
    AnalyticsSnapshotDto snapshot;
    seedSummaryTotals(snapshot, summary);
    addStatusBreakdown(snapshot);

    QMap<int, AnalyticsSnapshotDto::WarehouseStats> warehouseMap;
    QMap<QString, CategoryAccumulator> categoryMap;
    QMap<QString, ProductTypeAccumulator> typeMap;

    const QMap<int, int> orderCapturedUnits = capturedUnitsByOrder(records);
    addOrderDerivedFields(snapshot, summary, logs, warehouseMap, orderCapturedUnits);
    addRecordDerivedFields(snapshot, records, warehouseMap, categoryMap, typeMap);
    applyBackendManagerMetrics(snapshot, analyticsManager, warehouseMap, categoryMap, typeMap);
    appendFlattenedMetrics(snapshot, warehouseMap, categoryMap, typeMap);

    snapshot.hasIncompleteItemAnalytics =
        snapshot.expectedLineItems > 0 && snapshot.capturedLineItems < snapshot.expectedLineItems;
    snapshot.hasData = snapshot.totalOrders > 0 || snapshot.hasCapturedAnalytics;
    if (snapshot.hasData) {
        snapshot.generatedAt = QDateTime::currentDateTime();
    }

    return snapshot;
}

std::vector<AnalyticsRecord> recordsFromLogs(const LogsSnapshotDto &logs)
{
    std::vector<AnalyticsRecord> records;
    records.reserve(static_cast<size_t>(logs.analyticsRecords.size()));

    for (const ProcessedAnalyticsRecordDto &recordDto : logs.analyticsRecords) {
        AnalyticsRecord record(recordDto.orderId,
                               recordDto.productId,
                               recordDto.quantity,
                               recordDto.warehouseId,
                               recordDto.itemSucceeded);
        record.setProductType(recordDto.productType.toStdString());
        record.setCategory(recordDto.category.toStdString());
        record.setFinalPrice(recordDto.finalPrice);
        record.setOrderStatus(recordDto.orderStatus.toStdString());
        records.push_back(record);
    }

    return records;
}

} // namespace

namespace AnalyticsAggregator {

AnalyticsSnapshotDto buildAnalytics(const OrdersSummaryDto &summary,
                                    const LogsSnapshotDto &logs)
{
    return buildAnalyticsInternal(summary, logs, nullptr, recordsFromLogs(logs));
}

AnalyticsSnapshotDto buildAnalytics(const OrdersSummaryDto &summary,
                                    const LogsSnapshotDto &logs,
                                    const AnalyticsManager &analyticsManager)
{
    return buildAnalyticsInternal(summary, logs, &analyticsManager, analyticsManager.getAllRecords());
}

} // namespace AnalyticsAggregator
