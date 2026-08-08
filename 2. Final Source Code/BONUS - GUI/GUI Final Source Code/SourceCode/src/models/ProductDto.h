#ifndef PRODUCTDTO_H
#define PRODUCTDTO_H

#include <QString>

// Immutable product data copied out of the backend for safe GUI use.
struct ProductDto {
    int id = 0;
    QString name;
    QString category;
    double basePrice = 0.0;
    double finalPrice = 0.0;
    int quantity = 0;
    QString type;
};

#endif // PRODUCTDTO_H
