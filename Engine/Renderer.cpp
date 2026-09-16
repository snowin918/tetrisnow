#include "Engine/Renderer.h"

#include <QImage>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <glm/gtc/matrix_transform.hpp>

#include "Engine/Camera.h"
#include "Engine/GLMQt.h"

namespace
{
// Unit quad in [0,1]x[0,1], interleaved position + UV. Per-quad placement
// and size are applied on the CPU as a model matrix uniform rather than
// re-uploading geometry, so one buffer serves every quad drawn.
constexpr float kQuadVertices[] = {
    // position    uv
    0.0f, 0.0f,   0.0f, 0.0f,
    1.0f, 0.0f,   1.0f, 0.0f,
    1.0f, 1.0f,   1.0f, 1.0f,

    0.0f, 0.0f,   0.0f, 0.0f,
    1.0f, 1.0f,   1.0f, 1.0f,
    0.0f, 1.0f,   0.0f, 1.0f,
};
} // namespace

Renderer::Renderer() = default;

Renderer::~Renderer()
{
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }
}

void Renderer::initialize()
{
    initializeOpenGLFunctions();

    m_quadShader = m_shaderManager.load(
        QStringLiteral("quad"), QStringLiteral(":/Shaders/quad.vert"), QStringLiteral(":/Shaders/quad.frag"));

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVertices), kQuadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

    glBindVertexArray(0);

    QImage whiteImage(1, 1, QImage::Format_RGBA8888);
    whiteImage.fill(Qt::white);
    m_whiteTexture = std::make_unique<QOpenGLTexture>(whiteImage);
    m_whiteTexture->setMinificationFilter(QOpenGLTexture::Nearest);
    m_whiteTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
}

void Renderer::beginFrame(const Camera& camera)
{
    m_quadShader->bind();
    m_quadShader->setUniformValue("uViewProj", toQMatrix4x4(camera.viewProjectionMatrix()));
    m_quadShader->setUniformValue("uTexture", 0);
    glBindVertexArray(m_vao);
}

void Renderer::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
    glActiveTexture(GL_TEXTURE0);
    m_whiteTexture->bind();
    drawQuadInternal(position, size, color);
}

void Renderer::drawQuad(const glm::vec2& position, const glm::vec2& size, QOpenGLTexture* texture, const glm::vec4& tint)
{
    if (texture == nullptr) {
        drawQuad(position, size, tint);
        return;
    }
    glActiveTexture(GL_TEXTURE0);
    texture->bind();
    drawQuadInternal(position, size, tint);
}

void Renderer::drawQuadInternal(const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint)
{
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

    m_quadShader->setUniformValue("uModel", toQMatrix4x4(model));
    m_quadShader->setUniformValue("uTint", QVector4D(tint.r, tint.g, tint.b, tint.a));

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::endFrame()
{
    glBindVertexArray(0);
    m_quadShader->release();
}
