#pragma once

#include <memory>
#include <string>

#include <glm/glm.hpp>

#include "Engine/OpenGLLoader.h"
#include "Engine/ShaderManager.h"
#include "FaceAvatar/FaceLandmarkDetector.h"

// Independent subsystem that renders a player's uploaded photo as a
// square portrait overlay (see the project brief's layered diagram: this
// sits alongside, not inside, the Character Sprite System). As of Phase 2
// it detects facial landmarks on load (FaceAvatar/FaceLandmarkDetector)
// and builds a deformable grid mesh from them (FaceAvatar/FaceMesh), so
// the portrait can be warped live via setEyeScale()/setMouthOpen()/
// setBrowPosition()/setFaceRotation() — Phase 3's FaceExpressionController
// will be what actually drives those from gameplay emotions. If no face
// is detected, it falls back to a flat, non-deformable grid (Phase 1's
// static-portrait behavior) rather than failing.
//
// Deliberately owns its own tiny GL pipeline (shader + mesh + texture)
// instead of going through Engine/Renderer: that renderer's projection is
// locked to the world Camera for the board/character scene, while a face
// portrait is a screen-space (pixel-positioned) overlay with its own
// vertex format and deformation uniforms Engine/Renderer's shared shader
// has no reason to carry.
class FaceAvatarSystem
{
public:
    FaceAvatarSystem();
    ~FaceAvatarSystem();

    // Compiles the shader and prepares GPU buffers. Must be called once
    // with a current OpenGL context, after loadOpenGLFunctions().
    // modelPath: path to dlib's shape_predictor_68_face_landmarks.dat.
    void initialize(std::string modelPath);

    // Loads an image file (PNG/JPG/...) from disk, uploads it as the
    // portrait texture, runs face-landmark detection on it, and rebuilds
    // the deformable mesh accordingly. Replaces any previously loaded
    // image/mesh. Returns false and leaves the current texture/mesh
    // untouched if the file couldn't be read.
    bool loadPlayerImage(const std::string& path);

    bool hasImage() const { return m_texture != 0; }
    bool hasFaceLandmarks() const { return m_hasFaceLandmarks; }

    // Where and how large to draw the portrait, in framebuffer pixels
    // with a top-left origin. Defaults to a small box in the corner.
    void setScreenRect(glm::vec2 topLeftPx, float sizePx);

    // Deformation controls — see Assets/Shaders/faceMesh.vert for exactly
    // how each one moves the mesh. Neutral pose is (1, 0, 0, 0).
    void setEyeScale(float scale) { m_eyeScale = scale; }
    void setMouthOpen(float amount) { m_mouthOpen = amount; }
    void setBrowPosition(float amount) { m_browPosition = amount; }
    void setFaceRotation(float radians) { m_faceRotation = radians; }

    // Shader-level look, driven by FaceExpressionController (Phase 3) —
    // see Assets/Shaders/faceMesh.frag. Neutral is tint (1,1,1), contrast 1.
    void setTint(glm::vec3 color) { m_tint = color; }
    void setContrast(float contrast) { m_contrast = contrast; }

    // Draws the portrait mesh. Safe to call every frame regardless of
    // whether an image has loaded yet (draws nothing until it has).
    // viewportWidthPx/viewportHeightPx must be the current framebuffer
    // size, since the portrait is positioned in screen space rather than
    // through the world Camera.
    void render(int viewportWidthPx, int viewportHeightPx) const;

private:
    void uploadMesh(const class FaceMesh& mesh);

    ShaderManager m_shaderManager;
    GLuint m_program = 0;
    GLint m_locProjection = -1;
    GLint m_locModel = -1;
    GLint m_locTexture = -1;
    GLint m_locEyeScale = -1;
    GLint m_locMouthOpen = -1;
    GLint m_locBrowPosition = -1;
    GLint m_locFaceRotation = -1;
    GLint m_locTintColor = -1;
    GLint m_locContrast = -1;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    int m_indexCount = 0;

    GLuint m_texture = 0;
    bool m_hasFaceLandmarks = false;

    // Constructed in initialize() once the model path is known.
    std::unique_ptr<FaceLandmarkDetector> m_landmarkDetector;

    float m_eyeScale = 1.0f;
    float m_mouthOpen = 0.0f;
    float m_browPosition = 0.0f;
    float m_faceRotation = 0.0f;
    glm::vec3 m_tint{1.0f};
    float m_contrast = 1.0f;

    glm::vec2 m_screenTopLeftPx{24.0f, 24.0f};
    float m_screenSizePx = 160.0f;
};
