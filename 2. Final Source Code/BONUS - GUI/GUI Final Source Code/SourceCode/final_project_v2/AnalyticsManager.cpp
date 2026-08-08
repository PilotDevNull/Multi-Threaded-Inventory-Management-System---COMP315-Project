#include "AnalyticsManager.h"
#include <iostream>
#include <iomanip>
#include <memory>
#include <fstream>
#include <filesystem>
#include <map>
#include <set>
#include <vector>
#include <string>

//This Class is Responsible for All our Analytics
//It Accepts an Analytics Record, which is the used to
//Generate Analytics. The idea behinds this class
//Was to make it "Modular", which allows us to
//"Wire" up our GUI easier


AnalyticsManager::AnalyticsManager() {}

void AnalyticsManager::addRecord(const AnalyticsRecord& record) {
    std::lock_guard<std::mutex> lock(mtx);
    records.push_back(record);
}

std::vector<AnalyticsRecord> AnalyticsManager::getAllRecords() const {
    std::lock_guard<std::mutex> lock(mtx);
    return records;
}

size_t AnalyticsManager::size() {
    std::lock_guard<std::mutex> lock(mtx);
    return records.size();
}

void AnalyticsManager::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    records.clear();
}

void AnalyticsManager::printAll() {
    std::lock_guard<std::mutex> lock(mtx);

    std::cout << "\n--- Analytics Records ---\n";
    for (const auto& r : records) {
        std::cout << "ProductID: " << r.getProductID()
                  << " | Qty: " << r.getQuantity()
                  << " | Warehouse: " << r.getWarehouseID()
                  << " | Category: " << r.getCategory()
                  << " | Final Price: R" << r.getFinalPrice()
                  << " | Status: " << (r.getStatus() ? "SUCCESS" : "FAILED")
                  << "\n";
    }
    std::cout << "Total Records: " << records.size() << "\n";
}

// -------------------------
//          HELPER
// -------------------------
//Assists with Category related Analytics
std::vector<AnalyticsRecord> AnalyticsManager::filterByWarehouse(int warehouseID) const {
    std::vector<AnalyticsRecord> result;

    for (const auto& r : records)
        if (r.getWarehouseID() == warehouseID)
            result.push_back(r);

    return result;
}

// -------------------------
// WAREHOUSE ANALYTICS
// -------------------------
//These functions are for getting analytics Related to a Warehouse by Accepting
// a warehouse ID
double AnalyticsManager::getWarehouseRevenue(int warehouseID) const {
    double total = 0;
    for (const auto& r : filterByWarehouse(warehouseID))
        if (r.getStatus())
            total += r.getFinalPrice() * r.getQuantity();
    return total;
}

double AnalyticsManager::getWarehouseLostRevenue(int warehouseID) const {
    double total = 0;
    for (const auto& r : filterByWarehouse(warehouseID))
        if (!r.getStatus())
            total += r.getFinalPrice() * r.getQuantity();
    return total;
}

int AnalyticsManager::getWarehouseTotalItems(int warehouseID) const {
    int total = 0;
    for (const auto& r : filterByWarehouse(warehouseID))
        total += r.getQuantity();
    return total;
}

std::map<std::string, double> AnalyticsManager::getWarehouseCategoryPerformance(int warehouseID) const {
    std::map<std::string, double> result;

    for (const auto& r : filterByWarehouse(warehouseID))
        result[r.getCategory()] += r.getFinalPrice() * r.getQuantity();

    return result;
}

int AnalyticsManager::getWarehouseBestItem(int warehouseID) const {
    std::map<int, int> itemCount;

    for (const auto& r : filterByWarehouse(warehouseID))
        itemCount[r.getProductID()] += r.getQuantity();

    int bestItem = 0;
    int bestQty = 0;

    for (const auto& i : itemCount) {
        if (i.second > bestQty) {
            bestQty = i.second;
            bestItem = i.first;
        }
    }

    return bestItem;
}

// -------------------------
// GLOBAL ANALYTICS
// -------------------------

double AnalyticsManager::getGlobalRevenue() const {
    double total = 0;

    for (const auto& r : records)
        if (r.getStatus())
            total += r.getFinalPrice() * r.getQuantity();

    return total;
}

double AnalyticsManager::getGlobalLostRevenue() const {
    double total = 0;

    for (const auto& r : records)
        if (!r.getStatus())
            total += r.getFinalPrice() * r.getQuantity();

    return total;
}

int AnalyticsManager::getGlobalTotalItems() const {
    int total = 0;

    for (const auto& r : records)
        total += r.getQuantity();

    return total;
}

int AnalyticsManager::getGlobalMostPopularItem() const {
    std::map<int, int> itemCounts;

    for (const auto& r : getAllRecords()) {
        itemCounts[r.getProductID()] += r.getQuantity();
    }

    int bestItem = 0;
    int maxCount = 0;

    for (const auto& i : itemCounts) {
        if (i.second > maxCount) {
            maxCount = i.second;
            bestItem = i.first;
        }
    }

    return bestItem;
}

int AnalyticsManager::getGlobalTotalItemsSold() const {
    int totalItems = 0;

    for (const auto& r : getAllRecords()) {
        totalItems += r.getQuantity();
    }

    return totalItems;
}

std::map<std::string, int> AnalyticsManager::getGlobalItemsSoldPerCategory() const {
    std::map<std::string, int> result;

    for (const auto& r : getAllRecords()) {
        result[r.getCategory()] += r.getQuantity();
    }

    return result;
}

std::map<std::string, double> AnalyticsManager::getGlobalCategoryPerformance() const {
    std::map<std::string, double> result;

    for (const auto& r : records)
        result[r.getCategory()] += r.getFinalPrice() * r.getQuantity();

    return result;
}

// -------------------------
// PRODUCT TYPE ANALYTICS (NEW)
// -------------------------

std::map<std::string, double> AnalyticsManager::getGlobalRevenueByType() const {
    std::map<std::string, double> result;

    for (const auto& r : records) {
        if (r.getStatus())
            result[r.getType()] += r.getFinalPrice() * r.getQuantity();
    }

    return result;
}

std::map<std::string, int> AnalyticsManager::getGlobalQuantityByType() const {
    std::map<std::string, int> result;

    for (const auto& r : records) {
        result[r.getType()] += r.getQuantity();
    }

    return result;
}

std::map<std::string, double> AnalyticsManager::getGlobalFailureRateByType() const {
    std::map<std::string, int> total;
    std::map<std::string, int> failed;
    std::map<std::string, double> result;

    for (const auto& r : records) {
        total[r.getType()]++;
        if (!r.getStatus())
            failed[r.getType()]++;
    }

    for (const auto& t : total) {
        result[t.first] = (double)failed[t.first] / t.second;
    }

    return result;
}

std::map<std::string, double> AnalyticsManager::getGlobalAveragePriceByType() const {
    std::map<std::string, double> total;
    std::map<std::string, int> count;
    std::map<std::string, double> result;

    for (const auto& r : records) {
        total[r.getType()] += r.getFinalPrice();
        count[r.getType()]++;
    }

    for (const auto& t : total) {
        result[t.first] = t.second / count[t.first];
    }

    return result;
}

// -------------------------
// WAREHOUSE TYPE ANALYTICS (NEW)
// -------------------------

std::map<std::string, double> AnalyticsManager::getWarehouseRevenueByType(int warehouseID) const {
    std::map<std::string, double> result;

    for (const auto& r : records) {
        if (r.getWarehouseID() == warehouseID && r.getStatus())
            result[r.getType()] += r.getFinalPrice() * r.getQuantity();
    }

    return result;
}

std::map<std::string, int> AnalyticsManager::getWarehouseQuantityByType(int warehouseID) const {
    std::map<std::string, int> result;

    for (const auto& r : records) {
        if (r.getWarehouseID() == warehouseID)
            result[r.getType()] += r.getQuantity();
    }

    return result;
}

std::map<std::string, double> AnalyticsManager::getWarehouseFailureRateByType(int warehouseID) const {
    std::map<std::string, int> total;
    std::map<std::string, int> failed;
    std::map<std::string, double> result;

    for (const auto& r : records) {
        if (r.getWarehouseID() == warehouseID) {
            total[r.getType()]++;
            if (!r.getStatus())
                failed[r.getType()]++;
        }
    }

    for (const auto& t : total) {
        result[t.first] = (double)failed[t.first] / t.second;
    }

    return result;
}

std::map<std::string, double> AnalyticsManager::getWarehouseAveragePriceByType(int warehouseID) const {
    std::map<std::string, double> total;
    std::map<std::string, int> count;
    std::map<std::string, double> result;

    for (const auto& r : records) {
        if (r.getWarehouseID() == warehouseID) {
            total[r.getType()] += r.getFinalPrice();
            count[r.getType()]++;
        }
    }

    for (const auto& t : total) {
        result[t.first] = t.second / count[t.first];
    }

    return result;
}

// -------------------------
// EXPORT FUNCTIONS
// -------------------------

void AnalyticsManager::exportGlobalAnalyticsCSV(const std::string& basePath) const {
    std::filesystem::create_directories(basePath);
    std::ofstream itemsFile(basePath + "GlobalItemRecords.csv");

    itemsFile << "OrderID,WarehouseID,ProductID,ProductType,Category,Quantity,FinalPrice,ItemStatus,OrderStatus\n";

    for (const auto& r : getAllRecords()) {
        itemsFile << r.getOrderID() << ","
                  << r.getWarehouseID() << ","
                  << r.getProductID() << ","
                  << r.getProductType() << ","
                  << r.getCategory() << ","
                  << r.getQuantity() << ","
                  << r.getFinalPrice() << ","
                  << (r.getStatus() ? "SUCCESS" : "FAILED") << ","
                  << r.getOrderStatus()
                  << "\n";
    }

    itemsFile.close();
}

void AnalyticsManager::exportWarehouseAnalyticsCSV(int warehouseID, const std::string& basePath) const {
    std::string dir = basePath + "Warehouse " + std::to_string(warehouseID) + "/";
    std::filesystem::create_directories(dir);

    std::ofstream itemsFile(
        dir + "Warehouse" + std::to_string(warehouseID) + "ItemRecords.csv"
    );

    itemsFile << "OrderID,WarehouseID,ProductID,ProductType,Category,Quantity,FinalPrice,ItemStatus,OrderStatus\n";

    for (const auto& r : getAllRecords()) {
        if (r.getWarehouseID() != warehouseID)
            continue;

        itemsFile << r.getOrderID() << ","
                  << r.getWarehouseID() << ","
                  << r.getProductID() << ","
                  << r.getProductType() << ","
                  << r.getCategory() << ","
                  << r.getQuantity() << ","
                  << r.getFinalPrice() << ","
                  << (r.getStatus() ? "SUCCESS" : "FAILED") << ","
                  << r.getOrderStatus()
                  << "\n";
    }

    itemsFile.close();
}
