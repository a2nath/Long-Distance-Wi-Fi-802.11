#include <iostream>
#include "../inc/GUI.h"
#ifdef SHOWGUI
using namespace std;
typedef shared_ptr<QLabel> ptrLabel;
typedef shared_ptr<FrameBox> ptrFBox;
bool FrameBox::timer_displayed = false;

Window::Window(const gcelltable &guihelper)
{
	GUI_WINDOW = new QMainWindow();
	GUI_WINDOW->resize(WIDTH, HEIGHT);
	mainWidget = new QWidget(GUI_WINDOW);
	gridEPG = new Grid(mainWidget);
	mainWidget->setLayout(gridEPG);

	uint left_pane_width = 72, col_num = 0;
	auto labelDate = new QLabel("TIME (us):");
	labelDate->setFixedWidth(left_pane_width);
	labelDate->setAlignment(Qt::AlignCenter);
	gridEPG->addWidget(labelDate, 0, col_num);

	//* labels in the left pane */
	uint row = 0;
	for (uint station = 0; station < Global::station_count; ++station)
	{
		auto timeline = new QLabel("Station " + QString::number(station));
		timeline->setFixedWidth(left_pane_width);
		timeline->setAlignment(Qt::AlignCenter);
		gridEPG->addWidget(timeline, station + 1, col_num);
	}

	// Top row (row 0) for displaying the Time Block
	//uint body_width = 35, max_columns = 1000, gridID = 0;
	uint start = 0, body_width = 35, max_columns = Global::DEBUG_END > 50000 ? 75000 : Global::DEBUG_END, gridID = 0;
	for (uint col = (dur2slots(start, true) * dot11a_slot_time) + 1; col <= max_columns; ++col)
	{
		for (uint row = 0; row < Global::station_count; ++row)
		{
			if (row == 0)
			{
				LabelBox *microsecond = new LabelBox(gridID++, QString::number(col - 1));
				microsecond->setFixedWidth(body_width);
				microsecond->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
				//widgetList.push_back(microsecond);
				gridEPG->addWidget(microsecond, row, col);
			}

			FrameBox *frame = new FrameBox(gridID++, row + 1, guihelper[row][col - 1]);
			frame->setFixedWidth(body_width);
			frame->setStyleSheet("background-color: #ffffff");
			//widgetList.push_back(frame);
			gridEPG->addWidget(frame, row + 1, col);
		}
		gridEPG->setColumnStretch(col, 50);
	}
	auto scrollArea = new Scroller(gridEPG);
	scrollArea->setup(mainWidget);
	//scrollArea->setGeometry(QRect(0, 0, WIDTH, HEIGHT));

	gridEPG->setVerticalSpacing(10);
	scrollArea->setWidgetResizable(false);
	scrollArea->verticalScrollBar()->setEnabled(false);
	mainWidget->setFixedHeight(500);

	GUI_WINDOW->setCentralWidget(scrollArea);
	GUI_WINDOW->setWindowTitle(QApplication::translate("MontanaGUI", "Timeline of 802.11a Simulation", Q_NULLPTR));
	GUI_WINDOW->show();
}
void Grid::mouseMoveEvent(QMouseEvent *event)
{
}

void Scroller::wheelEvent(QWheelEvent * event)
{
	this->horizontalScrollBar()->setValue(this->horizontalScrollBar()->value()
		+ (event->angleDelta().y() > 0 ? scroll_speed : -scroll_speed));
	event->accept();
}

/* ========================= Zooming function ========================= */
void Scroller::mousePressEvent(QMouseEvent *event)
{
	if (event->buttons() == Qt::LeftButton)
	{
		current_width *= zoomf;
		current_zoom_factor *= zoomf;
	}
	else if (event->buttons() == Qt::MiddleButton)
	{
		current_width /= zoomf;
		current_zoom_factor /= zoomf;
	}
	else if (event->buttons() == Qt::RightButton)
	{
		current_width = initial_width / pow(1.1, 10);
		current_zoom_factor = initial_zoom_factor / pow(1.1, 10);
	}

	for (int idx = Global::station_count + 1; idx < gridLayout->count(); idx++)
	{
		QLayoutItem * const item = gridLayout->itemAt(idx);
		auto i = (dynamic_cast<QWidget *>(item->widget()));
		i->setFixedWidth(current_width);
	}
	position_factor = this->horizontalScrollBar()->value() / (float)this->horizontalScrollBar()->maximum();
	this->widget()->resize(QSize(current_zoom_factor, this->widget()->height()));
	this->horizontalScrollBar()->setValue(position_factor * this->horizontalScrollBar()->maximum());

}

void LabelBox::paintEvent(QPaintEvent * e)
{
	QPainter painter(this);
	painter.setFont(QFont("Arial", font_size));
	painter.setPen(QPen(QColor(77, 116, 168)));
	painter.drawText(QPoint(2, this->height() / 2), label);

	/* draw time-slot separation */
	if (!(this->gridID % divisor))
	{
		painter.setPen(QPen(QBrush("#C3C3C3"), 3, Qt::DashDotDotLine));
		painter.drawLine(0, 0, 0, this->height());
	}
}

void FrameBox::paintEvent(QPaintEvent *e)
{
	uint width = this->width() * scale, height = floor(this->height() / 2);
	QPainter painter(this);

	/* draw time-slot separation */
	if (this->gridID % divisor == row)
	{
		painter.setPen(QPen(QBrush("#C3C3C3"), 3, Qt::DashDotDotLine));
		painter.drawLine(0, 0, 0, this->height());
	}

	/* draw timeline's grey line */
	painter.setPen(QColor("#C3C3C3"));
	painter.drawLine(0, height, width, height);

	if (tx_state != NULL)
	{
		uint h;
		if (tx_state->wstates > none)
		{
			if (tx_state->wstates == difs)
			{
				h = (height / 2);
				painter.setPen(QPen(QColor("#C8BFE7"), 5));
				painter.drawLine(0, h, width, h);
			}
			else if (tx_state->wstates == cts_timeout)
			{
				h = (height / 2);
				painter.setPen(QPen(QColor("#B97A57"), 5));
				painter.drawLine(0, h, width, h);
			}
			else if (tx_state->wstates == dat_timeout)
			{
				h = (height / 2);
				painter.setPen(QPen(QColor("#FFF200"), 5));
				painter.drawLine(0, h, width, h);
			}
			else if (tx_state->wstates == ack_timeout)
			{
				h = (height / 2);
				painter.setPen(QPen(QColor("#FF7F27"), 5));
				painter.drawLine(0, h, width, h);
			}

			if(tx_state->wstates == nav_rts)
			{
				h = (height / 2) + 4;
				painter.setPen(QPen(QColor("#3F48CC"), 5));
				painter.drawLine(0, h, width, h);
			}

			if (tx_state->wstates == nav_cts)
			{
				h = (height / 2) + 8;
				painter.setPen(QPen(QColor("#800040"), 5));
				painter.drawLine(0, h, width, h);
			}

			if (tx_state->wstates == backoff)
			{
				h = (height / 2);
				painter.setPen(QPen(QColor(Qt::black), 5));
				painter.drawLine(0, h, width, h);
			}
		}
		else
		{
			/* for transmit frames */
			QColor color = tx_state->mode != IDLE ? QColor(0, 255, 0, 180) : Qt::transparent;
			if (color != Qt::transparent)
			{
				auto &frame_source = tx_state->source;
				auto &frame_destination = tx_state->destination;
				auto &sequence_num = tx_state->sequence_count;
				QString ftype = tx_state->subvalue == Global::_RTS ? "R:" :
					(tx_state->subvalue == Global::_CTS ? "C:" : (tx_state->subvalue == Global::_DATA ? "D:" : "A:"));;

				QRect rect(0, 0, width, height);
				painter.setPen(color);
				painter.fillRect(rect, QBrush(color));
				painter.drawRect(rect);

				painter.setFont(QFont("Arial", font_size, QFont::Weight::Bold));
				painter.setPen(QPen(QColor(Qt::black)));
				painter.drawText(rect, Qt::AlignTop | Qt::AlignLeft, "S:" + QString::number(frame_source));
				painter.drawText(rect, Qt::AlignVCenter | Qt::AlignLeft, "D:" + QString::number(frame_destination));
				painter.drawText(rect, Qt::AlignBottom | Qt::AlignLeft, ftype + QString::number(sequence_num));
				timer_displayed = false;
			}
		}
	}
	/* for receive frames */
	if (rx_state == NULL) return;
	QColor color = rx_state->mode != IDLE ? QColor(185, 122, 87, 180) : Qt::transparent;
	if (color != Qt::transparent)
	{
		auto &frame_source = rx_state->source;
		auto &frame_destination = rx_state->destination;
		auto &sequence_num = rx_state->sequence_count;
		QString ftype = rx_state->subvalue == Global::_RTS ? "R:" :
			(rx_state->subvalue == Global::_CTS ? "C:" : (rx_state->subvalue == Global::_DATA ? "D:" : "A:"));;

		if (!rx_state->discarded)
			color = QColor(255, 0, 0, 180);
		QRect rect = QRect(0, height, width, height);
		painter.setPen(color);
		painter.fillRect(rect, QBrush(color));
		painter.drawRect(rect);

		painter.setFont(QFont("Arial", font_size, QFont::Weight::Bold));
		painter.setPen(QPen(QColor("#f0f0f0")));
		painter.drawText(rect, Qt::AlignTop | Qt::AlignLeft, "S:" + QString::number(frame_source));
		painter.drawText(rect, Qt::AlignVCenter | Qt::AlignLeft, "D:" + QString::number(frame_destination));
		painter.drawText(rect, Qt::AlignBottom | Qt::AlignLeft, ftype + QString::number(sequence_num));
	}
}
#endif