#include "FaceAvatar/FaceLandmarkDetector.h"

#include <algorithm>
#include <cstdio>
#include <exception>
#include <vector>

#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>

struct FaceLandmarkDetector::Impl
{
    dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();
    dlib::shape_predictor shapePredictor;
};

FaceLandmarkDetector::FaceLandmarkDetector(std::string modelPath)
    : m_modelPath(std::move(modelPath))
    , m_impl(std::make_unique<Impl>())
{
}

FaceLandmarkDetector::~FaceLandmarkDetector() = default;

bool FaceLandmarkDetector::ensureModelLoaded()
{
    if (m_modelLoadAttempted) {
        return m_modelLoaded;
    }
    m_modelLoadAttempted = true;

    try {
        dlib::deserialize(m_modelPath) >> m_impl->shapePredictor;
        m_modelLoaded = true;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FaceLandmarkDetector: failed to load model '%s': %s\n", m_modelPath.c_str(), e.what());
        m_modelLoaded = false;
    }
    return m_modelLoaded;
}

FaceLandmarks FaceLandmarkDetector::detect(const unsigned char* rgbaPixels, int width, int height)
{
    FaceLandmarks result;
    if (!ensureModelLoaded()) {
        return result;
    }

    dlib::array2d<dlib::rgb_pixel> image;
    image.set_size(height, width);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const unsigned char* px = rgbaPixels + (static_cast<size_t>(y) * width + x) * 4;
            image[y][x] = dlib::rgb_pixel(px[0], px[1], px[2]);
        }
    }

    const std::vector<dlib::rectangle> detections = m_impl->detector(image);
    if (detections.empty()) {
        return result;
    }

    // Largest detected face by area, in case more than one face is in frame.
    const dlib::rectangle& faceRect = *std::max_element(
        detections.begin(), detections.end(),
        [](const dlib::rectangle& a, const dlib::rectangle& b) { return a.area() < b.area(); });

    const dlib::full_object_detection shape = m_impl->shapePredictor(image, faceRect);
    if (shape.num_parts() != 68) {
        return result;
    }

    const auto pt = [&shape](unsigned long i) {
        return glm::vec2(static_cast<float>(shape.part(i).x()), static_cast<float>(shape.part(i).y()));
    };

    for (int i = 0; i < 17; ++i) {
        result.jaw[static_cast<size_t>(i)] = pt(static_cast<unsigned long>(i));
    }
    for (int i = 0; i < 5; ++i) {
        result.eyebrowLeft[static_cast<size_t>(i)] = pt(static_cast<unsigned long>(17 + i));
    }
    for (int i = 0; i < 5; ++i) {
        result.eyebrowRight[static_cast<size_t>(i)] = pt(static_cast<unsigned long>(22 + i));
    }
    for (int i = 0; i < 9; ++i) {
        result.nose[static_cast<size_t>(i)] = pt(static_cast<unsigned long>(27 + i));
    }
    for (int i = 0; i < 6; ++i) {
        result.eyeLeft[static_cast<size_t>(i)] = pt(static_cast<unsigned long>(36 + i));
    }
    for (int i = 0; i < 6; ++i) {
        result.eyeRight[static_cast<size_t>(i)] = pt(static_cast<unsigned long>(42 + i));
    }
    for (int i = 0; i < 12; ++i) {
        result.mouthOuter[static_cast<size_t>(i)] = pt(static_cast<unsigned long>(48 + i));
    }
    for (int i = 0; i < 8; ++i) {
        result.mouthInner[static_cast<size_t>(i)] = pt(static_cast<unsigned long>(60 + i));
    }

    result.faceBoundsMin = glm::vec2(static_cast<float>(faceRect.left()), static_cast<float>(faceRect.top()));
    result.faceBoundsMax = glm::vec2(static_cast<float>(faceRect.right()), static_cast<float>(faceRect.bottom()));
    result.valid = true;
    return result;
}
