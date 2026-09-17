#include "FaceAvatar/FaceAvatarSystem.h"

#include <cstddef>
#include <cstdio>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "FaceAvatar/FaceMesh.h"

FaceAvatarSystem::FaceAvatarSystem() = default;

FaceAvatarSystem::~FaceAvatarSystem()
{
    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }
    if (m_texture != 0) {
        glDeleteTextures(1, &m_texture);
    }
}

void FaceAvatarSystem::initialize(std::string modelPath)
{
    m_landmarkDetector = std::make_unique<FaceLandmarkDetector>(std::move(modelPath));

    m_program = m_shaderManager.load(
        "faceMesh", TETRISNOW_ASSETS_DIR "/Shaders/faceMesh.vert", TETRISNOW_ASSETS_DIR "/Shaders/faceMesh.frag");

    m_locProjection = glGetUniformLocation(m_program, "uProjection");
    m_locModel = glGetUniformLocation(m_program, "uModel");
    m_locTexture = glGetUniformLocation(m_program, "uTexture");
    m_locEyeScale = glGetUniformLocation(m_program, "uEyeScale");
    m_locMouthOpen = glGetUniformLocation(m_program, "uMouthOpen");
    m_locBrowPosition = glGetUniformLocation(m_program, "uBrowPosition");
    m_locFaceRotation = glGetUniformLocation(m_program, "uFaceRotation");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
}

bool FaceAvatarSystem::loadPlayerImage(const std::string& path)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        std::fprintf(stderr, "FaceAvatarSystem: failed to load '%s'\n", path.c_str());
        return false;
    }

    const FaceLandmarks landmarks = m_landmarkDetector->detect(pixels, width, height);
    m_hasFaceLandmarks = landmarks.valid;
    if (m_hasFaceLandmarks) {
        std::fprintf(
            stderr,
            "FaceAvatarSystem: detected face landmarks in '%s' (bbox %.0f,%.0f - %.0f,%.0f)\n",
            path.c_str(),
            landmarks.faceBoundsMin.x,
            landmarks.faceBoundsMin.y,
            landmarks.faceBoundsMax.x,
            landmarks.faceBoundsMax.y);
    } else {
        std::fprintf(
            stderr, "FaceAvatarSystem: no face detected in '%s'; using a static (non-deformable) crop\n", path.c_str());
    }
    uploadMesh(FaceMesh::build(landmarks, width, height));

    if (m_texture != 0) {
        glDeleteTextures(1, &m_texture);
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    // Unlike Engine/TextureManager's GL_NEAREST (chosen for pixel-art
    // sprites), a photo needs smooth minification/magnification.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    stbi_image_free(pixels);
    return true;
}

void FaceAvatarSystem::uploadMesh(const FaceMesh& mesh)
{
    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(mesh.vertices().size() * sizeof(FaceMeshVertex)),
        mesh.vertices().data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(mesh.indices().size() * sizeof(uint32_t)),
        mesh.indices().data(),
        GL_STATIC_DRAW);
    m_indexCount = static_cast<int>(mesh.indices().size());

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE, sizeof(FaceMeshVertex), reinterpret_cast<void*>(offsetof(FaceMeshVertex, position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, sizeof(FaceMeshVertex), reinterpret_cast<void*>(offsetof(FaceMeshVertex, uv)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 1, GL_FLOAT, GL_FALSE, sizeof(FaceMeshVertex), reinterpret_cast<void*>(offsetof(FaceMeshVertex, regionId)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(FaceMeshVertex),
        reinterpret_cast<void*>(offsetof(FaceMeshVertex, localOffset)));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        4,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(FaceMeshVertex),
        reinterpret_cast<void*>(offsetof(FaceMeshVertex, regionWeight)));

    glBindVertexArray(0);
}

void FaceAvatarSystem::setScreenRect(glm::vec2 topLeftPx, float sizePx)
{
    m_screenTopLeftPx = topLeftPx;
    m_screenSizePx = sizePx;
}

void FaceAvatarSystem::render(int viewportWidthPx, int viewportHeightPx) const
{
    if (m_texture == 0 || m_program == 0 || m_indexCount == 0) {
        return;
    }

    // Pixel-space projection, top-left origin, Y increasing downward —
    // ordinary screen/UI convention, independent of the world Camera used
    // for the board/character scene.
    const glm::mat4 projection = glm::ortho(
        0.0f, static_cast<float>(viewportWidthPx), static_cast<float>(viewportHeightPx), 0.0f, -1.0f, 1.0f);
    // Mesh vertex positions span [-0.5, 0.5], centered at the mesh's own
    // center, so translate to the rect's center rather than its top-left.
    const glm::vec2 rectCenterPx = m_screenTopLeftPx + glm::vec2(m_screenSizePx * 0.5f);
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(rectCenterPx, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(m_screenSizePx, m_screenSizePx, 1.0f));

    glUseProgram(m_program);
    glUniformMatrix4fv(m_locProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(m_locModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1f(m_locEyeScale, m_eyeScale);
    glUniform1f(m_locMouthOpen, m_mouthOpen);
    glUniform1f(m_locBrowPosition, m_browPosition);
    glUniform1f(m_locFaceRotation, m_faceRotation);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glUniform1i(m_locTexture, 0);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glUseProgram(0);
}
