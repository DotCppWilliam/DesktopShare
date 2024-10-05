#include "desktop_share.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    DesktopShare w;
	w.show();

    return a.exec();
}
