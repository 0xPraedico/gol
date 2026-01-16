#ifndef HISTORY_H
#define HISTORY_H

#include <stdbool.h>
#include <stddef.h>

#include "grid.h"

/*
 * Ring-buffer history (bounded timeline).
 * - cap: maximum capacity in snapshots
 * - start: physical index of the oldest snapshot
 * - len: number of stored snapshots (0..cap)
 * - cur: current relative position (0..len-1)
 */
typedef struct History {
  Grid **buf;
  size_t cap;
  size_t start;
  size_t len;
  size_t cur;
} History;

bool history_init(History *h, const Grid *initial, size_t cap);
void history_free(History *h);

Grid *history_current(History *h);
const Grid *history_current_const(const History *h);

void history_clear_forward(History *h);
bool history_push(History *h, const Grid *g);

bool history_can_back(const History *h);
bool history_can_forward(const History *h);
bool history_back(History *h);
bool history_forward(History *h);

#endif /* HISTORY_H */
