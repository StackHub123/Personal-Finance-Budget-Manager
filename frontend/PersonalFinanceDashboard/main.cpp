#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Personal Finance & Budgeting Dashboard");
    QApplication::setOrganizationName("PFBM");

    MainWindow window;
    window.show();

    return app.exec();
}
