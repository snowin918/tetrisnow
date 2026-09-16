#include "Engine/TextureManager.h"

#include <QDebug>
#include <QImage>
#include <QOpenGLTexture>
#include <QPainter>

TextureManager::~TextureManager() = default;

QOpenGLTexture* TextureManager::load(const QString& name, const QString& path)
{
    QImage image(path);
    if (image.isNull()) {
        qWarning() << "TextureManager: failed to load image" << path;
        return nullptr;
    }
    return store(name, image);
}

QOpenGLTexture* TextureManager::createCheckerboard(const QString& name, int sizePx, int checkPx)
{
    QImage image(sizePx, sizePx, QImage::Format_RGBA8888);
    image.fill(QColor(230, 240, 255));

    QPainter painter(&image);
    const QColor dark(140, 170, 210);
    for (int y = 0; y < sizePx; y += checkPx) {
        for (int x = 0; x < sizePx; x += checkPx) {
            const bool alt = ((x / checkPx) + (y / checkPx)) % 2 == 0;
            if (alt) {
                painter.fillRect(x, y, checkPx, checkPx, dark);
            }
        }
    }
    painter.end();

    return store(name, image);
}

QOpenGLTexture* TextureManager::get(const QString& name) const
{
    const auto it = m_textures.find(name);
    return it != m_textures.end() ? it->get() : nullptr;
}

QOpenGLTexture* TextureManager::store(const QString& name, const QImage& image)
{
    // OpenGL's texture origin is bottom-left; QImage's is top-left.
    auto texture = std::make_unique<QOpenGLTexture>(image.mirrored(false, true));
    texture->setMinificationFilter(QOpenGLTexture::Nearest);
    texture->setMagnificationFilter(QOpenGLTexture::Nearest);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);

    QOpenGLTexture* raw = texture.get();
    m_textures.insert(name, std::move(texture));
    return raw;
}
