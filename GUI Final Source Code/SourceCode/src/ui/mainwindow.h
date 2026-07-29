#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QString>
#include <QStyle>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QStackedWidget;
class QLabel;
class QFrame;
class DashboardPage;
class InventoryPage;
class OrdersPage;
class LogsPage;
class AnalyticsPage;
class SettingsPage;
class AppController;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void showDashboardPage();
    void showInventoryPage();
    void showOrdersPage();
    void showLogsPage();
    void showAnalyticsPage();
    void showSettingsPage();

private slots:
    void on_centralwidget_customContextMenuRequested(const QPoint &pos);

private:
    enum class TopBarPage {
        Dashboard,
        Inventory,
        Orders,
        Logs,
        Analytics,
        Settings
    };

    Ui::MainWindow *ui;
    QStackedWidget *m_mainStackedWidget = nullptr;

    DashboardPage *m_dashboardPage = nullptr;
    InventoryPage *m_inventoryPage = nullptr;
    OrdersPage *m_ordersPage = nullptr;
    LogsPage *m_logsPage = nullptr;
    AnalyticsPage *m_analyticsPage = nullptr;
    SettingsPage *m_settingsPage = nullptr;
    std::unique_ptr<AppController> m_appController;

    QLabel *m_headerTitleLabel = nullptr;
    QLabel *m_headerCaretLabel = nullptr;
    QFrame *m_topBarBrandDividerFrame = nullptr;
    QFrame *m_topBarProfileDividerFrame = nullptr;
    QPushButton *m_topBarStatusButton = nullptr;
    QPushButton *m_topBarActionOneButton = nullptr;
    QPushButton *m_topBarActionTwoButton = nullptr;
    QPushButton *m_topBarActionThreeButton = nullptr;
    QPushButton *m_adminProfileButton = nullptr;

    void setupPages();
    void setupShellChrome();
    void buildDynamicTopBar();
    void setupNavButton(QPushButton *button,
                        const QString &navKind,
                        QStyle::StandardPixmap icon);
    void setupNavButton(QPushButton *button,
                        const QString &navKind,
                        const QString &iconPath);
    void updateShellShapeForPage(TopBarPage page);
    void updateTopBarForPage(TopBarPage page);
    void setHeaderContext(const QString &context,
                          const QString &title,
                          const QString &statusText,
                          const QString &statusRole);
    void configureTopBarButton(QPushButton *button,
                               const QString &text,
                               const QString &role,
                               QStyle::StandardPixmap icon,
                               const QString &actionId = QString(),
                               bool visible = true,
                               bool enabled = true);
    void handleTopBarActionTriggered();
    void refreshHeaderActionCluster();
    void refreshCurrentPage();
    void initializeInventoryIntegration();
    void applyInventorySnapshot();
    void refreshOrdersUi();
    void updateActiveNav(QPushButton* activeBtn);
};

#endif // MAINWINDOW_H
