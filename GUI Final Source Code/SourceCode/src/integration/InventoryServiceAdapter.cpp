#include "InventoryServiceAdapter.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include "InventorySystem.h"

namespace {

QString canonicalFilePath(const QString &candidatePath)
{
    const QFileInfo info(candidatePath);
    if (!info.exists() || !info.isFile()) {
        return {};
    }

    const QString canonicalPath = info.canonicalFilePath();
    return canonicalPath.isEmpty() ? info.absoluteFilePath() : canonicalPath;
}

} // namespace

QString InventoryServiceAdapter::resolveProductsCsvPath() const
{
    const QString relativeProductsPath = QStringLiteral("final_project_v2/Catalog/Products.csv");

    QDir searchDir(QCoreApplication::applicationDirPath());
    for (int depth = 0; depth < 6; ++depth) {
        const QString candidate = canonicalFilePath(searchDir.filePath(relativeProductsPath));
        if (!candidate.isEmpty()) {
            return candidate;
        }

        if (!searchDir.cdUp()) {
            break;
        }
    }

#ifdef NEXUS_SOURCE_DIR
    const QString sourceCandidate =
        canonicalFilePath(QDir(QStringLiteral(NEXUS_SOURCE_DIR)).filePath(relativeProductsPath));
    if (!sourceCandidate.isEmpty()) {
        return sourceCandidate;
    }
#endif

    return {};
}

InventorySnapshotDto InventoryServiceAdapter::loadInventorySnapshot() const
{
    InventorySnapshotDto snapshot;
    snapshot.sourcePath = resolveProductsCsvPath();

    if (snapshot.sourcePath.isEmpty()) {
        snapshot.loadError =
            QStringLiteral("Could not resolve final_project_v2/Catalog/Products.csv from the application path.");
        return snapshot;
    }

    // Build a short-lived backend instance so the GUI only receives immutable data.
    InventorySystem inventory;
    inventory.loadFromCSV(snapshot.sourcePath.toStdString());

    const int lastProductId = inventory.getLastProductId();
    snapshot.products.reserve(lastProductId);

    for (int productId = 1; productId <= lastProductId; ++productId) {
        const std::shared_ptr<Product> product = inventory.getProduct(productId);
        if (!product) {
            continue;
        }

        ProductDto dto;
        dto.id = product->getId();
        dto.name = QString::fromStdString(product->getName());
        dto.category = QString::fromStdString(product->getCategory());
        dto.basePrice = product->getPrice();
        dto.finalPrice = product->calculateFinalPrice();
        dto.quantity = product->getQuantity();
        dto.type = QString::fromStdString(product->getType());

        snapshot.totalStockUnits += dto.quantity;
        if (dto.quantity <= kLowStockThreshold) {
            ++snapshot.lowStockItems;
        }

        snapshot.products.push_back(dto);
    }

    snapshot.totalProducts = snapshot.products.size();
    return snapshot;
}
