#include "StandardProduct.h"

StandardProduct::StandardProduct(int id, const std::string& name, const std::string& category, double price, int qty)
    : Product(id, name, category, price, qty) {}

double StandardProduct::calculateFinalPrice() const {
    return price;
}

std::string StandardProduct::getType() const {
    return "Standard";
}
