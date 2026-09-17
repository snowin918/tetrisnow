#include "FaceAvatar/FaceAvatarSystem.h"

#include <cstdio>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

namespace
{
// Unit quad in [0,1]x[0,1], interleaved position + UV — same layout as
// Engine/Renderer's, so the vertex attribute setup mirrors it exactly.
constexpr float kQuadVertices[] = {
    0.0f, 0.0f,   0.0f, 0.0f,
    1.0f, 0.0f,   1.0f, 0.0f,
    1.0f, 1.0f,   1.0f, 1.0f,

    0.0f, 0.0f,   0.0f, 0.0f,
    1.0f, 1.0f,   1.0f, 1.0f,
    0.0f, 1.0f,   0.0f, 1.0f,
};
} // namespace

FaceAvatarSystem::~FaceAvatarSystem()
{
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

void FaceAvatarSystem::initialize()
{
    m_program = m_shaderManager.load(
        "faceAvatar", TETRISNOW_ASSETS_DIR "/Shaders/faceAvatar.vert", TETRISNOW_ASSETS_DIR "/Shaders/faceAvatar.frag");

    m_locProjection = glGetUniformLocation(m_program, "uProjection");
    m_locModel = glGetUniformLocation(m_program, "uModel");
    m_locUvOffset = glGetUniformLocation(m_program, "uUvOffset");
    m_locUvScale = glGetUniformLocation(m_program, "uUvScale");
    m_locTexture = glGetUniformLocation(m_program, "uTexture");

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

    // Center-crop to square via UV rect rather than resampling pixels —
    // cheap, and consistent with how Engine/Renderer already expresses
    // sprite-sheet sub-rects as UV offset/scale.
    if (width > height) {
        m_uvScale = glm::vec2(static_cast<float>(height) / static_cast<float>(width), 1.0f);
        m_uvOffset = glm::vec2((1.0f - m_uvScale.x) / 2.0f, 0.0f);
    } else {
        m_uvScale = glm::vec2(1.0f, static_cast<float>(width) / static_cast<float>(height));
        m_uvOffset = glm::vec2(0.0f, (1.0f - m_uvScale.y) / 2.0f);
    }

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

void FaceAvatarSystem::setScreenRect(glm::vec2 topLeftPx, float sizePx)
{
    m_screenTopLeftPx = topLeftPx;
    m_screenSizePx = sizePx;
}

void FaceAvatarSystem::render(int viewportWidthPx, int viewportHeightPx) const
{
    if (m_texture == 0 || m_program == 0) {
        return;
    }

    // Pixel-space projection, top-left origin, Y increasing downward —
    // ordinary screen/UI convention, independent of the world Camera used
    // for the board/character scene.
    const glm::mat4 projection = glm::ortho(
        0.0f, static_cast<float>(viewportWidthPx), static_cast<float>(viewportHeightPx), 0.0f, -1.0f, 1.0f);
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(m_screenTopLeftPx, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(m_screenSizePx, m_screenSizePx, 1.0f));

    glUseProgram(m_program);
    glUniformMatrix4fv(m_locProjection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(m_locModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform2f(m_locUvOffset, m_uvOffset.x, m_uvOffset.y);
    glUniform2f(m_locUvScale, m_uvScale.x, m_uvScale.y);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glUniform1i(m_locTexture, 0);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}
