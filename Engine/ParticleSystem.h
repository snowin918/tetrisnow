#pragma once

#include <cstddef>
#include <random>
#include <vector>

#include <glm/glm.hpp>

class Renderer;

// A simple CPU-simulated particle system, rendered as individual quads via
// the existing Renderer (one draw call per particle). That's not
// GPU-instanced, but it's entirely adequate at the scale this game needs —
// a few hundred particles for line-clear bursts, attack trails, and
// ambient snowfall — and keeps the implementation small. Instancing would
// be the natural next step if particle counts ever grew much larger.
class ParticleSystem
{
public:
    enum class Shape { Snow, Shard, Mist, WeatherSnow };
    struct EmitParams
    {
        glm::vec2 position{0.0f};
        glm::vec2 velocityMin{-1.0f, -1.0f};
        glm::vec2 velocityMax{1.0f, 1.0f};
        glm::vec4 color{1.0f};
        float sizeMin = 0.1f;
        float sizeMax = 0.2f;
        float lifetimeMin = 0.3f;
        float lifetimeMax = 0.6f;
        Shape shape = Shape::Snow;
        float growth = 0.0f;
        float drag = 0.0f;
        float gravity = 0.0f; // added to velocity.y each second (world Y is down-positive)
    };

    explicit ParticleSystem(size_t maxParticles = 2000);

    void emit(const EmitParams& params, int count);
    void update(float deltaTime);
    void draw(Renderer& renderer) const;

    size_t activeCount() const { return m_particles.size(); }

private:
    struct Particle
    {
        glm::vec2 position;
        glm::vec2 velocity;
        glm::vec4 color;
        float size;
        float age = 0.0f;
        float lifetime;
        float gravity;
        Shape shape;
        float growth, drag, rotation, spin;
    };

    std::vector<Particle> m_particles;
    size_t m_maxParticles;
    std::mt19937 m_rng;
};
