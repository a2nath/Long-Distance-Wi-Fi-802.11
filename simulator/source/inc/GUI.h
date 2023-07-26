#pragma once
#include "common.h"
#ifdef SHOWGUI
#include <QtWidgets>
#include "qpainter.h"
#include "Lines.h"
#include "Scroller.h"
#include "FrameBox.h"
#include "LabelBox.h"
#include "Grid.h"

#define WIDTH 1920
#define HEIGHT 600

class Window : public QWidget
{
	Q_OBJECT
private:

	QMainWindow *GUI_WINDOW;
	QWidget *mainWidget;
	QScrollArea *scrollArea;
	Grid *gridEPG;

public:
	Window(const gcelltable &guihelper);
	~Window()
	{
		delete GUI_WINDOW, mainWidget, scrollArea, gridEPG;
	}
};
#endif