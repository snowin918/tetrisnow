#pragma once

#include <QMainWindow>

class GameWindow;

// Top-level application window. Owns the OpenGL game surface and will later
// host the menu bar, lobby screen, and HUD overlays (Milestone 7).
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    GameWindow* m_gameWindow;
};
