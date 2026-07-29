#include "AnalyticsRecord.h"

// We Needed a way to track the items being ordered
// Thus,the  Analytics Record Class was created
// This Class creates a record of EVERY item in an orders cart
// and stores it for the Analytics Manager to Hold and Process


//Our main idea behind the analytics aspect is to
//collect them without making any major changes to how
//Multi-threading works. Because of that, we had to declare
// A default constructor, since some aspects (such as orderStatus)
//can only be set later on.
AnalyticsRecord::AnalyticsRecord()
    : orderID(-1),
      productID(-1),
      quantity(0),
      warehouseID(-1),
      status(false),
      orderStatus("PENDING"),
      category("UNKNOWN"),
      finalPrice(0.0)
{
}

AnalyticsRecord::AnalyticsRecord(
    int orderID,
    int productID,
    int quantity,
    int warehouseID,
    bool status)
    : orderID(orderID),
      productID(productID),
      type("UNKNOWN"),
      quantity(quantity),
      warehouseID(warehouseID),
      status(status),
      orderStatus("PENDING"),
      category("UNKNOWN"),
      finalPrice(0.0)
{
}

int AnalyticsRecord::getOrderID() const {
    return orderID;
}

int AnalyticsRecord::getProductID() const {
    return productID;
}
std::string AnalyticsRecord::getType() const{
    return type;
}

int AnalyticsRecord::getQuantity() const {
    return quantity;
}

int AnalyticsRecord::getWarehouseID() const {
    return warehouseID;
}

bool AnalyticsRecord::getStatus() const {
    return status;
}

std::string AnalyticsRecord::getOrderStatus() const {
    return orderStatus;
}

std::string AnalyticsRecord::getCategory() const {
    return category;
}
std::string AnalyticsRecord::getProductType() const {
    return type;
}

double AnalyticsRecord::getFinalPrice() const {
    return finalPrice;
}

void AnalyticsRecord::setCategory(const std::string& category) {
    this->category = category;
}

void AnalyticsRecord::setFinalPrice(double price) {
    this->finalPrice = price;
}

void AnalyticsRecord::setOrderStatus(const std::string& status) {
    this->orderStatus = status;
}
void AnalyticsRecord::setProductType(const std::string& type) {
    this->type = type;
}
