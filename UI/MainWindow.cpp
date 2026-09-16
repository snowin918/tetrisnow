#include "UI/MainWindow.h"

#include "Engine/GameWindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_gameWindow(new GameWindow(this))
{
    setWindowTitle(tr("Tetrisnow"));
    setCentralWidget(m_gameWindow);
}
