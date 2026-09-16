#include "Engine/GameWindow.h"

namespace
{
constexpr int kTargetFps = 60;
constexpr int kTickIntervalMs = 1000 / kTargetFps;
} // namespace

GameWindow::GameWindow(QWidget* parent)
    : QOpenGLWidget(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &GameWindow::onTick);
}

void GameWindow::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.05f, 0.08f, 0.12f, 1.0f);

    m_clock.start();
    m_lastElapsedNs = m_clock.nsecsElapsed();
    m_timer.start(kTickIntervalMs);
}

void GameWindow::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void GameWindow::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    // Milestone 2 introduces Engine::Renderer, which will draw here.
}

void GameWindow::onTick()
{
    const qint64 nowNs = m_clock.nsecsElapsed();
    const float deltaTime = static_cast<float>(nowNs - m_lastElapsedNs) / 1e9f;
    m_lastElapsedNs = nowNs;

    tick(deltaTime);
    update(); // QOpenGLWidget::update() — schedules a repaint (calls paintGL).
}

void GameWindow::tick(float deltaTime)
{
    Q_UNUSED(deltaTime);
    // Hook for Game::GameManager in a later milestone.
}
