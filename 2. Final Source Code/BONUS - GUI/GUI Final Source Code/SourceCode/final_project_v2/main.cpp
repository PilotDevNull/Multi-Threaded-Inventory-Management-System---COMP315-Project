#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include <thread>
#include <string>
#include <limits>
#include <chrono>
#include "InventorySystem.h"
#include "StandardProduct.h"
#include "TaxableProduct.h"
#include "DiscountedProduct.h"
#include "OrderPool.h"
#include "Warehouse.h"
#include "WorkerThread.h"
#include "AnalyticsManager.h"
#include <iomanip>
#include <fstream>
#include <iostream>
#include <limits>

//||||******************START OF Forward declarations******************||||

void cliLayout();
void pauseForUser();
void optionOneAddProduct(InventorySystem& inventory);
void optionTwoRemoveProduct(InventorySystem& inventory);
void optionThreeEditProductDetails(InventorySystem& inventory);
void optionFourDisplay(InventorySystem& inventory);
void optionFiveSearch(InventorySystem& inventory);
void optionSixSort(InventorySystem& inventory);
void optionSevenProcessOrders(
    InventorySystem& inventory,
    OrderPool& pool,
    AnalyticsManager& analyticsManager,
    std::vector<std::unique_ptr<WorkerThread>>& workers
);

//Analytics Functions
void optionEightAnalytics(
    AnalyticsManager& analyticsManager,
    const std::vector<std::unique_ptr<WorkerThread>>& workers
);
void GlobalReport(
    const AnalyticsManager& analyticsManager,
    const std::vector<std::unique_ptr<WorkerThread>>& workers);

void WarehouseReport(
    int warehouseID,
    const AnalyticsManager& analyticsManager,
    const std::vector<std::unique_ptr<WorkerThread>>& workers);
void optionNineShutdown(InventorySystem& inventory);
void optionTenAddManualOrder(OrderPool& pool, InventorySystem& inventory);


//||||******************END OF Forward declarations******************||||





//||||************** START OF INPUT VALIDATION FUNCTIONS*************||||
//Helper to safely read an int from cin
//Clears the error state and flushes bad input if user types letters
int readInt(const std::string& prompt) {
    int value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  Invalid input. Please enter a number.\n";
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
    }
}

//Same but for doubles
double readDouble(const std::string& prompt) {
    double value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  Invalid input. Please enter a number.\n";
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
    }
}
//||||************** END OF INPUT VALIDATION FUNCTIONS*************||||



//||||****************START OF MAIN WITH CLI************************||||

void cliLayout() {
    std::cout << "\n-------------------------------------------------------------\n\n";
    std::cout << "       ||---- Inventory Management System ----||          \n";
    std::cout << "      Powered by Group Twelve Software Solutions LTD     \n\n";
    std::cout << "-------------------------------------------------------------\n\n";
    std::cout << "         ||---- Main Menu (Enter 1 - 10) ----||             \n\n";
    std::cout << "           1. Add a new product\n";
    std::cout << "           2. Remove a product\n";
    std::cout << "           3. Edit a product\n";
    std::cout << "           4. Display all products\n";
    std::cout << "           5. Search\n";
    std::cout << "           6. Sort products\n";
    std::cout << "           7. Process orders (Multi-Threaded)\n";
    std::cout << "           8. Analytics \n";
    std::cout << "           9. Save & Shutdown\n";
    std::cout << "           10. Add a manual order\n";
    std::cout << "\n-------------------------------------------------------------\n";
}
void pauseForUser()
{
    std::cout << "\nPress ENTER to return to menu...";
    std::cin.get();
}


int main() {

    InventorySystem inventory;
    AnalyticsManager analyticsManager;
    //adding a flag so that the Analytics Manager Cannot be run
    //without Processing Orders
    bool analyticsManagerFlag = false;
    std::vector<std::unique_ptr<WorkerThread>> workers;

    //Load product catalogue from CSV on startup
    inventory.loadFromCSV("Catalog/Products.csv");
    std::cout << "System ready. " << inventory.getProductCount()
              << " products loaded.\n";

    //Phase 1: warehouse threads load orders into the pool
    std::cout << "\n=== PHASE 1: Warehouse Loading ===\n";
    std::cout << "Launching 5 warehouse threads to read order files...\n\n";

    OrderPool orderPool;

    std::vector<std::unique_ptr<Warehouse>> warehouses;
    for (int i = 1; i <= 5; ++i) {
        std::string csvFile = "Orders/Warehouse" + std::to_string(i) + "_Orders.csv";
        warehouses.push_back(std::make_unique<Warehouse>(i, csvFile));
    }

    //Start all 5 warehouse threads at once
    for (auto& w : warehouses)
        w->load(orderPool);

    //Wait for all warehouse threads to finish
    for (auto& w : warehouses)
        w->join();

    //Tell pool that loading is done (workers spawned later will know when to stop)
    orderPool.markLoadingComplete();

    std::cout << "\n=== All warehouses loaded. "
              << orderPool.size() << " orders queued. ===\n";

    //Main menu
    while (true) {
        cliLayout();
        int choice = readInt("Enter option: ");

        switch (choice) {
            case 1: optionOneAddProduct(inventory);              break;
            case 2: optionTwoRemoveProduct(inventory);           break;
            case 3: optionThreeEditProductDetails(inventory); break;
            case 4: optionFourDisplay(inventory);               break;
            case 5: optionFiveSearch(inventory);                 break;
            case 6: optionSixSort(inventory);                   break;
            case 7: optionSevenProcessOrders(inventory, orderPool,analyticsManager,workers);
                     analyticsManagerFlag = true;break;
            case 8: if(analyticsManagerFlag){
                optionEightAnalytics(analyticsManager,workers);
                } else {
                    std::cout << "\nOrder Processing is required before running the Analytics Manager\n";
                }
                 break;
            case 9: optionNineShutdown(inventory); return 0;
            case 10: optionTenAddManualOrder(orderPool, inventory); break;
            default: std::cout << "\nInvalid option. Try 1-10.\n";
        }

        std::cout << "\nPress ENTER to return to menu...";
        std::cin.get();
    }

    return 0;
}

//||||****************END OF MAIN WITH CLI************************||||



//||||****************Option 1: Add product***********************||||

void optionOneAddProduct(InventorySystem& inventory) {
    int id = inventory.getLastProductId() + 1;

    // -------- PRODUCT TYPE MENU --------
    std::cout << "\n||------- Select Product Type -------||\n\n";
    std::cout << "     1.  Standard (Zero-Rated Item - Perishables ONLY)\n";
    std::cout << "     2.  Taxable\n";
    std::cout << "     3.  Discounted\n\n";
    std::cout << "Choice: ";

    int typeChoice;
    std::cin >> typeChoice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // -------- NAME --------
    std::cout << "  Name: ";
    std::string name;
    std::getline(std::cin, name);

    // -------- CATEGORY MENU --------
    std::cout << "\n||------- Select Warehouse -------||\n\n";
    std::cout << "\n||------- Select Category -------||\n\n";
    std::cout << "     1.  Electronics\n";
    std::cout << "     2.  Perishables\n";
    std::cout << "     3.  Clothing\n";
    std::cout << "     4.  Home\n";
    std::cout << "     5.  Sports\n";
    std::cout << "     6.  Toys\n";
    std::cout << "     7.  Automotive\n";
    std::cout << "     8.  Books\n";
    std::cout << "     9.  Beauty\n";
    std::cout << "     10. Garden\n";
    std::cout << "     11. Office\n";

    int choice = readInt("\n  Category Choice: ");
    std::string category;

    switch (choice) {
        case 1: category = "Electronics"; break;
        case 2: category = "Perishables"; break;
        case 3: category = "Clothing"; break;
        case 4: category = "Home"; break;
        case 5: category = "Sports"; break;
        case 6: category = "Toys"; break;
        case 7: category = "Automotive"; break;
        case 8: category = "Books"; break;
        case 9: category = "Beauty"; break;
        case 10: category = "Garden"; break;
        case 11: category = "Office"; break;
        default: category = "Unknown"; break;
    }

    // -------- CLEAN CATEGORY (IMPORTANT FIX) --------
    category.erase(category.find_last_not_of(" \n\r\t") + 1);
    category.erase(0, category.find_first_not_of(" \n\r\t"));

    // -------- RULE ENFORCEMENT --------
    if (typeChoice == 1 && category != "Perishables") {
        std::cout << "\n Invalid Category for Standard Products.\n";
        std::cout << "   Standard (Tax-Free) products are only allowed in the 'Perishables' category.\n";
        std::cout << "   Product was NOT added. Returning to main menu...\n\n";
        return;
    }

    // -------- PRICE / QUANTITY --------
    double price = readDouble("  Price: R");
    int quantity = readInt("  Quantity: ");

    // -------- PRODUCT CREATION --------
    switch (typeChoice) {
        case 2: {
            double rate = readDouble("  Tax rate (e.g. 0.15): ");
            inventory.addProduct(std::make_shared<TaxableProduct>(
                id, name, category, price, quantity, rate));
            break;
        }

        case 3: {
            double rate = readDouble("  Discount rate (e.g. 0.10): ");
            inventory.addProduct(std::make_shared<DiscountedProduct>(
                id, name, category, price, quantity, rate));
            break;
        }

        case 1:
        default: {
            inventory.addProduct(std::make_shared<StandardProduct>(
                id, name, category, price, quantity));
            break;
        }
    }

    std::cout << "\n  Product Added | ID=" << id << " | " << name
              << " | " << category << " | Stock=" << quantity << "\n";
}
//||||*****************************************************************||||



////||||****************Option 2: Remove product***********************||||

void optionTwoRemoveProduct(InventorySystem& inventory) {

    std::cout << "\n     1. Remove by ID\n";
    std::cout << "     2. Search and Remove\n";
    int choice = readInt("\nChoice: ");


    int id = -1;

    //Sub-Menu
    if (choice == 1) {
        id = readInt("\n  Enter Product ID: ");
    }
    else if (choice == 2) {

        std::cout << "\  Enter product name (or part of it): ";
        std::string keyword;
        std::getline(std::cin, keyword);

        std::vector<std::shared_ptr<Product>> results = inventory.searchByName(keyword);

        if (results.empty()) {
            std::cout << "\n  No products found.\n";
            return;
        }

        std::cout << "\n||---- Search Results ( " << results.size() <<" ) ----||\n";

        for (const auto& p : results) {
            std::cout << "    ID: " << p->getId()
                      << " | " << p->getName()
                      << " | Category: " << p->getCategory()
                      << " | Final: R" << p->calculateFinalPrice()
                      << " | Stock: " << p->getQuantity()
                      << "\n";
        }

        id = readInt("\n  Enter ID of product to remove: ");
    }
    else {
        std::cout << "  Invalid choice.\n";
        return;
    }

    auto product = inventory.getProduct(id);

    if (!product) {
        std::cout << "  Product not found.\n";
        return;
    }
    std::cout << "\n||------- Product Details -------||\n";
    std::cout << "\n    ID: " << product->getId()
              << " | Name: " << product->getName()
              << " | Category: " << product->getCategory()
              << " | Base: R" << product->getPrice()
              << " | Stock: " << product->getQuantity()
              << "\n";

    char confirm;
//Making the user confirm if they actually want to remove the product
    while (true) {
        std::cout << "\n  ARE YOU SURE? (Y/N): ";
        std::cin >> confirm;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (confirm == 'Y' || confirm == 'y') {
            if (inventory.removeProduct(id))
                std::cout << "  Product " << id << " removed.\n";
            else
                std::cout << "  Error removing product.\n";
            break;
        }
        else if (confirm == 'N' || confirm == 'n') {
            std::cout << "  Deletion cancelled.\n";
            break;
        }
        else {
            std::cout << "  Invalid option. Please enter Y or N.\n";
        }
    }
}
//||||*****************************************************************||||


////||||****************Option 3: Edit a Product***********************||||
void optionThreeEditProductDetails(InventorySystem& inventory) {
    std::cout << "\n     1. Search by Name\n";
    std::cout << "     2. Search by ID\n\n";

    int choice;
    do {
        choice = readInt("Choice: ");
        if (choice != 1 && choice != 2)
            std::cout << "  Invalid option. Please enter 1 or 2.\n";
    } while (choice != 1 && choice != 2);

    std::shared_ptr<Product> product = nullptr;
    int id = 0;

    if (choice == 1) {
        std::cout << "\n  Enter product name: ";
        std::string keyword;

        do {
            std::getline(std::cin, keyword);
            if (keyword.empty())
                std::cout << "  Invalid input. Enter a product name: ";
        } while (keyword.empty());

        auto results = inventory.searchByName(keyword);

        if (results.empty()) {
            std::cout << "  No products found.\n";
            return;
        }

        std::cout << "\n||-------- Search Results -------||\n\n";
        for (const auto& p : results) {
            std::cout << "  ID: " << p->getId()
                      << " | Name: " << p->getName()
                      << " | Category: " << p->getCategory()
                      << " | Stock: " << p->getQuantity()
                      << "\n";
        }

        do {
            id = readInt("\n  Enter ID to edit: ");
            product = inventory.getProduct(id);
            if (!product)
                std::cout << "  Invalid ID. Try again.\n";
        } while (!product);

    } else {
        do {
            id = readInt("\n  Enter Product ID: ");
            product = inventory.getProduct(id);
            if (!product)
                std::cout << "  Product not found. Try again.\n";
        } while (!product);
    }

    std::cout << "\n||--- Current Product ---||\n\n";
    std::cout << "ID: " << product->getId()
              << " | Name: " << product->getName()
              << " | Category: " << product->getCategory()
              << " | Stock: " << product->getQuantity()
              << "\n";

    while (true) {

        std::cout << "\n||--------- Edit Product ----------||\n\n";
        std::cout << "     1. Name\n";
        std::cout << "     2. Category\n";
        std::cout << "     3. Price\n";
        std::cout << "     4. Quantity\n";
        std::cout << "     5. Exit\n";

        int option = readInt("\n  Select attribute to edit: ");

        // ---------------- NAME ----------------
        if (option == 1) {
            std::string name;
            do {
                std::cout << "  Enter new Name: ";
                std::getline(std::cin, name);
            } while (name.empty());

            product->setName(name);
        }

        // ---------------- CATEGORY ----------------
        else if (option == 2) {

            if (product->getCategory() == "Perishables") {
                std::cout << "  Perishable products cannot change category.\n";
                std::cout << "  Please remove the product and add it via the Menu.\n";
                continue;
            }

            std::cout << "\n||------- Select Category -------||\n\n";
            std::cout << "     1. Electronics\n";
            std::cout << "     2. Perishables\n";
            std::cout << "     3. Clothing\n";
            std::cout << "     4. Home\n";
            std::cout << "     5. Sports\n";
            std::cout << "     6. Toys\n";
            std::cout << "     7. Automotive\n";
            std::cout << "     8. Books\n";
            std::cout << "     9. Beauty\n";
            std::cout << "     10. Garden\n";
            std::cout << "     11. Office\n\n";

            int catChoice;
            do {
                catChoice = readInt("  Category: ");
                if (catChoice < 1 || catChoice > 11)
                    std::cout << "  Invalid category. Choose 1-11.\n";
            } while (catChoice < 1 || catChoice > 11);

            std::string category;

            switch (catChoice) {
                case 1: category = "Electronics"; break;
                case 2: category = "Perishables"; break;
                case 3: category = "Clothing"; break;
                case 4: category = "Home"; break;
                case 5: category = "Sports"; break;
                case 6: category = "Toys"; break;
                case 7: category = "Automotive"; break;
                case 8: category = "Books"; break;
                case 9: category = "Beauty"; break;
                case 10: category = "Garden"; break;
                case 11: category = "Office"; break;
            }

            if (category == "Perishables" && product->getCategory() != "Perishables") {
                std::cout << "  Cannot assign Perishables category.\n";
                std::cout << "  Please remove the product and add it via the Menu.\n";
                continue;
            }

            product->setCategory(category);
        }

        // ---------------- PRICE ----------------
        else if (option == 3) {
            double price;
            do {
                price = readDouble("  Enter new Price: R");
                if (price <= 0)
                    std::cout << "  Invalid price. Must be > 0.\n";
            } while (price <= 0);

            product->setPrice(price);
        }

        // ---------------- QUANTITY ----------------
        else if (option == 4) {
            int qty;
            do {
                qty = readInt("  Enter new Quantity: ");
                if (qty < 0)
                    std::cout << "  Invalid quantity. Cannot be negative.\n";
            } while (qty < 0);

            product->setQuantity(qty);
        }

        // ---------------- EXIT ----------------
        else {
            break;
        }

        std::cout << "\n||-------- Updated Product -------||\n\n";
        std::cout << "ID: " << product->getId()
                  << " | Name: " << product->getName()
                  << " | Category: " << product->getCategory()
                  << " | Price: R" << product->getPrice()
                  << " | Stock: " << product->getQuantity()
                  << "\n";
    }
}
//||||*****************************************************************||||


////||||***************Option 4: Display Products***********************||||
void optionFourDisplay(InventorySystem& inventory) {
    inventory.displayInventory();
}
//||||*****************************************************************||||

////||||******************Option 5: Search*****************************||||
void optionFiveSearch(InventorySystem& inventory) {

    std::cout << "\n     1. Search by Name\n";
    std::cout << "     2. Search by product ID\n";
    int choice = readInt("\nChoice: ");

    std::vector<std::shared_ptr<Product>> results;

    if (choice == 1) {
        std::cout << "\n  Enter product name (or part of it): ";
        std::string keyword;
        std::getline(std::cin, keyword);

        results = inventory.searchByName(keyword);

    } else if (choice == 2) {
        int id = readInt("\n  Enter product product ID: ");

        auto product = inventory.getProduct(id);
        if (product) {
            results.push_back(product);
        }

    } else {
        std::cout << "  Invalid choice.\n";
        return;
    }

    if (results.empty()) {
        std::cout << "\n  No products found.\n";
        return;
    }
    std::cout << "\n||---- Search Results ( " << results.size() <<" ) ----||\n";

    for (const auto& p : results) {
        std::cout << "    ID: " << p->getId()
                  << " | " << p->getName()
                  << " | Category: " << p->getCategory()
                  << " | Final: R" << p->calculateFinalPrice()
                  << " | Stock: " << p->getQuantity()
                  << "\n";
    }
}
//||||*****************************************************************||||


////||||**********************Option 6: Sort***************************||||
void optionSixSort(InventorySystem& inventory) {
    std::cout << "\n     1. Sort by Price\n";
    std::cout << "     2. Sort by Quantity\n";
    std::cout << "     3. Sort by Type\n";
    std::cout << "     4. Sort Alphabetically\n";
    std::cout << "     5. Sort Categorically\n";

    int choice = readInt("\nChoice: ");

    std::vector<std::shared_ptr<Product>> sorted;
    std::string label;

    switch (choice) {

        case 1:
            sorted = inventory.getSortedByPrice();
            label = "Final Price (asc)";
            break;

        case 2:
            sorted = inventory.getSortedByQuantity();
            label = "Quantity (asc)";
            break;

        case 3:
            sorted = inventory.getSortedByType();
            label = "Type (A-Z)";
            break;

        case 4:
            sorted = inventory.getSortedByName();
            label = "Name (A-Z)";
            break;

        case 5:
            sorted = inventory.getSortedByCategory();
            label = "Category (A-Z)";
            break;

        default:
            std::cout << "Invalid choice.\n";
            return;
    }

    std::cout << "\n--- Sorted by " << label << " ---\n";

    for (const auto& p : sorted) {
        std::cout << "    ID: " << p->getId()
                  << " | " << p->getName()
                  << " | " << p->getCategory()
                  << " | Final: R" << p->calculateFinalPrice()
                  << " | Stock: " << p->getQuantity()
                  << " | Stock: " << p->getType()
                  << "\n";
    }
}
//||||*****************************************************************||||

////||||*****************Option 7: Process orders *********************||||
//Workers pull orders from the pool and fulfill them against inventory
void optionSevenProcessOrders(
    InventorySystem& inventory,
    OrderPool& pool,
    AnalyticsManager& analyticsManager,
    std::vector<std::unique_ptr<WorkerThread>>& workers
) {

    if (pool.empty()) {
        std::cout << "\n  Order pool is empty - nothing to process.\n";
        std::cout << "  (Orders are loaded once at startup from warehouse CSV files.)\n";
        return;
    }
    std::cout << "\n-------- Order Processing ---------\n\n";
    std::cout << "  Orders in pool: " << pool.size() << "\n";
    int threadCount = readInt("  Select number of worker threads (e.g. 4 / 8 / 16): ");

    if (threadCount < 1)  threadCount = 1;
    if (threadCount > 32) threadCount = 32;

    //Reset the complete flag so workers dont exit immediately (it was set to true after Phase 1 loading finished)
    pool.prepareForProcessing();

    std::cout << "\n  Launching " << threadCount << " worker threads...\n\n";

    //Start timing from when workers launch
    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= threadCount; ++i) {
        auto w = std::make_unique<WorkerThread>(i);
        w->start(pool, inventory, analyticsManager);
        workers.push_back(std::move(w));
    }

    //Signal workers that no new orders will be added
    //They will drain whats left and exit
    pool.markLoadingComplete();

    //Wait for all workers to finish
    for (auto& w : workers)
        w->join();

    //Stop timing after all workers are done
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    //Summary report
    int total = 0, successes = 0, partials = 0, failures = 0;
    for (const auto& w : workers) {
        for (const auto& order : w->getProcessed()) {
            ++total;
            switch (order.getStatus()) {
                case Order::Status::SUCCESS: ++successes; break;
                case Order::Status::PARTIAL: ++partials;  break;
                case Order::Status::FAILED:  ++failures;  break;
                default: break;
            }
        }
    }

    std::cout << "\n-----------------------------------------------------\n";
    std::cout << "                   ORDER SUMMARY\n";
    std::cout <<   "-----------------------------------------------------\n\n";
    std::cout << "            Threads used:      " << threadCount       << "\n";
    std::cout << "            Total processed:   " << total             << "\n";
    std::cout << "            Fully succeeded:   " << successes         << "\n";
    std::cout << "            Partially filled:  " << partials          << "\n";
    std::cout << "            Failed:            " << failures          << "\n";
    std::cout << "            Time taken:        " << duration.count()  << " ms\n\n";
    std::cout <<   "------------------------------------------------------\n";

}
//||||*****************************************************************||||

////||||****************option 8: Analytics Manager********************||||

void optionEightAnalytics(
    AnalyticsManager& analyticsManager,
    const std::vector<std::unique_ptr<WorkerThread>>& workers)
{
    bool inAnalyticsMenu = true;

    while (inAnalyticsMenu) {

        std::cout << "\n||------- Analytics Menu -------||\n\n";
        std::cout << "     1. Global Analytics\n";
        std::cout << "     2. Warehouse Analytics\n";
        std::cout << "     3. Export Analytics\n";
        std::cout << "     4. Return to Main Menu\n";

        int choice = readInt("\nChoice: ");

        switch (choice) {

        case 1:
            GlobalReport(analyticsManager, workers);
            pauseForUser();
            break;

        case 2: {

            while (true) {

                std::cout << "\n||----- Warehouse Analytics -----||\n\n";
                std::cout << "     1. Warehouse 1\n";
                std::cout << "     2. Warehouse 2\n";
                std::cout << "     3. Warehouse 3\n";
                std::cout << "     4. Warehouse 4\n";
                std::cout << "     5. Warehouse 5\n";
                std::cout << "     6. Back\n\n";

                int wChoice = readInt("Choice: ");

                if (wChoice >= 1 && wChoice <= 5) {
                    WarehouseReport(wChoice, analyticsManager, workers);
                    pauseForUser();
                }
                else if (wChoice == 6) {
                    break;
                }
                else {
                    std::cout << "Invalid option. Try again.\n";
                }
            }

            break;
        }

        case 3: {

            bool inExportMenu = true;

            while (inExportMenu) {

                std::cout << "\n||------- Export Analytics -------||\n\n";
                std::cout << "     1. Export Global Analytics\n";
                std::cout << "     2. Export Warehouse Analytics\n";
                std::cout << "     3. Back\n";

                int exportChoice = readInt("\nChoice: ");

                if (exportChoice == 1) {

                    analyticsManager.exportGlobalAnalyticsCSV();;
                    std::cout << "Global analytics exported.\n";
                    pauseForUser();
                }

                else if (exportChoice == 2) {

                    while (true) {

                        std::cout << "\n||------- Select Warehouse -------||\n\n";
                        std::cout << "     1. Warehouse 1\n";
                        std::cout << "     2. Warehouse 2\n";
                        std::cout << "     3. Warehouse 3\n";
                        std::cout << "     4. Warehouse 4\n";
                        std::cout << "     5. Warehouse 5\n";
                        std::cout << "     6. Back to Export Menu\n";
                        std::cout << "     7. Back to Analytics Menu\n";
                        std::cout << "     8. Return to Main Menu\n";

                        int wChoice = readInt("\nChoice: ");

                        if (wChoice >= 1 && wChoice <= 5) {

                            analyticsManager.exportWarehouseAnalyticsCSV(wChoice);
                            std::cout << "Warehouse " << wChoice << " exported.\n";
                            pauseForUser();
                        }

                        else if (wChoice == 6) {
                            break;
                        }

                        else if (wChoice == 7) {
                            inExportMenu = false;
                            break;
                        }

                        else if (wChoice == 8) {
                            return;
                        }

                        else {
                            std::cout << "Invalid option. Try again.\n";
                        }
                    }
                }

                else if (exportChoice == 3) {
                    inExportMenu = false;
                }

                else {
                    std::cout << "Invalid option. Try again.\n";
                }
            }

            break;
        }

        case 4:
            return;

        default:
            std::cout << "Invalid option. Try again.\n";
            break;
        }
    }
}


////||||******************Global Report*****************************||||

////Warehouse ID is passed into this so it can be reused for all warehouses
void WarehouseReport(
    int warehouseID,
    const AnalyticsManager& analyticsManager,
    const std::vector<std::unique_ptr<WorkerThread>>& workers)
{
    // --------------------------------------------------------//
    // GETTING ITEM-LEVEL ANALYTICS (FROM ANALYTICS MANAGER)//
    // --------------------------------------------------------//
    double totalRevenue = analyticsManager.getWarehouseRevenue(warehouseID);
    double lostRevenue = analyticsManager.getWarehouseLostRevenue(warehouseID);
    int totalItems = analyticsManager.getWarehouseTotalItems(warehouseID);

    auto categoryRevenue = analyticsManager.getWarehouseCategoryPerformance(warehouseID);
    int bestItem = analyticsManager.getWarehouseBestItem(warehouseID);

    // NEW: Product Type Analytics
    auto typeRevenue = analyticsManager.getWarehouseRevenueByType(warehouseID);
    auto typeQuantity = analyticsManager.getWarehouseQuantityByType(warehouseID);
    auto typeFailure = analyticsManager.getWarehouseFailureRateByType(warehouseID);

    // Find best category
    std::string bestCategory;
    double bestCategoryRevenue = 0.0;

    for (const auto& c : categoryRevenue) {
        if (c.second > bestCategoryRevenue) {
            bestCategoryRevenue = c.second;
            bestCategory = c.first;
        }
    }

    // ------------------------------------------------------//
    // GETTING ORDER-LEVEL ANALYTICS VIA THE WORKER THREADS//
    // ------------------------------------------------------//
    int successOrders = 0;
    int partialOrders = 0;
    int failedOrders = 0;

    for (const auto& w : workers) {
        for (const auto& order : w->getProcessed()) {

            if (order.getWarehouseID() != warehouseID)
                continue;

            switch (order.getStatus()) {
                case Order::Status::SUCCESS: successOrders++; break;
                case Order::Status::PARTIAL: partialOrders++; break;
                case Order::Status::FAILED:  failedOrders++; break;
                default: break;
            }
        }
    }

    int totalOrders = successOrders + partialOrders + failedOrders;

    double successRate = (totalOrders == 0)
        ? 0.0
        : (double(successOrders) / totalOrders) * 100.0;

    // -------------------------
    //    Printing the Report
    // -------------------------
    std::cout << "\n|||=================================================|||\n";
    std::cout << "                 WAREHOUSE REPORT: " << warehouseID << "\n";
    std::cout <<   "|||=================================================|||\n\n";
    std::cout << "      Overall Warehouse Sales: R" << totalRevenue << "\n";
    std::cout << "      Most Popular Category: " << bestCategory << "\n";
    std::cout << "\n      Sales per Category:\n";
    for (const auto& c : categoryRevenue) {
        std::cout << "           " << c.first << " -> R" << c.second << "\n";
    }
    std::cout << "\n      Sales per Product Type:\n";
    for (const auto& t : typeRevenue) {
        std::cout << "           " << t.first << " -> R" << t.second << "\n";
    }
    std::cout << "\n      Quantity per Product Type:\n";
    for (const auto& t : typeQuantity) {
        std::cout << "           " << t.first << " -> " << t.second << "\n";
    }
    std::cout << "\n      Failure Rate per Product Type:\n";
    for (const auto& t : typeFailure) {
        std::cout << "           " << t.first << " -> " << (t.second * 100.0) << "%\n";
    }
    std::cout << "\n      Most Popular Item ID: " << bestItem << "\n";
    std::cout << "\n      Order Stats:\n";
    std::cout << "           Successful Orders: " << successOrders << "\n";
    std::cout << "           Partial Orders:    " << partialOrders << "\n";
    std::cout << "           Failed Orders:     " << failedOrders << "\n";
    std::cout << "      Success Rate: " << successRate << "%\n";
    std::cout << "      Total Items Sold: " << totalItems << "\n";
    std::cout << "      Lost Revenue: R" << lostRevenue << "\n\n";
    std::cout << "|||=========== Warehouse "<< warehouseID << " Report Printed ==========|||\n";
}
//||||*****************************************************************||||
////||||*********************Global Report*****************************||||
void GlobalReport(
    const AnalyticsManager& analyticsManager,
    const std::vector<std::unique_ptr<WorkerThread>>& workers)
{
    // --------------------------------------------------------//
    // ITEM-LEVEL ANALYTICS
    // --------------------------------------------------------//
    double totalRevenue = analyticsManager.getGlobalRevenue();
    double lostRevenue = analyticsManager.getGlobalLostRevenue();

    auto categoryRevenue = analyticsManager.getGlobalCategoryPerformance();
    auto itemsPerCategory = analyticsManager.getGlobalItemsSoldPerCategory();

    int bestItem = analyticsManager.getGlobalMostPopularItem();
    int totalItems = analyticsManager.getGlobalTotalItemsSold();

    // NEW: Product Type Analytics
    auto typeRevenue = analyticsManager.getGlobalRevenueByType();
    auto typeQuantity = analyticsManager.getGlobalQuantityByType();
    auto typeFailure = analyticsManager.getGlobalFailureRateByType();

    // Find best category
    std::string bestCategory;
    double bestCatRevenue = 0.0;

    for (const auto& c : categoryRevenue) {
        if (c.second > bestCatRevenue) {
            bestCatRevenue = c.second;
            bestCategory = c.first;
        }
    }

    // ------------------------------------------------------//
    // ORDER-LEVEL ANALYTICS
    // ------------------------------------------------------//
    int successOrders = 0;
    int partialOrders = 0;
    int failedOrders = 0;

    std::map<int, int> warehouseOrders;

    for (const auto& w : workers) {
        for (const auto& order : w->getProcessed()) {

            warehouseOrders[order.getWarehouseID()]++;

            switch (order.getStatus()) {
                case Order::Status::SUCCESS: successOrders++; break;
                case Order::Status::PARTIAL: partialOrders++; break;
                case Order::Status::FAILED:  failedOrders++; break;
                default: break;
            }
        }
    }

    int bestWarehouse = 0;
    int bestWarehouseOrders = 0;

    for (const auto& w : warehouseOrders) {
        if (w.second > bestWarehouseOrders) {
            bestWarehouseOrders = w.second;
            bestWarehouse = w.first;
        }
    }

    int totalOrders = successOrders + partialOrders + failedOrders;

    double successRate = (totalOrders == 0)
        ? 0.0
        : (double(successOrders) / totalOrders) * 100.0;

    // -------------------------
    // PRINT REPORT
    // -------------------------
    std::cout << "\n|||=================================================|||\n";
    std::cout << "                     GLOBAL REPORT\n";
    std::cout <<   "|||=================================================|||\n\n";
    std::cout << "      Overall Warehouse Sales: R" << totalRevenue << "\n";
    std::cout << "      Most Popular Warehouse: " << bestWarehouse << "\n";
    std::cout << "      Most Popular Category: " << bestCategory << "\n";
    std::cout << "\n      Sales per Category:\n";
    for (const auto& c : categoryRevenue) {
        std::cout << "           " << c.first << " -> R" << c.second << "\n";
    }
    std::cout << "\n      Sales per Product Type:\n";
    for (const auto& t : typeRevenue) {
        std::cout << "           " << t.first << " -> R" << t.second << "\n";
    }
    std::cout << "\n      Quantity per Product Type:\n";
    for (const auto& t : typeQuantity) {
        std::cout << "           " << t.first << " -> " << t.second << "\n";
    }
    std::cout << "\n      Failure Rate per Product Type:\n";
    for (const auto& t : typeFailure) {
        std::cout << "           " << t.first << " -> " << (t.second * 100.0) << "%\n";
    }
    std::cout << "\n      Items Sold per Category:\n";
    for (const auto& c : itemsPerCategory) {
        std::cout << "           " << c.first << " -> " << c.second << "\n";
    }
    std::cout << "\n      Most Popular Item ID: " << bestItem << "\n";
    std::cout << "\n      Order Stats:\n";
    std::cout << "           Successful Orders: " << successOrders << "\n";
    std::cout << "           Partial Orders:    " << partialOrders << "\n";
    std::cout << "           Failed Orders:     " << failedOrders << "\n";
    std::cout << "      Success Rate: " << successRate << "%\n";
    std::cout << "      Total Orders Processed: " << totalOrders << "\n";
    std::cout << "      Total Items Sold: " << totalItems << "\n";
    std::cout << "      Lost Revenue: R" << lostRevenue << "\n\n";
    std::cout << "|||============ Global Report Printed ==============|||\n";
}
//||||*****************************************************************||||

////||||****************Option 9: Save & Exit**************************||||
void optionNineShutdown(InventorySystem& inventory) {
    std::cout << "\nSaving inventory to Catalog/Updated_Products.csv...\n";
    inventory.exportToCSV("Catalog/Updated_Products.csv"); //Does not overwrite the starter file for testing purposes
    std::cout << "Goodbye!\n";
}
//||||*****************************************************************||||


////||||***************Option 10: Add Manual Order*********************||||
//Lets the user create an order from the menu and push it into the pool
//so it gets processed in the next batch (option 7). This simulates
//a real-time order coming in while the system is live.
void optionTenAddManualOrder(OrderPool& pool, InventorySystem& inventory) {
    std::cout << "\n||------- Add Manual Order -------||" << std::endl;
    std::cout << "\n  Select source warehouse (1-5): ";

    int warehouseId = readInt("  Warehouse ID: ");
    if (warehouseId < 1 || warehouseId > 5) {
        std::cout << "  Invalid warehouse. Must be 1-5.\n";
        return;
    }

    Order order(warehouseId);
    int itemCount = 0;

    //Loop so user can add multiple items to one order
    while (true) {
        std::cout << "\n  Add item to order (or enter 0 to finish):\n";
        int productId = readInt("    Product ID: ");
        if (productId == 0) break;

        //Quick check that the product exists so user gets feedback
        auto product = inventory.getProduct(productId);
        if (!product) {
            std::cout << "    Product ID " << productId << " not found in catalogue. Skipping.\n";
            continue;
        }

        int quantity = readInt("    Quantity: ");
        if (quantity <= 0) {
            std::cout << "    Quantity must be positive. Skipping.\n";
            continue;
        }

        order.addItem(productId, quantity);
        ++itemCount;
        std::cout << "    Added: " << product->getName()
                  << " x" << quantity
                  << " (stock: " << product->getQuantity() << ")\n";
    }

    if (itemCount == 0) {
        std::cout << "\n  No items added. Order cancelled.\n";
        return;
    }

    //Push into the shared pool so next process batch picks it up
    pool.addManualOrder(order);

    std::cout << "\n  Order #" << order.getOrderID()
              << " queued (Warehouse " << warehouseId
              << ", " << itemCount << " items)."
              << "\n  Run option 7 to process it.\n";
}
//||||*****************************************************************||||



