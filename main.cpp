#include <QApplication>
#include <QSurfaceFormat>

#include "UI/MainWindow.h"

int main(int argc, char* argv[])
{
    // Request an OpenGL 3.3 core-profile context before any widget creates
    // one. Must happen before QApplication is constructed.
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 3);
    format.setSamples(4); // MSAA — smooths block edges.
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

    MainWindow window;
    window.resize(1280, 720);
    window.show();

    return app.exec();
}
