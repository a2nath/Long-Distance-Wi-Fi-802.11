#include "../inc/Program.h"

#ifdef SHOWGUI
#include "../inc/GUI.h"
#endif

int main(int argc, char *argv[])
{

	chdir("../");

	unordered_map<string, string> arglist;

	for (int i = 0; i < argc; ++i)
	{
		string arg = string(argv[i]);
		int idx = arg.find(":");
		arglist.emplace(arg.substr(0, idx), arg.substr(idx + 1));
	}

	Program program(arglist);
#ifdef SHOWGUI
	auto gui_helper = program.getgui();
	QApplication GUI_Application(argc, argv);
	Window window(gui_helper);
	return GUI_Application.exec();
#endif // SHOWGUI
	return 0;
}
