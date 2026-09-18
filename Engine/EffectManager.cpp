#include "Engine/EffectManager.h"

#include <algorithm>
#include <cmath>

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

void EffectManager::update(float deltaTime, float intensity)
{
    if (intensity <= 0.0f) {
        m_particles.update(deltaTime);
        m_camera.update(deltaTime);
        return;
    }
    const bool initialSnow = m_weatherSeconds == 0.0f;
    m_weatherSeconds += deltaTime;
    constexpr float kAmbientInterval = 0.025f;
    const float interval = kAmbientInterval / std::max(intensity, 0.05f);
    m_ambientSnowTimer += deltaTime;
    if (initialSnow) m_ambientSnowTimer += 6.0f;

    // Shared gust direction and depth-dependent speeds keep flakes in one air mass.
    const float wind = 1.5f + 0.7f * std::sin(m_weatherSeconds * 0.6f) + 0.3f * std::sin(m_weatherSeconds * 1.7f);
    std::uniform_real_distribution<float> xDist(m_ambientMinX - 18.0f, m_ambientMaxX + 8.0f);
    std::uniform_real_distribution<float> windSignDist(-1.0f, 1.0f);
    while (m_ambientSnowTimer >= interval) {
        m_ambientSnowTimer -= interval;

        const float depth = 0.5f + (windSignDist(m_ambientRng) + 1.0f) * 0.65f;
        const float gust = wind * depth;
        ParticleSystem::EmitParams params;
        params.shape = ParticleSystem::Shape::WeatherSnow;
        params.position = glm::vec2(xDist(m_ambientRng), -7.0f);
        if (initialSnow) params.position.y = -7.0f + (windSignDist(m_ambientRng)+1.0f)*16.0f;
        params.velocityMin = glm::vec2(gust - 0.2f, 2.5f * depth);
        params.velocityMax = glm::vec2(gust + 0.2f, 3.5f * depth);
        params.color = glm::vec4(0.9f, 0.95f, 1.0f, 0.22f + depth * 0.12f);
        params.sizeMin = 0.035f * depth;
        params.sizeMax = 0.13f * depth;
        params.lifetimeMin = 13.0f / depth;
        params.lifetimeMax = 16.0f / depth;
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

    // A single line doesn't send an attack across (see Match::
    // onLinesCleared) — it just melts away in place, so it gets a gentle
    // fog dissipate instead of the multi-line shatter/storm below, and no
    // camera shake (there's no impact to sell).
    if (lineCount == 1) {
        for (const Board::ClearedLine& line : clearedLines) {
            for (int col = 0; col < Board::kWidth; ++col) {
                const BlockType type = line.cells[static_cast<size_t>(col)];
                if (type == BlockType::Empty) {
                    continue;
                }

                const glm::vec2 cellCenter(
                    originX + static_cast<float>(col) + 0.5f, static_cast<float>(line.row) + 0.5f);

                ParticleSystem::EmitParams fog;
                fog.shape = ParticleSystem::Shape::Mist;
                fog.growth = 2.0f;
                fog.drag = 1.6f;
                fog.position = cellCenter;
                fog.velocityMin = glm::vec2(-0.4f, -0.9f);
                fog.velocityMax = glm::vec2(0.4f, -0.3f);
                fog.color = glm::mix(glm::vec4(0.9f, 0.97f, 1.0f, 0.5f), colorForBlockType(type), 0.3f);
                fog.sizeMin = 0.3f;
                fog.sizeMax = 0.6f;
                fog.lifetimeMin = 0.4f;
                fog.lifetimeMax = 0.7f;
                fog.gravity = -0.2f;
                m_particles.emit(fog, 6);
            }
        }
        return;
    }

    for (const Board::ClearedLine& line : clearedLines) {
        for (int col = 0; col < Board::kWidth; ++col) {
            const BlockType type = line.cells[static_cast<size_t>(col)];
            if (type == BlockType::Empty) {
                continue;
            }

            const glm::vec2 cellCenter(originX + static_cast<float>(col) + 0.5f, static_cast<float>(line.row) + 0.5f);

            // The block itself, shattering into colored ice shards.
            ParticleSystem::EmitParams shard;
            shard.shape = ParticleSystem::Shape::Shard;
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

    m_camera.triggerShake(0.045f * static_cast<float>(lineCount), 0.18f + 0.04f * static_cast<float>(lineCount));
}

void EffectManager::spawnSnowExplosion(float impactX, int power)
{
    const float bottomY = static_cast<float>(Board::kHeight);
    const glm::vec2 impactPoint(impactX, bottomY);

    // An instant bright flash the moment the attack lands — big, punchy,
    // and gone within a couple of frames. This is what actually sells the
    // "impact" beat; the debris below reads as the aftermath, but without
    // this flash the whole explosion was too small/subtle at typical
    // camera zoom to reliably catch the eye (reported as "blinks, often
    // doesn't display" — the burst was real but easy to miss entirely).
    ParticleSystem::EmitParams flash;
    flash.shape = ParticleSystem::Shape::Mist;
    flash.position = impactPoint;
    flash.velocityMin = glm::vec2(0.0f, 0.0f);
    flash.velocityMax = glm::vec2(0.0f, 0.0f);
    flash.color = glm::vec4(0.97f, 0.99f, 1.0f, 0.95f);
    flash.sizeMin = 2.4f + static_cast<float>(power) * 0.18f;
    flash.sizeMax = flash.sizeMin;
    flash.lifetimeMin = 0.22f;
    flash.lifetimeMax = 0.22f;
    flash.growth = 4.5f;
    flash.drag = 0.0f;
    m_particles.emit(flash, 1);

    // The impact core: the snowball bursting into a spray of small round
    // snow chunks. Deliberately round (the default soft-circle shape, not
    // Shape::Shard's angular faceted-ice-cube look) and white — each
    // bullet is a snowball, not an ice tile, so its explosion shouldn't
    // read as broken masonry. Smaller and more numerous than the old
    // chunky shard debris so it reads as a burst of snow, not gravel.
    ParticleSystem::EmitParams core;
    core.position = impactPoint;
    core.velocityMin = glm::vec2(-5.5f, -5.5f);
    core.velocityMax = glm::vec2(5.5f, -1.5f);
    core.color = glm::vec4(0.97f, 0.99f, 1.0f, 1.0f);
    core.sizeMin = 0.13f;
    core.sizeMax = 0.28f;
    core.lifetimeMin = 0.5f;
    core.lifetimeMax = 0.95f;
    core.gravity = 5.0f;
    m_particles.emit(core, 24 + power * 7);
    ParticleSystem::EmitParams mist;
    mist.shape = ParticleSystem::Shape::Mist;
    mist.position = impactPoint;
    mist.velocityMin = glm::vec2(-3.5f, -3.2f);
    mist.velocityMax = glm::vec2(3.5f, -0.5f);
    mist.color = glm::vec4(0.72f, 0.88f, 1.0f, 0.4f);
    mist.sizeMin = 0.8f;
    mist.sizeMax = 1.4f;
    mist.lifetimeMin = 0.5f;
    mist.lifetimeMax = 0.9f;
    mist.drag = 2.0f;
    mist.growth = 2.2f;
    m_particles.emit(mist, 10 + power);

    // A wider, longer-lived blast of fine snow — a squall thrown up by the
    // impact, scaled with the attack's power so an Avalanche visibly
    // engulfs the board rather than just landing on it.
    ParticleSystem::EmitParams squall;
    squall.drag = 1.8f;
    squall.position = impactPoint;
    const float spread = 5.0f + static_cast<float>(power) * 0.8f;
    squall.velocityMin = glm::vec2(-spread, -6.0f - static_cast<float>(power) * 0.5f);
    squall.velocityMax = glm::vec2(spread, -1.5f);
    squall.color = glm::vec4(0.95f, 0.98f, 1.0f, 0.7f);
    squall.sizeMin = 0.1f;
    squall.sizeMax = 0.22f;
    squall.lifetimeMin = 0.9f;
    squall.lifetimeMax = 1.5f + static_cast<float>(power) * 0.12f;
    squall.gravity = 2.0f;
    m_particles.emit(squall, 18 + power * 6);

    m_camera.triggerShake(0.14f + 0.03f * static_cast<float>(power), 0.35f);
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

void EffectManager::emitRotationPuff(glm::vec2 position, glm::vec4 color)
{
    ParticleSystem::EmitParams puff;
    puff.position = position;
    puff.velocityMin = glm::vec2(-1.6f, -1.6f);
    puff.velocityMax = glm::vec2(1.6f, 1.6f);
    puff.color = color;
    puff.sizeMin = 0.05f;
    puff.sizeMax = 0.1f;
    puff.lifetimeMin = 0.12f;
    puff.lifetimeMax = 0.22f;
    puff.gravity = 0.0f;
    m_particles.emit(puff, 3);
}

void EffectManager::spawnWallBounce(glm::vec2 position, glm::vec4 color)
{
    ParticleSystem::EmitParams puff;
    puff.position = position;
    puff.velocityMin = glm::vec2(-2.5f, -2.0f);
    puff.velocityMax = glm::vec2(2.5f, 1.0f);
    puff.color = color;
    puff.sizeMin = 0.06f;
    puff.sizeMax = 0.14f;
    puff.lifetimeMin = 0.15f;
    puff.lifetimeMax = 0.3f;
    puff.gravity = 1.0f;
    m_particles.emit(puff, 6);

    // A light "punch" — noticeably smaller than a real landing's shake so
    // the eventual spawnSnowExplosion() still reads as the bigger moment.
    m_camera.triggerShake(0.06f, 0.1f);
}

void EffectManager::spawnHardDropFog(const std::array<glm::ivec2, 4>& cells, float originX, glm::vec4 color)
{
    for (const glm::ivec2& cell : cells) {
        if (cell.y < 0) {
            continue;
        }

        ParticleSystem::EmitParams fog;
        fog.shape = ParticleSystem::Shape::Mist;
        fog.growth = 2.2f;
        fog.drag = 2.0f;
        fog.position = glm::vec2(originX + static_cast<float>(cell.x) + 0.5f, static_cast<float>(cell.y) + 0.5f);
        fog.velocityMin = glm::vec2(-0.45f, -0.25f);
        fog.velocityMax = glm::vec2(0.45f, 0.65f);
        fog.color = glm::vec4(0.88f, 0.96f, 1.0f, 0.42f);
        fog.sizeMin = 0.35f;
        fog.sizeMax = 0.7f;
        fog.lifetimeMin = 0.28f;
        fog.lifetimeMax = 0.48f;
        fog.gravity = -0.25f;
        m_particles.emit(fog, 5);

        ParticleSystem::EmitParams tintMist = fog;
        tintMist.color = glm::vec4(glm::mix(glm::vec3(0.9f, 0.98f, 1.0f), glm::vec3(color), 0.25f), 0.28f);
        tintMist.sizeMin = 0.18f;
        tintMist.sizeMax = 0.38f;
        m_particles.emit(tintMist, 2);
    }
}

void EffectManager::spawnHardDropImpact(const std::array<glm::ivec2, 4>& cells, float originX, glm::vec4 color)
{
    for (const glm::ivec2& cell : cells) {
        if (cell.y < 0) {
            continue;
        }

        ParticleSystem::EmitParams bang;
        bang.shape = ParticleSystem::Shape::Shard;
        bang.position = glm::vec2(originX + static_cast<float>(cell.x) + 0.5f, static_cast<float>(cell.y) + 0.85f);
        bang.velocityMin = glm::vec2(-2.4f, -2.2f);
        bang.velocityMax = glm::vec2(2.4f, 0.8f);
        bang.color = glm::vec4(glm::mix(glm::vec3(color), glm::vec3(1.0f), 0.45f), 0.9f);
        bang.sizeMin = 0.08f;
        bang.sizeMax = 0.22f;
        bang.lifetimeMin = 0.18f;
        bang.lifetimeMax = 0.34f;
        bang.gravity = 3.5f;
        m_particles.emit(bang, 6);
    }

    m_camera.triggerShake(0.12f, 0.12f);
}
