#include "Engine/EffectManager.h"

#include "Engine/BlockColors.h"

EffectManager::EffectManager(Camera& camera)
    : m_camera(camera)
{
}

void EffectManager::setAmbientSnowSpan(float minX, float maxX)
{
    m_ambientMinX = minX;
    m_ambientMaxX = maxX;
}

void EffectManager::update(float deltaTime)
{
    constexpr float kAmbientInterval = 0.06f;
    m_ambientSnowTimer += deltaTime;

    std::uniform_real_distribution<float> xDist(m_ambientMinX, m_ambientMaxX);
    while (m_ambientSnowTimer >= kAmbientInterval) {
        m_ambientSnowTimer -= kAmbientInterval;

        ParticleSystem::EmitParams params;
        params.position = glm::vec2(xDist(m_ambientRng), -2.0f);
        params.velocityMin = glm::vec2(-0.3f, 1.0f);
        params.velocityMax = glm::vec2(0.3f, 2.0f);
        params.color = glm::vec4(0.9f, 0.95f, 1.0f, 0.5f);
        params.sizeMin = 0.06f;
        params.sizeMax = 0.14f;
        params.lifetimeMin = 5.0f;
        params.lifetimeMax = 8.0f;
        params.gravity = 0.0f;
        m_particles.emit(params, 1);
    }

    m_particles.update(deltaTime);
    m_camera.update(deltaTime);
}

void EffectManager::draw(Renderer& renderer) const
{
    m_particles.draw(renderer);
}

void EffectManager::spawnBlockClearEffect(float originX, const std::vector<Board::ClearedLine>& clearedLines)
{
    for (const Board::ClearedLine& line : clearedLines) {
        for (int col = 0; col < Board::kWidth; ++col) {
            const BlockType type = line.cells[static_cast<size_t>(col)];
            if (type == BlockType::Empty) {
                continue;
            }

            ParticleSystem::EmitParams params;
            params.position = glm::vec2(originX + static_cast<float>(col) + 0.5f, static_cast<float>(line.row) + 0.5f);
            params.velocityMin = glm::vec2(-2.5f, -3.5f);
            params.velocityMax = glm::vec2(2.5f, -0.5f);
            params.color = colorForBlockType(type);
            params.sizeMin = 0.12f;
            params.sizeMax = 0.28f;
            params.lifetimeMin = 0.35f;
            params.lifetimeMax = 0.65f;
            params.gravity = 6.0f;
            m_particles.emit(params, 6);
        }
    }

    m_camera.triggerShake(0.12f * static_cast<float>(clearedLines.size()), 0.2f);
}

void EffectManager::spawnSnowExplosion(float originX, int power)
{
    const float centerX = originX + static_cast<float>(Board::kWidth) / 2.0f;
    const float bottomY = static_cast<float>(Board::kHeight);

    ParticleSystem::EmitParams params;
    params.position = glm::vec2(centerX, bottomY);
    params.velocityMin = glm::vec2(-4.0f, -4.0f);
    params.velocityMax = glm::vec2(4.0f, -1.0f);
    params.color = glm::vec4(0.85f, 0.92f, 1.0f, 1.0f);
    params.sizeMin = 0.15f;
    params.sizeMax = 0.35f;
    params.lifetimeMin = 0.4f;
    params.lifetimeMax = 0.8f;
    params.gravity = 5.0f;
    m_particles.emit(params, 10 + power * 4);

    m_camera.triggerShake(0.2f + 0.12f * static_cast<float>(power), 0.3f);
}

void EffectManager::emitAttackTrail(glm::vec2 position, glm::vec4 color)
{
    ParticleSystem::EmitParams trail;
    trail.position = position;
    trail.velocityMin = glm::vec2(-0.5f, -0.5f);
    trail.velocityMax = glm::vec2(0.5f, 0.5f);
    trail.color = color;
    trail.sizeMin = 0.06f;
    trail.sizeMax = 0.14f;
    trail.lifetimeMin = 0.15f;
    trail.lifetimeMax = 0.3f;
    trail.gravity = 0.0f;
    m_particles.emit(trail, 2);
}
