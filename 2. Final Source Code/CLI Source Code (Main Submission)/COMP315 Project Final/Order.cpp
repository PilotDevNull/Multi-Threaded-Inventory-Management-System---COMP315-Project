#include "Order.h"

//Starts at 1, increments atomically across all threads
std::atomic<int> Order::nextID{1};

Order::Order(int warehouseID)
    : warehouseID(warehouseID), status(Status::PENDING) {
    //fetch_add returns the old value and increments atomically
    orderID = nextID.fetch_add(1);
}

int Order::getOrderID() const { return orderID; }
int Order::getWarehouseID() const { return warehouseID; }
const std::map<int, int>& Order::getCart() const { return cart; }
Order::Status Order::getStatus() const { return status; }

std::string Order::getStatusString() const {
    switch (status) {
        case Status::PENDING:  return "PENDING"; //Not processed yet
        case Status::SUCCESS:  return "SUCCESS"; //All items processed
        case Status::PARTIAL:  return "PARTIAL"; //Some items processed (others did not get processed due to stock)
        case Status::FAILED:   return "FAILED";  //Nothing could be processed
        default:               return "UNKNOWN"; //Protects against unexpected issues or errors
    }
}

void Order::addItem(int productId, int quantity) {
    cart[productId] = quantity;
}

void Order::setStatus(Status s) {
    status = s;
}
