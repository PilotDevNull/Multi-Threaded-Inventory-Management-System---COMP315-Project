#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <thread>
#include <vector>
#include <mutex>
#include "OrderPool.h"
#include "InventorySystem.h"
#include "AnalyticsRecord.h"
#include "AnalyticsManager.h"



/*

A WorkerThread continuously pulls Orders from the OrderPool and attempts to fulfill each item in the order's cart

Multiple WorkerThreads run in parallel, the number is chosen by the user at runtime (4, 8, 16, etc.) to scale to hardware

*/

class WorkerThread {
private:
    int workerId;
    std::thread worker;

    //Orders this thread has processed (for reporting)
    std::vector<Order> processed;
    mutable std::mutex resultsMutex;

public:
    WorkerThread(int id);

    //Start processing, pulls from pool until empty + loading done
    void start(OrderPool& pool, InventorySystem& inventory,AnalyticsManager& analyticsManager);

    void join();

    //Stats for summary report
    std::vector<Order> getProcessed() const;
    int getSuccessCount() const;
    int getFailCount() const;
};

#endif
