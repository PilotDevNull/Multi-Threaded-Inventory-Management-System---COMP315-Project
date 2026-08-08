#ifndef DISCOUNTEDPRODUCT_H
#define DISCOUNTEDPRODUCT_H

#include "Product.h"

//Represents products that have a discount applied
class DiscountedProduct : public Product { //Inherits from Product, applies a discount
private:
   double discountRate; //Example: 0.10 for 10%

public:
   DiscountedProduct(int id, const std::string& name, const std::string& category, double price, int qty, double discountRate);

   double getDiscountRate() const;

   //Override pricing logic to apply discount
   double calculateFinalPrice() const override;

   //Overide used to return discounted for product type
   std::string getType() const override;
};

#endif
