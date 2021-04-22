#pragma once
#include <QtWidgets>



class Line : public QWidget
{
	Q_OBJECT
private:
	QWidget *lineWidget;
	//QPen pen;

public:
	Line(QWidget *parent = 0) : QWidget(parent)//, pen(inpen)
	{
		//lineWidget = new QWidget;
	}

	//void paintEvent(QPaintEvent *e)
	void mouseMoveEvent(QMouseEvent *event)
	{
		auto xcords = event->globalX();

		QPainter painter(this);
		uint height = this->height();
		painter.setPen(QColor("#C3C3C3"));
		painter.drawLine(xcords, 0, xcords, height);
		//painter.end();
	}
};
//
//class Cursor : public QWidget
//{
//	Q_OBJECT
//private:
//	QWidget *lineWidget;
//	QPen pen;
//
//public:
//	Cursor(QPen inpen, QWidget *parent = 0) : QWidget(parent), pen(inpen)
//	{
//		lineWidget = new QWidget;
//	}
//
//	void mouseMoveEvent(QMouseEvent *event)
//	{
//		GLfloat dx = GLfloat(event->x() - lastPos.x()) / width();
//		GLfloat dy = GLfloat(event->y() - lastPos.y()) / height();
//
//
//			GLfloat dx = GLfloat(event->x() - lastPos.x()) / width();
//			GLfloat dy = GLfloat(event->y() - lastPos.y()) / height();
//
//
//
//
//		QPainter painter(this);
//		//painter.begin(lineWidget);
//		painter.setPen(pen);
//		painter.drawLine(5, 0, 5, 10);
//		//painter.end();
//	}
//};