#ifndef ORDERPOOL_H
#define ORDERPOOL_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include "Order.h"

//Thread-safe order queue shared between producers (warehouses) and consumers (workers)
//Uses condition_variable so workers sleep instead of busy-waiting (wastes CPU)
class OrderPool {
private:
    std::queue<Order> orders;
    mutable std::mutex poolMutex; //Have to use mutable to lock const functions
    std::condition_variable cv;

    //When true, workers know no more orders are coming
    bool loadingComplete;

public:
    OrderPool();

    void push(Order order);

    //Blocks until an order is available or pool is done
    //Returns false when pool is empty and complete -> worker should exit
    bool pop(Order& out);

    //Called after all warehouses finish loading
    void markLoadingComplete();

    //Resets the complete flag so workers can be launched again
    void prepareForProcessing();

    int size() const;
    bool empty() const;
};

#endif
