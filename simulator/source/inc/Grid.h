#pragma once
#include "../inc/common.h"
#ifdef SHOWGUI
#include <QtWidgets>
#include <QWheelEvent>
#include <qlabel.h>

/*
Arguments: source-address, destination-address, type of frame in subvalue(1100 e.g.), frame is RX or TX, Viewport object, average SNR of the frame
Returns: itself as object
*/
class Grid : public QGridLayout
{
	Q_OBJECT
private:
	float scale;
public:

	Grid(QWidget* parent = 0) : QGridLayout(parent), scale(2) {}
	~Grid() {}
protected:
	//void mousePressEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent * event);
};
#endif