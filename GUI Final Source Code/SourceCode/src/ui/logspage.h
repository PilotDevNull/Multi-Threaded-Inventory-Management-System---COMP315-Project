#ifndef LOGSPAGE_H
#define LOGSPAGE_H

#include <QWidget>
#include <vector>
#include <QStringList>

#include "LogsSnapshotDto.h"

class QEvent;
class QObject;
class QResizeEvent;

namespace Ui {
class LogsPage;
}

class LogsPage : public QWidget
{
    Q_OBJECT

public:
    explicit LogsPage(QWidget *parent = nullptr);
    ~LogsPage();

    // Replace the displayed table with data from the last completed batch.
    void setLogsSnapshot(const LogsSnapshotDto &snapshot);
    void exportAllCsv();

    // Legacy helper kept for compatibility; not used by the main data path.
    void setLogs(const std::vector<QStringList>& logs);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::LogsPage *ui;
    LogsSnapshotDto m_snapshot;

    void setupUiDefaults();
    void updateDetailPanel(int row);
    void clearDetailPanel();
};

#endif // LOGSPAGE_H
