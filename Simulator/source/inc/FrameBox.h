#pragma once
#include <QtWidgets>
#include <QWheelEvent>
#include "gcommon.h"
#include <iostream>
/*
Arguments: source-address, destination-address, type of frame in subvalue(1100 e.g.), frame is RX or TX, Viewport object, average SNR of the frame
Returns: itself as object
*/
class FrameBox : public QWidget
{
	Q_OBJECT
private:
	static bool timer_displayed;
	uint gridID, row, divisor;
	float scale;
	gui_frame_stat* tx_state, *rx_state;
public:
	uint font_size;
	FrameBox(uint id, uint grow, gcell state) : gridID(id), font_size(6), tx_state(NULL), rx_state(NULL)
	{
		tx_state = state.transmitter;
		rx_state = state.receiver;

		scale = 1.0;
		row = grow;
		divisor = (Global::station_count + 1) * dot11a_slot_time;
	}

protected:
	void paintEvent(QPaintEvent * e) override;
};
