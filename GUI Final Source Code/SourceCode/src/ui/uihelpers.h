#ifndef UIHELPERS_H
#define UIHELPERS_H

#include <QColor>
#include <QIcon>
#include <QMargins>
#include <QSize>
#include <QStyle>
#include <Qt>
#include <QString>
#include <QVariant>

class QAbstractButton;
class QLabel;
class QPushButton;
class QSplitter;
class QTableWidget;
class QWidget;

namespace UiHelpers {

void polish(QWidget *widget);
void repolishDeep(QWidget *widget);
void setProperty(QWidget *widget, const char *name, const QVariant &value);
void setTextRole(QWidget *widget, const QString &role);
void setSurface(QWidget *widget, const QString &surface, const QString &accent = QString());
void setButtonRole(QPushButton *button, const QString &role);
void setBadge(QLabel *label, const QString &role);
void setBadgeText(QLabel *label, const QString &text, const QString &role);
QLabel *makeBadge(const QString &text, const QString &role, QWidget *parent = nullptr);
QLabel *makeIcon(const QString &text, const QString &color, int fontSize = 10, QWidget *parent = nullptr);
void setStandardIcon(QAbstractButton *button, QStyle::StandardPixmap standardPixmap, const QSize &size = QSize(16, 16));
void tuneLayout(QWidget *widget, const QMargins &margins, int spacing = -1);
void setPageLayout(QWidget *page, const QMargins &margins = QMargins(28, 28, 28, 28), int spacing = 20);
void applyCardShadow(QWidget *widget,
                     int blurRadius = 34,
                     int verticalOffset = 8,
                     const QColor &color = QColor(15, 23, 42, 30));
QString productNameToSnakeCase(const QString &productName);
QString resolveProductIconPath(const QString &productName);
QIcon productIconForName(const QString &productName);
void setupTable(QTableWidget *table, int rowHeight = 46, bool alternating = true);
void setupReadOnlyTable(QTableWidget *table, int rowHeight = 46, bool alternating = true);
void setTableText(QTableWidget *table,
                  int row,
                  int column,
                  const QString &text,
                  Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter,
                  const QColor &foreground = QColor());
void setTableBadge(QTableWidget *table, int row, int column, const QString &text, const QString &role);
void setTableChip(QTableWidget *table, int row, int column, const QString &text, const QString &role);
void setTableBadgeAligned(QTableWidget *table,
                          int row,
                          int column,
                          const QString &text,
                          const QString &role,
                          Qt::Alignment alignment);
void setTableChipAligned(QTableWidget *table,
                         int row,
                         int column,
                         const QString &text,
                         const QString &role,
                         Qt::Alignment alignment);
void softenSplitter(QSplitter *splitter);

} // namespace UiHelpers

#endif // UIHELPERS_H
