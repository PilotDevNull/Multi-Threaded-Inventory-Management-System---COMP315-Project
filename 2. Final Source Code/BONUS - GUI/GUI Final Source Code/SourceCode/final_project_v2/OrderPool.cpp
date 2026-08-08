#include "OrderPool.h"

OrderPool::OrderPool() : loadingComplete(false) {}

//Protect the shared queue
void OrderPool::push(Order order) {
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        orders.push(order);
    }
    cv.notify_one(); //Wake one sleeping worker
}

bool OrderPool::pop(Order& out) {
    std::unique_lock<std::mutex> lock(poolMutex);

    //Sleep until there's an order or we know no more are coming
    cv.wait(lock, [this] {
        return !orders.empty() || loadingComplete;
    });

    //Pool is done and empty, worker should stop
    if (orders.empty())
        return false;

    out = orders.front();
    orders.pop();
    return true;
}

void OrderPool::markLoadingComplete() {
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        loadingComplete = true;
    }
    cv.notify_all(); //Wake all workers so they can check and exit
}

//Reset flag before launching workers
//Without this, workers see loadingComplete = true immediately and exit
void OrderPool::prepareForProcessing() {
    std::lock_guard<std::mutex> lock(poolMutex);
    loadingComplete = false;
}

//Inject a manual order into the pool at runtime
//Sets loadingComplete to false so workers re-enter their wait loop,
//pushes the order, then marks loading complete again.
//If no workers are running the order just sits in the queue
//until the next process batch picks it up.
void OrderPool::addManualOrder(Order order) {
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        orders.push(order);
    }
    cv.notify_one();
}

int OrderPool::size() const {
    std::lock_guard<std::mutex> lock(poolMutex);
    return static_cast<int>(orders.size());
}

bool OrderPool::empty() const {
    std::lock_guard<std::mutex> lock(poolMutex);
    return orders.empty();
}
