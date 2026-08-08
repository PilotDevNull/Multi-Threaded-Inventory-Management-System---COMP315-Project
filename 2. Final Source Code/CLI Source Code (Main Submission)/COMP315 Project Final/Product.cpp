#include "Product.h"

Product::Product(int id, const std::string& name, const std::string& category, double price, int qty)
    : productId(id), name(name), category(category), price(price), quantity(qty) {}

int Product::getId() const {
    return productId;
}

std::string Product::getName() const {
    return name;
}
std::string Product::getCategory() const {
    return category;
}
//Setter implementations
void Product::setName(const std::string& n) { name = n; }
void Product::setCategory(const std::string& c) { category = c; }
void Product::setPrice(double p) { price = p; }
void Product::setQuantity(int q) { quantity = q; }

//Returns base price
double Product::getPrice() const {
    return price;
}

int Product::getQuantity() const {
    return quantity;
}

void Product::increaseStock(int amount) {
   quantity += amount;
}

//Returns false if not enough stock, prevents negative stock counts
bool Product::decreaseStock(int amount) {
   if (quantity >= amount) {
      quantity -= amount;
      return true;
   }
   return false;
}
