#include "settingspage.h"
#include "ui_settingspage.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>

#include "uihelpers.h"

namespace {

void appendTransferredItem(QBoxLayout *targetLayout, QLayoutItem *item)
{
    if (!targetLayout || !item) {
        delete item;
        return;
    }

    if (QWidget *widget = item->widget()) {
        targetLayout->addWidget(widget);
        delete item;
        return;
    }

    if (QLayout *layout = item->layout()) {
        targetLayout->addLayout(layout);
        delete item;
        return;
    }

    if (QSpacerItem *spacer = item->spacerItem()) {
        targetLayout->addSpacerItem(spacer);
        return;
    }

    delete item;
}

void appendSettingsBodyContainer(QVBoxLayout *scrollLayout, QWidget *scrollContent, QLayoutItem *item)
{
    if (!scrollLayout || !scrollContent || !item) {
        delete item;
        return;
    }

    QLayout *sourceLayout = item->layout();
    if (!sourceLayout) {
        appendTransferredItem(scrollLayout, item);
        return;
    }

    auto *bodyContainer = new QWidget(scrollContent);
    bodyContainer->setObjectName(QStringLiteral("settingsBodyContainer"));
    bodyContainer->setAttribute(Qt::WA_StyledBackground, false);

    auto *bodyLayout = new QHBoxLayout(bodyContainer);
    bodyLayout->setContentsMargins(sourceLayout->contentsMargins());
    bodyLayout->setSpacing(sourceLayout->spacing());

    while (sourceLayout->count() > 0) {
        appendTransferredItem(bodyLayout, sourceLayout->takeAt(0));
    }

    scrollLayout->addWidget(bodyContainer);
    delete item;
}

void makePageScrollable(QWidget *page, const QMargins &margins, int spacing)
{
    auto *outerLayout = qobject_cast<QVBoxLayout *>(page ? page->layout() : nullptr);
    if (!outerLayout || outerLayout->count() == 0) {
        return;
    }

    auto *scrollArea = new QScrollArea(page);
    scrollArea->setObjectName(QStringLiteral("settingsPageScrollArea"));
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->viewport()->setObjectName(QStringLiteral("settingsPageScrollViewport"));
    scrollArea->viewport()->setAttribute(Qt::WA_StyledBackground, false);

    auto *scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName(QStringLiteral("settingsPageScrollContent"));
    scrollContent->setAttribute(Qt::WA_StyledBackground, false);

    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(margins);
    scrollLayout->setSpacing(spacing);

    while (outerLayout->count() > 0) {
        QLayoutItem *item = outerLayout->takeAt(0);
        if (item && item->layout()) {
            appendSettingsBodyContainer(scrollLayout, scrollContent, item);
            continue;
        }

        appendTransferredItem(scrollLayout, item);
    }

    scrollArea->setWidget(scrollContent);

    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    outerLayout->addWidget(scrollArea);
}

} // namespace

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsPage)
{
    ui->setupUi(this);
    setupUiDefaults();
}

SettingsPage::~SettingsPage()
{
    delete ui;
}

void SettingsPage::setupUiDefaults()
{
    makePageScrollable(this, QMargins(30, 28, 30, 28), 18);
    UiHelpers::setPageLayout(this, QMargins(0, 0, 0, 0), 0);
    UiHelpers::setProperty(ui->settingsTitleLabel, "textRole", "pageTitle");

    const QList<QWidget *> compressiblePanels = {
        ui->settingsLeftColumnFrame,
        ui->settingsRightColumnFrame,
        ui->fileManagementGroupBox,
        ui->catalogueExampleGroupBox,
        ui->threadSettingsGroupBox,
        ui->csvFormatGroupBox,
        ui->ordersFormatGroupBox
    };
    for (QWidget *panel : compressiblePanels) {
        panel->setMinimumWidth(0);
        panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    UiHelpers::setSurface(ui->fileManagementGroupBox, "card");
    UiHelpers::setSurface(ui->catalogueExampleGroupBox, "card");
    UiHelpers::setSurface(ui->threadSettingsGroupBox, "card");
    UiHelpers::setSurface(ui->csvFormatGroupBox, "card");
    UiHelpers::setSurface(ui->ordersFormatGroupBox, "card");
    UiHelpers::applyCardShadow(ui->fileManagementGroupBox, 34, 8, QColor(24, 39, 62, 22));
    UiHelpers::applyCardShadow(ui->catalogueExampleGroupBox, 32, 8, QColor(24, 39, 62, 20));
    UiHelpers::applyCardShadow(ui->threadSettingsGroupBox, 28, 8, QColor(24, 39, 62, 18));
    UiHelpers::applyCardShadow(ui->csvFormatGroupBox, 34, 8, QColor(24, 39, 62, 22));
    UiHelpers::applyCardShadow(ui->ordersFormatGroupBox, 32, 8, QColor(24, 39, 62, 20));

    UiHelpers::tuneLayout(ui->fileManagementGroupBox, QMargins(16, 12, 16, 16), 14);
    UiHelpers::tuneLayout(ui->catalogueExampleGroupBox, QMargins(16, 12, 16, 16), 12);
    UiHelpers::tuneLayout(ui->threadSettingsGroupBox, QMargins(16, 12, 16, 16), 14);
    UiHelpers::tuneLayout(ui->csvFormatGroupBox, QMargins(16, 12, 16, 16), 12);
    UiHelpers::tuneLayout(ui->ordersFormatGroupBox, QMargins(16, 12, 16, 16), 12);

    UiHelpers::setProperty(ui->fileManagementTitleLabel,  "textRole", "cardTitle");
    UiHelpers::setProperty(ui->threadSettingsTitleLabel,  "textRole", "cardTitle");
    UiHelpers::setProperty(ui->catalogueExampleTitleLabel, "textRole", "cardTitle");
    UiHelpers::setProperty(ui->csvFormatTitleLabel,        "textRole", "cardTitle");
    UiHelpers::setProperty(ui->ordersFormatTitleLabel,     "textRole", "cardTitle");

    const QList<QLabel *> formLabels = {
        ui->exportPathLabel,
        ui->simulatedDelayLabel,
        ui->maxThreadsLabel
    };
    for (QLabel *label : formLabels) {
        UiHelpers::setProperty(label, "textRole", "labelTitle");
    }

    UiHelpers::setProperty(ui->fileManagementHelperLabel, "textRole", "muted");
    UiHelpers::setProperty(ui->threadSettingsHelperLabel, "textRole", "muted");
    UiHelpers::setProperty(ui->csvFormatHelperLabel, "textRole", "muted");
    UiHelpers::setProperty(ui->ordersFormatHelperLabel, "textRole", "muted");

    const QList<QLabel *> wrappingLabels = {
        ui->fileManagementHelperLabel,
        ui->threadSettingsHelperLabel,
        ui->csvFormatHelperLabel,
        ui->ordersFormatHelperLabel
    };
    for (QLabel *label : wrappingLabels) {
        label->setWordWrap(true);
        label->setMinimumWidth(0);
        label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    }

    ui->fileManagementTitleLabel->setText("Analytics Export");
    ui->fileManagementHelperLabel->setText(
        "Choose where analytics CSV exports are saved. "
        "Use Browse to pick a folder, Test Export to verify write access, "
        "or Reset Paths to restore defaults.");
    ui->exportPathLabel->setText("Export Folder");
    ui->exportPathLineEdit->setText("Analytics");
    ui->exportPathLineEdit->setEnabled(true);
    ui->autoExportCheckBox->setText("Auto-export analytics on batch completion");
    ui->autoExportCheckBox->setChecked(false);
    ui->autoExportCheckBox->setEnabled(true);
    ui->browseExportPathButton->setEnabled(true);
    ui->testExportButton->setEnabled(true);
    ui->resetPathsButton->setEnabled(true);
    // Wire buttons
    connect(ui->browseExportPathButton, &QPushButton::clicked, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Select Export Folder"), ui->exportPathLineEdit->text());
        if (!dir.isEmpty()) ui->exportPathLineEdit->setText(QDir::toNativeSeparators(dir));
    });
    connect(ui->testExportButton, &QPushButton::clicked, this, [this]() {
        const QString folder = ui->exportPathLineEdit->text();
        QDir().mkpath(folder);
        const QString testFile = folder + "/nexus_export_test.tmp";
        QFile f(testFile);
        if (f.open(QIODevice::WriteOnly)) {
            f.close(); f.remove();
            QMessageBox::information(this, tr("Test Export"), tr("Export path is writable."));
        } else {
            QMessageBox::warning(this, tr("Test Export"),
                tr("Cannot write to folder:\n%1").arg(folder));
        }
    });
    connect(ui->resetPathsButton, &QPushButton::clicked, this, [this]() {
        ui->exportPathLineEdit->setText(QStringLiteral("Analytics"));
        ui->simulatedDelaySpinBox->setValue(500);
        ui->maxThreadsSpinBox->setValue(5);
        QMessageBox::information(this, tr("Reset"), tr("Paths and runtime settings reset to defaults."));
    });

    ui->threadSettingsTitleLabel->setText("Backend Runtime");
    ui->threadSettingsHelperLabel->setText(
        "Configure the default worker thread count for Order Processing batches. "
        "Simulated delay adds artificial latency per order for testing.");
    ui->simulatedDelaySpinBox->setSuffix(" ms");
    ui->simulatedDelaySpinBox->setValue(0);
    ui->simulatedDelaySpinBox->setEnabled(true);
    ui->maxThreadsSpinBox->setValue(5);
    ui->maxThreadsSpinBox->setEnabled(true);

    ui->threadSafetyLabel->setText("Thread safety: mutex enabled");
    ui->warehousesFixedLabel->setText("Warehouses: 5 fixed");
    UiHelpers::setBadge(ui->threadSafetyLabel, "success");
    UiHelpers::setBadge(ui->warehousesFixedLabel, "info");

    UiHelpers::setButtonRole(ui->browseExportPathButton, "secondary");
    UiHelpers::setButtonRole(ui->testExportButton, "primary");
    UiHelpers::setButtonRole(ui->resetPathsButton, "secondary");

    ui->catalogueExampleTitleLabel->setText("Product CSV Example");
    UiHelpers::setupReadOnlyTable(ui->catalogueExampleTableWidget, 46, false);
    ui->catalogueExampleTableWidget->setMinimumHeight(246);
    ui->catalogueExampleTableWidget->setMinimumWidth(0);
    ui->catalogueExampleTableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->catalogueExampleTableWidget->setTextElideMode(Qt::ElideRight);
    ui->catalogueExampleTableWidget->setColumnCount(4);
    ui->catalogueExampleTableWidget->setHorizontalHeaderLabels({"id / name", "Category", "Type", "price / qty / rates"});
    ui->catalogueExampleTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->catalogueExampleTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->catalogueExampleTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->catalogueExampleTableWidget->setColumnWidth(2, 160);
    ui->catalogueExampleTableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->catalogueExampleTableWidget->setRowCount(3);
    UiHelpers::setTableText(ui->catalogueExampleTableWidget, 0, 0, "1 / Gloves Touchscreen");
    UiHelpers::setTableText(ui->catalogueExampleTableWidget, 0, 1, "Clothing");
    UiHelpers::setTableChipAligned(ui->catalogueExampleTableWidget, 0, 2, "T = Taxable", "info",
                                   Qt::AlignLeft | Qt::AlignVCenter);
    UiHelpers::setTableText(ui->catalogueExampleTableWidget, 0, 3, "599 / 120 / 0.15,0");
    UiHelpers::setTableText(ui->catalogueExampleTableWidget, 1, 0, "14 / Lat Pulldown Bar");
    UiHelpers::setTableText(ui->catalogueExampleTableWidget, 1, 1, "Sports");
    UiHelpers::setTableChipAligned(ui->catalogueExampleTableWidget, 1, 2, "D = Discounted", "discounted",
                                   Qt::AlignLeft | Qt::AlignVCenter);
    UiHelpers::setTableText(ui->catalogueExampleTableWidget, 1, 3, "449 / 100 / 0,0.1");
    UiHelpers::setTableText(ui->catalogueExampleTableWidget, 2, 0, "35 / Hot Sauce Habanero");
    UiHelpers::setTableText(ui->catalogueExampleTableWidget, 2, 1, "Perishables");
    UiHelpers::setTableChipAligned(ui->catalogueExampleTableWidget, 2, 2, "S = Standard", "success",
                                   Qt::AlignLeft | Qt::AlignVCenter);
    UiHelpers::setTableText(ui->catalogueExampleTableWidget, 2, 3, "69 / 140 / 0,0");

    ui->csvFormatTitleLabel->setText("Product Type Reference");
    ui->csvFormatHelperLabel->setText(
        "Products load from final_project_v2/Catalog/Products.csv with columns "
        "id,name,category,price,quantity,type,taxRate,discountRate.");
    UiHelpers::setupReadOnlyTable(ui->catalogueFormatTableWidget, 44, false);
    ui->catalogueFormatTableWidget->setMinimumHeight(334);
    ui->catalogueFormatTableWidget->setMinimumWidth(0);
    ui->catalogueFormatTableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->catalogueFormatTableWidget->setTextElideMode(Qt::ElideRight);
    ui->catalogueFormatTableWidget->setColumnCount(3);
    ui->catalogueFormatTableWidget->setHorizontalHeaderLabels({"Field", "Meaning", "v2 rule"});
    ui->catalogueFormatTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->catalogueFormatTableWidget->setColumnWidth(0, 70);
    ui->catalogueFormatTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->catalogueFormatTableWidget->setColumnWidth(1, 140);
    ui->catalogueFormatTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->catalogueFormatTableWidget->setRowCount(6);
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 0, 0, "id");
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 0, 1, "Numeric product key");
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 0, 2, "First column; used by warehouse orders");
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 1, 0, "category");
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 1, 1, "Product grouping");
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 1, 2, "Stored on every v2 product class");
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 2, 0, "S");
    UiHelpers::setTableChipAligned(ui->catalogueFormatTableWidget, 2, 1, "Standard", "success",
                                   Qt::AlignLeft | Qt::AlignVCenter);
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 2, 2,
                            "StandardProduct is category-backed; no rate adjustment");
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 3, 0, "T");
    UiHelpers::setTableChipAligned(ui->catalogueFormatTableWidget, 3, 1, "Taxable", "info",
                                   Qt::AlignLeft | Qt::AlignVCenter);
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 3, 2, "TaxableProduct uses taxRate");
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 4, 0, "D");
    UiHelpers::setTableChipAligned(ui->catalogueFormatTableWidget, 4, 1, "Discounted", "discounted",
                                   Qt::AlignLeft | Qt::AlignVCenter);
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 4, 2, "DiscountedProduct uses discountRate");
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 5, 0, "price");
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 5, 1, "Base price");
    UiHelpers::setTableText(ui->catalogueFormatTableWidget, 5, 2,
                            "Final price is calculated by the backend product type");

    ui->ordersFormatHelperLabel->setText(
        "Each final_project_v2/Orders/WarehouseN_Orders.csv row is one order "
        "made of product/quantity pairs. The old $, -, and * order tokens are "
        "not part of the v2 warehouse input format.");
    UiHelpers::setupReadOnlyTable(ui->ordersFormatTableWidget, 44, false);
    ui->ordersFormatTableWidget->setMinimumHeight(246);
    ui->ordersFormatTableWidget->setMinimumWidth(0);
    ui->ordersFormatTableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->ordersFormatTableWidget->setTextElideMode(Qt::ElideRight);
    ui->ordersFormatTableWidget->setColumnCount(3);
    ui->ordersFormatTableWidget->setHorizontalHeaderLabels({"Input", "Meaning", "Example"});
    ui->ordersFormatTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->ordersFormatTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->ordersFormatTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->ordersFormatTableWidget->setRowCount(5);
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 0, 0, "WarehouseN_Orders.csv");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 0, 1, "N maps to warehouse queues 1 through 5");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 0, 2, "Warehouse1_Orders.csv");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 1, 0, "One row");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 1, 1, "One order for that warehouse");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 1, 2, "33,2");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 2, 0, "Pair format");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 2, 1, "productId,quantity repeated until the row ends");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 2, 2, "20,1,2,5,9,2,25,5");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 3, 0, "Quantity");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 3, 1, "Positive integer quantities are queued");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 3, 2, "28,1");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 4, 0, "Start All");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 4, 1,
                            "Loads the warehouse files and processes the batch");
    UiHelpers::setTableText(ui->ordersFormatTableWidget, 4, 2,
                            "Logs and Analytics update from completed records");
}
