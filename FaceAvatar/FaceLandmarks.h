#pragma once

#include <array>

#include <glm/glm.hpp>

// 68 facial landmark points in the classic iBUG 300-W layout (what dlib's
// shape_predictor_68_face_landmarks model produces), grouped by feature so
// callers don't need to memorize raw point indices — see
// FaceAvatar/FaceLandmarkDetector.h for how these get filled in, and
// FaceAvatar/FaceMesh.h for how they map into mesh/UV space. Coordinates
// are pixel positions in the source image.
//
// Groups are named by where they sit in the image (Left/Right = image
// space), not by anatomical left/right eye, since the iBUG numbering
// convention for that is a frequent source of off-by-one-side mixups and
// nothing here needs the anatomical distinction.
struct FaceLandmarks
{
    bool valid = false; // false if no face was detected, or the model failed to load

    // Bounding box of the whole detected face (the detector's rectangle,
    // not a hull of the points below), in source-image pixels.
    glm::vec2 faceBoundsMin{0.0f};
    glm::vec2 faceBoundsMax{0.0f};

    std::array<glm::vec2, 17> jaw{};         // points 0-16
    std::array<glm::vec2, 5> eyebrowLeft{};  // 17-21
    std::array<glm::vec2, 5> eyebrowRight{}; // 22-26
    std::array<glm::vec2, 9> nose{};         // 27-35
    std::array<glm::vec2, 6> eyeLeft{};      // 36-41
    std::array<glm::vec2, 6> eyeRight{};     // 42-47
    std::array<glm::vec2, 12> mouthOuter{};  // 48-59
    std::array<glm::vec2, 8> mouthInner{};   // 60-67
};
