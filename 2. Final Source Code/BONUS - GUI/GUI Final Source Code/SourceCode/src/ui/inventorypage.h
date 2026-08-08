#ifndef INVENTORYPAGE_H
#define INVENTORYPAGE_H

#include <QVector>
#include <QWidget>
#include <QString>

#include "ProductDto.h"

class QEvent;
class QObject;

namespace Ui {
class InventoryPage;
}

class InventoryPage : public QWidget
{
    Q_OBJECT

public:
    explicit InventoryPage(QWidget *parent = nullptr);
    ~InventoryPage();

    void setInventoryData(const QVector<ProductDto>& products, const QString& sourcePath);
    void exportCatalogueCsv();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    Ui::InventoryPage *ui;
    bool m_backendReadOnlyMode = false;
    QVector<ProductDto> m_products;   // live editable product list
    int m_nextId = 100;               // auto-increment for new products

    void applyReadOnlyMode();
    void setupUiDefaults();

    // Feature implementations
    void filterAndSortTable();
    void previewCsvFile(const QString &path);
    void importCsvFile(const QString &path);
    void addNewProduct();
    void removeSelectedProduct();
    void updateSelectedProduct();
    bool parseCsvRow(const QString &line, QStringList &fields);
    void rebuildTableFromProducts();
    void syncProductFromRow(int row);
};

#endif // INVENTORYPAGE_H
