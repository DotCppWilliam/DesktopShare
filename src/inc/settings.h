#pragma once

#include "ui_settings.h"
#include "clickable_widget.h"

#include <QWidget>
#include <unordered_map>

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsClass; };
QT_END_NAMESPACE

class Settings : public QWidget
{
	Q_OBJECT
public:
	Settings(QWidget *parent = nullptr);
	~Settings();
public slots:
	void SettingOptionsSlot(QWidget* widget);
private:
	std::unordered_map<QWidget*, int> page_index_map_;
	Ui::SettingsClass *ui;
};
