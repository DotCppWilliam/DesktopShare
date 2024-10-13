#include "desktop_share.h"
#include "clickable_label.h"

#include <QScreen>
#include <QPushButton>
#include <QDebug>
#include <QDialog>

DesktopShare::DesktopShare(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    
    setWindowTitle(QString::fromLocal8Bit("桌面共享"));
// 获取分辨率
    QScreen* screen = QApplication::primaryScreen();    
    QRect screen_geometry = screen->geometry();

// 设置窗口的大小,按照当前分辨率比例设置
    int win_width = screen_geometry.width() * 0.65;
    int win_height = screen_geometry.height() * 0.85;
    resize(win_width, win_height);  // 设置窗口大小

// 设置窗口位于屏幕中央
	int x = (screen_geometry.width() - win_width) / 2;
	int y = (screen_geometry.height() - win_height) / 2;
	this->move(x, y);

// 设置最大长度,避免最大化,屏幕显示不
    ui.openGLWidget->setMaximumWidth(screen_geometry.width() * 0.8);   
    ui.widget->setMaximumWidth(screen_geometry.width() * 0.8);

    ui.recording_pause_label->setFixedWidth(0);
// 设置按钮按下的信号和槽
    connect(ui.rtmp_push_btn, SIGNAL(Pressed()), this, SLOT(RtmpPush()));
    connect(ui.rtsp_push_btn, SIGNAL(Pressed()), this, SLOT(RtspPush()));
    connect(ui.recording_btn, SIGNAL(Pressed()), this, SLOT(Recording()));
    connect(ui.setting_btn, SIGNAL(Pressed()), this, SLOT(Setting()));

    
}

DesktopShare::~DesktopShare()
{}

void DesktopShare::changeEvent(QEvent * event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        if (windowState() & Qt::WindowMaximized)
        {
			int x = (this->width() - ui.openGLWidget->maximumWidth()) / 2;
			ui.openGLWidget->resize(ui.openGLWidget->maximumWidth(), ui.openGLWidget->height());
			ui.openGLWidget->move(x, 0);
        }
    }
    QWidget::changeEvent(event);
}


void SetLabelStyle(ClickableLabel* label, bool& flag, const char* press_str, const char* release_str)
{
    if (!flag)
    {
        flag = true;
        label->setStyleSheet("QLabel {"
            "background: #191b26;"
            "}");
        label->setText(QString::fromLocal8Bit(press_str));
        return;
    }
    label->setStyleSheet("");
	label->setStyleSheet("QLabel{"
		"background: #3c404b;"
		"}"
		"QLabel:hover{"
		"background-color: #4f535e;    /* 鼠标悬停时的背景颜色 */"
		"}");
    label->setText(QString::fromLocal8Bit(release_str));
    flag = false;
}

void DesktopShare::RtmpPush()
{
    static bool kRtmpPushing = false;
    SetLabelStyle(ui.rtmp_push_btn, kRtmpPushing, "正在RTMP推流", "RTMP 推流");
}

void DesktopShare::RtspPush()
{
    static bool kRtspPushing = false;
    SetLabelStyle(ui.rtsp_push_btn, kRtspPushing, "正在RTSP推流", "RTSP 推流");
}

void DesktopShare::Recording()
{
    static bool kRecording = false;
    if (kRecording == false)
	{
		ui.recording_pause_label->setFixedWidth(ui.rtmp_push_btn->height());
		ui.recording_pause_label->setFixedHeight(ui.rtmp_push_btn->height());
        ui.recording_pause_label->setPixmap(QPixmap(":/DesktopShare/res/pause.png"));
        ui.recording_pause_label->setScaledContents(true);
        
        kRecording = true;
        return;
    }

	ui.recording_pause_label->setFixedWidth(0);
	ui.recording_pause_label->setText(QString::fromLocal8Bit("停止录制"));
    kRecording = false;
    qDebug() << "recording";
}

void DesktopShare::Exit()
{

    qDebug() << "exit";
}

void DesktopShare::Setting()
{
    settings.setWindowModality(Qt::ApplicationModal);
    settings.show();
}


