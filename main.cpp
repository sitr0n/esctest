#include "mainwindow.h"
#include <QApplication>

#include "palm.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowIcon(QIcon(":/icons/wingslot_icon.png"));
    w.show();

    return a.exec();
}
