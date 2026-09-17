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
    void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    void drawQuad(const glm::vec2& position, const glm::vec2& size, GLuint texture, const glm::vec4& tint = glm::vec4(1.0f));
    void drawQuad(
        const glm::vec2& position,
        const glm::vec2& size,
        GLuint texture,
        const glm::vec2& uvOffset,
        const glm::vec2& uvScale,
        const glm::vec4& tint = glm::vec4(1.0f));
    void endFrame();

private:
    void drawQuadInternal(const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint);

    ShaderManager m_shaderManager;
    GLuint m_quadShader = 0;

    // Cached once after linking, rather than re-queried every draw call.
    GLint m_locViewProj = -1;
    GLint m_locModel = -1;
    GLint m_locTint = -1;
    GLint m_locTexture = -1;
    GLint m_locUvOffset = -1;
    GLint m_locUvScale = -1;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_whiteTexture = 0;
};
