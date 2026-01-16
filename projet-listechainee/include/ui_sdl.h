#ifndef UI_SDL_H
#define UI_SDL_H

#include <stdbool.h>
#include <stdint.h>

#include <SDL.h>

#include "grid.h"

typedef enum UiAction {
  UI_ACT_NONE = 0,
  UI_ACT_QUIT,
  UI_ACT_TOGGLE_PLAY,
  UI_ACT_STEP,
  UI_ACT_BACK,
  UI_ACT_FORWARD,
  UI_ACT_SAVE,
  UI_ACT_RESIZE
} UiAction;

typedef struct UiSdl {
  SDL_Window *win;
  SDL_Renderer *ren;
  int win_w;
  int win_h;
} UiSdl;

bool ui_init(UiSdl *ui, const char *title, int win_w, int win_h);
void ui_shutdown(UiSdl *ui);

/* Renders the grid with colors (alive/dead). */
void ui_render_grid(UiSdl *ui, const Grid *g);

/* Polls SDL, returns an action (or UI_ACT_NONE). */
UiAction ui_poll_action(bool *out_quit);

#endif /* UI_SDL_H */
