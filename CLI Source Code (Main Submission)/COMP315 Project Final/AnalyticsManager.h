#ifndef ANALYTICSMANAGER_H
#define ANALYTICSMANAGER_H

#include "AnalyticsRecord.h"
#include <vector>
#include <string>
#include <map>
#include <mutex>

class AnalyticsManager {
private:
    std::vector<AnalyticsRecord> records;
    mutable std::mutex mtx;

public:
    AnalyticsManager();

    // -------------------------
    //      CORE FUNCTIONS
    // -------------------------
    void addRecord(const AnalyticsRecord& record);
    std::vector<AnalyticsRecord> getAllRecords() const;
    size_t size();
    void clear();
    void printAll();

    // -------------------------
    //           HELPER
    // -------------------------
    std::vector<AnalyticsRecord> filterByWarehouse(int warehouseID) const;

    // -------------------------
    //    WAREHOUSE ANALYTICS
    // -------------------------
    double getWarehouseRevenue(int warehouseID) const;
    double getWarehouseLostRevenue(int warehouseID) const;
    int getWarehouseTotalItems(int warehouseID) const;
    std::map<std::string, double> getWarehouseCategoryPerformance(int warehouseID) const;
    int getWarehouseBestItem(int warehouseID) const;
    std::map<std::string, double> getWarehouseRevenueByType(int warehouseID) const;
    std::map<std::string, int> getWarehouseQuantityByType(int warehouseID) const;
    std::map<std::string, double> getWarehouseFailureRateByType(int warehouseID) const;
    std::map<std::string, double> getWarehouseAveragePriceByType(int warehouseID) const;

    // -------------------------
    //   GLOBAL ANALYTICS
    // -------------------------
    double getGlobalRevenue() const;
    double getGlobalLostRevenue() const;
    int getGlobalTotalItems() const;
    int getGlobalMostPopularItem() const;
    int getGlobalTotalItemsSold() const;
    std::map<std::string, int> getGlobalItemsSoldPerCategory() const;
    std::map<std::string, double> getGlobalCategoryPerformance() const;
    std::map<std::string, double> getGlobalRevenueByType() const;
    std::map<std::string, int> getGlobalQuantityByType() const;
    std::map<std::string, double> getGlobalFailureRateByType() const;
    std::map<std::string, double> getGlobalAveragePriceByType() const;


    // -------------------------
    //    EXPORT FUNCTIONS
    // -------------------------
    void exportGlobalAnalyticsCSV() const;
    void exportWarehouseAnalyticsCSV(int warehouseID) const;
};

#endif
