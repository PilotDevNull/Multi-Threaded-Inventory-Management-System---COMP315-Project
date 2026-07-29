#include "Warehouse.h"
#include "Logger.h"
#include <fstream>
#include <sstream>

Warehouse::Warehouse(int id, const std::string& csvFile)
    : warehouseId(id), csvFile(csvFile) {}

//Lambda capture list, takes current object and OrderPool by reference so the thread is pushed into shared queue without copies
void Warehouse::load(OrderPool& pool) {
    worker = std::thread([this, &pool]() {

        std::ifstream file(csvFile);
        if (!file) {
            LOG("[Warehouse " << warehouseId << "] ERROR: Could not open " << csvFile);
            return; //Thread exits early
        }

        LOG("[Warehouse " << warehouseId << "] Loading orders from " << csvFile << "...");

        int loaded = 0; //Track number of orders loaded
        std::string line;

        //Each line is one order with one or more items
        //Format: productId,qty,productId,qty,...
        while (std::getline(file, line)) {
            if (line.empty()) continue; //Don't process blank lines

            //Windows newline fix
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            std::stringstream ss(line);
            std::string token;
            Order order(warehouseId);
            int itemCount = 0;

            //Read pairs of productId,quantity until the line runs out
            try {
                while (std::getline(ss, token, ',')) {
                    int productId = std::stoi(token);
                    if (!std::getline(ss, token, ',')) break; //No quantity for this id
                    int quantity = std::stoi(token);

                    if (quantity > 0) {
                        order.addItem(productId, quantity);
                        ++itemCount;
                    }
                }
            } catch (...) {
                continue; //Skip malformed lines
            }

            //Only push if the order actually has items
            if (itemCount > 0) {
                pool.push(order);
                ++loaded;
            }
        }

        file.close();
        LOG("[Warehouse " << warehouseId << "] Finished - loaded " << loaded << " orders into pool.");
    });
}

//Wait for the warehouse thread to finish
void Warehouse::join() {
    if (worker.joinable())
        worker.join();
}

int Warehouse::getId() const { return warehouseId; }
