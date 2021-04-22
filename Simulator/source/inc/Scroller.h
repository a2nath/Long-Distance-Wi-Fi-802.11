#pragma once
#pragma once
#include <QtWidgets>
#include <QWheelEvent>
#include "common.h"
/*
Arguments: source-address, destination-address, type of frame in subvalue(1100 e.g.), frame is RX or TX, Viewport object, average SNR of the frame
Returns: itself as object
*/

class Scroller : public QScrollArea
{
	Q_OBJECT
private:
	QGridLayout *gridLayout;
	//std::vector<QWidget*> &widgetList;
	float scroll_speed;

	const float zoomf = 2.0;
	float initial_zoom_factor;
	float current_zoom_factor;

	float initial_width;
	float current_width;
	float position_factor;
	//uint font_size_tracker, font_size_initial;
public:
	Scroller(QGridLayout *scroll_widget)
		: gridLayout(scroll_widget), initial_width(INFINITE)
	{
		scroll_speed = 85.0;
		QLayoutItem * const item = (gridLayout->itemAt(10));
		initial_width = item->widget()->width();
		current_width = initial_width;
		gridLayout->setHorizontalSpacing(1);
		//font_size_initial = 6;
		//font_size_tracker = font_size_initial;
	}

	void setup(QWidget *widget)
	{
		this->setWidget(widget);
		initial_zoom_factor = this->widget()->width();
		current_zoom_factor = initial_zoom_factor;
	}

protected:
	void wheelEvent(QWheelEvent * event);
	void mousePressEvent(QMouseEvent *event);
};
