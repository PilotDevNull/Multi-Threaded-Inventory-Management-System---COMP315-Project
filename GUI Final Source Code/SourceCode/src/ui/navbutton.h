#ifndef NAVBUTTON_H
#define NAVBUTTON_H

#include <QPushButton>

class NavButton : public QPushButton
{
public:
    explicit NavButton(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // NAVBUTTON_H
