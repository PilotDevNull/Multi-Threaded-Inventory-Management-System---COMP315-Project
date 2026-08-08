#include "DiscountedProduct.h"

DiscountedProduct::DiscountedProduct(int id, const std::string& name, const std::string& category, double price, int qty, double discountRate)
    : Product(id, name, category, price, qty), discountRate(discountRate) {}

double DiscountedProduct::getDiscountRate() const {
   return discountRate;
}

//Price after discount
double DiscountedProduct::calculateFinalPrice() const {
   return price * (1 - discountRate) * (1 + 0.15); //Flat Tax Rate for Discounted Products

}

std::string DiscountedProduct::getType() const {
   return "Discounted";
}
