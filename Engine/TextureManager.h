#pragma once

#include <string>
#include <unordered_map>

#include "Engine/OpenGLLoader.h"

// Loads and caches textures for sprite rendering. Must only be used once an
// OpenGL context is current.
class TextureManager
{
public:
    ~TextureManager();

    // Generates a procedural checkerboard texture — used to exercise the
    // textured-quad draw path before real sprite art existed.
    GLuint createCheckerboard(const std::string& name, int sizePx, int checkPx);

    // Loads an image file (PNG/JPG/...) from disk via stb_image and
    // uploads it as an RGBA texture. Returns 0 (and caches nothing under
    // `name`) if the file couldn't be loaded — callers should fall back
    // to something else rather than treat 0 as a valid texture.
    GLuint loadFromFile(const std::string& name, const std::string& path);

    // Returns 0 if no texture with this name has been created/loaded.
    GLuint get(const std::string& name) const;

private:
    GLuint store(const std::string& name, int width, int height, const unsigned char* rgbaPixels);

    std::unordered_map<std::string, GLuint> m_textures;
};
