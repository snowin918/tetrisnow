#include "Engine/ShaderManager.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace
{
std::string readFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file) {
        std::fprintf(stderr, "ShaderManager: failed to open %s\n", path.c_str());
        return {};
    }
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

// Compiles one shader stage; returns 0 on failure (after logging why).
GLuint compileStage(GLenum stage, const std::string& source, const std::string& debugName)
{
    const GLuint shader = glCreateShader(stage);
    const char* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<size_t>(logLength) + 1, '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        std::fprintf(stderr, "ShaderManager: failed to compile %s:\n%s\n", debugName.c_str(), log.data());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}
} // namespace

ShaderManager::~ShaderManager()
{
    for (const auto& [name, program] : m_programs) {
        glDeleteProgram(program);
    }
}

GLuint ShaderManager::load(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath)
{
    const std::string vertexSource = readFile(vertexPath);
    const std::string fragmentSource = readFile(fragmentPath);

    const GLuint vertexShader = compileStage(GL_VERTEX_SHADER, vertexSource, vertexPath);
    const GLuint fragmentShader = compileStage(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);
    if (vertexShader == 0 || fragmentShader == 0) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Shader objects are refcounted by the program once attached; safe to
    // delete our references right after linking.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linkSuccess = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linkSuccess);
    if (!linkSuccess) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<size_t>(logLength) + 1, '\0');
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        std::fprintf(stderr, "ShaderManager: failed to link program '%s':\n%s\n", name.c_str(), log.data());
        glDeleteProgram(program);
        return 0;
    }

    m_programs[name] = program;
    return program;
}

GLuint ShaderManager::get(const std::string& name) const
{
    const auto it = m_programs.find(name);
    return it != m_programs.end() ? it->second : 0;
}
