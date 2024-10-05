#pragma once

#include <QLabel>
#include <QMouseEvent>

class ClickableLabel : public QLabel
{
	Q_OBJECT
public:
	explicit ClickableLabel(QWidget* parent = nullptr)
		: QLabel(parent) {}
	~ClickableLabel() = default;
signals:
	void Pressed();
	void Released();
protected:
	void mousePressEvent(QMouseEvent* ev) override;
	void mouseReleaseEvent(QMouseEvent* ev) override;
};

