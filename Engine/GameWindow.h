#pragma once

#include <QElapsedTimer>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QTimer>

// The OpenGL rendering surface and owner of the fixed-rate game loop.
//
// Qt already provides an event loop (QApplication::exec), so rather than
// spinning our own while-loop, we drive gameplay ticks off a QTimer at a
// fixed interval and let paintGL() handle rendering. This keeps the loop
// simple while still giving Game::GameManager (Milestone 3) a clean,
// regular tick() to hook into.
class GameWindow : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit GameWindow(QWidget* parent = nullptr);

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

private slots:
    void onTick();

private:
    // Advances game state by deltaTime (seconds). Currently a placeholder;
    // will delegate to Game::GameManager once gameplay logic exists.
    void tick(float deltaTime);

    QTimer m_timer;
    QElapsedTimer m_clock;
    qint64 m_lastElapsedNs = 0;
};
