#pragma once

#include <glm/glm.hpp>

#include "Engine/OpenGLLoader.h"
#include "Engine/ShaderManager.h"

class Camera;

// Minimal 2D quad renderer. Every game object in Tetrisnow — board cells,
// tetromino blocks, snow particles, UI panels — is ultimately a rectangle,
// so rather than building a general mesh system, the Renderer exposes one
// drawQuad() call backed by a single shared unit-quad and shader program.
// Solid-color and textured quads share the same shader: solid color draws
// bind a 1x1 white texture, so "sample * tint" reduces to the flat color.
class Renderer
{
public:
    Renderer();
    ~Renderer();

    // Must be called once, with a current OpenGL context and after
    // loadOpenGLFunctions() has succeeded.
    void initialize();

    void beginFrame(const Camera& camera);
    void drawWinterLandscape(const glm::vec2& position, const glm::vec2& size);
    void drawIcePanel(const glm::vec2& position, const glm::vec2& size);
    void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    void drawQuad(const glm::vec2& position, const glm::vec2& size, GLuint texture, const glm::vec4& tint = glm::vec4(1.0f));
    void drawQuad(
        const glm::vec2& position,
        const glm::vec2& size,
        GLuint texture,
        const glm::vec2& uvOffset,
        const glm::vec2& uvScale,
        const glm::vec4& tint = glm::vec4(1.0f), float rotationRadians = 0.0f);

    // Draws a filled Tetris cell (locked stack or the active piece) as a
    // faceted ice cube via Assets/Shaders/block.frag, instead of a flat
    // color square — see that file for the look. Uses its own shader
    // program (switched to on demand, see useProgram()), so it can be
    // freely interleaved with drawQuad() calls in the same frame.
    // rotationRadians spins the quad around its own visual center (not its
    // corner) — 0 for every existing board-cell use, nonzero for a flying
    // attack projectile that should tumble as it travels.
    void drawBlock(
        const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint, float rotationRadians = 0.0f);

    // Soft snow, mist and projectile bodies; solid ice shards use drawBlock().
    // Same rotation convention as drawBlock().
    void drawSoftCircle(
        const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint, float rotationRadians = 0.0f);

    void endFrame();

private:
    void drawQuadInternal(const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint);
    // Shared by drawBlock()/drawSoftCircle() — both just switch which
    // shader program is active first, then use this to build the model
    // matrix and issue the draw call.
    void drawRotatedInternal(
        GLint modelLoc, GLint tintLoc, const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint,
        float rotationRadians);
    void useProgram(GLuint program);

    ShaderManager m_shaderManager;
    GLuint m_quadShader = 0;
    GLuint m_blockShader = 0;
    GLuint m_landscapeShader = 0;
    GLint m_locLandscapeModel = -1;
    GLint m_locLandscapeTint = -1;
    GLint m_locLandscapeViewProj = -1;
    GLuint m_icePanelShader = 0;
    GLint m_locIcePanelModel = -1;
    GLint m_locIcePanelTint = -1;
    GLint m_locIcePanelViewProj = -1;
    GLuint m_softCircleShader = 0;

    // Cached once after linking, rather than re-queried every draw call.
    GLint m_locViewProj = -1;
    GLint m_locModel = -1;
    GLint m_locTint = -1;
    GLint m_locTexture = -1;
    GLint m_locUvOffset = -1;
    GLint m_locUvScale = -1;

    GLint m_locBlockViewProj = -1;
    GLint m_locBlockModel = -1;
    GLint m_locBlockTint = -1;

    GLint m_locSoftCircleViewProj = -1;
    GLint m_locSoftCircleModel = -1;
    GLint m_locSoftCircleTint = -1;

    // Cached each beginFrame() so useProgram() can re-upload it to
    // whichever shader becomes active next, without needing the Camera
    // reference again outside beginFrame().
    glm::mat4 m_viewProj{1.0f};
    GLuint m_currentProgram = 0;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_whiteTexture = 0;
};
