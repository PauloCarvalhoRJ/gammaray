#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName(APP_NAME);
    QApplication::setOrganizationDomain("geostats.gammaray.com");
    QApplication::setApplicationName(APP_NAME_VER);
    MainWindow w;
    w.show();

    return a.exec();
}
