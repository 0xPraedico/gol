#ifndef GRID_H
#define GRID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Grid {
  int w;
  int h;
  uint8_t *cells; /* contiguous 1D: cells[y*w + x] ∈ {0,1} */
} Grid;

/* Allocates a w*h grid, initialized to 0. */
bool grid_create(Grid *g, int w, int h);

/* Frees the internal buffer (does not free the Grid pointer itself). */
void grid_free(Grid *g);

/* Destroys a heap-allocated grid (Grid* + cells). */
void grid_destroy(Grid *g);

/* Sets all cells to 0. */
void grid_clear(Grid *g);

/* Safe read: out-of-bounds => 0. */
uint8_t grid_get(const Grid *g, int x, int y);

/* Write: ignored if out-of-bounds. v is normalized to {0,1}. */
void grid_set(Grid *g, int x, int y, uint8_t v);

/*
 * Resizes (via reallocation). Preserves the overlap (min(w), min(h)).
 * New cells (when growing) are initialized to 0.
 */
bool grid_resize(Grid *g, int new_w, int new_h);

/* Heap clone. Returns NULL on error. */
Grid *grid_clone(const Grid *src);

/* Swaps contents (w,h,cells). */
void grid_swap(Grid *a, Grid *b);

#endif /* GRID_H */
