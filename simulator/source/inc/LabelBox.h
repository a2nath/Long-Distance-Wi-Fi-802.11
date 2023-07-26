#pragma once
#include <QtWidgets>
#include <QWheelEvent>
#include <qlabel.h>
#include "common.h"
/*
Arguments: source-address, destination-address, type of frame in subvalue(1100 e.g.), frame is RX or TX, Viewport object, average SNR of the frame
Returns: itself as object
*/
class LabelBox : public QLabel
{
	Q_OBJECT
private:
	QString label;
	uint gridID, divisor;
public:
	uint font_size;
	LabelBox(uint id, const QString& text = "") :gridID(id), label(text), font_size(6)
	{
		divisor = (Global::station_count + 1) * dot11a_slot_time;
	}
	~LabelBox() {}
protected:
	void paintEvent(QPaintEvent * e);
};
