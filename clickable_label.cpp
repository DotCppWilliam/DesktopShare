#include "clickable_label.h"

void ClickableLabel::mousePressEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
		emit Pressed();
	QLabel::mousePressEvent(ev);
}


void ClickableLabel::mouseReleaseEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton)
		emit Released();
	QLabel::mouseReleaseEvent(ev);
}