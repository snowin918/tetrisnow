#include "Engine/OpenGLLoader.h"

#define GLFW_INCLUDE_NONE // we provide our own GL declarations; don't let GLFW pull in its own.
#include <GLFW/glfw3.h>

#include <cstdio>

PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;

PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;
PFNGLUNIFORM4FVPROC glUniform4fv = nullptr;
PFNGLUNIFORM1IPROC glUniform1i = nullptr;

PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;

PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;

namespace
{
template <typename FunctionPointer>
bool loadOne(FunctionPointer& outFunction, const char* name)
{
    outFunction = reinterpret_cast<FunctionPointer>(glfwGetProcAddress(name));
    if (outFunction == nullptr) {
        std::fprintf(stderr, "loadOpenGLFunctions: failed to resolve %s\n", name);
        return false;
    }
    return true;
}
} // namespace

bool loadOpenGLFunctions()
{
    bool ok = true;

    ok &= loadOne(glCreateShader, "glCreateShader");
    ok &= loadOne(glShaderSource, "glShaderSource");
    ok &= loadOne(glCompileShader, "glCompileShader");
    ok &= loadOne(glGetShaderiv, "glGetShaderiv");
    ok &= loadOne(glGetShaderInfoLog, "glGetShaderInfoLog");
    ok &= loadOne(glDeleteShader, "glDeleteShader");
    ok &= loadOne(glCreateProgram, "glCreateProgram");
    ok &= loadOne(glAttachShader, "glAttachShader");
    ok &= loadOne(glLinkProgram, "glLinkProgram");
    ok &= loadOne(glGetProgramiv, "glGetProgramiv");
    ok &= loadOne(glGetProgramInfoLog, "glGetProgramInfoLog");
    ok &= loadOne(glUseProgram, "glUseProgram");
    ok &= loadOne(glDeleteProgram, "glDeleteProgram");

    ok &= loadOne(glGetUniformLocation, "glGetUniformLocation");
    ok &= loadOne(glUniformMatrix4fv, "glUniformMatrix4fv");
    ok &= loadOne(glUniform4fv, "glUniform4fv");
    ok &= loadOne(glUniform1i, "glUniform1i");

    ok &= loadOne(glGenVertexArrays, "glGenVertexArrays");
    ok &= loadOne(glBindVertexArray, "glBindVertexArray");
    ok &= loadOne(glDeleteVertexArrays, "glDeleteVertexArrays");
    ok &= loadOne(glGenBuffers, "glGenBuffers");
    ok &= loadOne(glBindBuffer, "glBindBuffer");
    ok &= loadOne(glBufferData, "glBufferData");
    ok &= loadOne(glDeleteBuffers, "glDeleteBuffers");
    ok &= loadOne(glVertexAttribPointer, "glVertexAttribPointer");
    ok &= loadOne(glEnableVertexAttribArray, "glEnableVertexAttribArray");

    ok &= loadOne(glActiveTexture, "glActiveTexture");

    return ok;
}
