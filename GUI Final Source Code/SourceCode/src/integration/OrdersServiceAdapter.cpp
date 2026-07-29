#include "OrdersServiceAdapter.h"

#include <algorithm>
#include <exception>
#include <map>
#include <memory>
#include <vector>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include "AnalyticsManager.h"
#include "AnalyticsAggregator.h"
#include "InventorySystem.h"
#include "LogsSnapshotDto.h"
#include "Order.h"
#include "OrderPool.h"
#include "Warehouse.h"
#include "WorkerThread.h"

namespace {

QString canonicalFilePath(const QString &candidatePath)
{
    const QFileInfo info(candidatePath);
    if (!info.exists() || !info.isFile()) {
        return {};
    }

    const QString canonicalPath = info.canonicalFilePath();
    return canonicalPath.isEmpty() ? info.absoluteFilePath() : canonicalPath;
}

QString resolveProjectFilePath(const QString &relativePath)
{
    QDir searchDir(QCoreApplication::applicationDirPath());
    for (int depth = 0; depth < 6; ++depth) {
        const QString candidate = canonicalFilePath(searchDir.filePath(relativePath));
        if (!candidate.isEmpty()) {
            return candidate;
        }

        if (!searchDir.cdUp()) {
            break;
        }
    }

#ifdef NEXUS_SOURCE_DIR
    const QString sourceCandidate =
        canonicalFilePath(QDir(QStringLiteral(NEXUS_SOURCE_DIR)).filePath(relativePath));
    if (!sourceCandidate.isEmpty()) {
        return sourceCandidate;
    }
#endif

    return {};
}

} // namespace

QString OrdersServiceAdapter::resolveProductsCsvPath() const
{
    return resolveProjectFilePath(QStringLiteral("final_project_v2/Catalog/Products.csv"));
}

QString OrdersServiceAdapter::resolveWarehouseOrdersPath(int warehouseId) const
{
    return resolveProjectFilePath(
        QStringLiteral("final_project_v2/Orders/Warehouse%1_Orders.csv").arg(warehouseId));
}

OrdersBatchResultDto OrdersServiceAdapter::processDefaultWarehouses(int workerCount,
                                                                     const QVector<ManualOrderEntry> &manualOrders) const
{
    OrdersBatchResultDto result;

    try {
        const QString productsCsvPath = resolveProductsCsvPath();
        if (productsCsvPath.isEmpty()) {
            result.errorMessage =
                QStringLiteral("Could not resolve final_project_v2/Catalog/Products.csv from the application path.");
            return result;
        }

        InventorySystem inventory;
        inventory.loadFromCSV(productsCsvPath.toStdString());

        OrderPool orderPool;
        std::vector<std::unique_ptr<Warehouse>> warehouses;
        warehouses.reserve(kWarehouseCount);

        for (int warehouseId = 1; warehouseId <= kWarehouseCount; ++warehouseId) {
            const QString warehouseOrdersPath = resolveWarehouseOrdersPath(warehouseId);
            if (warehouseOrdersPath.isEmpty()) {
                result.errorMessage = QStringLiteral(
                    "Could not resolve final_project_v2/Orders/Warehouse%1_Orders.csv from the application path.")
                                          .arg(warehouseId);
                return result;
            }

            warehouses.push_back(std::make_unique<Warehouse>(
                warehouseId, warehouseOrdersPath.toStdString()));
        }

        for (const auto &warehouse : warehouses) {
            warehouse->load(orderPool);
        }

        for (const auto &warehouse : warehouses) {
            warehouse->join();
        }

        orderPool.markLoadingComplete();

        //Inject any manual orders the user queued through the GUI
        for (const ManualOrderEntry &entry : manualOrders) {
            Order manualOrder(entry.warehouseId);
            for (const auto &[productId, qty] : entry.items) {
                if (qty > 0) {
                    manualOrder.addItem(productId, qty);
                }
            }
            if (!manualOrder.getCart().empty()) {
                orderPool.addManualOrder(manualOrder);
            }
        }

        const int clampedWorkerCount = std::clamp(workerCount, 1, kMaxWorkerCount);
        if (!orderPool.empty()) {
            orderPool.prepareForProcessing();

            AnalyticsManager analyticsManager;
            std::vector<std::unique_ptr<WorkerThread>> workers;
            workers.reserve(clampedWorkerCount);
            for (int workerId = 1; workerId <= clampedWorkerCount; ++workerId) {
                auto worker = std::make_unique<WorkerThread>(workerId);
                worker->start(orderPool, inventory, analyticsManager);
                workers.push_back(std::move(worker));
            }

            orderPool.markLoadingComplete();

            for (const auto &worker : workers) {
                worker->join();
            }

            const std::vector<AnalyticsRecord> analyticsRecords = analyticsManager.getAllRecords();
            result.logsSnapshot.analyticsRecords.reserve(
                static_cast<qsizetype>(analyticsRecords.size()));
            for (const AnalyticsRecord &record : analyticsRecords) {
                ProcessedAnalyticsRecordDto recordDto;
                recordDto.orderId = record.getOrderID();
                recordDto.productId = record.getProductID();
                recordDto.productType = QString::fromStdString(record.getProductType());
                recordDto.category = QString::fromStdString(record.getCategory());
                recordDto.quantity = record.getQuantity();
                recordDto.warehouseId = record.getWarehouseID();
                recordDto.itemSucceeded = record.getStatus();
                recordDto.itemStatus =
                    recordDto.itemSucceeded ? QStringLiteral("SUCCESS") : QStringLiteral("FAILED");
                recordDto.orderStatus = QString::fromStdString(record.getOrderStatus());
                recordDto.finalPrice = record.getFinalPrice();
                result.logsSnapshot.analyticsRecords.push_back(std::move(recordDto));
            }
            std::sort(result.logsSnapshot.analyticsRecords.begin(),
                      result.logsSnapshot.analyticsRecords.end(),
                      [](const ProcessedAnalyticsRecordDto &lhs,
                         const ProcessedAnalyticsRecordDto &rhs) {
                          if (lhs.orderId != rhs.orderId) {
                              return lhs.orderId < rhs.orderId;
                          }
                          if (lhs.productId != rhs.productId) {
                              return lhs.productId < rhs.productId;
                          }
                          return lhs.warehouseId < rhs.warehouseId;
                      });

            std::map<int, OrdersWarehouseCountDto> perWarehouseCounts;
            for (int warehouseId = 1; warehouseId <= kWarehouseCount; ++warehouseId) {
                OrdersWarehouseCountDto counts;
                counts.warehouseId = warehouseId;
                perWarehouseCounts.emplace(warehouseId, counts);
            }

            for (const auto &worker : workers) {
                const std::vector<Order> processedOrders = worker->getProcessed();
                for (const Order &order : processedOrders) {
                    ++result.summary.totalOrders;

                    OrdersWarehouseCountDto &warehouseCounts =
                        perWarehouseCounts[order.getWarehouseID()];
                    ++warehouseCounts.totalOrders;

                    switch (order.getStatus()) {
                    case Order::Status::SUCCESS:
                        ++result.summary.successCount;
                        ++warehouseCounts.successCount;
                        break;
                    case Order::Status::PARTIAL:
                        ++result.summary.partialCount;
                        ++warehouseCounts.partialCount;
                        break;
                    case Order::Status::FAILED:
                        ++result.summary.failedCount;
                        ++warehouseCounts.failedCount;
                        break;
                    case Order::Status::PENDING:
                    default:
                        break;
                    }

                    // Build per-order DTO for the Logs page.
                    // Reads only public accessors on Order — backend logic untouched.
                    ProcessedOrderDto orderDto;
                    orderDto.runtimeOrderId = order.getOrderID();
                    orderDto.warehouseId    = order.getWarehouseID();
                    orderDto.status = QString::fromStdString(order.getStatusString());
                    for (const auto &[productId, qty] : order.getCart()) {
                        ProcessedOrderItemDto itemDto;
                        itemDto.productId = productId;
                        itemDto.quantity  = qty;
                        orderDto.items.push_back(itemDto);
                    }
                    result.logsSnapshot.orders.push_back(std::move(orderDto));
                }
            }

            // Mirror the per-order totals into the snapshot counters.
            result.logsSnapshot.totalCount   = result.summary.totalOrders;
            result.logsSnapshot.successCount = result.summary.successCount;
            result.logsSnapshot.partialCount = result.summary.partialCount;
            result.logsSnapshot.failedCount  = result.summary.failedCount;

            for (int warehouseId = 1; warehouseId <= kWarehouseCount; ++warehouseId) {
                result.summary.perWarehouseCounts.push_back(perWarehouseCounts[warehouseId]);
            }

            result.analyticsSnapshot = AnalyticsAggregator::buildAnalytics(
                result.summary, result.logsSnapshot, analyticsManager);
        } else {
            for (int warehouseId = 1; warehouseId <= kWarehouseCount; ++warehouseId) {
                OrdersWarehouseCountDto counts;
                counts.warehouseId = warehouseId;
                result.summary.perWarehouseCounts.push_back(counts);
            }

            result.analyticsSnapshot = AnalyticsAggregator::buildAnalytics(
                result.summary, result.logsSnapshot);
        }
    } catch (const std::exception &exception) {
        result.errorMessage = QString::fromLocal8Bit(exception.what());
    } catch (...) {
        result.errorMessage = QStringLiteral("Unknown error while running the orders batch.");
    }

    return result;
}
