#include "../inc/Program.h"
#ifdef SHOWGUI
#include "../inc/GUI.h"
#endif

int main(int argc, char *argv[])
{
	Program program;
	auto gui_helper = program.getgui();
#ifdef SHOWGUI
	QApplication GUI_Application(argc, argv);
	Window window(gui_helper);
	return GUI_Application.exec();
#endif // SHOWGUI
	return 0;
}
