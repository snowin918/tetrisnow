#pragma once

#include <QHash>
#include <QString>
#include <memory>

class QOpenGLTexture;
class QImage;

// Loads and caches textures for sprite rendering. Must only be used once an
// OpenGL context is current (i.e., from initializeGL() onward).
class TextureManager
{
public:
    ~TextureManager();

    QOpenGLTexture* load(const QString& name, const QString& path);

    // Generates a procedural checkerboard texture — used to exercise the
    // textured-quad draw path before real sprite art exists.
    QOpenGLTexture* createCheckerboard(const QString& name, int sizePx, int checkPx);

    QOpenGLTexture* get(const QString& name) const;

private:
    QOpenGLTexture* store(const QString& name, const QImage& image);

    QHash<QString, std::unique_ptr<QOpenGLTexture>> m_textures;
};
