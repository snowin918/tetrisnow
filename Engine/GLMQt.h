#pragma once

#include <QMatrix4x4>
#include <glm/glm.hpp>

// Qt's shader uniform API (QOpenGLShaderProgram::setUniformValue) takes
// QMatrix4x4, while our math types are GLM. This is the one conversion seam
// between them. glm::mat4 is column-major (m[col][row]); QMatrix4x4's
// element constructor takes arguments in row-major order, hence the
// transpose-by-indexing below.
inline QMatrix4x4 toQMatrix4x4(const glm::mat4& m)
{
    return QMatrix4x4(
        m[0][0], m[1][0], m[2][0], m[3][0],
        m[0][1], m[1][1], m[2][1], m[3][1],
        m[0][2], m[1][2], m[2][2], m[3][2],
        m[0][3], m[1][3], m[2][3], m[3][3]);
}
