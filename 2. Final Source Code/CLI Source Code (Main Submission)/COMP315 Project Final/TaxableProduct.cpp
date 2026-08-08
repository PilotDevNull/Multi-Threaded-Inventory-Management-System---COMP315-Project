#include "TaxableProduct.h"

TaxableProduct::TaxableProduct(int id, const std::string& name, const std::string& category, double price, int qty, double taxRate)
    : Product(id, name, category, price, qty), taxRate(taxRate) {}

double TaxableProduct::getTaxRate() const {
   return taxRate;
}

//Base price + tax
double TaxableProduct::calculateFinalPrice() const {
   return price * (1 + taxRate);
}

std::string TaxableProduct::getType() const {
   return "Taxable";
}
