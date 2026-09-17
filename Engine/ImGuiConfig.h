#pragma once

// Dear ImGui's imgui.h includes whatever IMGUI_USER_CONFIG points at (set
// via a CMake compile definition) before anything else. Its OpenGL3
// backend would otherwise bundle its own gl3w-derived loader, which
// declares globals under the exact same names ours already does (e.g.
// `glGenBuffers`) — linking both would fail with duplicate symbols. This
// tells it to skip that and rely on declarations already visible in the
// translation unit, which OpenGLLoader.h below provides (plus the small
// set of extra functions/enums ImGui itself needs — see the bottom of
// that file).
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include "Engine/OpenGLLoader.h"
