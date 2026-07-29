#ifndef TAXABLEPRODUCT_H
#define TAXABLEPRODUCT_H

#include "Product.h"

//Represents products that include tax
class TaxableProduct : public Product { //Inherits from Product, adds tax to price
private:
   double taxRate; //Example: 0.15 for 15%

public:
   TaxableProduct(int id, const std::string& name, const std::string& category, double price, int qty, double taxRate);

   double getTaxRate() const;

   //Override pricing logic to include tax
   double calculateFinalPrice() const override;

   //Overide used to return taxable for product type
   std::string getType() const override;
};

#endif
