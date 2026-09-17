#pragma once

// Minimal OpenGL 3.3 core function loader.
//
// On Windows, opengl32.dll only statically exports up through OpenGL 1.1
// (glClear, glViewport, glEnable, glTexImage2D, ...). Everything this
// project uses beyond that — shaders, VAOs/VBOs, uniforms — is OpenGL 1.2+
// and must be resolved at runtime from the driver via the current context.
//
// Rather than pull in GLEW or GLAD for the ~25 functions we actually call,
// this declares and loads exactly those, keeping the dependency list at
// just GLFW (for glfwGetProcAddress) and the platform's own <GL/gl.h>.
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // required by <GL/gl.h> for APIENTRY/WINGDIAPI
#endif

#include <GL/gl.h>

using GLchar = char;
using GLsizeiptr = ptrdiff_t;
using GLintptr = ptrdiff_t;

// Shaders
using PFNGLCREATESHADERPROC = GLuint(APIENTRY*)(GLenum type);
using PFNGLSHADERSOURCEPROC = void(APIENTRY*)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
using PFNGLCOMPILESHADERPROC = void(APIENTRY*)(GLuint shader);
using PFNGLGETSHADERIVPROC = void(APIENTRY*)(GLuint shader, GLenum pname, GLint* params);
using PFNGLGETSHADERINFOLOGPROC = void(APIENTRY*)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
using PFNGLDELETESHADERPROC = void(APIENTRY*)(GLuint shader);
using PFNGLCREATEPROGRAMPROC = GLuint(APIENTRY*)();
using PFNGLATTACHSHADERPROC = void(APIENTRY*)(GLuint program, GLuint shader);
using PFNGLLINKPROGRAMPROC = void(APIENTRY*)(GLuint program);
using PFNGLGETPROGRAMIVPROC = void(APIENTRY*)(GLuint program, GLenum pname, GLint* params);
using PFNGLGETPROGRAMINFOLOGPROC = void(APIENTRY*)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
using PFNGLUSEPROGRAMPROC = void(APIENTRY*)(GLuint program);
using PFNGLDELETEPROGRAMPROC = void(APIENTRY*)(GLuint program);

// Uniforms
using PFNGLGETUNIFORMLOCATIONPROC = GLint(APIENTRY*)(GLuint program, const GLchar* name);
using PFNGLUNIFORMMATRIX4FVPROC = void(APIENTRY*)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
using PFNGLUNIFORM4FVPROC = void(APIENTRY*)(GLint location, GLsizei count, const GLfloat* value);
using PFNGLUNIFORM1IPROC = void(APIENTRY*)(GLint location, GLint v0);

// Vertex arrays / buffers
using PFNGLGENVERTEXARRAYSPROC = void(APIENTRY*)(GLsizei n, GLuint* arrays);
using PFNGLBINDVERTEXARRAYPROC = void(APIENTRY*)(GLuint array);
using PFNGLDELETEVERTEXARRAYSPROC = void(APIENTRY*)(GLsizei n, const GLuint* arrays);
using PFNGLGENBUFFERSPROC = void(APIENTRY*)(GLsizei n, GLuint* buffers);
using PFNGLBINDBUFFERPROC = void(APIENTRY*)(GLenum target, GLuint buffer);
using PFNGLBUFFERDATAPROC = void(APIENTRY*)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
using PFNGLDELETEBUFFERSPROC = void(APIENTRY*)(GLsizei n, const GLuint* buffers);
using PFNGLVERTEXATTRIBPOINTERPROC =
    void(APIENTRY*)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
using PFNGLENABLEVERTEXATTRIBARRAYPROC = void(APIENTRY*)(GLuint index);

// Textures (GL_TEXTURE1.. requires runtime loading; unit 0 does not, but we
// load it for symmetry with the rest of the texture API used here).
using PFNGLACTIVETEXTUREPROC = void(APIENTRY*)(GLenum texture);

// clang-format off
extern PFNGLCREATESHADERPROC       glCreateShader;
extern PFNGLSHADERSOURCEPROC       glShaderSource;
extern PFNGLCOMPILESHADERPROC      glCompileShader;
extern PFNGLGETSHADERIVPROC        glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC   glGetShaderInfoLog;
extern PFNGLDELETESHADERPROC       glDeleteShader;
extern PFNGLCREATEPROGRAMPROC      glCreateProgram;
extern PFNGLATTACHSHADERPROC       glAttachShader;
extern PFNGLLINKPROGRAMPROC        glLinkProgram;
extern PFNGLGETPROGRAMIVPROC       glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC  glGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC         glUseProgram;
extern PFNGLDELETEPROGRAMPROC      glDeleteProgram;

extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORMMATRIX4FVPROC   glUniformMatrix4fv;
extern PFNGLUNIFORM4FVPROC         glUniform4fv;
extern PFNGLUNIFORM1IPROC          glUniform1i;

extern PFNGLGENVERTEXARRAYSPROC    glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC    glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
extern PFNGLGENBUFFERSPROC         glGenBuffers;
extern PFNGLBINDBUFFERPROC         glBindBuffer;
extern PFNGLBUFFERDATAPROC         glBufferData;
extern PFNGLDELETEBUFFERSPROC      glDeleteBuffers;
extern PFNGLVERTEXATTRIBPOINTERPROC     glVertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;

extern PFNGLACTIVETEXTUREPROC      glActiveTexture;
// clang-format on

// Resolves every function above via glfwGetProcAddress. Must be called once,
// after glfwMakeContextCurrent(). Returns false (and logs which symbol) if
// any function isn't available from the driver.
bool loadOpenGLFunctions();

// GL 1.2+ enum used by glBufferData/etc. that <GL/gl.h> (a GL 1.1 header)
// doesn't define.
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
