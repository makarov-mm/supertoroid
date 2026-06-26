// supertoroid.cpp
// Supertoroid renderer — C++, WinAPI, WGL, OpenGL 4.3 core
// Zero external dependencies. Single file.
//
// Parametric surface:
//   R(u,v) = (cos^n(v) + sin^n(v))^(-1/n)
//   x = [a + R*cos(t*u + v)] * cos(u)
//   y = [a + R*cos(t*u + v)] * sin(u)
//   z = R * sin(t*u + v)
//   u in [0, 2pi], v in [0, 2pi]
//
// Controls:
//   Mouse drag    — rotate
//   Scroll        — zoom
//   N / M         — decrease / increase exponent n
//   T / Y         — decrease / increase twist t
//   R             — reset
//   F11           — fullscreen
//   ESC           — exit

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <GL/gl.h>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>

using std::min;
using std::max;

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

typedef char      GLchar;
typedef ptrdiff_t GLsizeiptr;

// ---- WGL extensions ----
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);
typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC);

#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_CONTEXT_FLAGS_ARB             0x2094

// ---- GL function pointers ----
typedef GLuint(APIENTRY* PFNGLCREATESHADERPROC)(GLenum);
typedef void  (APIENTRY* PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar**, const GLint*);
typedef void  (APIENTRY* PFNGLCOMPILESHADERPROC)(GLuint);
typedef GLuint(APIENTRY* PFNGLCREATEPROGRAMPROC)();
typedef void  (APIENTRY* PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void  (APIENTRY* PFNGLLINKPROGRAMPROC)(GLuint);
typedef void  (APIENTRY* PFNGLUSEPROGRAMPROC)(GLuint);
typedef void  (APIENTRY* PFNGLDELETESHADERPROC)(GLuint);
typedef void  (APIENTRY* PFNGLDELETEPROGRAMPROC)(GLuint);
typedef void  (APIENTRY* PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void  (APIENTRY* PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void  (APIENTRY* PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void  (APIENTRY* PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef GLint(APIENTRY* PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void  (APIENTRY* PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void  (APIENTRY* PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void  (APIENTRY* PFNGLUNIFORM3FPROC)(GLint, GLfloat, GLfloat, GLfloat);
typedef void  (APIENTRY* PFNGLUNIFORMMATRIX4FVPROC)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void  (APIENTRY* PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void  (APIENTRY* PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void  (APIENTRY* PFNGLDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint*);
typedef void  (APIENTRY* PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void  (APIENTRY* PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void  (APIENTRY* PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void  (APIENTRY* PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void  (APIENTRY* PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void  (APIENTRY* PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void  (APIENTRY* PFNGLDRAWELEMENTSBASEVERTEXPROC)(GLenum, GLsizei, GLenum, const void*, GLint);
typedef void  (APIENTRY* PFNGLDEBUGMESSAGECALLBACKPROC)(void*, const void*);

#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84

static PFNGLCREATESHADERPROC            glCreateShader;
static PFNGLSHADERSOURCEPROC            glShaderSource;
static PFNGLCOMPILESHADERPROC           glCompileShader;
static PFNGLCREATEPROGRAMPROC           glCreateProgram;
static PFNGLATTACHSHADERPROC            glAttachShader;
static PFNGLLINKPROGRAMPROC             glLinkProgram;
static PFNGLUSEPROGRAMPROC              glUseProgram;
static PFNGLDELETESHADERPROC            glDeleteShader;
static PFNGLDELETEPROGRAMPROC           glDeleteProgram;
static PFNGLGETSHADERIVPROC             glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC        glGetShaderInfoLog;
static PFNGLGETPROGRAMIVPROC            glGetProgramiv;
static PFNGLGETPROGRAMINFOLOGPROC       glGetProgramInfoLog;
static PFNGLGETUNIFORMLOCATIONPROC      glGetUniformLocation;
static PFNGLUNIFORM1FPROC               glUniform1f;
static PFNGLUNIFORM1IPROC               glUniform1i;
static PFNGLUNIFORM3FPROC               glUniform3f;
static PFNGLUNIFORMMATRIX4FVPROC        glUniformMatrix4fv;
static PFNGLGENVERTEXARRAYSPROC         glGenVertexArrays;
static PFNGLBINDVERTEXARRAYPROC         glBindVertexArray;
static PFNGLDELETEVERTEXARRAYSPROC      glDeleteVertexArrays;
static PFNGLGENBUFFERSPROC              glGenBuffers;
static PFNGLBINDBUFFERPROC              glBindBuffer;
static PFNGLBUFFERDATAPROC              glBufferData;
static PFNGLDELETEBUFFERSPROC           glDeleteBuffers;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
static PFNGLVERTEXATTRIBPOINTERPROC     glVertexAttribPointer;
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
static PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT;

#define LOAD_GL(type, name) name = (type)wglGetProcAddress(#name); if(!name) { MessageBoxA(0, "Failed to load " #name, "GL Error", MB_OK); return false; }

// ---- Math ----
struct Vec3 { float x, y, z; };
struct Mat4 { float m[16]; };

static Mat4 mat4Identity() {
    Mat4 r = {};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

// Standard column-major multiply: result = a * b
// (when used as M*v in the shader, b is applied first, then a)
static Mat4 mat4Mul(const Mat4& a, const Mat4& b) {
    Mat4 r = {};
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++)
                s += a.m[k * 4 + row] * b.m[col * 4 + k];
            r.m[col * 4 + row] = s;
        }
    return r;
}

static Mat4 mat4Perspective(float fovY, float aspect, float zNear, float zFar) {
    float f = 1.0f / tanf(fovY * 0.5f);
    Mat4 r = {};
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (zFar + zNear) / (zNear - zFar);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    return r;
}

static Mat4 mat4RotX(float a) {
    Mat4 r = mat4Identity();
    r.m[5] = cosf(a); r.m[6] = sinf(a);
    r.m[9] = -sinf(a); r.m[10] = cosf(a);
    return r;
}

static Mat4 mat4RotY(float a) {
    Mat4 r = mat4Identity();
    r.m[0] = cosf(a); r.m[2] = -sinf(a);
    r.m[8] = sinf(a); r.m[10] = cosf(a);
    return r;
}

static Mat4 mat4Translate(float x, float y, float z) {
    Mat4 r = mat4Identity();
    r.m[12] = x; r.m[13] = y; r.m[14] = z;
    return r;
}

// ---- Shaders ----
static const char* VS = R"GLSL(
#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aUV;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat4 uNormalMat;

out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vUV;

void main() {
    vNormal   = normalize((uNormalMat * vec4(aNorm, 0.0)).xyz);
    vWorldPos = (uModel * vec4(aPos, 1.0)).xyz;
    vUV       = aUV;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* FS = R"GLSL(
#version 430 core
in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUV;

uniform vec3 uLightDir;
uniform float uTime;

out vec4 fragColor;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(-vWorldPos);
    vec3 H = normalize(L + V);

    // Hue from UV + time for rainbow animation
    float hue = fract(vUV.x + vUV.y * 0.3 + uTime * 0.08);
    vec3 baseColor = hsv2rgb(vec3(hue, 0.75, 1.0));

    float ambient  = 0.12;
    float diffuse  = max(dot(N, L), 0.0) * 0.65;
    float specular = pow(max(dot(N, H), 0.0), 64.0) * 0.6;

    // Rim light
    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0) * 0.3;

    vec3 col = baseColor * (ambient + diffuse) + vec3(1.0) * specular + baseColor * rim;

    // Subtle grid lines along UV
    float grid = smoothstep(0.96, 1.0, max(
        abs(sin(vUV.x * 3.14159 * 48.0)),
        abs(sin(vUV.y * 3.14159 * 48.0))
    ));
    col = mix(col, col * 0.35, grid * 0.6);

    fragColor = vec4(col, 1.0);
}
)GLSL";

// ---- Supertoroid mesh generation ----
struct Vertex { float x, y, z, nx, ny, nz, u, v; };

static void buildSupertoroid(
    float n,     // exponent (shape)
    float twist, // twist parameter t
    float a,     // major radius
    int   Nu,    // u divisions
    int   Nv,    // v divisions
    std::vector<Vertex>& verts,
    std::vector<unsigned int>& indices)
{
    verts.clear();
    indices.clear();

    float inv_n = 1.0f / n;
    float pi2 = 2.0f * 3.14159265358979f;

    // Generate vertices
    for (int iv = 0; iv <= Nv; iv++) {
        float vp = pi2 * iv / Nv;
        for (int iu = 0; iu <= Nu; iu++) {
            float up = pi2 * iu / Nu;

            // R(v) = (|cos(v)|^n + |sin(v)|^n)^(-1/n)
            float cv = fabsf(cosf(vp)); if (cv < 1e-9f) cv = 1e-9f;
            float sv = fabsf(sinf(vp)); if (sv < 1e-9f) sv = 1e-9f;
            float R = powf(powf(cv, n) + powf(sv, n), -inv_n);

            float phi = twist * up + vp;
            float cphi = cosf(phi), sphi = sinf(phi);
            float cu = cosf(up), su = sinf(up);

            float r = a + R * cphi;

            Vertex vtx;
            vtx.x = r * cu;
            vtx.y = r * su;
            vtx.z = R * sphi;

            // Numerical normal via finite difference
            float eps = 1e-4f;
            auto pos = [&](float u2, float v2) -> Vec3 {
                float cv2 = fabsf(cosf(v2)); if (cv2 < 1e-9f) cv2 = 1e-9f;
                float sv2 = fabsf(sinf(v2)); if (sv2 < 1e-9f) sv2 = 1e-9f;
                float R2 = powf(powf(cv2, n) + powf(sv2, n), -inv_n);
                float phi2 = twist * u2 + v2;
                float r2 = a + R2 * cosf(phi2);
                return { r2 * cosf(u2), r2 * sinf(u2), R2 * sinf(phi2) };
                };
            Vec3 pu1 = pos(up + eps, vp);
            Vec3 pu0 = pos(up - eps, vp);
            Vec3 pv1 = pos(up, vp + eps);
            Vec3 pv0 = pos(up, vp - eps);

            float dux = (pu1.x - pu0.x) / (2 * eps);
            float duy = (pu1.y - pu0.y) / (2 * eps);
            float duz = (pu1.z - pu0.z) / (2 * eps);

            float dvx = (pv1.x - pv0.x) / (2 * eps);
            float dvy = (pv1.y - pv0.y) / (2 * eps);
            float dvz = (pv1.z - pv0.z) / (2 * eps);

            // normal = du x dv
            float nx = duy * dvz - duz * dvy;
            float ny = duz * dvx - dux * dvz;
            float nz = dux * dvy - duy * dvx;
            float nl = sqrtf(nx * nx + ny * ny + nz * nz);
            if (nl < 1e-9f) nl = 1.0f;
            vtx.nx = nx / nl;
            vtx.ny = ny / nl;
            vtx.nz = nz / nl;

            vtx.u = (float)iu / Nu;
            vtx.v = (float)iv / Nv;

            verts.push_back(vtx);
        }
    }

    // Generate indices
    for (int iv = 0; iv < Nv; iv++) {
        for (int iu = 0; iu < Nu; iu++) {
            int i0 = iv * (Nu + 1) + iu;
            int i1 = i0 + 1;
            int i2 = i0 + (Nu + 1);
            int i3 = i2 + 1;
            indices.push_back(i0); indices.push_back(i1); indices.push_back(i2);
            indices.push_back(i1); indices.push_back(i3); indices.push_back(i2);
        }
    }
}

// ---- App state ----
struct App {
    HWND  hwnd = nullptr;
    HDC   hdc = nullptr;
    HGLRC hrc = nullptr;
    int   width = 1080, height = 1080;
    bool  running = true;

    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint prog = 0;
    GLint  locMVP = -1, locModel = -1, locNorm = -1;
    GLint  locLight = -1, locTime = -1;

    int    indexCount = 0;

    // Camera
    float  rotX = 0.3f, rotY = 0.5f;
    float  zoom = 15.0f;
    bool   dragging = false;
    int    lastMX = 0, lastMY = 0;

    // Supertoroid params
    float  n = 4.0f;
    float  twist = 2.0f;
    float  a = 3.5f;
    int    Nu = 256, Nv = 128;

    float  time = 0.0f;

    bool   fullscreen = false;
    RECT   savedWndRect = {};
    DWORD  savedStyle = 0;
};

static App g_app;

static bool loadGLProcs() {
    LOAD_GL(PFNGLCREATESHADERPROC, glCreateShader);
    LOAD_GL(PFNGLSHADERSOURCEPROC, glShaderSource);
    LOAD_GL(PFNGLCOMPILESHADERPROC, glCompileShader);
    LOAD_GL(PFNGLCREATEPROGRAMPROC, glCreateProgram);
    LOAD_GL(PFNGLATTACHSHADERPROC, glAttachShader);
    LOAD_GL(PFNGLLINKPROGRAMPROC, glLinkProgram);
    LOAD_GL(PFNGLUSEPROGRAMPROC, glUseProgram);
    LOAD_GL(PFNGLDELETESHADERPROC, glDeleteShader);
    LOAD_GL(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
    LOAD_GL(PFNGLGETSHADERIVPROC, glGetShaderiv);
    LOAD_GL(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
    LOAD_GL(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
    LOAD_GL(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
    LOAD_GL(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
    LOAD_GL(PFNGLUNIFORM1FPROC, glUniform1f);
    LOAD_GL(PFNGLUNIFORM1IPROC, glUniform1i);
    LOAD_GL(PFNGLUNIFORM3FPROC, glUniform3f);
    LOAD_GL(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
    LOAD_GL(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
    LOAD_GL(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
    LOAD_GL(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays);
    LOAD_GL(PFNGLGENBUFFERSPROC, glGenBuffers);
    LOAD_GL(PFNGLBINDBUFFERPROC, glBindBuffer);
    LOAD_GL(PFNGLBUFFERDATAPROC, glBufferData);
    LOAD_GL(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
    LOAD_GL(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
    LOAD_GL(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    return true;
}

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048]; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        MessageBoxA(0, log, "Shader error", MB_OK);
    }
    return s;
}

static GLuint buildProgram(const char* vs, const char* fs) {
    GLuint v = compileShader(GL_VERTEX_SHADER, vs);
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048]; glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        MessageBoxA(nullptr, log, "Link error", MB_OK);
    }
    glDeleteShader(v);
	glDeleteShader(f);
    return p;
}

static void uploadMesh() {
    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;
    buildSupertoroid(g_app.n, g_app.twist, g_app.a, g_app.Nu, g_app.Nv, verts, indices);
    g_app.indexCount = static_cast<int>(indices.size());

    glBindVertexArray(g_app.vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_app.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_app.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);
}

static bool initGL() {
    if (!loadGLProcs()) return false;
    if (wglSwapIntervalEXT) wglSwapIntervalEXT(1);

    g_app.prog = buildProgram(VS, FS);
    g_app.locMVP = glGetUniformLocation(g_app.prog, "uMVP");
    g_app.locModel = glGetUniformLocation(g_app.prog, "uModel");
    g_app.locNorm = glGetUniformLocation(g_app.prog, "uNormalMat");
    g_app.locLight = glGetUniformLocation(g_app.prog, "uLightDir");
    g_app.locTime = glGetUniformLocation(g_app.prog, "uTime");

    glGenVertexArrays(1, &g_app.vao);
    glGenBuffers(1, &g_app.vbo);
    glGenBuffers(1, &g_app.ebo);
    uploadMesh();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.04f, 0.04f, 0.06f, 1.0f);
    return true;
}

static void render(float dt) {
    g_app.time += dt;
    glViewport(0, 0, g_app.width, g_app.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = (float)g_app.width / (float)g_app.height;
    Mat4 proj = mat4Perspective(0.8f, aspect, 0.1f, 200.0f);
    Mat4 view = mat4Translate(0, 0, -g_app.zoom);
    Mat4 rotx = mat4RotX(g_app.rotX);
    Mat4 roty = mat4RotY(g_app.rotY);
    Mat4 model = mat4Mul(rotx, roty);
    Mat4 mv = mat4Mul(view, model);
    Mat4 mvp = mat4Mul(proj, mv);

    // Normal matrix = transpose(inverse(model)) — for uniform scale just use model
    // For this demo model is rotation-only so it's fine
    Mat4 normMat = model;

    glUseProgram(g_app.prog);
    glUniformMatrix4fv(g_app.locMVP, 1, GL_FALSE, mvp.m);
    glUniformMatrix4fv(g_app.locModel, 1, GL_FALSE, model.m);
    glUniformMatrix4fv(g_app.locNorm, 1, GL_FALSE, normMat.m);
    glUniform3f(g_app.locLight, 0.6f, 1.0f, 0.8f);
    glUniform1f(g_app.locTime, g_app.time);

    glBindVertexArray(g_app.vao);
    glDrawElements(GL_TRIANGLES, g_app.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    SwapBuffers(g_app.hdc);
}

static void toggleFullscreen() {
    if (!g_app.fullscreen) {
        GetWindowRect(g_app.hwnd, &g_app.savedWndRect);
        g_app.savedStyle = GetWindowLong(g_app.hwnd, GWL_STYLE);
        SetWindowLong(g_app.hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        HMONITOR mon = MonitorFromWindow(g_app.hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(mon, &mi);
        RECT r = mi.rcMonitor;
        SetWindowPos(g_app.hwnd, HWND_TOP,
            r.left, r.top, r.right - r.left, r.bottom - r.top,
            SWP_FRAMECHANGED);
        g_app.fullscreen = true;
    }
    else {
        SetWindowLong(g_app.hwnd, GWL_STYLE, g_app.savedStyle);
        RECT r = g_app.savedWndRect;
        SetWindowPos(g_app.hwnd, nullptr,
            r.left, r.top, r.right - r.left, r.bottom - r.top,
            SWP_FRAMECHANGED | SWP_NOZORDER);
        g_app.fullscreen = false;
    }
}

static void updateTitle() {
    char buf[256];
    snprintf(buf, sizeof(buf),
        "Supertoroid  |  n=%.1f  twist=%.1f  |  N/M: n  T/Y: twist  R: reset  F11: fullscreen",
        g_app.n, g_app.twist);
    SetWindowTextA(g_app.hwnd, buf);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CLOSE:
        g_app.running = false;
        return 0;

    case WM_SIZE:
        g_app.width = LOWORD(lp);
        g_app.height = HIWORD(lp);
        if (g_app.height == 0) g_app.height = 1;
        return 0;

    case WM_KEYDOWN:
        switch (wp) {
        case VK_ESCAPE: g_app.running = false; break;
        case VK_F11:    toggleFullscreen(); break;
        case 'N': g_app.n = max(2.1f, g_app.n - 0.5f); uploadMesh(); updateTitle(); break;
        case 'M': g_app.n = min(16.0f, g_app.n + 0.5f); uploadMesh(); updateTitle(); break;
        case 'T': g_app.twist = max(1.0f, g_app.twist - 1.0f); uploadMesh(); updateTitle(); break;
        case 'Y': g_app.twist = min(8.0f, g_app.twist + 1.0f); uploadMesh(); updateTitle(); break;
        case 'R':
            g_app.n = 4.0f; g_app.twist = 2.0f;
            g_app.rotX = 0.3f; g_app.rotY = 0.5f; g_app.zoom = 15.0f;
            uploadMesh(); updateTitle(); break;
        }
        return 0;

    case WM_LBUTTONDOWN:
        g_app.dragging = true;
        g_app.lastMX = LOWORD(lp);
        g_app.lastMY = HIWORD(lp);
        SetCapture(hwnd);
        return 0;

    case WM_LBUTTONUP:
        g_app.dragging = false;
        ReleaseCapture();
        return 0;

    case WM_MOUSEMOVE:
        if (g_app.dragging) {
            int mx = LOWORD(lp), my = HIWORD(lp);
            g_app.rotY += (mx - g_app.lastMX) * 0.008f;
            g_app.rotX += (my - g_app.lastMY) * 0.008f;
            g_app.lastMX = mx; g_app.lastMY = my;
        }
        return 0;

    case WM_MOUSEWHEEL: {
        float delta = (float)(short)HIWORD(wp) / WHEEL_DELTA;
        g_app.zoom -= delta * 0.4f;
        g_app.zoom = std::max(g_app.zoom, 1.5f);
        g_app.zoom = std::min(g_app.zoom, 30.0f);
        return 0;
    }
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static bool createWindow() {
    WNDCLASSA wc = {};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "SuperToroidClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&wc);

    RECT r = {.left = 0, .top = 0, .right = g_app.width, .bottom = g_app.height };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);

    g_app.hwnd = CreateWindowA(
        "SuperToroidClass", "Supertoroid",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left, r.bottom - r.top,
        nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);

    if (!g_app.hwnd) return false;
    g_app.hdc = GetDC(g_app.hwnd);
    return true;
}

static bool createGLContext() {
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;

    int fmt = ChoosePixelFormat(g_app.hdc, &pfd);
    SetPixelFormat(g_app.hdc, fmt, &pfd);

    // Dummy context to get wglCreateContextAttribsARB
    HGLRC dummy = wglCreateContext(g_app.hdc);
    wglMakeCurrent(g_app.hdc, dummy);

    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");

    if (!wglCreateContextAttribsARB) {
        MessageBoxA(0, "wglCreateContextAttribsARB not found", "Error", MB_OK);
        return false;
    }

    const int attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    g_app.hrc = wglCreateContextAttribsARB(g_app.hdc, nullptr, attribs);
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(dummy);
    wglMakeCurrent(g_app.hdc, g_app.hrc);
    return g_app.hrc != nullptr;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    if (!createWindow())    return 1;
    if (!createGLContext()) return 1;
    if (!initGL())          return 1;

    updateTitle();

    LARGE_INTEGER freq, prev, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    MSG msg = {};
    while (g_app.running) {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        QueryPerformanceCounter(&now);
        float dt = static_cast<float>(now.QuadPart - prev.QuadPart) / static_cast<float>(freq.QuadPart);
        prev = now;
        dt = std::min(dt, 0.05f);
        render(dt);
    }

    glDeleteVertexArrays(1, &g_app.vao);
    glDeleteBuffers(1, &g_app.vbo);
    glDeleteBuffers(1, &g_app.ebo);
    glDeleteProgram(g_app.prog);
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(g_app.hrc);
    ReleaseDC(g_app.hwnd, g_app.hdc);
    DestroyWindow(g_app.hwnd);
    return 0;
}
