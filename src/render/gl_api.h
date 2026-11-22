#ifndef GF_RENDER_GL_API_H
#define GF_RENDER_GL_API_H

#include <stddef.h>
#include <stdint.h>

/* Portable GL constants matching Desktop OpenGL 3.3 Core and GLES 3.0 / WebGL2 */
#define GF_GL_DEPTH_BUFFER_BIT   0x00000100u
#define GF_GL_COLOR_BUFFER_BIT   0x00004000u
#define GF_GL_TRIANGLES          0x0004u
#define GF_GL_SRC_ALPHA          0x0302u
#define GF_GL_ONE_MINUS_SRC_ALPHA 0x0303u
#define GF_GL_DEPTH_TEST         0x0B71u
#define GF_GL_BLEND              0x0BE2u
#define GF_GL_TEXTURE_2D         0x0DE1u
#define GF_GL_UNSIGNED_BYTE      0x1401u
#define GF_GL_UNSIGNED_INT       0x1405u
#define GF_GL_FLOAT              0x1406u
#define GF_GL_RGBA               0x1908u
#define GF_GL_NEAREST            0x2600
#define GF_GL_CLAMP_TO_EDGE      0x812F
#define GF_GL_TEXTURE_MAG_FILTER 0x2800u
#define GF_GL_TEXTURE_MIN_FILTER 0x2801u
#define GF_GL_TEXTURE_WRAP_S     0x2802u
#define GF_GL_TEXTURE_WRAP_T     0x2803u
#define GF_GL_TEXTURE0           0x84C0u
#define GF_GL_ARRAY_BUFFER       0x8892u
#define GF_GL_ELEMENT_ARRAY_BUFFER 0x8893u
#define GF_GL_STATIC_DRAW        0x88E4u
#define GF_GL_DYNAMIC_DRAW       0x88E8u
#define GF_GL_FRAGMENT_SHADER    0x8B30u
#define GF_GL_VERTEX_SHADER      0x8B31u
#define GF_GL_COMPILE_STATUS     0x8B81u
#define GF_GL_LINK_STATUS        0x8B82u
#define GF_GL_FRAMEBUFFER        0x8D40u
#define GF_GL_COLOR_ATTACHMENT0  0x8CE0u

typedef uint32_t GFGLuint;
typedef int32_t  GFGLint;
typedef uint32_t GFGLenum;
typedef int32_t  GFGLsizei;
typedef ptrdiff_t GFGLsizeiptr;
typedef uint8_t  GFGLboolean;

#if defined(_WIN32)
#define GF_APIENTRY __stdcall
#else
#define GF_APIENTRY
#endif

typedef struct GFGlApi {
    GFGLuint (GF_APIENTRY *CreateShader)(GFGLenum type);
    void     (GF_APIENTRY *ShaderSource)(GFGLuint shader, GFGLsizei count, const char *const *str, const GFGLint *len);
    void     (GF_APIENTRY *CompileShader)(GFGLuint shader);
    void     (GF_APIENTRY *GetShaderiv)(GFGLuint shader, GFGLenum pname, GFGLint *params);
    void     (GF_APIENTRY *GetShaderInfoLog)(GFGLuint shader, GFGLsizei maxLen, GFGLsizei *len, char *log);
    void     (GF_APIENTRY *DeleteShader)(GFGLuint shader);

    GFGLuint (GF_APIENTRY *CreateProgram)(void);
    void     (GF_APIENTRY *AttachShader)(GFGLuint prog, GFGLuint shader);
    void     (GF_APIENTRY *LinkProgram)(GFGLuint prog);
    void     (GF_APIENTRY *GetProgramiv)(GFGLuint prog, GFGLenum pname, GFGLint *params);
    void     (GF_APIENTRY *GetProgramInfoLog)(GFGLuint prog, GFGLsizei maxLen, GFGLsizei *len, char *log);
    void     (GF_APIENTRY *DeleteProgram)(GFGLuint prog);
    void     (GF_APIENTRY *UseProgram)(GFGLuint prog);
    GFGLint  (GF_APIENTRY *GetUniformLocation)(GFGLuint prog, const char *name);

    void     (GF_APIENTRY *Uniform1f)(GFGLint loc, float v0);
    void     (GF_APIENTRY *Uniform1i)(GFGLint loc, GFGLint v0);
    void     (GF_APIENTRY *Uniform2f)(GFGLint loc, float v0, float v1);
    void     (GF_APIENTRY *Uniform3f)(GFGLint loc, float v0, float v1, float v2);
    void     (GF_APIENTRY *Uniform4f)(GFGLint loc, float v0, float v1, float v2, float v3);
    void     (GF_APIENTRY *UniformMatrix4fv)(GFGLint loc, GFGLsizei count, GFGLboolean transpose, const float *val);

    void     (GF_APIENTRY *GenVertexArrays)(GFGLsizei n, GFGLuint *arrays);
    void     (GF_APIENTRY *BindVertexArray)(GFGLuint array);
    void     (GF_APIENTRY *DeleteVertexArrays)(GFGLsizei n, const GFGLuint *arrays);

    void     (GF_APIENTRY *GenBuffers)(GFGLsizei n, GFGLuint *buffers);
    void     (GF_APIENTRY *BindBuffer)(GFGLenum target, GFGLuint buffer);
    void     (GF_APIENTRY *BufferData)(GFGLenum target, GFGLsizeiptr size, const void *data, GFGLenum usage);
    void     (GF_APIENTRY *DeleteBuffers)(GFGLsizei n, const GFGLuint *buffers);
    void     (GF_APIENTRY *EnableVertexAttribArray)(GFGLuint idx);
    void     (GF_APIENTRY *VertexAttribPointer)(GFGLuint idx, GFGLint size, GFGLenum type, GFGLboolean norm, GFGLsizei stride, const void *ptr);

    void     (GF_APIENTRY *GenTextures)(GFGLsizei n, GFGLuint *textures);
    void     (GF_APIENTRY *BindTexture)(GFGLenum target, GFGLuint texture);
    void     (GF_APIENTRY *TexImage2D)(GFGLenum target, GFGLint level, GFGLint internalformat, GFGLsizei w, GFGLsizei h, GFGLint border, GFGLenum fmt, GFGLenum type, const void *data);
    void     (GF_APIENTRY *TexParameteri)(GFGLenum target, GFGLenum pname, GFGLint param);
    void     (GF_APIENTRY *DeleteTextures)(GFGLsizei n, const GFGLuint *textures);
    void     (GF_APIENTRY *ActiveTexture)(GFGLenum texture);

    void     (GF_APIENTRY *Viewport)(GFGLint x, GFGLint y, GFGLsizei w, GFGLsizei h);
    void     (GF_APIENTRY *Enable)(GFGLenum cap);
    void     (GF_APIENTRY *Disable)(GFGLenum cap);
    void     (GF_APIENTRY *BlendFunc)(GFGLenum sfactor, GFGLenum dfactor);
    void     (GF_APIENTRY *ClearColor)(float r, float g, float b, float a);
    void     (GF_APIENTRY *Clear)(GFGLenum mask);
    void     (GF_APIENTRY *DrawElements)(GFGLenum mode, GFGLsizei count, GFGLenum type, const void *indices);
    void     (GF_APIENTRY *DrawArrays)(GFGLenum mode, GFGLint first, GFGLsizei count);
    void     (GF_APIENTRY *ReadPixels)(GFGLint x, GFGLint y, GFGLsizei w, GFGLsizei h, GFGLenum fmt, GFGLenum type, void *pixels);
} GFGlApi;

#endif
