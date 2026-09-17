#pragma once

#include <string>
#include <unordered_map>

#include "Engine/OpenGLLoader.h"

// Loads and caches textures for sprite rendering. Must only be used once an
// OpenGL context is current.
//
// File-based loading (e.g. via stb_image) will be added once the project
// has real sprite art to load; for now the only source is a procedurally
// generated placeholder texture.
class TextureManager
{
public:
    ~TextureManager();

    // Generates a procedural checkerboard texture — used to exercise the
    // textured-quad draw path before real sprite art exists.
    GLuint createCheckerboard(const std::string& name, int sizePx, int checkPx);

    // Returns 0 if no texture with this name has been created/loaded.
    GLuint get(const std::string& name) const;

private:
    GLuint store(const std::string& name, int width, int height, const unsigned char* rgbaPixels);

    std::unordered_map<std::string, GLuint> m_textures;
};
