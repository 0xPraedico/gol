#ifndef LIFE_H
#define LIFE_H

#include "grid.h"

/* Counts the 8 living neighbors (out-of-bounds => dead). */
int count_neighbors(const Grid *g, int x, int y);

/*
 * Computes the next generation from cur into next.
 * next must be allocated with the same dimensions as cur.
 * (No in-place update.)
 */
void life_step(const Grid *cur, Grid *next);

#endif /* LIFE_H */
