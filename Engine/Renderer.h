#pragma once

#include <QOpenGLFunctions_3_3_Core>
#include <glm/glm.hpp>
#include <memory>

#include "Engine/ShaderManager.h"

class Camera;
class QOpenGLShaderProgram;
class QOpenGLTexture;

// Minimal 2D quad renderer. Every game object in Tetrisnow — board cells,
// tetromino blocks, snow particles, UI panels — is ultimately a rectangle,
// so rather than building a general mesh system, the Renderer exposes one
// drawQuad() call backed by a single shared unit-quad and shader program.
// Solid-color and textured quads share the same shader: solid color draws
// bind a 1x1 white texture, so "sample * tint" reduces to the flat color.
class Renderer : protected QOpenGLFunctions_3_3_Core
{
public:
    Renderer();
    ~Renderer();

    // Must be called once, with a current OpenGL context (from
    // GameWindow::initializeGL).
    void initialize();

    void beginFrame(const Camera& camera);
    void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    void drawQuad(const glm::vec2& position, const glm::vec2& size, QOpenGLTexture* texture,
                  const glm::vec4& tint = glm::vec4(1.0f));
    void endFrame();

private:
    void drawQuadInternal(const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint);

    ShaderManager m_shaderManager;
    QOpenGLShaderProgram* m_quadShader = nullptr;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;

    std::unique_ptr<QOpenGLTexture> m_whiteTexture;
};
