#include "life.h"

#include <stddef.h>

int count_neighbors(const Grid *g, int x, int y) {
  int n = 0;
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      n += (grid_get(g, x + dx, y + dy) ? 1 : 0);
    }
  }
  return n;
}

void life_step(const Grid *cur, Grid *next) {
  if (!cur || !next || !cur->cells || !next->cells) {
    return;
  }
  if (cur->w != next->w || cur->h != next->h) {
    return;
  }

  for (int y = 0; y < cur->h; y++) {
    for (int x = 0; x < cur->w; x++) {
      uint8_t alive = grid_get(cur, x, y);
      int n = count_neighbors(cur, x, y);
      uint8_t out = 0;
      if (alive) {
        out = (n == 2 || n == 3) ? 1u : 0u;
      } else {
        out = (n == 3) ? 1u : 0u;
      }
      next->cells[(size_t)y * (size_t)cur->w + (size_t)x] = out;
    }
  }
}
