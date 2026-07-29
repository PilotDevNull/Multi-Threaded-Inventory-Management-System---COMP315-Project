#include "WorkerThread.h"
#include "Logger.h"
#include <chrono>
#include "AnalyticsRecord.h"
#include "AnalyticsManager.h"

WorkerThread::WorkerThread(int id) : workerId(id) {}

//Start worker thread with reference to shared queue and inventory
void WorkerThread::start(OrderPool& pool, InventorySystem& inventory,AnalyticsManager& analyticsManager) {
    worker = std::thread([this, &pool, &inventory,&analyticsManager]() {

        //Temporary order object reused for each iteration (pool.pop(order) fills it)
        //We use this since its more efficient than allocating a new order each time
        Order order(0);

        while (pool.pop(order)) {

            //Track results per order
            int successCount = 0;
            int failCount    = 0;

            //Collect one record per item; order status is set after the loop
            std::vector<AnalyticsRecord> itemRecords;

            for (const auto& item : order.getCart()) {
                int productId = item.first;
                int quantity  = item.second;

                bool ok = inventory.fulfillOrder(productId, quantity);
                if (ok) {
                    ++successCount;
                } else {
                    ++failCount;
                }
                auto product = inventory.getProduct(productId);
                if (!product) {
                    continue;
                }
                AnalyticsRecord rec(order.getOrderID(), productId, quantity, order.getWarehouseID(), ok);
                rec.setProductType(product->getType());
                rec.setCategory(product->getCategory());
                rec.setFinalPrice(product->calculateFinalPrice());
                itemRecords.push_back(rec);
            }

            //Simulate real world processing delay (database write, network call, etc.)
            //Without this the work is too fast for thread scaling to be visible
            //Currently the bottle neck with such few orders is the logging despite having the lock external
            //Also for some reason anything under 1000 microseconds does not delay it all so thats why the sleep delay is set to 1000 (remove or use a lower number like 100 to check actual results)
            std::this_thread::sleep_for(std::chrono::microseconds(1000));

            if      (failCount == 0)    order.setStatus(Order::Status::SUCCESS);
            else if (successCount == 0) order.setStatus(Order::Status::FAILED);
            else                        order.setStatus(Order::Status::PARTIAL);

            for (auto& rec : itemRecords) {
                rec.setOrderStatus(order.getStatusString());
                analyticsManager.addRecord(rec);
            }

            //Log once per order so the print mutex doesnt bottleneck the threads
            LOG("[Worker " << workerId
                << " | Thread " << std::this_thread::get_id() << "] "
                << "Order #" << order.getOrderID()
                << " | Warehouse " << order.getWarehouseID()
                << " | Items: " << (successCount + failCount)
                << " | " << order.getStatusString());

            {
                std::lock_guard<std::mutex> lock(resultsMutex);
                processed.push_back(order);
            }
        }

        LOG("[Worker " << workerId << "] No more orders - shutting down.");
    });
}

void WorkerThread::join() {
    if (worker.joinable())
        worker.join();
}

//Return processed orders
std::vector<Order> WorkerThread::getProcessed() const {
    std::lock_guard<std::mutex> lock(resultsMutex);
    return processed;
}

int WorkerThread::getSuccessCount() const {
    std::lock_guard<std::mutex> lock(resultsMutex);
    int count = 0;
    for (const auto& o : processed)
        if (o.getStatus() == Order::Status::SUCCESS) ++count;
    return count;
}

int WorkerThread::getFailCount() const {
    std::lock_guard<std::mutex> lock(resultsMutex);
    int count = 0;
    for (const auto& o : processed)
        if (o.getStatus() == Order::Status::FAILED) ++count;
    return count;
}
