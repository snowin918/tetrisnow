#pragma once

#include <memory>
#include <string>

#include "FaceAvatar/FaceLandmarks.h"

// Wraps dlib's HOG face detector + 68-point shape predictor — the classical
// CV pipeline (face detection -> facial landmarks) Phase 2 of the brief
// calls for. dlib alone covers both halves of the brief's "OpenCV + Dlib"
// alternative (detection and landmark prediction are both dlib APIs); we
// don't also add OpenCV since its only role here would have been image
// I/O, which stb_image already handles.
//
// dlib's own heavy template headers are kept out of this header (pimpl),
// so dlib is an implementation detail contained entirely in
// FaceLandmarkDetector.cpp — nothing else in the codebase needs its
// include paths, and a different backend (e.g. MediaPipe) could replace
// this one class without touching FaceMesh or FaceAvatarSystem.
class FaceLandmarkDetector
{
public:
    // modelPath: path to shape_predictor_68_face_landmarks.dat. Loading is
    // lazy (on first detect() call) since the model is a ~95MB file and
    // not every run of the game necessarily needs it loaded immediately.
    explicit FaceLandmarkDetector(std::string modelPath);
    ~FaceLandmarkDetector();

    FaceLandmarkDetector(const FaceLandmarkDetector&) = delete;
    FaceLandmarkDetector& operator=(const FaceLandmarkDetector&) = delete;

    // Runs detection on an RGBA8 pixel buffer (as produced by stb_image).
    // Returns landmarks for the largest detected face, or a FaceLandmarks
    // with valid=false if none was found or the model failed to load —
    // callers should treat that as "no landmarks available" and fall back
    // to a non-deformable portrait rather than an error.
    FaceLandmarks detect(const unsigned char* rgbaPixels, int width, int height);

private:
    bool ensureModelLoaded();

    std::string m_modelPath;
    bool m_modelLoadAttempted = false;
    bool m_modelLoaded = false;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
