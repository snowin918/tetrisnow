#pragma once

#include <string>
#include <unordered_map>

#include "Engine/OpenGLLoader.h"

// Compiles, links, and caches GLSL shader programs by name. Source is kept
// in .vert/.frag files under Assets/Shaders rather than inline C++ strings,
// so shaders can be edited without recompiling.
class ShaderManager
{
public:
    ~ShaderManager();

    // Compiles and links a program from the given vertex/fragment source
    // file paths. Logs a warning and returns 0 on failure.
    GLuint load(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);

    // Returns 0 if no program with this name has been loaded.
    GLuint get(const std::string& name) const;

private:
    std::unordered_map<std::string, GLuint> m_programs;
};
