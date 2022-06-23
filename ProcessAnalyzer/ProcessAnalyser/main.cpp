#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <QApplication>
#include <QMessageBox>

#include "mainwindow.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    int rv;
    try {
        rv = a.exec();
    }  catch (const char* msg) {
        QMessageBox::critical(nullptr, "Handled exception", msg);
        return -1;
    }

    return rv;
}
