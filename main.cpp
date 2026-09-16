#include <QApplication>

#include "UI/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.resize(1280, 720);
    window.show();

    return app.exec();
}
