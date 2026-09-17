#include "FaceAvatar/FaceMesh.h"

#include <algorithm>
#include <cmath>

namespace
{
struct Bounds
{
    glm::vec2 min{1e9f, 1e9f};
    glm::vec2 max{-1e9f, -1e9f};

    void include(glm::vec2 p)
    {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }

    glm::vec2 center() const { return (min + max) * 0.5f; }
    glm::vec2 halfExtent() const { return (max - min) * 0.5f; }
};

template <typename... Arrays>
Bounds boundsOf(const Arrays&... arrays)
{
    Bounds b;
    (
        [&b](const auto& arr) {
            for (const glm::vec2& p : arr) {
                b.include(p);
            }
        }(arrays),
        ...);
    return b;
}

// One named facial-feature region: where it sits (in mesh-local centered
// space, already run through imageToMeshLocal), how far the deformation
// falloff extends past its tight bounding box, and which region tag/
// uniform it drives.
struct RegionShape
{
    FaceMeshRegion id = FaceMeshRegion::None;
    glm::vec2 center{0.0f};
    glm::vec2 innerHalfExtent{0.0f}; // full weight within this box
    glm::vec2 outerHalfExtent{0.0f}; // weight fades to 0 by this box's edge
};

float axisFade(float distance, float innerHalf, float outerHalf)
{
    const float absDist = std::fabs(distance);
    if (absDist <= innerHalf) {
        return 1.0f;
    }
    if (absDist >= outerHalf || outerHalf <= innerHalf) {
        return 0.0f;
    }
    const float t = (absDist - innerHalf) / (outerHalf - innerHalf);
    return 1.0f - t;
}

// "inner" is padded slightly beyond the raw landmark points (so the
// feature's full range of motion, e.g. a wide-open eye, still reads as
// "fully in region"); "outer" extends further still, over which the
// deformation feathers out to 0 so it blends into the surrounding static
// mesh instead of tearing at the region boundary.
RegionShape makeRegion(FaceMeshRegion id, const Bounds& tight)
{
    RegionShape region;
    region.id = id;
    region.center = tight.center();
    region.innerHalfExtent = tight.halfExtent() * 1.15f;
    region.outerHalfExtent = tight.halfExtent() * 1.8f + glm::vec2(0.02f);
    return region;
}
} // namespace

FaceMesh FaceMesh::build(const FaceLandmarks& landmarks, int imageWidth, int imageHeight, int gridResolution)
{
    FaceMesh mesh;
    gridResolution = std::max(gridResolution, 2);

    const glm::vec2 imageSize(static_cast<float>(imageWidth), static_cast<float>(imageHeight));

    // 1. Pick a square crop region of the source photo, in image pixels.
    glm::vec2 cropMin;
    glm::vec2 cropMax;
    if (landmarks.valid) {
        const glm::vec2 faceCenter = (landmarks.faceBoundsMin + landmarks.faceBoundsMax) * 0.5f;
        const glm::vec2 faceSize = landmarks.faceBoundsMax - landmarks.faceBoundsMin;
        // Pad generously beyond the raw detector box: enough headroom
        // above for forehead/hair and below for chin/shoulders, matching
        // the brief's head-and-shoulders portrait mockup rather than a
        // tight face crop.
        const float squareSize = std::max(faceSize.x, faceSize.y) * 1.9f;
        cropMin = faceCenter - glm::vec2(squareSize * 0.5f);
        cropMax = faceCenter + glm::vec2(squareSize * 0.5f);
    } else {
        // No detection: fall back to a blind center-square crop of the
        // whole image (Phase 1's behavior), so an undetectable photo
        // still renders as a plain, non-deformable portrait instead of
        // nothing.
        const float squareSize = std::min(imageSize.x, imageSize.y);
        const glm::vec2 imageCenter = imageSize * 0.5f;
        cropMin = imageCenter - glm::vec2(squareSize * 0.5f);
        cropMax = imageCenter + glm::vec2(squareSize * 0.5f);
    }

    // Clamp into the image and re-square around the clamped box's center,
    // so a face near an edge shrinks the crop instead of stretching it.
    {
        const float left = std::max(cropMin.x, 0.0f);
        const float top = std::max(cropMin.y, 0.0f);
        const float right = std::min(cropMax.x, imageSize.x);
        const float bottom = std::min(cropMax.y, imageSize.y);
        const float clampedSize = std::max(1.0f, std::min(right - left, bottom - top));
        const glm::vec2 clampedCenter((left + right) * 0.5f, (top + bottom) * 0.5f);
        cropMin = clampedCenter - glm::vec2(clampedSize * 0.5f);
        cropMax = clampedCenter + glm::vec2(clampedSize * 0.5f);
    }

    const glm::vec2 cropSize = cropMax - cropMin;
    const glm::vec2 uvMin = cropMin / imageSize;
    const glm::vec2 uvMax = cropMax / imageSize;

    // Maps an image-pixel point into mesh-local centered space ([-0.5,
    // 0.5] within the crop square) — used both for grid vertices and for
    // placing landmark-derived region shapes in that same space.
    const auto imageToMeshLocal = [&](glm::vec2 imagePt) { return (imagePt - cropMin) / cropSize - glm::vec2(0.5f); };

    // 2. Build the named deformation regions from landmarks, in mesh-local space.
    std::vector<RegionShape> regions;
    if (landmarks.valid) {
        const auto localBounds = [&](const Bounds& boundsInImageSpace) {
            Bounds local;
            local.include(imageToMeshLocal(boundsInImageSpace.min));
            local.include(imageToMeshLocal(boundsInImageSpace.max));
            return local;
        };

        regions.push_back(makeRegion(FaceMeshRegion::Eye, localBounds(boundsOf(landmarks.eyeLeft))));
        regions.push_back(makeRegion(FaceMeshRegion::Eye, localBounds(boundsOf(landmarks.eyeRight))));
        regions.push_back(makeRegion(FaceMeshRegion::Eyebrow, localBounds(boundsOf(landmarks.eyebrowLeft))));
        regions.push_back(makeRegion(FaceMeshRegion::Eyebrow, localBounds(boundsOf(landmarks.eyebrowRight))));
        regions.push_back(
            makeRegion(FaceMeshRegion::Mouth, localBounds(boundsOf(landmarks.mouthOuter, landmarks.mouthInner))));
    }

    // 3. Generate the grid.
    mesh.m_vertices.reserve(static_cast<size_t>(gridResolution) * static_cast<size_t>(gridResolution));
    for (int row = 0; row < gridResolution; ++row) {
        const float v = static_cast<float>(row) / static_cast<float>(gridResolution - 1);
        for (int col = 0; col < gridResolution; ++col) {
            const float u = static_cast<float>(col) / static_cast<float>(gridResolution - 1);

            FaceMeshVertex vertex;
            vertex.position = glm::vec2(u, v) - glm::vec2(0.5f);
            vertex.uv = uvMin + glm::vec2(u, v) * (uvMax - uvMin);

            float bestWeight = 0.0f;
            const RegionShape* bestRegion = nullptr;
            for (const RegionShape& region : regions) {
                const glm::vec2 fromCenter = vertex.position - region.center;
                const float weight = axisFade(fromCenter.x, region.innerHalfExtent.x, region.outerHalfExtent.x)
                    * axisFade(fromCenter.y, region.innerHalfExtent.y, region.outerHalfExtent.y);
                if (weight > bestWeight) {
                    bestWeight = weight;
                    bestRegion = &region;
                }
            }

            if (bestRegion != nullptr) {
                vertex.regionId = static_cast<float>(static_cast<int>(bestRegion->id));
                vertex.localOffset = vertex.position - bestRegion->center;
                vertex.regionWeight = bestWeight;
            }

            mesh.m_vertices.push_back(vertex);
        }
    }

    // 4. Two triangles per grid cell.
    mesh.m_indices.reserve(static_cast<size_t>(gridResolution - 1) * static_cast<size_t>(gridResolution - 1) * 6);
    for (int row = 0; row < gridResolution - 1; ++row) {
        for (int col = 0; col < gridResolution - 1; ++col) {
            const uint32_t topLeft = static_cast<uint32_t>(row * gridResolution + col);
            const uint32_t topRight = topLeft + 1;
            const uint32_t bottomLeft = static_cast<uint32_t>((row + 1) * gridResolution + col);
            const uint32_t bottomRight = bottomLeft + 1;

            mesh.m_indices.push_back(topLeft);
            mesh.m_indices.push_back(bottomLeft);
            mesh.m_indices.push_back(topRight);

            mesh.m_indices.push_back(topRight);
            mesh.m_indices.push_back(bottomLeft);
            mesh.m_indices.push_back(bottomRight);
        }
    }

    return mesh;
}
