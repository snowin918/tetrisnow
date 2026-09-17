#include "Engine/TextureManager.h"

#include <cstdio>
#include <vector>

#include <stb_image.h>

TextureManager::~TextureManager()
{
    for (const auto& [name, texture] : m_textures) {
        glDeleteTextures(1, &texture);
    }
}

GLuint TextureManager::createCheckerboard(const std::string& name, int sizePx, int checkPx)
{
    std::vector<unsigned char> pixels(static_cast<size_t>(sizePx) * sizePx * 4);

    constexpr unsigned char kLight[4] = {230, 240, 255, 255};
    constexpr unsigned char kDark[4] = {140, 170, 210, 255};

    for (int y = 0; y < sizePx; ++y) {
        for (int x = 0; x < sizePx; ++x) {
            const bool alt = ((x / checkPx) + (y / checkPx)) % 2 == 0;
            const unsigned char* color = alt ? kDark : kLight;
            const size_t offset = (static_cast<size_t>(y) * sizePx + x) * 4;
            pixels[offset + 0] = color[0];
            pixels[offset + 1] = color[1];
            pixels[offset + 2] = color[2];
            pixels[offset + 3] = color[3];
        }
    }

    return store(name, sizePx, sizePx, pixels.data());
}

GLuint TextureManager::loadFromFile(const std::string& name, const std::string& path)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        std::fprintf(stderr, "TextureManager: failed to load '%s'\n", path.c_str());
        return 0;
    }

    const GLuint texture = store(name, width, height, pixels);
    stbi_image_free(pixels);
    return texture;
}

GLuint TextureManager::get(const std::string& name) const
{
    const auto it = m_textures.find(name);
    return it != m_textures.end() ? it->second : 0;
}

GLuint TextureManager::store(const std::string& name, int width, int height, const unsigned char* rgbaPixels)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels);

    m_textures[name] = texture;
    return texture;
}
