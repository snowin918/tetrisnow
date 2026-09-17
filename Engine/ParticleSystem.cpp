#include "Engine/ParticleSystem.h"

#include "Engine/Renderer.h"

ParticleSystem::ParticleSystem(size_t maxParticles)
    : m_maxParticles(maxParticles)
    , m_rng(std::random_device{}())
{
    m_particles.reserve(maxParticles);
}

void ParticleSystem::emit(const EmitParams& params, int count)
{
    std::uniform_real_distribution<float> velX(params.velocityMin.x, params.velocityMax.x);
    std::uniform_real_distribution<float> velY(params.velocityMin.y, params.velocityMax.y);
    std::uniform_real_distribution<float> sizeDist(params.sizeMin, params.sizeMax);
    std::uniform_real_distribution<float> lifeDist(params.lifetimeMin, params.lifetimeMax);

    for (int i = 0; i < count; ++i) {
        if (m_particles.size() >= m_maxParticles) {
            break; // drop excess particles rather than growing unbounded
        }

        Particle particle;
        particle.position = params.position;
        particle.velocity = glm::vec2(velX(m_rng), velY(m_rng));
        particle.color = params.color;
        particle.size = sizeDist(m_rng);
        particle.lifetime = lifeDist(m_rng);
        particle.age = 0.0f;
        particle.gravity = params.gravity;
        m_particles.push_back(particle);
    }
}

void ParticleSystem::update(float deltaTime)
{
    for (size_t i = 0; i < m_particles.size();) {
        Particle& particle = m_particles[i];
        particle.age += deltaTime;

        if (particle.age >= particle.lifetime) {
            // Order doesn't matter for particles, so swap-and-pop is fine.
            particle = m_particles.back();
            m_particles.pop_back();
            continue;
        }

        particle.velocity.y += particle.gravity * deltaTime;
        particle.position += particle.velocity * deltaTime;
        ++i;
    }
}

void ParticleSystem::draw(Renderer& renderer) const
{
    for (const Particle& particle : m_particles) {
        const float lifeFraction = 1.0f - (particle.age / particle.lifetime); // 1 -> 0 over its life
        glm::vec4 fadedColor = particle.color;
        fadedColor.a *= lifeFraction;

        const glm::vec2 size(particle.size);
        renderer.drawQuad(particle.position - size * 0.5f, size, fadedColor);
    }
}
