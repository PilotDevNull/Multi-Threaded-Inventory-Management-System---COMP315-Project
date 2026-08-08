#include "navbutton.h"

#include <QColor>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStylePainter>

namespace {

constexpr int kIconLeftInset = 18;
constexpr int kIconTextGap = 12;
constexpr int kTextHorizontalPadding = 14;
const QSize kFallbackIconSize(18, 18);

QColor buttonTextColor(const QStyleOptionButton &option)
{
    const QPalette::ColorGroup colorGroup =
        !(option.state & QStyle::State_Enabled) ? QPalette::Disabled
                                                : option.palette.currentColorGroup();
    return option.palette.color(colorGroup, QPalette::ButtonText);
}

QIcon::Mode iconModeForState(const QStyleOptionButton &option)
{
    if (!(option.state & QStyle::State_Enabled)) {
        return QIcon::Disabled;
    }

    if (option.state & (QStyle::State_Sunken | QStyle::State_On)) {
        return QIcon::Selected;
    }

    if (option.state & QStyle::State_MouseOver) {
        return QIcon::Active;
    }

    return QIcon::Normal;
}

QPixmap tintPixmap(const QPixmap &source, const QColor &color)
{
    if (source.isNull()) {
        return source;
    }

    QPixmap tinted(source.size());
    tinted.setDevicePixelRatio(source.devicePixelRatio());
    tinted.fill(Qt::transparent);

    QPainter painter(&tinted);
    painter.drawPixmap(0, 0, source);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(tinted.rect(), color);
    return tinted;
}

} // namespace

NavButton::NavButton(QWidget *parent)
    : QPushButton(parent)
{
}

void NavButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QStyleOptionButton option;
    initStyleOption(&option);

    const QString buttonText = option.text;
    const QIcon buttonIcon = option.icon;
    const QSize iconSize = option.iconSize.isValid() ? option.iconSize
                                                     : kFallbackIconSize;

    QStyleOptionButton backgroundOption(option);
    backgroundOption.text.clear();
    backgroundOption.icon = QIcon();

    QStylePainter painter(this);
    painter.drawControl(QStyle::CE_PushButton, backgroundOption);

    const QRect buttonRect = rect();
    const QRect textBounds =
        buttonRect.adjusted(kTextHorizontalPadding, 0, -kTextHorizontalPadding, 0);
    const QFontMetrics metrics(font());
    const int textHeight = metrics.height();

    QRect iconRect;
    if (!buttonIcon.isNull()) {
        const int iconY = (buttonRect.height() - iconSize.height()) / 2;
        iconRect = QRect(kIconLeftInset, iconY, iconSize.width(), iconSize.height());

        const qreal devicePixelRatio = painter.device()->devicePixelRatioF();
        QPixmap iconPixmap = buttonIcon.pixmap(iconSize * devicePixelRatio,
                                               iconModeForState(option),
                                               isChecked() ? QIcon::On : QIcon::Off);
        if (!iconPixmap.isNull()) {
            iconPixmap.setDevicePixelRatio(devicePixelRatio);
            painter.drawPixmap(iconRect.topLeft(),
                               tintPixmap(iconPixmap, buttonTextColor(option)));
        }
    }

    const int centeredWidth = qMin(metrics.horizontalAdvance(buttonText), textBounds.width());
    int textLeft = textBounds.center().x() - (centeredWidth / 2);
    if (!iconRect.isNull()) {
        textLeft = qMax(textLeft, iconRect.right() + kIconTextGap);
    }

    textLeft = qMax(textBounds.left(), textLeft);
    const int maxTextWidth = qMax(0, textBounds.right() - textLeft + 1);
    const QString displayText =
        metrics.elidedText(buttonText, Qt::ElideRight, maxTextWidth);
    const QRect textRect(textLeft,
                         (buttonRect.height() - textHeight) / 2,
                         maxTextWidth,
                         textHeight);

    painter.setPen(buttonTextColor(option));
    painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, displayText);
}
