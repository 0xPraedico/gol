#include "history.h"

#include <stdlib.h>

static void history_zero(History *h) {
  if (!h) {
    return;
  }
  h->head = NULL;
  h->tail = NULL;
  h->cur = NULL;
  h->len = 0;
  h->cap = 0;
}

static void node_free(HistoryNode *n) {
  if (!n) {
    return;
  }
  grid_destroy(n->grid);
  free(n);
}

static void history_evict_oldest_if_needed(History *h) {
  if (!h || h->cap == 0) {
    return;
  }
  while (h->len > h->cap && h->head) {
    HistoryNode *old = h->head;
    h->head = old->next;
    if (h->head) {
      h->head->prev = NULL;
    } else {
      h->tail = NULL;
      h->cur = NULL;
    }
    node_free(old);
    h->len--;
  }
}

bool history_init(History *h, const Grid *initial, size_t cap) {
  if (!h || !initial || !initial->cells) {
    return false;
  }
  history_zero(h);
  h->cap = cap;

  Grid *snap = grid_clone(initial);
  if (!snap) {
    return false;
  }
  HistoryNode *n = (HistoryNode *)malloc(sizeof(HistoryNode));
  if (!n) {
    grid_destroy(snap);
    return false;
  }
  n->grid = snap;
  n->prev = NULL;
  n->next = NULL;

  h->head = n;
  h->tail = n;
  h->cur = n;
  h->len = 1;
  return true;
}

void history_free(History *h) {
  if (!h) {
    return;
  }
  HistoryNode *it = h->head;
  while (it) {
    HistoryNode *next = it->next;
    node_free(it);
    it = next;
  }
  history_zero(h);
}

Grid *history_current(History *h) {
  if (!h || !h->cur) {
    return NULL;
  }
  return h->cur->grid;
}

const Grid *history_current_const(const History *h) {
  if (!h || !h->cur) {
    return NULL;
  }
  return h->cur->grid;
}

void history_clear_forward(History *h) {
  if (!h || !h->cur) {
    return;
  }

  HistoryNode *it = h->cur->next;
  h->cur->next = NULL;
  h->tail = h->cur;

  while (it) {
    HistoryNode *next = it->next;
    node_free(it);
    h->len--;
    it = next;
  }
}

bool history_push(History *h, const Grid *g) {
  if (!h || !h->cur || !g || !g->cells) {
    return false;
  }

  if (h->cur != h->tail) {
    history_clear_forward(h);
  }

  Grid *snap = grid_clone(g);
  if (!snap) {
    return false;
  }

  HistoryNode *n = (HistoryNode *)malloc(sizeof(HistoryNode));
  if (!n) {
    grid_destroy(snap);
    return false;
  }
  n->grid = snap;
  n->prev = h->tail;
  n->next = NULL;

  h->tail->next = n;
  h->tail = n;
  h->cur = n;
  h->len++;

  history_evict_oldest_if_needed(h);

  /* If we evicted nodes, cur must still be valid. */
  if (!h->cur) {
    /* cap==0 cannot happen here; cap==0 => no eviction */
    return false;
  }
  return true;
}

bool history_can_back(const History *h) {
  return (h && h->cur && h->cur->prev);
}

bool history_can_forward(const History *h) {
  return (h && h->cur && h->cur->next);
}

bool history_back(History *h) {
  if (!history_can_back(h)) {
    return false;
  }
  h->cur = h->cur->prev;
  return true;
}

bool history_forward(History *h) {
  if (!history_can_forward(h)) {
    return false;
  }
  h->cur = h->cur->next;
  return true;
}
