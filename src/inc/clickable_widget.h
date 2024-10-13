#pragma once

#include <QWidget>
#include <QMouseEvent>

class ClickableWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ClickableWidget(QWidget* parent = nullptr)
		: QWidget(parent) {}
	~ClickableWidget() = default;
signals:
	void Pressed(QWidget* widget);
	void Released();
protected:
	void mousePressEvent(QMouseEvent* ev) override;
	void mouseReleaseEvent(QMouseEvent* ev) override;
};
