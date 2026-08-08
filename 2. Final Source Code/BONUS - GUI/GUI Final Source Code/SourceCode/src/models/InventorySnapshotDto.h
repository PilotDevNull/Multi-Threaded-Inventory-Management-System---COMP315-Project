#ifndef INVENTORYSNAPSHOTDTO_H
#define INVENTORYSNAPSHOTDTO_H

#include <QString>
#include <QVector>

#include "ProductDto.h"

// Immutable inventory snapshot shared with the GUI pages.
struct InventorySnapshotDto {
    QVector<ProductDto> products;
    int totalProducts = 0;
    int totalStockUnits = 0;
    int lowStockItems = 0;
    QString sourcePath;
    QString loadError;

    bool isValid() const {
        return loadError.isEmpty() && !sourcePath.isEmpty();
    }
};

#endif // INVENTORYSNAPSHOTDTO_H
