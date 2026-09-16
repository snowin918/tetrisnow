#pragma once

#include <QHash>
#include <QString>
#include <memory>

class QOpenGLShaderProgram;

// Compiles, links, and caches GLSL shader programs by name. Source is kept
// in .vert/.frag files under Assets/Shaders (embedded via assets.qrc) rather
// than inline C++ strings, so shaders can be edited without recompiling.
class ShaderManager
{
public:
    ~ShaderManager();

    // Compiles and links a program from the given vertex/fragment sources
    // (file paths or Qt resource paths like ":/Shaders/quad.vert"). Logs a
    // warning and returns a possibly-unlinked program on failure.
    QOpenGLShaderProgram* load(const QString& name, const QString& vertexPath, const QString& fragmentPath);

    QOpenGLShaderProgram* get(const QString& name) const;

private:
    QHash<QString, std::unique_ptr<QOpenGLShaderProgram>> m_programs;
};
