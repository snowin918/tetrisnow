#include "Engine/Renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Engine/Camera.h"

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

constexpr unsigned char kWhitePixel[4] = {255, 255, 255, 255};
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
    if (m_whiteTexture != 0) {
        glDeleteTextures(1, &m_whiteTexture);
    }
}

void Renderer::initialize()
{
    m_quadShader = m_shaderManager.load(
        "quad", TETRISNOW_ASSETS_DIR "/Shaders/quad.vert", TETRISNOW_ASSETS_DIR "/Shaders/quad.frag");

    m_locViewProj = glGetUniformLocation(m_quadShader, "uViewProj");
    m_locModel = glGetUniformLocation(m_quadShader, "uModel");
    m_locTint = glGetUniformLocation(m_quadShader, "uTint");
    m_locTexture = glGetUniformLocation(m_quadShader, "uTexture");

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

    glGenTextures(1, &m_whiteTexture);
    glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, kWhitePixel);
}

void Renderer::beginFrame(const Camera& camera)
{
    glUseProgram(m_quadShader);
    glUniformMatrix4fv(m_locViewProj, 1, GL_FALSE, glm::value_ptr(camera.viewProjectionMatrix()));
    glUniform1i(m_locTexture, 0);
    glBindVertexArray(m_vao);
}

void Renderer::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
    drawQuadInternal(position, size, color);
}

void Renderer::drawQuad(const glm::vec2& position, const glm::vec2& size, GLuint texture, const glm::vec4& tint)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture != 0 ? texture : m_whiteTexture);
    drawQuadInternal(position, size, tint);
}

void Renderer::drawQuadInternal(const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint)
{
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

    glUniformMatrix4fv(m_locModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(m_locTint, 1, glm::value_ptr(tint));

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::endFrame()
{
    glBindVertexArray(0);
    glUseProgram(0);
}
