#pragma once

#include <string>

#include <glm/glm.hpp>

#include "Engine/OpenGLLoader.h"
#include "Engine/ShaderManager.h"

// Independent subsystem that renders a player's uploaded photo as a fixed
// square portrait overlay (see the project brief's layered diagram: this
// sits alongside, not inside, the Character Sprite System). Phase 1 only
// proves photo -> GL texture -> on-screen quad; no landmarks, mesh
// deformation, or emotion logic live here yet (later phases extend the
// same shader/quad rather than replacing it).
//
// Deliberately owns its own tiny GL pipeline (shader + quad + texture)
// instead of going through Engine/Renderer: that renderer's projection is
// locked to the world Camera for the board/character scene, while a face
// portrait is a screen-space (pixel-positioned) overlay, and Phase 2 will
// need custom vertex-shader uniforms a shared shader can't carry alone.
class FaceAvatarSystem
{
public:
    ~FaceAvatarSystem();

    // Compiles the shader and builds the quad geometry. Must be called
    // once with a current OpenGL context, after loadOpenGLFunctions().
    void initialize();

    // Loads an image file (PNG/JPG/...) from disk and uploads it as the
    // portrait texture, replacing any previously loaded image. Non-square
    // images are center-cropped (via UV offset/scale, not pixel
    // resampling) so the portrait always renders as a square. Returns
    // false and leaves the current texture untouched if the file couldn't
    // be read.
    bool loadPlayerImage(const std::string& path);

    bool hasImage() const { return m_texture != 0; }

    // Where and how large to draw the portrait, in framebuffer pixels
    // with a top-left origin. Defaults to a small box in the corner.
    void setScreenRect(glm::vec2 topLeftPx, float sizePx);

    // Draws the portrait quad. Safe to call every frame regardless of
    // whether an image has loaded yet (draws nothing until it has).
    // viewportWidthPx/viewportHeightPx must be the current framebuffer
    // size, since the portrait is positioned in screen space rather than
    // through the world Camera.
    void render(int viewportWidthPx, int viewportHeightPx) const;

private:
    ShaderManager m_shaderManager;
    GLuint m_program = 0;
    GLint m_locProjection = -1;
    GLint m_locModel = -1;
    GLint m_locUvOffset = -1;
    GLint m_locUvScale = -1;
    GLint m_locTexture = -1;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;

    GLuint m_texture = 0;
    glm::vec2 m_uvOffset{0.0f, 0.0f};
    glm::vec2 m_uvScale{1.0f, 1.0f};

    glm::vec2 m_screenTopLeftPx{24.0f, 24.0f};
    float m_screenSizePx = 160.0f;
};
