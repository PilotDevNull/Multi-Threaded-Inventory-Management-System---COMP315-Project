#ifndef ORDERSPAGE_H
#define ORDERSPAGE_H

#include <QWidget>
#include <QString>

#include "OrdersSummaryDto.h"

class QResizeEvent;

namespace Ui {
class OrdersPage;
}

class OrdersPage : public QWidget
{
    Q_OBJECT

public:
    explicit OrdersPage(QWidget *parent = nullptr);
    ~OrdersPage();

    void appendConsoleEvent(const QString& time,
                            const QString& warehouse,
                            const QString& event,
                            const QString& message,
                            const QString& state = QStringLiteral("Info"));
    void setBatchRunning(bool running);
    void setBatchStatus(const QString& text, const QString& badgeRole);
    void setOrdersSummary(const OrdersSummaryDto& summary);
    void clearOrdersSummary();
    void updateQueueCount(int count);

signals:
    //Emitted when the user submits a manual order from the form.
    //MainWindow connects this to AppController::addManualOrder.
    void manualOrderRequested(int warehouseId,
                              QVector<QPair<int,int>> items);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::OrdersPage *ui;
    void setupUiDefaults();
    void setWarehouseSummary(int warehouseId,
                             const QString& statusText,
                             const QString& statusRole,
                             const QString& successText,
                             const QString& successRole,
                             const QString& failedText,
                             const QString& failedRole);
};

#endif // ORDERSPAGE_H
