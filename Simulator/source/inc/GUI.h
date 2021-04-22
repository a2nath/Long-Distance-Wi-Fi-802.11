#pragma once
#include "common.h"
#ifdef SHOWGUI
#include <QtWidgets>
#include "../inc/qpainter.h"
#include "../inc/Lines.h"
#include "../inc/Scroller.h"
#include "../inc/FrameBox.h"
#include "../inc/LabelBox.h"
#include "../inc/Grid.h"

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