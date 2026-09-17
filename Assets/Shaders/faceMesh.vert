#version 330 core

// Attribute layout matches FaceAvatar/FaceMesh.h's FaceMeshVertex exactly.
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in float aRegionId;
layout(location = 3) in vec2 aLocalOffset;
layout(location = 4) in float aRegionWeight;

uniform mat4 uProjection;
uniform mat4 uModel;

// Phase 2's four deformation uniforms, driven later by
// FaceExpressionController (Phase 3+): 1.0/0.0/0.0/0.0 is the neutral pose.
uniform float uEyeScale;     // vertical scale of each eye region about its own center; 1.0 = neutral
uniform float uMouthOpen;    // additional vertical mouth separation (mesh units); 0.0 = neutral
uniform float uBrowPosition; // eyebrow vertical shift (mesh units), +up/-down; 0.0 = neutral
uniform float uFaceRotation; // whole-mesh rotation about its center, radians

out vec2 vUV;

const float REGION_EYE = 1.0;
const float REGION_EYEBROW = 2.0;
const float REGION_MOUTH = 3.0;

void main()
{
    vec2 pos = aPos;

    if (aRegionId == REGION_EYE) {
        // Scale the eye region vertically about its own center: >1 opens
        // wide (surprise), <1 narrows (squint/happy).
        pos.y += aLocalOffset.y * (uEyeScale - 1.0) * aRegionWeight;
    } else if (aRegionId == REGION_EYEBROW) {
        // Eyebrows move together as a rigid shift, not a scale.
        pos.y += uBrowPosition * aRegionWeight;
    } else if (aRegionId == REGION_MOUTH) {
        // Push the upper half up and lower half down (or the reverse for
        // negative values), proportional to distance from the mouth's
        // own vertical center.
        pos.y += aLocalOffset.y * uMouthOpen * aRegionWeight;
    }

    float c = cos(uFaceRotation);
    float s = sin(uFaceRotation);
    pos = mat2(c, s, -s, c) * pos;

    vUV = aUV;
    gl_Position = uProjection * uModel * vec4(pos, 0.0, 1.0);
}
