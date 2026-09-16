#include "Engine/ShaderManager.h"

#include <QDebug>
#include <QOpenGLShaderProgram>

ShaderManager::~ShaderManager() = default;

QOpenGLShaderProgram* ShaderManager::load(const QString& name, const QString& vertexPath, const QString& fragmentPath)
{
    auto program = std::make_unique<QOpenGLShaderProgram>();

    if (!program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertexPath)) {
        qWarning() << "ShaderManager: failed to compile vertex shader" << vertexPath << "-" << program->log();
    }
    if (!program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentPath)) {
        qWarning() << "ShaderManager: failed to compile fragment shader" << fragmentPath << "-" << program->log();
    }
    if (!program->link()) {
        qWarning() << "ShaderManager: failed to link program" << name << "-" << program->log();
    }

    QOpenGLShaderProgram* raw = program.get();
    m_programs.insert(name, std::move(program));
    return raw;
}

QOpenGLShaderProgram* ShaderManager::get(const QString& name) const
{
    const auto it = m_programs.find(name);
    return it != m_programs.end() ? it->get() : nullptr;
}
