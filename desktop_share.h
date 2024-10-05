#pragma once

#include <QtWidgets/QWidget>
#include "ui_desktop_share.h"

class DesktopShare : public QWidget
{
    Q_OBJECT

public:
    DesktopShare(QWidget *parent = nullptr);
    ~DesktopShare();
public:
    void changeEvent(QEvent* event) override;

public slots:
    void RtmpPush();
    void RtspPush();
    void Recording();
    void Exit();
    void Setting();

private:
    Ui::DesktopShareClass ui;
};
