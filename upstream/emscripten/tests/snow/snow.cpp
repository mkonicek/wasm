#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

#include <emscripten.h>
#include <emscripten/html5.h> // Where is this file? How come emcc can find it?

#include "util.h"

#define WIDTH 1024
#define HEIGHT 768

#define NUM_FLAKES 10000
#define FLAKE_SIZE 4
#define FLAKE_SPEED 0.04
#define SNOWINESS 0.998

typedef struct { double x, y, vx, vy; } Flake;

static Flake flakes[NUM_FLAKES] = {};

void on_mouse_up(int button, int state, int mx, int my)
{
    int x = mx;
    // Canvas y=0 is at the top
    int y = HEIGHT - my;
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
    {
        printf("on_mouse_up: x=%d, y=%d\n", x, y);
        // Explode flakes away from the mouse
        for (int i = 0; i < NUM_FLAKES; ++i) {
          double dx = x - flakes[i].x;
          double dy = y - flakes[i].y;
          double distSquared = dx * dx + dy * dy;
          flakes[i].vx = dx * 5 / distSquared;
          flakes[i].vy = -dy * 5 / distSquared;
        }
    }
}

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
      flakes[i].vx = (emscripten_random() - 0.5) * 0.02;
      flakes[i].vy = FLAKE_SPEED + i * 0.05 / NUM_FLAKES;
    }
  }

  return EM_TRUE;
}

int main(int argc, char *argv[])
{
  init_webgl(WIDTH, HEIGHT);
  glutInit(&argc, argv);
  glutMouseFunc(&on_mouse_up);
  emscripten_request_animation_frame_loop(&draw_frame, 0);
}
