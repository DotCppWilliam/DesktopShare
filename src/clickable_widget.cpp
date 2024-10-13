#include "clickable_widget.h"


void ClickableWidget::mousePressEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
		emit Pressed(this);
	QWidget::mousePressEvent(ev);
}


void ClickableWidget::mouseReleaseEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
		emit Released();
	QWidget::mouseReleaseEvent(ev);
}