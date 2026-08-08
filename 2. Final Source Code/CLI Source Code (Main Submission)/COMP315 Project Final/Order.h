#ifndef ORDER_H
#define ORDER_H

#include <map>
#include <string>
#include <atomic>

//Represents a single customer order loaded from a warehouse CSV
//Cart maps productId -> quantity requested
//Status starts as pending, updated after worker processes it
class Order {
public:
    enum class Status { PENDING, SUCCESS, PARTIAL, FAILED };

private:
    //Atomic so multiple warehouse threads can safely create orders at the same time
    static std::atomic<int> nextID;

    int orderID;
    int warehouseID;
    std::map<int, int> cart; //productId -> quantity
    Status status;

public:
    Order(int warehouseID);

    int getOrderID() const;
    int getWarehouseID() const;
    const std::map<int, int>& getCart() const;
    Status getStatus() const;
    std::string getStatusString() const; //Used to convert enum status to string

    void addItem(int productId, int quantity);
    void setStatus(Status s);
};

#endif
