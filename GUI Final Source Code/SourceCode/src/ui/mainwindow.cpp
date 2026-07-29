#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QList>
#include <QMenuBar>
#include <QResource>
#include <QSize>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringList>
#include <QStyle>
#include <QTime>
#include <QVBoxLayout>

#include "analyticspage.h"
#include "AppController.h"
#include "dashboardpage.h"
#include "inventorypage.h"
#include "logspage.h"
#include "orderspage.h"
#include "settingspage.h"
#include "uihelpers.h"
#include <QComboBox>
#include <QCloseEvent>
#include <QMessageBox>

namespace {

QString makeWindowTitle(const QString &title) {
  if (title.isEmpty()) {
    return QStringLiteral("Nexus Logistics");
  }

  return QStringLiteral("Nexus Logistics - %1").arg(title);
}

struct TopBarActionSpec {
  QString text;
  QString role;
  QStyle::StandardPixmap icon = QStyle::SP_CustomBase;
  QString actionId;
  bool visible = false;
  bool enabled = true;
};

struct TopBarConfig {
  QString context;
  QString title;
  bool showHeaderCaret = false;
  TopBarActionSpec statusSpec;
  TopBarActionSpec actionOneSpec;
  TopBarActionSpec actionTwoSpec;
  TopBarActionSpec actionThreeSpec;
  QString adminText = QStringLiteral("Admin");
};

TopBarActionSpec makeAction(const QString &text, const QString &role,
                            QStyle::StandardPixmap icon,
                            const QString &actionId = QString(),
                            bool enabled = true) {
  return {text, role, icon, actionId, true, enabled};
}

QString currentConsoleTime() {
  return QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
}

QString formatOrdersSummaryLine(const OrdersSummaryDto &summary) {
  return QStringLiteral("Success: %1 | Partial: %2 | Failed: %3")
      .arg(summary.successCount)
      .arg(summary.partialCount)
      .arg(summary.failedCount);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  resize(1460, 940);
  setMinimumSize(1320, 860);
  menuBar()->hide();
  statusBar()->hide();

  setupShellChrome();
  setupPages();
  initializeInventoryIntegration();

  connect(ui->dashboardNavButton, &QPushButton::clicked, this,
          &MainWindow::showDashboardPage);
  connect(ui->inventoryNavButton, &QPushButton::clicked, this,
          &MainWindow::showInventoryPage);
  connect(ui->ordersNavButton, &QPushButton::clicked, this,
          &MainWindow::showOrdersPage);
  connect(ui->logsNavButton, &QPushButton::clicked, this,
          &MainWindow::showLogsPage);
  connect(ui->analyticsNavButton, &QPushButton::clicked, this,
          &MainWindow::showAnalyticsPage);
  connect(ui->settingsNavButton, &QPushButton::clicked, this,
          &MainWindow::showSettingsPage);

  // Wire Inventory export CSV button in top bar
  // (actionId "exportCatalogueCsv" handled in handleTopBarActionTriggered)

  // Wire thread-config panel Start All and Reset buttons
  if (m_ordersPage) {
    auto *startBtn = m_ordersPage->findChild<QPushButton*>("submitManualOrderButton");
    auto *resetBtn = m_ordersPage->findChild<QPushButton*>("clearManualOrderButton");
    if (startBtn) {
      startBtn->setProperty("topBarActionId", QStringLiteral("ordersStartAll"));
      connect(startBtn, &QPushButton::clicked, this, &MainWindow::handleTopBarActionTriggered);
    }
    if (resetBtn) {
      connect(resetBtn, &QPushButton::clicked, this, [this]() {
        if (m_appController && !m_appController->isOrdersBatchRunning()) {
          if (m_ordersPage) m_ordersPage->clearOrdersSummary();
          if (m_ordersPage) m_ordersPage->appendConsoleEvent(
              QTime::currentTime().toString("HH:mm:ss"),
              QStringLiteral("ALL"), QStringLiteral("Reset"),
              QStringLiteral("Console cleared. Ready for next batch."),
              QStringLiteral("Idle"));
        }
      });
    }

    //Connect the manual order form signal so queued orders get pushed into AppController
    connect(m_ordersPage, &OrdersPage::manualOrderRequested,
            this, [this](int warehouseId, QVector<QPair<int,int>> items) {
      if (!m_appController) return;
      const int orderId = m_appController->addManualOrder(warehouseId, items);
      if (orderId < 0) {
          m_ordersPage->appendConsoleEvent(
              QTime::currentTime().toString("HH:mm:ss"),
              QStringLiteral("W%1").arg(warehouseId),
              QStringLiteral("Order rejected"),
              QStringLiteral("Invalid warehouse or empty cart."),
              QStringLiteral("Failed"));
          return;
      }
      //Build a readable description of what was queued
      QStringList parts;
      for (const auto &[pid, qty] : items) {
          parts << QStringLiteral("P%1 x%2").arg(pid).arg(qty);
      }
      m_ordersPage->appendConsoleEvent(
          QTime::currentTime().toString("HH:mm:ss"),
          QStringLiteral("W%1").arg(warehouseId),
          QStringLiteral("Order queued"),
          QStringLiteral("Manual order #%1: %2").arg(orderId).arg(parts.join(", ")),
          QStringLiteral("Info"));
      m_ordersPage->updateQueueCount(m_appController->pendingManualOrders());
    });
  }
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setupShellChrome() {
  ui->topBarFrame->setFixedHeight(80);
  ui->topBarFrame->setStyleSheet(QString());
  ui->topBarContentFrame->setSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Expanding);
  ui->topBarLeftFrame->setSizePolicy(QSizePolicy::Expanding,
                                     QSizePolicy::Expanding);
  ui->topBarRightFrame->setSizePolicy(QSizePolicy::Expanding,
                                      QSizePolicy::Preferred);
  ui->topBarActionsFrame->setSizePolicy(QSizePolicy::Maximum,
                                        QSizePolicy::Preferred);
  ui->topBarButtonsFrame->setSizePolicy(QSizePolicy::Maximum,
                                        QSizePolicy::Preferred);
  ui->topBarProfileFrame->setSizePolicy(QSizePolicy::Maximum,
                                        QSizePolicy::Preferred);
  ui->horizontalLayout_5->setContentsMargins(20, 10, 20, 10);
  ui->horizontalLayout_5->setSpacing(18);
  ui->horizontalLayout_4->setStretch(0, 0);
  ui->horizontalLayout_4->setStretch(1, 1);
  ui->horizontalLayout_8->setContentsMargins(0, 0, 0, 0);
  ui->horizontalLayout_9->setStretch(0, 1);
  ui->horizontalLayout_9->setStretch(1, 0);
  ui->sidebarNavTopSpacer->setMinimumHeight(44);
  ui->sidebarNavTopSpacer->setMaximumHeight(44);

  UiHelpers::setSurface(ui->MainShellArea, "shell");
  UiHelpers::setSurface(ui->sidebarFrame, "sidebar");
  UiHelpers::setSurface(ui->topBarFrame, "topbar");
  UiHelpers::setSurface(ui->pageCanvasFrame, "canvas");
  UiHelpers::setProperty(ui->topBarFrame, "shellShape",
                         QStringLiteral("rounded"));
  UiHelpers::setProperty(ui->pageCanvasFrame, "shellShape",
                         QStringLiteral("rounded"));

  UiHelpers::applyCardShadow(ui->sidebarFrame, 42, 12, QColor(18, 30, 48, 36));
  UiHelpers::applyCardShadow(ui->pageCanvasFrame, 16, 4, QColor(18, 30, 48, 6));
  //UiHelpers::applyCardShadow(ui->pageCanvasFrame, 34, 10, QColor(18, 30, 48, 16));

  UiHelpers::setProperty(ui->brandNexusLabel, "textRole", "brandAccent");
  UiHelpers::setProperty(ui->brandLogisticsLabel, "textRole", "brandWord");

  setupNavButton(ui->dashboardNavButton, "primary",
                 QStringLiteral(":/icons/dashboard.png"));
  setupNavButton(ui->inventoryNavButton, "primary",
                 QStringLiteral(":/icons/inventory.png"));
  setupNavButton(ui->ordersNavButton, "primary",
                 QStringLiteral(":/icons/orders.png"));
  setupNavButton(ui->analyticsNavButton, "primary",
                 QStringLiteral(":/icons/analytics.png"));
  setupNavButton(ui->logsNavButton, "primary",
                 QStringLiteral(":/icons/logs.png"));
  setupNavButton(ui->settingsNavButton, "primary",
                 QStringLiteral(":/icons/settings.png"));

  buildDynamicTopBar();
  refreshHeaderActionCluster();
}

void MainWindow::buildDynamicTopBar() {
  ui->topBarButtonsFrame->setObjectName(QStringLiteral("headerActionClusterFrame"));
  UiHelpers::setSurface(ui->topBarButtonsFrame, "actionCluster");
  UiHelpers::polish(ui->topBarButtonsFrame);

  auto *leftLayout = qobject_cast<QHBoxLayout *>(ui->topBarLeftFrame->layout());
  leftLayout->setSpacing(18);

  auto *rightLayout = qobject_cast<QHBoxLayout *>(ui->topBarRightFrame->layout());
  if (rightLayout) {
    rightLayout->insertStretch(0, 1);
  }

  m_topBarBrandDividerFrame = new QFrame(ui->topBarLeftFrame);
  m_topBarBrandDividerFrame->setObjectName(QStringLiteral("topBarBrandDividerFrame"));
  m_topBarBrandDividerFrame->setFrameShape(QFrame::NoFrame);
  m_topBarBrandDividerFrame->setAttribute(Qt::WA_StyledBackground, true);
  m_topBarBrandDividerFrame->setFixedSize(1, 56);
  leftLayout->addWidget(m_topBarBrandDividerFrame, 0, Qt::AlignVCenter);

  auto *titleFrame = new QFrame(ui->topBarLeftFrame);
  titleFrame->setObjectName(QStringLiteral("headerContextFrame"));
  titleFrame->setFrameShape(QFrame::NoFrame);
  auto *titleLayout = new QHBoxLayout(titleFrame);
  titleLayout->setContentsMargins(0, 0, 0, 0);
  titleLayout->setSpacing(10);

  m_headerTitleLabel = new QLabel(titleFrame);
  m_headerTitleLabel->setObjectName(QStringLiteral("topBarTitleLabel"));
  UiHelpers::setProperty(m_headerTitleLabel, "textRole", "headerTitle");

  m_headerCaretLabel = new QLabel(QStringLiteral("v"), titleFrame);
  m_headerCaretLabel->setObjectName(QStringLiteral("topBarTitleCaretLabel"));
  UiHelpers::setProperty(m_headerCaretLabel, "textRole", "pageMeta");
  m_headerCaretLabel->hide();

  titleLayout->addWidget(m_headerTitleLabel);
  titleLayout->addWidget(m_headerCaretLabel);
  titleLayout->addStretch();
  leftLayout->addWidget(titleFrame, 1);

  auto *actionLayout = ui->horizontalLayout_11;
  actionLayout->setSpacing(10);

  actionLayout->removeWidget(ui->notificationButton);
  actionLayout->removeWidget(ui->refreshButton);
  ui->notificationButton->hide();
  ui->refreshButton->hide();

  m_topBarStatusButton = new QPushButton(ui->topBarButtonsFrame);
  m_topBarStatusButton->setObjectName(QStringLiteral("topBarStatusButton"));
  m_topBarStatusButton->setFocusPolicy(Qt::NoFocus);
  actionLayout->insertWidget(0, m_topBarStatusButton);

  m_topBarActionOneButton = new QPushButton(ui->topBarButtonsFrame);
  m_topBarActionOneButton->setObjectName(QStringLiteral("topBarActionOneButton"));
  m_topBarActionOneButton->setFocusPolicy(Qt::NoFocus);
  actionLayout->addWidget(m_topBarActionOneButton);

  m_topBarActionTwoButton = new QPushButton(ui->topBarButtonsFrame);
  m_topBarActionTwoButton->setObjectName(QStringLiteral("topBarActionTwoButton"));
  m_topBarActionTwoButton->setFocusPolicy(Qt::NoFocus);
  actionLayout->addWidget(m_topBarActionTwoButton);

  m_topBarActionThreeButton = new QPushButton(ui->topBarButtonsFrame);
  m_topBarActionThreeButton->setObjectName(QStringLiteral("topBarActionThreeButton"));
  m_topBarActionThreeButton->setFocusPolicy(Qt::NoFocus);
  actionLayout->addWidget(m_topBarActionThreeButton);

  m_topBarProfileDividerFrame = new QFrame(ui->topBarActionsFrame);
  m_topBarProfileDividerFrame->setObjectName(QStringLiteral("topBarProfileDividerFrame"));
  m_topBarProfileDividerFrame->setFrameShape(QFrame::NoFrame);
  m_topBarProfileDividerFrame->setAttribute(Qt::WA_StyledBackground, true);
  m_topBarProfileDividerFrame->setFixedSize(1, 40);
  ui->horizontalLayout_9->insertWidget(1, m_topBarProfileDividerFrame, 0, Qt::AlignVCenter);

  ui->topBarProfileFrame->setMinimumWidth(0);
  ui->topBarProfileFrame->setMaximumWidth(QWIDGETSIZE_MAX);
  ui->adminNameLabel->hide();
  ui->adminRoleLabel->hide();
  ui->topBarProfileFrame->hide();

  auto *profileLayout = qobject_cast<QVBoxLayout *>(ui->topBarProfileFrame->layout());
  profileLayout->setContentsMargins(0, 0, 0, 0);
  profileLayout->setSpacing(0);

  m_adminProfileButton = new QPushButton(ui->topBarProfileFrame);
  m_adminProfileButton->setObjectName(QStringLiteral("adminProfileButton"));
  m_adminProfileButton->setFocusPolicy(Qt::NoFocus);
  profileLayout->insertWidget(0, m_adminProfileButton, 0, Qt::AlignRight | Qt::AlignVCenter);

  for (QPushButton *button : {m_topBarStatusButton,
                              m_topBarActionOneButton,
                              m_topBarActionTwoButton,
                              m_topBarActionThreeButton,
                              m_adminProfileButton}) {
    button->setCursor(Qt::PointingHandCursor);
    connect(button, &QPushButton::clicked, this,
            &MainWindow::handleTopBarActionTriggered);
  }
}

void MainWindow::setupPages() {
  if (!ui->pageCanvasFrame->layout()) {
    auto *pageCanvasLayout = new QVBoxLayout(ui->pageCanvasFrame);
    pageCanvasLayout->setContentsMargins(0, 0, 0, 0);
    pageCanvasLayout->setSpacing(0);
  } else {
    ui->pageCanvasFrame->layout()->setContentsMargins(0, 0, 0, 0);
    ui->pageCanvasFrame->layout()->setSpacing(0);
  }

  m_mainStackedWidget = new QStackedWidget(ui->pageCanvasFrame);
  m_mainStackedWidget->setObjectName(
      QStringLiteral("runtimeMainStackedWidget"));
  m_mainStackedWidget->setContentsMargins(0, 0, 0, 0);
  ui->pageCanvasFrame->layout()->addWidget(m_mainStackedWidget);

  m_dashboardPage = new DashboardPage(m_mainStackedWidget);
  m_inventoryPage = new InventoryPage(m_mainStackedWidget);
  m_ordersPage = new OrdersPage(m_mainStackedWidget);
  m_logsPage = new LogsPage(m_mainStackedWidget);
  m_analyticsPage = new AnalyticsPage(m_mainStackedWidget);
  m_settingsPage = new SettingsPage(m_mainStackedWidget);

  m_mainStackedWidget->addWidget(m_dashboardPage);
  m_mainStackedWidget->addWidget(m_inventoryPage);
  m_mainStackedWidget->addWidget(m_ordersPage);
  m_mainStackedWidget->addWidget(m_logsPage);
  m_mainStackedWidget->addWidget(m_analyticsPage);
  m_mainStackedWidget->addWidget(m_settingsPage);

  showDashboardPage();
}

void MainWindow::setupNavButton(QPushButton *button, const QString &navKind,
                                QStyle::StandardPixmap icon) {
  if (!button) {
    return;
  }

  button->setCursor(Qt::PointingHandCursor);
  button->setProperty("navItem", true);
  button->setProperty("navKind", navKind);
  button->setProperty("active", false);
  UiHelpers::setStandardIcon(button, icon, QSize(18, 18));
  UiHelpers::polish(button);
}

void MainWindow::setupNavButton(QPushButton *button, const QString &navKind,
                                const QString &iconPath) {
  if (!button) {
    return;
  }

  button->setCursor(Qt::PointingHandCursor);
  button->setProperty("navItem", true);
  button->setProperty("navKind", navKind);
  button->setProperty("active", false);
  const QResource resource(iconPath);
  const QIcon icon(iconPath);
  qDebug() << "NAV ICON" << iconPath << "resourceValid =" << resource.isValid()
           << "iconNull =" << icon.isNull();
  if (!resource.isValid() || icon.isNull()) {
    qWarning() << "Failed to load nav icon resource:" << iconPath;
  }

  button->setIcon(icon);
  button->setIconSize(QSize(18, 18));

  UiHelpers::polish(button);
}

void MainWindow::setHeaderContext(const QString &context, const QString &title,
                                  const QString &statusText,
                                  const QString &statusRole) {
  Q_UNUSED(statusRole);
  const QString visibleTitle = title.isEmpty() ? context : title;
  setWindowTitle(makeWindowTitle(visibleTitle));

  if (m_headerTitleLabel) {
    m_headerTitleLabel->setText(visibleTitle);
  }

  const QStringList tooltipLines = {context, title, statusText};

  QStringList visibleLines;
  for (const QString &line : tooltipLines) {
    if (!line.isEmpty()) {
      visibleLines.append(line);
    }
  }

  const QString tooltip = visibleLines.join(QLatin1Char('\n'));
  ui->topBarFrame->setToolTip(tooltip);
  if (m_headerTitleLabel) {
    m_headerTitleLabel->setToolTip(tooltip);
  }
}

void MainWindow::configureTopBarButton(QPushButton *button, const QString &text,
                                       const QString &role,
                                       QStyle::StandardPixmap icon,
                                       const QString &actionId,
                                       bool visible,
                                       bool enabled) {
  Q_UNUSED(icon);

  if (!button) {
    return;
  }

  const bool shouldShow = visible && !text.isEmpty();
  button->setVisible(shouldShow);
  button->setEnabled(shouldShow && enabled);
  button->setProperty("topBarActionId", actionId);
  if (!shouldShow) {
    button->setText(QString());
    button->setIcon(QIcon());
    button->setToolTip(QString());
    return;
  }

  button->setText(text);
  UiHelpers::setButtonRole(button, role);
  button->setIcon(QIcon());
  button->setToolTip(text);
}

void MainWindow::updateShellShapeForPage(TopBarPage page) {
  const QString shellShape =
      (page == TopBarPage::Dashboard) ? QStringLiteral("rounded")
                                      : QStringLiteral("square");

  UiHelpers::setProperty(ui->topBarFrame, "shellShape", shellShape);
  UiHelpers::setProperty(ui->pageCanvasFrame, "shellShape", shellShape);
  UiHelpers::polish(ui->topBarFrame);
  UiHelpers::polish(ui->pageCanvasFrame);
}

void MainWindow::updateTopBarForPage(TopBarPage page) {
  TopBarConfig config;

  switch (page) {
  case TopBarPage::Dashboard:
    config.context = tr("Operations Overview");
    config.title = tr("Inventory & Order Processing");
    config.statusSpec =
        makeAction(tr("System Initialized"), QStringLiteral("status"),
                   QStyle::SP_DialogApplyButton);
    config.actionOneSpec =
        makeAction(tr("Shutdown"), QStringLiteral("danger"),
                   QStyle::SP_BrowserStop, QStringLiteral("shutdown"));
    config.adminText = tr("Admin");
    break;
  case TopBarPage::Inventory:
    config.context = tr("Inventory Workspace");
    config.title = tr("Catalogue & Stock Control");
    config.actionOneSpec =
        makeAction(tr("Load Catalogue CSV"), QStringLiteral("secondary"),
                   QStyle::SP_DialogOpenButton, QStringLiteral("reloadInventory"));
    config.actionTwoSpec =
        makeAction(tr("Export Catalogue CSV"), QStringLiteral("success"),
                   QStyle::SP_DialogSaveButton);
    config.actionThreeSpec =
        makeAction(tr("Refresh"), QStringLiteral("secondary"),
                   QStyle::SP_BrowserReload, QStringLiteral("refresh"));
    config.adminText = tr("Admin");
    break;
  case TopBarPage::Orders:
    {
      const bool ordersRunning = m_appController && m_appController->isOrdersBatchRunning();
      const bool hasOrdersSummary = m_appController && m_appController->hasOrdersSummary();
      const bool hasOrdersError =
          m_appController && !m_appController->lastOrdersError().isEmpty();

      config.context = tr("Protected Orders Batch");
      config.title = tr("Inventory & Order Processing");

      if (ordersRunning) {
        config.statusSpec =
            makeAction(tr("Processing"), QStringLiteral("status"), QStyle::SP_BrowserReload);
      } else if (hasOrdersSummary) {
        config.statusSpec = makeAction(tr("Summary Ready"),
                                       QStringLiteral("success"),
                                       QStyle::SP_DialogApplyButton);
      } else if (hasOrdersError) {
        config.statusSpec = makeAction(tr("Orders Failed"),
                                       QStringLiteral("danger"),
                                       QStyle::SP_MessageBoxWarning);
      } else {
        config.statusSpec =
            makeAction(tr("Ready"), QStringLiteral("status"), QStyle::SP_DialogApplyButton);
      }

      config.actionOneSpec =
          makeAction(tr("Start All"),
                     QStringLiteral("secondary"),
                     QStyle::SP_MediaPlay,
                     QStringLiteral("ordersStartAll"),
                     !ordersRunning);
      config.actionTwoSpec =
          makeAction(tr("Stop All"),
                     QStringLiteral("secondary"),
                     QStyle::SP_MediaStop,
                     QStringLiteral("ordersStopAll"),
                     false);
      config.actionThreeSpec =
          makeAction(tr("Join/Finish Processing"),
                     QStringLiteral("secondary"),
                     QStyle::SP_DialogApplyButton,
                     QStringLiteral("ordersShowSummary"),
                     true);
      config.adminText = tr("Admin v");
      break;
    }
  case TopBarPage::Logs:
    config.context = tr("Processed Orders");
    config.title   = tr("Last Batch Results");
    config.actionOneSpec =
        makeAction(tr("Export All CSV"), QStringLiteral("success"),
                   QStyle::SP_DialogSaveButton, QStringLiteral("exportAllLogs"));
    config.adminText = tr("Admin");
    break;
  case TopBarPage::Analytics:
    config.context = tr("Batch Reporting");
    config.title   = tr("Batch Analytics");
    config.actionOneSpec =
        makeAction(tr("Export Global CSV"), QStringLiteral("success"),
                   QStyle::SP_DialogSaveButton, QStringLiteral("exportGlobalAnalytics"));
    config.actionTwoSpec =
        makeAction(tr("Export Warehouse CSV"), QStringLiteral("secondary"),
                   QStyle::SP_DialogSaveButton, QStringLiteral("exportWarehouseAnalytics"));
    config.adminText = tr("Admin");
    break;
  case TopBarPage::Settings:
    config.context = tr("Administration");
    config.title = tr("Workspace Preferences");
    config.adminText = tr("Admin");
    break;
  }

  updateShellShapeForPage(page);
  setHeaderContext(config.context, config.title, config.statusSpec.text,
                   config.statusSpec.role);
  if (m_headerCaretLabel) {
    m_headerCaretLabel->setVisible(config.showHeaderCaret);
  }

  configureTopBarButton(m_topBarStatusButton, config.statusSpec.text,
                        config.statusSpec.role, config.statusSpec.icon,
                        config.statusSpec.actionId, config.statusSpec.visible,
                        config.statusSpec.enabled);
  configureTopBarButton(m_topBarActionOneButton, config.actionOneSpec.text,
                        config.actionOneSpec.role, config.actionOneSpec.icon,
                        config.actionOneSpec.actionId,
                        config.actionOneSpec.visible,
                        config.actionOneSpec.enabled);
  configureTopBarButton(m_topBarActionTwoButton, config.actionTwoSpec.text,
                        config.actionTwoSpec.role, config.actionTwoSpec.icon,
                        config.actionTwoSpec.actionId,
                        config.actionTwoSpec.visible,
                        config.actionTwoSpec.enabled);
  configureTopBarButton(m_topBarActionThreeButton, config.actionThreeSpec.text,
                        config.actionThreeSpec.role, config.actionThreeSpec.icon,
                        config.actionThreeSpec.actionId,
                        config.actionThreeSpec.visible,
                        config.actionThreeSpec.enabled);
  configureTopBarButton(m_adminProfileButton, config.adminText,
                        QStringLiteral("profile"), QStyle::SP_CustomBase,
                        QStringLiteral("settings"), false, false);

  refreshHeaderActionCluster();
}

void MainWindow::handleTopBarActionTriggered() {
  auto *button = qobject_cast<QPushButton *>(sender());
  if (!button) {
    return;
  }

  const QString actionId = button->property("topBarActionId").toString();
  if (actionId == QStringLiteral("refresh")
      || actionId == QStringLiteral("reloadInventory")) {
    if (m_appController && m_appController->refreshInventorySnapshot()) {
      applyInventorySnapshot();
      refreshCurrentPage();
      return;
    }

    if (m_appController) {
      qWarning() << "Inventory reload failed:" << m_appController->lastInventoryError();
    }
  } else if (actionId == QStringLiteral("settings")) {
    showSettingsPage();
  } else if (actionId == QStringLiteral("ordersStartAll")) {
    if (!m_appController) {
      return;
    }

    // Read selected thread count from the dropdown
    int workerCount = 4;
    if (m_ordersPage) {
      auto *combo = m_ordersPage->findChild<QComboBox*>("manualWarehouseComboBox");
      if (combo) workerCount = combo->currentData().toInt();
    }

    if (!m_appController->startOrdersBatch(workerCount)) {
      if (m_ordersPage) {
        m_ordersPage->appendConsoleEvent(currentConsoleTime(),
                                         QStringLiteral("ALL"),
                                         QStringLiteral("Processing"),
                                         QStringLiteral("A protected orders batch is already running."),
                                         QStringLiteral("Running"));
      }
    }

    refreshOrdersUi();
  } else if (actionId == QStringLiteral("ordersShowSummary")) {
    if (!m_appController || !m_ordersPage) {
      return;
    }

    if (m_appController->isOrdersBatchRunning()) {
      m_ordersPage->appendConsoleEvent(
          currentConsoleTime(),
          QStringLiteral("ALL"),
          QStringLiteral("Processing"),
          QStringLiteral("Batch still running. Summary will be available after completion."),
          QStringLiteral("Running"));
      refreshOrdersUi();
      return;
    }

    if (m_appController->hasOrdersSummary()) {
      refreshOrdersUi();
      m_ordersPage->appendConsoleEvent(currentConsoleTime(),
                                       QStringLiteral("ALL"),
                                       QStringLiteral("Summary ready"),
                                       formatOrdersSummaryLine(m_appController->ordersSummary()),
                                       QStringLiteral("Completed"));
      return;
    }

    if (!m_appController->lastOrdersError().isEmpty()) {
      m_ordersPage->appendConsoleEvent(currentConsoleTime(),
                                       QStringLiteral("ALL"),
                                       QStringLiteral("Processing failed"),
                                       m_appController->lastOrdersError(),
                                       QStringLiteral("Failed"));
      refreshOrdersUi();
      return;
    }

    m_ordersPage->appendConsoleEvent(
        currentConsoleTime(),
        QStringLiteral("ALL"),
        QStringLiteral("Summary unavailable"),
        QStringLiteral("Run Start All to process the protected warehouse batch."),
        QStringLiteral("Idle"));
  } else if (actionId == QStringLiteral("shutdown")) {
    close();
  } else if (actionId == QStringLiteral("exportGlobalAnalytics")) {
    if (m_analyticsPage) m_analyticsPage->exportGlobalCsv();
  } else if (actionId == QStringLiteral("exportWarehouseAnalytics")) {
    if (m_analyticsPage) m_analyticsPage->exportWarehouseCsv();
  } else if (actionId == QStringLiteral("exportAllLogs")) {
    if (m_logsPage) m_logsPage->exportAllCsv();
  } else if (actionId == QStringLiteral("exportCatalogueCsv")) {
    if (m_inventoryPage) m_inventoryPage->exportCatalogueCsv();
  }
}

void MainWindow::refreshHeaderActionCluster() {
  auto shouldShowButton = [](QPushButton *button) {
    return button && !button->isHidden();
  };

  const bool hasVisibleActions =
      shouldShowButton(m_topBarStatusButton) ||
      shouldShowButton(m_topBarActionOneButton) ||
      shouldShowButton(m_topBarActionTwoButton) ||
      shouldShowButton(m_topBarActionThreeButton);

  ui->topBarButtonsFrame->setVisible(hasVisibleActions);
  ui->topBarProfileFrame->setVisible(false);
  if (m_topBarProfileDividerFrame) {
    m_topBarProfileDividerFrame->setVisible(false);
  }
}

void MainWindow::refreshCurrentPage() {
  if (!m_mainStackedWidget) {
    return;
  }

  QWidget *currentPage = m_mainStackedWidget->currentWidget();
  if (currentPage == m_inventoryPage) {
    showInventoryPage();
    return;
  }

  if (currentPage == m_ordersPage) {
    showOrdersPage();
    return;
  }

  if (currentPage == m_logsPage) {
    showLogsPage();
    return;
  }

  if (currentPage == m_analyticsPage) {
    showAnalyticsPage();
    return;
  }

  if (currentPage == m_settingsPage) {
    showSettingsPage();
    return;
  }

  showDashboardPage();
}

void MainWindow::initializeInventoryIntegration()
{
  m_appController = std::make_unique<AppController>();

  connect(m_appController.get(), &AppController::ordersBatchStarted, this, [this]() {
    if (m_dashboardPage) {
      m_dashboardPage->setOrdersBatchRunning();
    }

    if (m_ordersPage) {
      m_ordersPage->clearOrdersSummary();
      m_ordersPage->setBatchRunning(true);
      m_ordersPage->updateQueueCount(0);
      m_ordersPage->appendConsoleEvent(
          currentConsoleTime(),
          QStringLiteral("ALL"),
          QStringLiteral("Batch started"),
          QStringLiteral("Processing default warehouse CSV files in the background."),
          QStringLiteral("Running"));
    }

    refreshOrdersUi();
  });

  connect(m_appController.get(), &AppController::ordersBatchFinished, this, [this]() {
    refreshOrdersUi();
    if (m_ordersPage) {
      m_ordersPage->appendConsoleEvent(currentConsoleTime(),
                                       QStringLiteral("ALL"),
                                       QStringLiteral("Processing complete"),
                                       formatOrdersSummaryLine(m_appController->ordersSummary()),
                                       QStringLiteral("Completed"));
    }
    // Push the per-order snapshot to the Logs page so it shows real data.
    if (m_logsPage && m_appController->hasLogsSnapshot()) {
      m_logsPage->setLogsSnapshot(m_appController->logsSnapshot());
    }
    // Push derived analytics snapshot to the Analytics page.
    if (m_analyticsPage && m_appController->hasAnalyticsSnapshot()) {
      m_analyticsPage->setAnalyticsSnapshot(m_appController->analyticsSnapshot());
    }
    if (m_dashboardPage && m_appController->hasAnalyticsSnapshot()) {
      m_dashboardPage->setAnalyticsSnapshot(m_appController->analyticsSnapshot());
    }
  });

  connect(m_appController.get(),
          &AppController::ordersBatchFailed,
          this,
          [this](const QString &errorMessage) {
            refreshOrdersUi();
            if (m_ordersPage) {
              m_ordersPage->appendConsoleEvent(currentConsoleTime(),
                                               QStringLiteral("ALL"),
                                               QStringLiteral("Processing failed"),
                                               errorMessage,
                                               QStringLiteral("Failed"));
            }
            if (m_dashboardPage) {
              m_dashboardPage->setOrdersBatchFailed(errorMessage);
            }
          });

  if (!m_appController->refreshInventorySnapshot()) {
    qWarning() << "Inventory integration failed:" << m_appController->lastInventoryError();
    return;
  }

  applyInventorySnapshot();
}

void MainWindow::applyInventorySnapshot()
{
  if (!m_appController) {
    return;
  }

  const InventorySnapshotDto &snapshot = m_appController->inventorySnapshot();
  if (m_inventoryPage) {
    m_inventoryPage->setInventoryData(snapshot.products, snapshot.sourcePath);
  }

  if (m_dashboardPage) {
    m_dashboardPage->setInventoryMetrics(snapshot.totalProducts,
                                         snapshot.totalStockUnits,
                                         snapshot.lowStockItems);
  }
}

void MainWindow::refreshOrdersUi()
{
  if (!m_ordersPage || !m_appController) {
    return;
  }

  if (m_appController->isOrdersBatchRunning()) {
    m_ordersPage->clearOrdersSummary();
    m_ordersPage->setBatchRunning(true);
  } else if (m_appController->hasOrdersSummary()) {
    m_ordersPage->setBatchRunning(false);
    m_ordersPage->setOrdersSummary(m_appController->ordersSummary());
  } else {
    m_ordersPage->clearOrdersSummary();
    if (!m_appController->lastOrdersError().isEmpty()) {
      m_ordersPage->setBatchStatus(QStringLiteral("Failed"), QStringLiteral("fail"));
    }
  }

  if (m_mainStackedWidget && m_mainStackedWidget->currentWidget() == m_ordersPage) {
    updateTopBarForPage(TopBarPage::Orders);
  }
}

void MainWindow::showDashboardPage() {
  if (m_mainStackedWidget) {
    m_mainStackedWidget->setCurrentWidget(m_dashboardPage);
  }

  updateTopBarForPage(TopBarPage::Dashboard);
  updateActiveNav(ui->dashboardNavButton);
  ui->pageCanvasFrame->update();
}

void MainWindow::showInventoryPage() {
  if (m_mainStackedWidget) {
    m_mainStackedWidget->setCurrentWidget(m_inventoryPage);
  }

  updateTopBarForPage(TopBarPage::Inventory);
  updateActiveNav(ui->inventoryNavButton);

  ui->pageCanvasFrame->repaint();
  if (m_inventoryPage) {
    m_inventoryPage->updateGeometry();
    m_inventoryPage->repaint();
  }
}

void MainWindow::showOrdersPage() {
  if (m_mainStackedWidget) {
    m_mainStackedWidget->setCurrentWidget(m_ordersPage);
  }

  refreshOrdersUi();
  updateTopBarForPage(TopBarPage::Orders);
  updateActiveNav(ui->ordersNavButton);
  ui->pageCanvasFrame->update();
}

void MainWindow::showLogsPage() {
  if (m_mainStackedWidget) {
    m_mainStackedWidget->setCurrentWidget(m_logsPage);
  }

  updateTopBarForPage(TopBarPage::Logs);
  updateActiveNav(ui->logsNavButton);
  ui->pageCanvasFrame->update();
}

void MainWindow::showAnalyticsPage() {
  if (m_mainStackedWidget) {
    m_mainStackedWidget->setCurrentWidget(m_analyticsPage);
  }

  updateTopBarForPage(TopBarPage::Analytics);
  updateActiveNav(ui->analyticsNavButton);
  ui->pageCanvasFrame->update();
}

void MainWindow::showSettingsPage() {
  if (m_mainStackedWidget) {
    m_mainStackedWidget->setCurrentWidget(m_settingsPage);
  }

  updateTopBarForPage(TopBarPage::Settings);
  updateActiveNav(ui->settingsNavButton);
  ui->pageCanvasFrame->update();
}

void MainWindow::updateActiveNav(QPushButton *activeBtn) {
  const QList<QPushButton *> buttons = {
      ui->dashboardNavButton, ui->inventoryNavButton, ui->ordersNavButton,
      ui->analyticsNavButton, ui->logsNavButton, ui->settingsNavButton};

  const QString defaultStyle = R"(
        QPushButton {
            background-color: rgb(235, 238, 242);
            color: rgb(160, 168, 178);
            border: none;
            text-align: center;
            padding: 10px 14px;
            border-radius: 8px;
        }
        QPushButton:hover {
            background-color: rgb(220, 225, 232);
        }
        QPushButton:pressed {
            background-color: rgb(40, 120, 215);
            color: white;
        }
    )";

  const QString activeStyle = R"(
        QPushButton {
            background-color: rgb(40, 120, 215);
            color: white;
            border: none;
            text-align: center;
            padding: 10px 14px;
            border-radius: 8px;
        }
    )";

  for (QPushButton *button : buttons) {
    if (!button) {
      continue;
    }

    if (button == activeBtn) {
      button->setStyleSheet(activeStyle);
      button->setProperty("active", true);
    } else {
      button->setStyleSheet(defaultStyle);
      button->setProperty("active", false);
    }

    UiHelpers::polish(button);
  }
}

void MainWindow::on_centralwidget_customContextMenuRequested(
    const QPoint &pos) {}
