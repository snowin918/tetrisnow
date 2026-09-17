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
    m_landscapeShader = m_shaderManager.load(
        "landscape", TETRISNOW_ASSETS_DIR "/Shaders/quad.vert", TETRISNOW_ASSETS_DIR "/Shaders/landscape.frag");
    m_locLandscapeModel = glGetUniformLocation(m_landscapeShader, "uModel");
    m_locLandscapeTint = glGetUniformLocation(m_landscapeShader, "uTint");
    m_locLandscapeViewProj = glGetUniformLocation(m_landscapeShader, "uViewProj");
    glUseProgram(m_landscapeShader);
    glUniform2f(glGetUniformLocation(m_landscapeShader, "uUvOffset"), 0.0f, 0.0f);
    glUniform2f(glGetUniformLocation(m_landscapeShader, "uUvScale"), 1.0f, 1.0f);
    m_icePanelShader = m_shaderManager.load(
        "icepanel", TETRISNOW_ASSETS_DIR "/Shaders/quad.vert", TETRISNOW_ASSETS_DIR "/Shaders/icepanel.frag");
    m_locIcePanelModel = glGetUniformLocation(m_icePanelShader, "uModel");
    m_locIcePanelTint = glGetUniformLocation(m_icePanelShader, "uTint");
    m_locIcePanelViewProj = glGetUniformLocation(m_icePanelShader, "uViewProj");
    glUseProgram(m_icePanelShader);
    glUniform2f(glGetUniformLocation(m_icePanelShader, "uUvOffset"), 0.0f, 0.0f);
    glUniform2f(glGetUniformLocation(m_icePanelShader, "uUvScale"), 1.0f, 1.0f);
    m_quadShader = m_shaderManager.load(
        "quad", TETRISNOW_ASSETS_DIR "/Shaders/quad.vert", TETRISNOW_ASSETS_DIR "/Shaders/quad.frag");

    m_locViewProj = glGetUniformLocation(m_quadShader, "uViewProj");
    m_locModel = glGetUniformLocation(m_quadShader, "uModel");
    m_locTint = glGetUniformLocation(m_quadShader, "uTint");
    m_locTexture = glGetUniformLocation(m_quadShader, "uTexture");
    m_locUvOffset = glGetUniformLocation(m_quadShader, "uUvOffset");
    m_locUvScale = glGetUniformLocation(m_quadShader, "uUvScale");

    // Shares quad.vert with the plain quad shader (same vertex layout), but
    // links against block.frag for the faceted ice-cube look.
    m_blockShader = m_shaderManager.load(
        "block", TETRISNOW_ASSETS_DIR "/Shaders/quad.vert", TETRISNOW_ASSETS_DIR "/Shaders/block.frag");

    m_locBlockViewProj = glGetUniformLocation(m_blockShader, "uViewProj");
    m_locBlockModel = glGetUniformLocation(m_blockShader, "uModel");
    m_locBlockTint = glGetUniformLocation(m_blockShader, "uTint");

    // The block shader never samples a texture atlas, so its inherited
    // uUvOffset/uUvScale uniforms (from quad.vert) just need setting once
    // to the identity — otherwise they'd default to (0,0), collapsing vUV
    // to a single point.
    glUseProgram(m_blockShader);
    glUniform2f(glGetUniformLocation(m_blockShader, "uUvOffset"), 0.0f, 0.0f);
    glUniform2f(glGetUniformLocation(m_blockShader, "uUvScale"), 1.0f, 1.0f);

    // Shares quad.vert too; links against softcircle.frag for the soft
    // round/glowing look used by particles and attack projectiles.
    m_softCircleShader = m_shaderManager.load(
        "softcircle", TETRISNOW_ASSETS_DIR "/Shaders/quad.vert", TETRISNOW_ASSETS_DIR "/Shaders/softcircle.frag");

    m_locSoftCircleViewProj = glGetUniformLocation(m_softCircleShader, "uViewProj");
    m_locSoftCircleModel = glGetUniformLocation(m_softCircleShader, "uModel");
    m_locSoftCircleTint = glGetUniformLocation(m_softCircleShader, "uTint");

    glUseProgram(m_softCircleShader);
    glUniform2f(glGetUniformLocation(m_softCircleShader, "uUvOffset"), 0.0f, 0.0f);
    glUniform2f(glGetUniformLocation(m_softCircleShader, "uUvScale"), 1.0f, 1.0f);

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
    m_viewProj = camera.viewProjectionMatrix();
    m_currentProgram = 0; // force the first draw call this frame to (re-)bind and upload uViewProj
    glBindVertexArray(m_vao);
}

void Renderer::useProgram(GLuint program)
{
    if (program == m_currentProgram) {
        return;
    }
    m_currentProgram = program;
    glUseProgram(program);

    if (program == m_landscapeShader) {
        glUniformMatrix4fv(m_locLandscapeViewProj, 1, GL_FALSE, glm::value_ptr(m_viewProj));
    } else if (program == m_icePanelShader) {
        glUniformMatrix4fv(m_locIcePanelViewProj, 1, GL_FALSE, glm::value_ptr(m_viewProj));
    } else if (program == m_quadShader) {
        glUniformMatrix4fv(m_locViewProj, 1, GL_FALSE, glm::value_ptr(m_viewProj));
        glUniform1i(m_locTexture, 0);
    } else if (program == m_blockShader) {
        glUniformMatrix4fv(m_locBlockViewProj, 1, GL_FALSE, glm::value_ptr(m_viewProj));
    } else if (program == m_softCircleShader) {
        glUniformMatrix4fv(m_locSoftCircleViewProj, 1, GL_FALSE, glm::value_ptr(m_viewProj));
    }
}

void Renderer::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
    useProgram(m_quadShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
    glUniform2f(m_locUvOffset, 0.0f, 0.0f);
    glUniform2f(m_locUvScale, 1.0f, 1.0f);
    drawQuadInternal(position, size, color);
}

void Renderer::drawQuad(const glm::vec2& position, const glm::vec2& size, GLuint texture, const glm::vec4& tint)
{
    useProgram(m_quadShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture != 0 ? texture : m_whiteTexture);
    glUniform2f(m_locUvOffset, 0.0f, 0.0f);
    glUniform2f(m_locUvScale, 1.0f, 1.0f);
    drawQuadInternal(position, size, tint);
}

void Renderer::drawQuad(
    const glm::vec2& position,
    const glm::vec2& size,
    GLuint texture,
    const glm::vec2& uvOffset,
    const glm::vec2& uvScale,
    const glm::vec4& tint, float rotationRadians)
{
    useProgram(m_quadShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture != 0 ? texture : m_whiteTexture);
    glUniform2f(m_locUvOffset, uvOffset.x, uvOffset.y);
    glUniform2f(m_locUvScale, uvScale.x, uvScale.y);
    drawRotatedInternal(m_locModel, m_locTint, position, size, tint, rotationRadians);
}

void Renderer::drawBlock(const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint, float rotationRadians)
{
    useProgram(m_blockShader);
    drawRotatedInternal(m_locBlockModel, m_locBlockTint, position, size, tint, rotationRadians);
}

void Renderer::drawSoftCircle(
    const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint, float rotationRadians)
{
    useProgram(m_softCircleShader);
    drawRotatedInternal(m_locSoftCircleModel, m_locSoftCircleTint, position, size, tint, rotationRadians);
}

void Renderer::drawRotatedInternal(
    GLint modelLoc, GLint tintLoc, const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint,
    float rotationRadians)
{
    glm::mat4 model;
    if (rotationRadians == 0.0f) {
        model = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
            * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
    } else {
        // Rotate around the quad's visual center rather than its corner:
        // move the center to the origin, rotate, then translate/scale from
        // there — position+size*0.5 is that center in world space.
        const glm::vec2 center = position + size * 0.5f;
        model = glm::translate(glm::mat4(1.0f), glm::vec3(center, 0.0f))
            * glm::rotate(glm::mat4(1.0f), rotationRadians, glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::translate(glm::mat4(1.0f), glm::vec3(-size * 0.5f, 0.0f))
            * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
    }

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(tintLoc, 1, glm::value_ptr(tint));

    glDrawArrays(GL_TRIANGLES, 0, 6);
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
    m_currentProgram = 0;
}

void Renderer::drawWinterLandscape(const glm::vec2& position, const glm::vec2& size)
{
    useProgram(m_landscapeShader);
    drawRotatedInternal(m_locLandscapeModel, m_locLandscapeTint, position, size, glm::vec4(1.0f), 0.0f);
}

void Renderer::drawIcePanel(const glm::vec2& position, const glm::vec2& size)
{
    useProgram(m_icePanelShader);
    drawRotatedInternal(m_locIcePanelModel, m_locIcePanelTint, position, size, glm::vec4(1.0f), 0.0f);
}
