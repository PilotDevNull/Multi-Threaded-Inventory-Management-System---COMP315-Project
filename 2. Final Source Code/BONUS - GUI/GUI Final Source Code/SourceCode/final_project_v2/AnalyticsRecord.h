#ifndef ANALYTICS_RECORD_H
#define ANALYTICS_RECORD_H

#include <string>

class AnalyticsRecord {
private:
    int orderID;
    int productID;
    std::string type;
    int quantity;
    int warehouseID;

    bool status; // item success/fail

    std::string orderStatus; // ORDER-level status (SUCCESS / PARTIAL / FAILED)

    std::string category;
    double finalPrice;

public:
    AnalyticsRecord();

    AnalyticsRecord(int orderID,
                     int productID,
                     int quantity,
                     int warehouseID,
                     bool status);

    int getOrderID() const;
    int getProductID() const;
    std::string getProductType() const;
    int getQuantity() const;
    int getWarehouseID() const;
    std::string getType() const;

    bool getStatus() const;

    std::string getOrderStatus() const;

    std::string getCategory() const;
    double getFinalPrice() const;

    void setCategory(const std::string& category);
    void setFinalPrice(double price);
    void setOrderStatus(const std::string& status);
    void setProductType(const std::string& type);
};

#endif
