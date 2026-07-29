#ifndef INVENTORYSYSTEM_H
#define INVENTORYSYSTEM_H

#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include "Product.h"

//Thread-safe inventory manager
//Mutex protects all reads/writes to the products map
class InventorySystem {
private:
    std::map<int, std::shared_ptr<Product>> products;
    std::mutex inventoryMutex;

public:
    void addProduct(std::shared_ptr<Product> product);
    bool removeProduct(int productId);
    std::shared_ptr<Product> getProduct(int productId);

    //Reduce stock for an order, returns true if stock was enough
    bool fulfillOrder(int productId, int quantity);

    void displayInventory();

    //Uses find_if to search by name (partial match)
    std::vector<std::shared_ptr<Product>> searchByName(const std::string& keyword);

    //Sorting, returns a copy so the original map is not affected
    std::vector<std::shared_ptr<Product>> getSortedByPrice();
    std::vector<std::shared_ptr<Product>> getSortedByQuantity();
    std::vector<std::shared_ptr<Product>> getSortedByName();
    std::vector<std::shared_ptr<Product>> getSortedByCategory();
    std::vector<std::shared_ptr<Product>> getSortedByType();

    int getLastProductId();
    int getProductCount();

    void loadFromCSV(const std::string& filename);
    void exportToCSV(const std::string& filename = "Products.csv");
};

#endif
