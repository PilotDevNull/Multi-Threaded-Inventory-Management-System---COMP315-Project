#ifndef INVENTORYSERVICEADAPTER_H
#define INVENTORYSERVICEADAPTER_H

#include "InventorySnapshotDto.h"

// Reads inventory data through the protected backend and converts it into
// immutable DTOs so the Qt pages never consume backend Product pointers.
class InventoryServiceAdapter
{
public:
    InventorySnapshotDto loadInventorySnapshot() const;

private:
    QString resolveProductsCsvPath() const;

    static constexpr int kLowStockThreshold = 100;
};

#endif // INVENTORYSERVICEADAPTER_H
