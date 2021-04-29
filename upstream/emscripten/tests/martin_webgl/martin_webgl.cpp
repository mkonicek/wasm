#include <webgl/webgl1.h> // For Emscripten WebGL API headers (see also webgl/webgl1_ext.h and webgl/webgl2.h)
#include <math.h>
#include <memory.h>
#include <string.h> // For NULL and strcmp()
#include <assert.h> // For assert()
#include <emscripten.h>
#include <emscripten/html5.h> // Where is this file? How come emcc can find it?

// Getting Started:
// Compile this file with:
//   emcc martin_webgl.cpp -o martin_webgl.html
// Then start a local webserver:
//   npx serve
// Open the HTML file in the browser.

#define WIDTH 1024
#define HEIGHT 768

// NOTE: The other example HelloTriangle is interesting, note that
//       it uses the "framework" called esUtil which has this comment:
//       "Just one iteration"
//       We would have to adapt that esUtil to use emscripten_request_animation_frame_loop
//       rather than the X-Window API XNextEvent.
//
// NOTE: Compiling C & C++ is tricky. Don't yet fully understand where the compiler & linker
//       look for imports. The glbook has a Makefile - use emmake. I've seen CMakeLists.txt
//       in other examples - what is that? Is CMake a different tool? And how to use it?
//       Is it emcmake?

// ========================================================
// START WEBGL LIB

static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext;
static GLuint quad, colorPos, matPos, solidColor;
static float pixelWidth, pixelHeight;

static GLuint compile_shader(GLenum shaderType, const char *src)
{
   GLuint shader = glCreateShader(shaderType);
   glShaderSource(shader, 1, &src, NULL);
   glCompileShader(shader);
   return shader;
}

static GLuint create_program(GLuint vertexShader, GLuint fragmentShader)
{
   GLuint program = glCreateProgram();
   glAttachShader(program, vertexShader);
   glAttachShader(program, fragmentShader);
   glBindAttribLocation(program, 0, "pos");
   glLinkProgram(program);
   glUseProgram(program);
   return program;
}

static GLuint create_texture()
{
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  return texture;
}

void init_webgl(int width, int height)
{
  double dpr = emscripten_get_device_pixel_ratio();
  emscripten_set_element_css_size("canvas", width / dpr, height / dpr);
  emscripten_set_canvas_element_size("canvas", width, height);

  EmscriptenWebGLContextAttributes attrs;
  emscripten_webgl_init_context_attributes(&attrs);
  attrs.alpha = 0;
#if MAX_WEBGL_VERSION >= 2
  attrs.majorVersion = 2;
#endif
  glContext = emscripten_webgl_create_context("canvas", &attrs);
  assert(glContext);
  emscripten_webgl_make_context_current(glContext);

  pixelWidth = 2.f / width;
  pixelHeight = 2.f / height;

  static const char vertex_shader[] =
    "attribute vec4 pos;"
    "varying vec2 uv;"
    "uniform mat4 mat;"
    "void main(){"
      "uv=pos.xy;"
      "gl_Position=mat*pos;"
    "}";
  GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);

  static const char fragment_shader[] =
    "precision lowp float;"
    "uniform sampler2D tex;"
    "varying vec2 uv;"
    "uniform vec4 color;"
    "void main(){"
      "gl_FragColor=color*texture2D(tex,uv);"
    "}";
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);

  GLuint program = create_program(vs, fs);
  colorPos = glGetUniformLocation(program, "color");
  matPos = glGetUniformLocation(program, "mat");
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glGenBuffers(1, &quad);
  glBindBuffer(GL_ARRAY_BUFFER, quad);
  const float pos[] = { 0, 0, 1, 0, 0, 1, 1, 1 };
  glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  solidColor = create_texture();
  unsigned int whitePixel = 0xFFFFFFFFu;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &whitePixel);
}

typedef void (*tick_func)(double t, double dt);

static EM_BOOL tick(double time, void *userData)
{
  static double t0;
  double dt = time - t0;
  t0 = time;
  tick_func f = (tick_func)(userData);
  f(time, dt);
  return EM_TRUE;
}

void clear_screen(float r, float g, float b, float a)
{
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);
}

static void fill_textured_rectangle(float x0, float y0, float x1, float y1, float r, float g, float b, float a, GLuint texture)
{
  float mat[16] = { (x1-x0)*pixelWidth, 0, 0, 0, 0, (y1-y0)*pixelHeight, 0, 0, 0, 0, 1, 0, x0*pixelWidth-1.f, y0*pixelHeight-1.f, 0, 1};
  glUniformMatrix4fv(matPos, 1, 0, mat);
  glUniform4f(colorPos, r, g, b, a);
  glBindTexture(GL_TEXTURE_2D, texture);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void fill_solid_rectangle(float x0, float y0, float x1, float y1, float r, float g, float b, float a)
{
  fill_textured_rectangle(x0, y0, x1, y1, r, g, b, a, solidColor);
}

typedef struct Texture
{
  // Image
  char *url;
  int w, h;

  GLuint texture;
} Texture;

#define MAX_TEXTURES 256
static Texture textures[MAX_TEXTURES] = {};

static Texture *find_or_cache_url(const char *url)
{
  for(int i = 0; i < MAX_TEXTURES; ++i) // Naive O(n) lookup for tiny code size
    if (!strcmp(textures[i].url, url))
      return textures+i;
    else if (!textures[i].url)
    {
      textures[i].url = strdup(url);
      textures[i].texture = create_texture();
      return textures+i;
    }
  return 0; // fail
}

void fill_image(float x0, float y0, float scale, float r, float g, float b, float a, const char *url)
{
  Texture *t = find_or_cache_url(url);
  fill_textured_rectangle(x0, y0, x0 + t->w * scale, y0 + t->h * scale, r, g, b, a, t->texture);
}

typedef struct Glyph
{
  // Font
  unsigned int ch;
  int charSize;
  int shadow;

  GLuint texture;
} Glyph;

#define MAX_GLYPHS 256
static Glyph glyphs[MAX_GLYPHS] = {};
static Glyph *find_or_cache_character(unsigned int ch, int charSize, int shadow)
{
  for(int i = 0; i < MAX_TEXTURES; ++i) // Naive O(n) lookup for tiny code size
    if (glyphs[i].ch == ch && glyphs[i].charSize == charSize && glyphs[i].shadow == shadow)
      return glyphs+i;
    else if (!glyphs[i].ch)
    {
      glyphs[i].ch = ch;
      glyphs[i].charSize = charSize;
      glyphs[i].shadow = shadow;
      glyphs[i].texture = create_texture();
      return glyphs+i;
    }
  return 0; // fail
}

// END WEBGL LIB
// ========================================================

float clamp01(float v)
{
  return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
}

float mod_dist(float v, float t)
{
  float d = fabsf(v - t);
  return fminf(d, 6.f - d);
}
// Per-frame animation tick.
EM_BOOL draw_frame(double t, void *)
{
  static double prevT;
  double dt = t - prevT;
  prevT = t;

  clear_screen(0.1f, 0.2f, 0.3f, 1.f);

  // moon
  fill_image(WIDTH-250.f, HEIGHT - 250.f, 2.f, 1.f, 1.f, 1.f, 1.f, "moon.png");

  // snow background
#define NUM_FLAKES 1000
#define FLAKE_SIZE 10
#define FLAKE_SPEED 0.05f
#define SNOWINESS 0.998
  static struct { float x, y; } flakes[NUM_FLAKES] = {};

#define SIM do { \
      flakes[i].y -= dt*(FLAKE_SPEED+i*0.05f/NUM_FLAKES); \
      flakes[i].x += dt*(fmodf(i*345362.f, 0.02f) - 0.01f); \
      float c = 0.5f + i*0.5/NUM_FLAKES; \
      if (flakes[i].y > -FLAKE_SIZE) fill_solid_rectangle(flakes[i].x, flakes[i].y, flakes[i].x+FLAKE_SIZE, flakes[i].y+FLAKE_SIZE, c, c, c, 1.f); \
      else if (emscripten_random() > SNOWINESS) flakes[i].y = HEIGHT, flakes[i].x = WIDTH*emscripten_random(); \
    } while(0)
  for(int i = 0; i < NUM_FLAKES/2; ++i) SIM;

  // ground
  fill_solid_rectangle(0.f, 0.f, WIDTH, 20.f, 0.8f, 0.8f, 0.8f, 1.f);

  // snow foreground
  for(int i = NUM_FLAKES/2; i < NUM_FLAKES; ++i) SIM;

  return EM_TRUE;
}

int main()
{
  init_webgl(WIDTH, HEIGHT);
  emscripten_request_animation_frame_loop(&draw_frame, 0);
}
