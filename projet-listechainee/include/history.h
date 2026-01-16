#ifndef HISTORY_H
#define HISTORY_H

#include <stdbool.h>
#include <stddef.h>

#include "grid.h"

typedef struct HistoryNode {
  Grid *grid; /* snapshot (owned by the node) */
  struct HistoryNode *prev;
  struct HistoryNode *next;
} HistoryNode;

typedef struct History {
  HistoryNode *head;
  HistoryNode *tail;
  HistoryNode *cur;
  size_t len;
  size_t cap; /* 0 = unlimited; otherwise keep at most cap snapshots (evict oldest) */
} History;

/* Initializes history with a copy of initial. */
bool history_init(History *h, const Grid *initial, size_t cap);

/* Frees all nodes and their grids. */
void history_free(History *h);

/* Returns the current grid (non-NULL if history is initialized). */
Grid *history_current(History *h);
const Grid *history_current_const(const History *h);

/* Deletes all "future" states after cur (when stepping from a past state). */
void history_clear_forward(History *h);

/*
 * Pushes a new snapshot (clone of g) after cur.
 * If cur is not at the end, clear_forward is applied.
 * If cap>0, evicts oldest snapshots to stay <= cap.
 */
bool history_push(History *h, const Grid *g);

bool history_can_back(const History *h);
bool history_can_forward(const History *h);

/* Moves cur if possible. */
bool history_back(History *h);
bool history_forward(History *h);

#endif /* HISTORY_H */
