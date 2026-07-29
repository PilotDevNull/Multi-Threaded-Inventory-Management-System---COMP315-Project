#ifndef PRODUCT_H
#define PRODUCT_H

#include <string>

//Abstract base class representing a generic product
//Forces child classes to implement pricing logic
class Product {
protected:
   int productId;    //Unique identifier for all products
   std::string name; //Product name
   double price;     //Base price
   int quantity;     //Amount in stock
   std::string category;

public:
   Product(int id, const std::string& name, const std::string& category, double price, int qty);
   virtual ~Product() = default; //Required for polymorphism and to delete objects without memory leaks (virtual destructor)

   //Getters
   int getId() const;
   std::string getName() const;
   std::string getCategory() const;
   double getPrice() const; //Returns base price
   int getQuantity() const;

   void increaseStock(int amount); //Increase stock by amount
   bool decreaseStock(int amount); //Attempts to reduce stock (will fail if not enough stock exists)

   //Each product type calculates its final price differently
   virtual double calculateFinalPrice() const = 0; //Pure virtual function (this makes the class abstract)

   virtual std::string getType() const = 0; //Pure virtual function that returns product name

   //Setters allowing us to edit product entries
   void setName(const std::string& name);
   void setCategory(const std::string& category);
   void setPrice(double price);
   void setQuantity(int qty);
};

#endif
