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

// The functions below aren't called by our own Renderer — they're loaded
// so Dear ImGui's OpenGL3 backend (Milestone 7) can use this same loader
// instead of bundling its own (which would redeclare globals like
// `glGenBuffers` under the exact same names ours already uses, causing
// duplicate-symbol link errors). See Engine/ImGuiConfig.h, which sets
// IMGUI_IMPL_OPENGL_LOADER_CUSTOM and includes this header in its place.
using PFNGLBLENDEQUATIONPROC = void(APIENTRY*)(GLenum mode);
using PFNGLBLENDEQUATIONSEPARATEPROC = void(APIENTRY*)(GLenum modeRGB, GLenum modeAlpha);
using PFNGLBLENDFUNCSEPARATEPROC =
    void(APIENTRY*)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
using PFNGLDETACHSHADERPROC = void(APIENTRY*)(GLuint program, GLuint shader);
using PFNGLGETATTRIBLOCATIONPROC = GLint(APIENTRY*)(GLuint program, const GLchar* name);
using PFNGLISPROGRAMPROC = GLboolean(APIENTRY*)(GLuint program);
using PFNGLGETSTRINGIPROC = const GLubyte*(APIENTRY*)(GLenum name, GLuint index);
using PFNGLBUFFERSUBDATAPROC = void(APIENTRY*)(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);

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

extern PFNGLBLENDEQUATIONPROC          glBlendEquation;
extern PFNGLBLENDEQUATIONSEPARATEPROC  glBlendEquationSeparate;
extern PFNGLBLENDFUNCSEPARATEPROC      glBlendFuncSeparate;
extern PFNGLDETACHSHADERPROC           glDetachShader;
extern PFNGLGETATTRIBLOCATIONPROC      glGetAttribLocation;
extern PFNGLISPROGRAMPROC              glIsProgram;
extern PFNGLGETSTRINGIPROC             glGetStringi;
extern PFNGLBUFFERSUBDATAPROC          glBufferSubData;
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

// GL 1.3+/1.4+/1.5+/2.0+/3.0+ enums <GL/gl.h> doesn't define, needed only by
// Dear ImGui's OpenGL3 backend (see the loader additions above).
#ifndef GL_ACTIVE_TEXTURE
#define GL_ACTIVE_TEXTURE 0x84E0
#endif
#ifndef GL_ARRAY_BUFFER_BINDING
#define GL_ARRAY_BUFFER_BINDING 0x8894
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STREAM_DRAW
#define GL_STREAM_DRAW 0x88E0
#endif
#ifndef GL_CURRENT_PROGRAM
#define GL_CURRENT_PROGRAM 0x8B8D
#endif
#ifndef GL_FUNC_ADD
#define GL_FUNC_ADD 0x8006
#endif
#ifndef GL_BLEND_EQUATION_RGB
#define GL_BLEND_EQUATION_RGB 0x8009
#endif
#ifndef GL_BLEND_EQUATION_ALPHA
#define GL_BLEND_EQUATION_ALPHA 0x883D
#endif
#ifndef GL_BLEND_SRC_RGB
#define GL_BLEND_SRC_RGB 0x80C9
#endif
#ifndef GL_BLEND_DST_RGB
#define GL_BLEND_DST_RGB 0x80C8
#endif
#ifndef GL_BLEND_SRC_ALPHA
#define GL_BLEND_SRC_ALPHA 0x80CB
#endif
#ifndef GL_BLEND_DST_ALPHA
#define GL_BLEND_DST_ALPHA 0x80CA
#endif
#ifndef GL_MAJOR_VERSION
#define GL_MAJOR_VERSION 0x821B
#endif
#ifndef GL_MINOR_VERSION
#define GL_MINOR_VERSION 0x821C
#endif
#ifndef GL_NUM_EXTENSIONS
#define GL_NUM_EXTENSIONS 0x821D
#endif
#ifndef GL_CONTEXT_PROFILE_MASK
#define GL_CONTEXT_PROFILE_MASK 0x9126
#endif
#ifndef GL_CONTEXT_COMPATIBILITY_PROFILE_BIT
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER_BINDING
#define GL_PIXEL_UNPACK_BUFFER_BINDING 0x88ED
#endif
#ifndef GL_VERTEX_ARRAY_BINDING
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#endif
