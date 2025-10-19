#include "mainwindow.h"

#include <QApplication>
#include <QDebug>

// int main(int argc, char *argv[])
// {
//     QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

//     qDebug() << "Program started";
//     QApplication a(argc, argv);
//     qDebug() << "QApplication created";
//     MainWindow w;
//     qDebug() << "MainWindow created";
//     w.show();
//     qDebug() << "MainWindow shown";
//     return a.exec();
// }
// ---------- Windows ----------
#ifdef Q_OS_WIN
#include <windows.h>
static int currCpu() {
    return static_cast<int>(GetCurrentProcessorNumber());
}
#endif

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    qDebug() << "Program started";

    // 打印启动瞬间所在核
    qDebug() << "Main thread（启动时）on CPU" << currCpu();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
