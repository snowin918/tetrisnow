#include "Engine/GameWindow.h"

namespace
{
constexpr int kTargetFps = 60;
constexpr int kTickIntervalMs = 1000 / kTargetFps;

// Standard Tetris board dimensions, used here only to size the Milestone 2
// test scene's camera and grid. Game::Board (Milestone 3) owns these for
// real.
constexpr int kBoardWidthCells = 10;
constexpr int kBoardHeightCells = 20;
} // namespace

GameWindow::GameWindow(QWidget* parent)
    : QOpenGLWidget(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &GameWindow::onTick);
}

GameWindow::~GameWindow()
{
    // m_renderer and m_textureManager are destroyed right after this body
    // runs (in reverse declaration order), before QOpenGLWidget's own
    // destructor tears the context down. Making the context current here
    // keeps it current for those member destructors, so their glDelete*
    // calls are valid.
    makeCurrent();
}

void GameWindow::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.05f, 0.08f, 0.12f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_renderer.initialize();
    m_testTexture = m_textureManager.createCheckerboard(QStringLiteral("testCheckerboard"), 64, 8);

    // Frame the placeholder board with a little margin above/below.
    m_camera.setWorldHeight(static_cast<float>(kBoardHeightCells) + 4.0f);
    m_camera.setPosition(glm::vec2(kBoardWidthCells / 2.0f, kBoardHeightCells / 2.0f));

    m_clock.start();
    m_lastElapsedNs = m_clock.nsecsElapsed();
    m_timer.start(kTickIntervalMs);
}

void GameWindow::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    m_camera.setViewportSize(width, height);
}

void GameWindow::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    drawTestScene();
}

void GameWindow::drawTestScene()
{
    m_renderer.beginFrame(m_camera);

    // Placeholder board grid — proves the camera/coordinate system lines up
    // with board-cell space ahead of Game::Board existing.
    for (int row = 0; row < kBoardHeightCells; ++row) {
        for (int col = 0; col < kBoardWidthCells; ++col) {
            const bool alt = (row + col) % 2 == 0;
            const glm::vec4 cellColor = alt ? glm::vec4(0.15f, 0.17f, 0.22f, 1.0f)
                                             : glm::vec4(0.12f, 0.14f, 0.18f, 1.0f);
            m_renderer.drawQuad(
                glm::vec2(static_cast<float>(col), static_cast<float>(row)), glm::vec2(0.95f, 0.95f), cellColor);
        }
    }

    // A 2x2 block of solid-color quads, standing in for a tetromino.
    const glm::vec4 blockColor(0.9f, 0.2f, 0.2f, 1.0f);
    m_renderer.drawQuad(glm::vec2(3.0f, 2.0f), glm::vec2(1.0f), blockColor);
    m_renderer.drawQuad(glm::vec2(4.0f, 2.0f), glm::vec2(1.0f), blockColor);
    m_renderer.drawQuad(glm::vec2(3.0f, 3.0f), glm::vec2(1.0f), blockColor);
    m_renderer.drawQuad(glm::vec2(4.0f, 3.0f), glm::vec2(1.0f), blockColor);

    // Textured quad — proves the texture-sampling draw path.
    m_renderer.drawQuad(glm::vec2(6.0f, 15.0f), glm::vec2(3.0f), m_testTexture);

    m_renderer.endFrame();
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
