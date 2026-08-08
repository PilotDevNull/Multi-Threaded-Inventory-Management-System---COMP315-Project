#ifndef WAREHOUSE_H
#define WAREHOUSE_H

#include <thread>
#include <string>
#include "OrderPool.h"

/*

Represents one physical warehouse

Each warehouse runs its own thread at startup and reads its assigned CSV file, pushing Order objects into the shared OrderPool

CSV format per row:  productId,quantity (one order per row, each row becomes one Order with one item)

*/

class Warehouse {
private:
    int warehouseId;
    std::string csvFile; //Path to this warehouse's order file
    std::thread worker;

public:
    Warehouse(int id, const std::string& csvFile);

    //Starts the loading thread
    void load(OrderPool& pool);

    //Waits for the loading thread to finish
    void join();

    int getId() const;
};

#endif
