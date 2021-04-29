#include <emscripten.h>
#include <emscripten/html5.h> // Where is this file? How come emcc can find it?

#include "util.h"

// Getting Started:
// Compile this file with:
//   emcc martin_webgl.cpp -o martin_webgl.html
// Then start a local webserver:
//   npx serve
// Open the HTML file in the browser.

#define WIDTH 1024
#define HEIGHT 768

#define NUM_FLAKES 2000
#define FLAKE_SIZE 5
#define FLAKE_SPEED 0.03
#define SNOWINESS 0.998

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

typedef struct { double x, y, vx, vy; } Flake;

static Flake flakes[NUM_FLAKES] = {};

// Per-frame animation tick.
EM_BOOL draw_frame(double t, void *userData)
{
  static double prevT;
  double dt = t - prevT;
  prevT = t;

  clear_screen(0.1f, 0.2f, 0.3f, 1.f);

  for (int i = 0; i < NUM_FLAKES; ++i) {
    flakes[i].y -= dt * flakes[i].vy;
    flakes[i].x += dt * flakes[i].vx;
    float c = 0.5f + i * 0.5 / NUM_FLAKES;
    if (flakes[i].y > 0) {
      fill_solid_rectangle(
        flakes[i].x,
        flakes[i].y,
        flakes[i].x+FLAKE_SIZE,
        flakes[i].y+FLAKE_SIZE,
        c, c, c,
        1.f);
    }
    else if (emscripten_random() > SNOWINESS) {
      // Move the flake back to the top
      flakes[i].x = WIDTH * emscripten_random();
      flakes[i].y = HEIGHT;
      flakes[i].vx = fmodf(i * 345362.f, 0.02) - 0.01;
      flakes[i].vy = FLAKE_SPEED + i * 0.05 / NUM_FLAKES;
    }
  }

  return EM_TRUE;
}

int main()
{
  init_webgl(WIDTH, HEIGHT);
  emscripten_request_animation_frame_loop(&draw_frame, 0);
}
