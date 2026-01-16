#include "ui_sdl.h"

#include <stdio.h>

static void color_alive(SDL_Renderer *ren) {
  /* green/blue */
  (void)SDL_SetRenderDrawColor(ren, 60, 220, 160, 255);
}

static void color_dead(SDL_Renderer *ren) {
  (void)SDL_SetRenderDrawColor(ren, 20, 20, 24, 255);
}

static void color_gridline(SDL_Renderer *ren) {
  (void)SDL_SetRenderDrawColor(ren, 45, 45, 55, 255);
}

bool ui_init(UiSdl *ui, const char *title, int win_w, int win_h) {
  if (!ui) {
    return false;
  }
  ui->win = NULL;
  ui->ren = NULL;
  ui->win_w = win_w;
  ui->win_h = win_h;

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
    return false;
  }

  ui->win = SDL_CreateWindow(title ? title : "Jeu de la vie",
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             win_w, win_h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!ui->win) {
    fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
    SDL_Quit();
    return false;
  }

  ui->ren = SDL_CreateRenderer(ui->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!ui->ren) {
    fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
    SDL_DestroyWindow(ui->win);
    ui->win = NULL;
    SDL_Quit();
    return false;
  }

  return true;
}

void ui_shutdown(UiSdl *ui) {
  if (!ui) {
    return;
  }
  if (ui->ren) {
    SDL_DestroyRenderer(ui->ren);
    ui->ren = NULL;
  }
  if (ui->win) {
    SDL_DestroyWindow(ui->win);
    ui->win = NULL;
  }
  SDL_Quit();
}

void ui_render_grid(UiSdl *ui, const Grid *g) {
  if (!ui || !ui->ren || !ui->win || !g || !g->cells || g->w <= 0 || g->h <= 0) {
    return;
  }

  int w = 0, h = 0;
  SDL_GetWindowSize(ui->win, &w, &h);
  if (w <= 0 || h <= 0) {
    return;
  }

  color_dead(ui->ren);
  (void)SDL_RenderClear(ui->ren);

  int cell_w = w / g->w;
  int cell_h = h / g->h;
  if (cell_w < 1) cell_w = 1;
  if (cell_h < 1) cell_h = 1;
  int cell = (cell_w < cell_h) ? cell_w : cell_h;
  if (cell < 1) cell = 1;

  int grid_px_w = cell * g->w;
  int grid_px_h = cell * g->h;
  int off_x = (w - grid_px_w) / 2;
  int off_y = (h - grid_px_h) / 2;

  /* living cells */
  color_alive(ui->ren);
  for (int y = 0; y < g->h; y++) {
    for (int x = 0; x < g->w; x++) {
      if (!grid_get(g, x, y)) {
        continue;
      }
      SDL_Rect r;
      r.x = off_x + x * cell;
      r.y = off_y + y * cell;
      r.w = cell;
      r.h = cell;
      (void)SDL_RenderFillRect(ui->ren, &r);
    }
  }

  /* subtle grid lines (if cells are large enough) */
  if (cell >= 6) {
    color_gridline(ui->ren);
    for (int x = 0; x <= g->w; x++) {
      int px = off_x + x * cell;
      (void)SDL_RenderDrawLine(ui->ren, px, off_y, px, off_y + grid_px_h);
    }
    for (int y = 0; y <= g->h; y++) {
      int py = off_y + y * cell;
      (void)SDL_RenderDrawLine(ui->ren, off_x, py, off_x + grid_px_w, py);
    }
  }

  SDL_RenderPresent(ui->ren);
}

UiAction ui_poll_action(bool *out_quit) {
  if (out_quit) {
    *out_quit = false;
  }

  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      if (out_quit) *out_quit = true;
      return UI_ACT_QUIT;
    }
    if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
      SDL_Keycode k = e.key.keysym.sym;
      if (k == SDLK_ESCAPE || k == SDLK_q) {
        if (out_quit) *out_quit = true;
        return UI_ACT_QUIT;
      }
      if (k == SDLK_SPACE) return UI_ACT_TOGGLE_PLAY;
      if (k == SDLK_n) return UI_ACT_STEP;
      if (k == SDLK_b) return UI_ACT_BACK;
      if (k == SDLK_f) return UI_ACT_FORWARD;
      if (k == SDLK_s) return UI_ACT_SAVE;
      if (k == SDLK_r) return UI_ACT_RESIZE;
    }
  }
  return UI_ACT_NONE;
}
