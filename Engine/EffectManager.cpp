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
    const int lineCount = static_cast<int>(clearedLines.size());

    for (const Board::ClearedLine& line : clearedLines) {
        for (int col = 0; col < Board::kWidth; ++col) {
            const BlockType type = line.cells[static_cast<size_t>(col)];
            if (type == BlockType::Empty) {
                continue;
            }

            const glm::vec2 cellCenter(originX + static_cast<float>(col) + 0.5f, static_cast<float>(line.row) + 0.5f);

            // The block itself, shattering into colored ice shards.
            ParticleSystem::EmitParams shard;
            shard.position = cellCenter;
            shard.velocityMin = glm::vec2(-2.5f, -3.5f);
            shard.velocityMax = glm::vec2(2.5f, -0.5f);
            shard.color = colorForBlockType(type);
            shard.sizeMin = 0.12f;
            shard.sizeMax = 0.28f;
            shard.lifetimeMin = 0.35f;
            shard.lifetimeMax = 0.65f;
            shard.gravity = 6.0f;
            m_particles.emit(shard, 6);

            // Fine white snow-dust swept up alongside the shards — the
            // "snowstorm" layer. Wider, longer-lived, and gustier the more
            // lines clear at once, so a bigger clear reads as stormier.
            ParticleSystem::EmitParams dust;
            dust.position = cellCenter;
            const float gust = static_cast<float>(lineCount) * 0.6f;
            dust.velocityMin = glm::vec2(-4.0f - gust, -2.5f);
            dust.velocityMax = glm::vec2(4.0f + gust, 1.0f);
            dust.color = glm::vec4(0.92f, 0.96f, 1.0f, 0.65f);
            dust.sizeMin = 0.05f;
            dust.sizeMax = 0.11f;
            dust.lifetimeMin = 0.5f + static_cast<float>(lineCount) * 0.1f;
            dust.lifetimeMax = 0.9f + static_cast<float>(lineCount) * 0.15f;
            dust.gravity = 1.0f;
            m_particles.emit(dust, 3 + lineCount * 2);
        }
    }

    // A Tetris gets its own flourish: a wide sparkle burst across the
    // cleared rows, reading as a snow avalanche rather than just a
    // handful of shattering blocks.
    if (lineCount >= 4) {
        const float centerX = originX + static_cast<float>(Board::kWidth) / 2.0f;
        float averageRow = 0.0f;
        for (const Board::ClearedLine& line : clearedLines) {
            averageRow += static_cast<float>(line.row);
        }
        averageRow /= static_cast<float>(lineCount);

        ParticleSystem::EmitParams burst;
        burst.position = glm::vec2(centerX, averageRow + 0.5f);
        burst.velocityMin = glm::vec2(-6.0f, -6.0f);
        burst.velocityMax = glm::vec2(6.0f, 6.0f);
        burst.color = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f);
        burst.sizeMin = 0.08f;
        burst.sizeMax = 0.2f;
        burst.lifetimeMin = 0.4f;
        burst.lifetimeMax = 0.8f;
        burst.gravity = 2.0f;
        m_particles.emit(burst, 40);
    }

    m_camera.triggerShake(0.12f * static_cast<float>(lineCount), 0.18f + 0.04f * static_cast<float>(lineCount));
}

void EffectManager::spawnSnowExplosion(float originX, int power)
{
    const float centerX = originX + static_cast<float>(Board::kWidth) / 2.0f;
    const float bottomY = static_cast<float>(Board::kHeight);
    const glm::vec2 impactPoint(centerX, bottomY);

    // The impact core: chunky ice fragments bursting outward.
    ParticleSystem::EmitParams core;
    core.position = impactPoint;
    core.velocityMin = glm::vec2(-4.0f, -4.0f);
    core.velocityMax = glm::vec2(4.0f, -1.0f);
    core.color = glm::vec4(0.85f, 0.92f, 1.0f, 1.0f);
    core.sizeMin = 0.15f;
    core.sizeMax = 0.35f;
    core.lifetimeMin = 0.4f;
    core.lifetimeMax = 0.8f;
    core.gravity = 5.0f;
    m_particles.emit(core, 10 + power * 4);

    // A wider, longer-lived blast of fine snow — a squall thrown up by the
    // impact, scaled with the attack's power so an Avalanche visibly
    // engulfs the board rather than just landing on it.
    ParticleSystem::EmitParams squall;
    squall.position = impactPoint;
    const float spread = 5.0f + static_cast<float>(power) * 0.8f;
    squall.velocityMin = glm::vec2(-spread, -6.0f - static_cast<float>(power) * 0.5f);
    squall.velocityMax = glm::vec2(spread, -1.5f);
    squall.color = glm::vec4(0.95f, 0.98f, 1.0f, 0.55f);
    squall.sizeMin = 0.06f;
    squall.sizeMax = 0.14f;
    squall.lifetimeMin = 0.6f;
    squall.lifetimeMax = 1.1f + static_cast<float>(power) * 0.1f;
    squall.gravity = 2.0f;
    m_particles.emit(squall, 14 + power * 5);

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
