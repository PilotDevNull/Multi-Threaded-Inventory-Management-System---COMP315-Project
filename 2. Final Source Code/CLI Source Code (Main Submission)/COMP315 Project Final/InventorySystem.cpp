#include "InventorySystem.h"
#include "StandardProduct.h"
#include "TaxableProduct.h"
#include "DiscountedProduct.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

void InventorySystem::addProduct(std::shared_ptr<Product> product) {
    std::lock_guard<std::mutex> lock(inventoryMutex);
    products[product->getId()] = product;
}

bool InventorySystem::removeProduct(int productId) {
    std::lock_guard<std::mutex> lock(inventoryMutex);
    auto it = products.find(productId);
    if (it != products.end()) {
        products.erase(it);
        return true;
    }
    return false;
}

std::shared_ptr<Product> InventorySystem::getProduct(int productId) {
    std::lock_guard<std::mutex> lock(inventoryMutex);
    auto it = products.find(productId);
    return (it != products.end()) ? it->second : nullptr;
}

//Lock ensures two workers cant reduce the same stock at the same time
bool InventorySystem::fulfillOrder(int productId, int quantity) {
    std::lock_guard<std::mutex> lock(inventoryMutex);
    auto it = products.find(productId);
    if (it != products.end())
        return it->second->decreaseStock(quantity);
    return false;
}

//Copy data under lock, then print outside so we dont hold the mutex during slow I/O
void InventorySystem::displayInventory() {
    std::vector<std::shared_ptr<Product>> snapshot;
    {
        std::lock_guard<std::mutex> lock(inventoryMutex);
        for (const auto& pair : products)
            snapshot.push_back(pair.second);
    }

    std::cout << "\n" << std::string(100, '-') << "\n";
    std::cout << "                            CURRENT INVENTORY (" << snapshot.size() << " products)\n";
    std::cout << std::string(100, '-') << "\n";
    std::cout << "  " << std::left
              << std::setw(6)  << "ID"    << "| "
              << std::setw(31) << "Name"  << "| "
              << std::setw(14) << "Category" << "| "
              << std::setw(12) << "Base Price" << "| "
              << std::setw(13) << "Final Price" << "| "
              << std::setw(7)  << "Stock" << "| "
              << "Type" << "\n";
    std::cout << "  " << std::string(96, '-') << "\n";
    for (const auto& p : snapshot) {
        std::string name = p->getName();
        if (name.size() > 30) name = name.substr(0, 27) + "...";
        std::cout << "  " << std::left
                  << std::setw(6)  << p->getId()       << "| "
                  << std::setw(31) << name             << "| "
                  << std::setw(14) << p->getCategory() << "| R"
                  << std::right << std::setw(9) << p->getPrice() << " | R"
                  << std::setw(10) << p->calculateFinalPrice() << " | "
                  << std::left  << std::setw(7)  << p->getQuantity() << "| "
                  << p->getType() << "\n";
    }
    std::cout << std::string(100, '-') << "\n\n";
}

//Uses std::find_if to do a partial name match (case-sensitive)
//find_if scans linearly which makes sense here since map is keyed by ID not name
std::vector<std::shared_ptr<Product>> InventorySystem::searchByName(const std::string& keyword) {
    std::lock_guard<std::mutex> lock(inventoryMutex);
    std::vector<std::shared_ptr<Product>> results;

    auto it = products.begin();
    while (it != products.end()) {
        //find_if returns the first match from current position onward
        it = std::find_if(it, products.end(),
            [&keyword](const std::pair<const int, std::shared_ptr<Product>>& entry) {
                return entry.second->getName().find(keyword) != std::string::npos;
            });

        if (it != products.end()) {
            results.push_back(it->second);
            ++it; //Move past this match to keep searching
        }
    }
    return results;
}

std::vector<std::shared_ptr<Product>> InventorySystem::getSortedByPrice() {
    std::lock_guard<std::mutex> lock(inventoryMutex);
    std::vector<std::shared_ptr<Product>> v;
    for (const auto& pair : products) v.push_back(pair.second);
    std::sort(v.begin(), v.end(),
        [](const std::shared_ptr<Product>& a, const std::shared_ptr<Product>& b) {
            return a->calculateFinalPrice() < b->calculateFinalPrice();
        });
    return v;
}

std::vector<std::shared_ptr<Product>> InventorySystem::getSortedByQuantity() {
    std::lock_guard<std::mutex> lock(inventoryMutex);
    std::vector<std::shared_ptr<Product>> v;
    for (const auto& pair : products) v.push_back(pair.second);
    std::sort(v.begin(), v.end(),
        [](const std::shared_ptr<Product>& a, const std::shared_ptr<Product>& b) {
            return a->getQuantity() < b->getQuantity();
        });
    return v;
}


std::vector<std::shared_ptr<Product>> InventorySystem::getSortedByName() {
    std::lock_guard<std::mutex> lock(inventoryMutex);
    std::vector<std::shared_ptr<Product>> v;
    for (const auto& pair : products) v.push_back(pair.second);
    std::sort(v.begin(), v.end(),
        [](const std::shared_ptr<Product>& a, const std::shared_ptr<Product>& b) {
            return a->getName() < b->getName();
        });
    return v;
}
std::vector<std::shared_ptr<Product>> InventorySystem::getSortedByCategory() {
    std::lock_guard<std::mutex> lock(inventoryMutex);

    std::vector<std::shared_ptr<Product>> v;
    for (const auto& pair : products)
        v.push_back(pair.second);

    std::sort(v.begin(), v.end(),
        [](const std::shared_ptr<Product>& a, const std::shared_ptr<Product>& b) {
            return a->getCategory() < b->getCategory();
        });

    return v;
}
std::vector<std::shared_ptr<Product>> InventorySystem::getSortedByType() {
    std::lock_guard<std::mutex> lock(inventoryMutex);

    std::vector<std::shared_ptr<Product>> v;

    for (const auto& pair : products)
        v.push_back(pair.second);

    std::sort(v.begin(), v.end(),
        [](const std::shared_ptr<Product>& a, const std::shared_ptr<Product>& b) {
            return a->getType() < b->getType();
        });

    return v;
}

int InventorySystem::getLastProductId() {
    std::lock_guard<std::mutex> lock(inventoryMutex);
    return products.empty() ? 0 : products.rbegin()->first;
}

int InventorySystem::getProductCount() {
    std::lock_guard<std::mutex> lock(inventoryMutex);
    return static_cast<int>(products.size());
}

//Format: id,name,price,quantity,type,taxRate,discountRate
void InventorySystem::loadFromCSV(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cout << "Error: could not open " << filename << "\n";
        return;
    }

    std::cout << "\nLoading inventory from " << filename << "...\n";
    int loaded = 0;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line.back() == '\r') line.pop_back();

        std::stringstream ss(line);
        std::string token;
        std::string category;
        int id, quantity;
        double price, taxRate, discountRate;
        char type;
        std::string name;

        try {
            std::getline(ss, token, ','); id           = std::stoi(token);
            std::getline(ss, name,  ',');
            std::getline(ss, category, ',');
            std::getline(ss, token, ','); price        = std::stod(token);
            std::getline(ss, token, ','); quantity     = std::stoi(token);
            std::getline(ss, token, ','); type         = token[0];
            std::getline(ss, token, ','); taxRate      = std::stod(token);
            std::getline(ss, token, ','); discountRate = std::stod(token);
        } catch (...) {
            continue; //Skip malformed lines
        }

        //Lock each insert since this could theoretically be called while threads run
        std::lock_guard<std::mutex> lock(inventoryMutex);

        if (type == 'T')
            products[id] = std::make_shared<TaxableProduct>(id, name, category, price, quantity, taxRate > 0 ? taxRate : 0.15);
        else if (type == 'D')
            products[id] = std::make_shared<DiscountedProduct>(id, name, category, price, quantity, discountRate > 0 ? discountRate : 0.10);
        else if (type == 'S')
            products[id] = std::make_shared<StandardProduct>(id, name, category, price, quantity);
        ++loaded;
    }

    file.close();
    std::cout << "Loaded " << loaded << " products into inventory.\n\n";
}

//Saves actual tax/discount rates using dynamic_cast to get the real type
void InventorySystem::exportToCSV(const std::string& filename) {
    std::lock_guard<std::mutex> lock(inventoryMutex);
    std::ofstream file(filename);

    for (const auto& pair : products) {
        auto& p = pair.second;
        char type;
        if (p->getType() == "Taxable") type = 'T';
        else if (p->getType() == "Discounted") type = 'D';
        else type = 'S';

        double taxRate = 0.0;
        double discountRate = 0.0;

        //Downcast to get the actual rate stored in the child class
        if (auto tp = dynamic_cast<TaxableProduct*>(p.get()))
            taxRate = tp->getTaxRate();
        else if (auto dp = dynamic_cast<DiscountedProduct*>(p.get()))
            discountRate = dp->getDiscountRate();

        file << p->getId()       << ","
             << p->getName()     << ","
             << p->getCategory() << ","
             << p->getPrice()    << ","
             << p->getQuantity() << ","
             << type             << ","
             << taxRate          << ","
             << discountRate     << "\n";
    }

    file.close();
    std::cout << "Inventory exported to " << filename << "\n";
}
