#ifndef STANDARDPRODUCT_H
#define STANDARDPRODUCT_H

#include "Product.h"

//Represents standard products with no tax or discount
class StandardProduct : public Product {
public:
   StandardProduct(int id, const std::string& name, const std::string& category, double price, int qty);

   //Returns base price (no modifications)
   double calculateFinalPrice() const override;

   //Returns type of product
   std::string getType() const override;
};

#endif
