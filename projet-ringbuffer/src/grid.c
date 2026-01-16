#include "grid.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static bool grid_valid_dims(int w, int h) {
  return (w > 0 && h > 0);
}

static bool grid_count_ok(int w, int h, size_t *out_count) {
  if (!grid_valid_dims(w, h)) {
    return false;
  }
  size_t ww = (size_t)w;
  size_t hh = (size_t)h;
  if (ww != 0 && hh > (SIZE_MAX / ww)) {
    return false;
  }
  *out_count = ww * hh;
  return true;
}

bool grid_create(Grid *g, int w, int h) {
  if (!g) {
    return false;
  }
  size_t count = 0;
  if (!grid_count_ok(w, h, &count)) {
    return false;
  }

  uint8_t *cells = (uint8_t *)calloc(count, sizeof(uint8_t));
  if (!cells) {
    return false;
  }

  g->w = w;
  g->h = h;
  g->cells = cells;
  return true;
}

void grid_free(Grid *g) {
  if (!g) {
    return;
  }
  free(g->cells);
  g->cells = NULL;
  g->w = 0;
  g->h = 0;
}

void grid_destroy(Grid *g) {
  if (!g) {
    return;
  }
  grid_free(g);
  free(g);
}

void grid_clear(Grid *g) {
  if (!g || !g->cells) {
    return;
  }
  size_t count = (size_t)g->w * (size_t)g->h;
  memset(g->cells, 0, count * sizeof(uint8_t));
}

uint8_t grid_get(const Grid *g, int x, int y) {
  if (!g || !g->cells) {
    return 0;
  }
  if (x < 0 || y < 0 || x >= g->w || y >= g->h) {
    return 0;
  }
  return g->cells[(size_t)y * (size_t)g->w + (size_t)x];
}

void grid_set(Grid *g, int x, int y, uint8_t v) {
  if (!g || !g->cells) {
    return;
  }
  if (x < 0 || y < 0 || x >= g->w || y >= g->h) {
    return;
  }
  g->cells[(size_t)y * (size_t)g->w + (size_t)x] = (v ? 1u : 0u);
}

bool grid_resize(Grid *g, int new_w, int new_h) {
  if (!g) {
    return false;
  }

  size_t new_count = 0;
  if (!grid_count_ok(new_w, new_h, &new_count)) {
    return false;
  }

  /* Si grille non initialisée, agir comme create. */
  if (!g->cells) {
    g->w = 0;
    g->h = 0;
    return grid_create(g, new_w, new_h);
  }

  uint8_t *new_cells = (uint8_t *)calloc(new_count, sizeof(uint8_t));
  if (!new_cells) {
    return false;
  }

  int copy_w = (g->w < new_w) ? g->w : new_w;
  int copy_h = (g->h < new_h) ? g->h : new_h;

  for (int y = 0; y < copy_h; y++) {
    const uint8_t *src_row = &g->cells[(size_t)y * (size_t)g->w];
    uint8_t *dst_row = &new_cells[(size_t)y * (size_t)new_w];
    memcpy(dst_row, src_row, (size_t)copy_w * sizeof(uint8_t));
  }

  free(g->cells);
  g->cells = new_cells;
  g->w = new_w;
  g->h = new_h;
  return true;
}

Grid *grid_clone(const Grid *src) {
  if (!src || !src->cells) {
    return NULL;
  }
  Grid *g = (Grid *)malloc(sizeof(Grid));
  if (!g) {
    return NULL;
  }
  g->w = 0;
  g->h = 0;
  g->cells = NULL;

  if (!grid_create(g, src->w, src->h)) {
    free(g);
    return NULL;
  }

  size_t count = (size_t)src->w * (size_t)src->h;
  memcpy(g->cells, src->cells, count * sizeof(uint8_t));
  return g;
}

void grid_swap(Grid *a, Grid *b) {
  if (!a || !b) {
    return;
  }
  Grid tmp = *a;
  *a = *b;
  *b = tmp;
}
