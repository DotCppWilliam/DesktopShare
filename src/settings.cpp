#include "settings.h"
#include <QScreen>
#include <QDebug>

static QWidget* kPrevWidget;

Settings::Settings(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::SettingsClass())
{
	ui->setupUi(this);
	kPrevWidget = ui->audio_widget;

	setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint & ~Qt::WindowMinimizeButtonHint);
	setWindowTitle(QString::fromLocal8Bit("设置"));

// 获取分辨率
	QScreen* screen = QApplication::primaryScreen();
	QRect screen_geometry = screen->geometry();

	int win_width = screen_geometry.width() * 0.4;
	int win_height = screen_geometry.height() * 0.5;
	resize(win_width, win_height);  // 设置窗口大小

	int x = (screen_geometry.width() - win_width) / 2;
	int y = (screen_geometry.height() - win_height) / 2;
	this->move(x, y);


	connect(ui->audio_widget, &ClickableWidget::Pressed, this, &Settings::SettingOptionsSlot);
	connect(ui->video_widget, &ClickableWidget::Pressed, this, &Settings::SettingOptionsSlot);
	connect(ui->record_widget, &ClickableWidget::Pressed, this, &Settings::SettingOptionsSlot);
	connect(ui->live_widget, &ClickableWidget::Pressed, this, &Settings::SettingOptionsSlot);

	page_index_map_ = {
		{ ui->audio_widget, 0},
		{ ui->video_widget, 1},
		{ ui->record_widget, 2},
		{ ui->live_widget, 3},
	};
}

Settings::~Settings()
{
	delete ui;
}

void Settings::SettingOptionsSlot(QWidget* widget)
{
	int index = page_index_map_.find(widget)->second;
	qDebug() << index;
	ui->stackedWidget->setCurrentIndex(index);

	if (widget != kPrevWidget)
	{
		widget->setStyleSheet("QWidget, QLabel {"
			"background: #284cb8;"
			"}");
		kPrevWidget->setStyleSheet("QWidget, QLabel {"
			"background: #1f212a;"
			"}");
		kPrevWidget = widget;
		return;
	}

}
