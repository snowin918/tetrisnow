#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "FaceAvatar/FaceLandmarks.h"

// Region tags a FaceMeshVertex can carry — matches the REGION_* constants
// in Assets/Shaders/faceMesh.vert, which is what actually interprets them.
enum class FaceMeshRegion
{
    None = 0,
    Eye = 1,
    Eyebrow = 2,
    Mouth = 3,
};

// Per-vertex layout matches Assets/Shaders/faceMesh.vert's attributes
// exactly (locations 0-4) — see that file for how each field drives GPU
// deformation.
struct FaceMeshVertex
{
    glm::vec2 position{0.0f};    // mesh-local space, in [-0.5, 0.5] on each axis, centered at the mesh's own center
    glm::vec2 uv{0.0f};          // texture UV within the full (uncropped) source photo
    float regionId = 0.0f;       // a FaceMeshRegion value
    glm::vec2 localOffset{0.0f}; // offset from this vertex's region center, mesh-local units (0 if regionId == None)
    float regionWeight = 0.0f;   // 0..1 feather factor, tapering to 0 at the region's outer edge
};

// A 2D deformable face mesh: a regular grid over the square region of the
// source photo containing the detected face, with each vertex tagged by
// which facial feature (if any) it belongs to so the vertex shader can
// move eyes/eyebrows/mouth independently via uniforms. Built once per
// loaded photo (see FaceAvatarSystem::loadPlayerImage); deformation after
// that happens entirely on the GPU, not by rebuilding the mesh.
class FaceMesh
{
public:
    // Builds a gridResolution x gridResolution grid of vertices. If
    // landmarks.valid is false, still builds a plain center-cropped grid
    // (every vertex regionId == None) so the system degrades to a static,
    // non-deformable portrait rather than failing outright — the same
    // crop behavior Phase 1 had before landmarks existed.
    static FaceMesh build(const FaceLandmarks& landmarks, int imageWidth, int imageHeight, int gridResolution = 24);

    const std::vector<FaceMeshVertex>& vertices() const { return m_vertices; }
    const std::vector<uint32_t>& indices() const { return m_indices; }

private:
    std::vector<FaceMeshVertex> m_vertices;
    std::vector<uint32_t> m_indices;
};
