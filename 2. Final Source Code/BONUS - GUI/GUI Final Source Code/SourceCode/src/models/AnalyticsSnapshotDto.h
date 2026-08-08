#ifndef ANALYTICSSNAPSHOTDTO_H
#define ANALYTICSSNAPSHOTDTO_H

#include <QDateTime>
#include <QString>
#include <QVector>

// Immutable analytics snapshot for the last completed orders batch.
// Revenue fields are captured from final_project_v2 AnalyticsManager records.
struct AnalyticsSnapshotDto {

    // Batch totals.
    int totalOrders  = 0;
    int successCount = 0;
    int partialCount = 0;
    int failedCount  = 0;

    // Quantity totals.
    int totalRequestedUnits = 0;
    int totalCapturedUnits  = 0;
    int totalProcessedUnits = 0;
    int totalLostUnits      = 0;
    int expectedLineItems   = 0;
    int capturedLineItems   = 0;

    // Backend financial totals. These represent captured analytics records.
    double globalRevenue     = 0.0;
    double globalLostRevenue = 0.0;

    struct WarehouseStats {
        int warehouseId    = 0;
        int orderCount     = 0;
        int successCount   = 0;
        int partialCount   = 0;
        int failedCount    = 0;
        int requestedUnits = 0;
        int capturedUnits  = 0;
        int processedUnits = 0;
        int lostUnits      = 0;
        double revenue     = 0.0;
        double lostRevenue = 0.0;
    };
    QVector<WarehouseStats> warehouseStats;

    struct StatusStats {
        QString status;
        int count = 0;
    };
    QVector<StatusStats> statusBreakdown;

    struct CategoryStats {
        QString category;
        int quantity = 0;
        double revenue = 0.0;
        double lostRevenue = 0.0;
    };
    QVector<CategoryStats> categoryStats;

    struct ProductTypeStats {
        QString productType;
        int quantity = 0;
        double revenue = 0.0;
        double lostRevenue = 0.0;
        double failureRate = 0.0;
    };
    QVector<ProductTypeStats> productTypeStats;

    // Table rows are capped so the page stays compact.
    struct SuccessRow {
        int orderId        = 0;
        int warehouseId    = 0;
        int lineItems      = 0;
        int requestedUnits = 0;
        int capturedUnits  = 0;
    };
    QVector<SuccessRow> successRows;

    struct NonSuccessRow {
        int orderId        = 0;
        int warehouseId    = 0;
        int requestedUnits = 0;
        int capturedUnits  = 0;
        QString status;            // "PARTIAL" or "FAILED"
    };
    QVector<NonSuccessRow> nonSuccessRows;

    bool hasData = false;
    bool hasCapturedAnalytics = false;
    bool hasIncompleteItemAnalytics = false;
    QDateTime generatedAt;
};

#endif // ANALYTICSSNAPSHOTDTO_H
