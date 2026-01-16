#include "history.h"

#include <stdlib.h>

static void history_zero(History *h) {
  if (!h) return;
  h->buf = NULL;
  h->cap = 0;
  h->start = 0;
  h->len = 0;
  h->cur = 0;
}

static size_t pos_phys(const History *h, size_t pos_rel) {
  return (h->start + pos_rel) % h->cap;
}

static void free_slot(History *h, size_t pos_rel) {
  if (!h || !h->buf || h->cap == 0) return;
  size_t p = pos_phys(h, pos_rel);
  if (h->buf[p]) {
    grid_destroy(h->buf[p]);
    h->buf[p] = NULL;
  }
}

bool history_init(History *h, const Grid *initial, size_t cap) {
  if (!h || !initial || !initial->cells) {
    return false;
  }
  history_zero(h);

  if (cap == 0) {
    cap = 512;
  }

  h->buf = (Grid **)calloc(cap, sizeof(Grid *));
  if (!h->buf) {
    history_zero(h);
    return false;
  }
  h->cap = cap;
  h->start = 0;
  h->len = 0;
  h->cur = 0;

  Grid *snap = grid_clone(initial);
  if (!snap) {
    free(h->buf);
    history_zero(h);
    return false;
  }
  h->buf[0] = snap;
  h->len = 1;
  h->cur = 0;
  return true;
}

void history_free(History *h) {
  if (!h) return;
  if (h->buf && h->cap > 0) {
    for (size_t rel = 0; rel < h->len; rel++) {
      free_slot(h, rel);
    }
    free(h->buf);
  }
  history_zero(h);
}

Grid *history_current(History *h) {
  if (!h || !h->buf || h->len == 0 || h->cap == 0) return NULL;
  return h->buf[pos_phys(h, h->cur)];
}

const Grid *history_current_const(const History *h) {
  if (!h || !h->buf || h->len == 0 || h->cap == 0) return NULL;
  return h->buf[pos_phys(h, h->cur)];
}

void history_clear_forward(History *h) {
  if (!h || !h->buf || h->len == 0) return;
  if (h->cur + 1 >= h->len) return;

  for (size_t rel = h->cur + 1; rel < h->len; rel++) {
    free_slot(h, rel);
  }
  h->len = h->cur + 1;
}

bool history_push(History *h, const Grid *g) {
  if (!h || !h->buf || h->cap == 0 || h->len == 0 || !g || !g->cells) {
    return false;
  }

  if (h->cur != (h->len - 1)) {
    history_clear_forward(h);
  }

  Grid *snap = grid_clone(g);
  if (!snap) {
    return false;
  }

  if (h->len < h->cap) {
    size_t phys = pos_phys(h, h->len);
    h->buf[phys] = snap;
    h->len++;
    h->cur = h->len - 1;
    return true;
  }

  /* Full buffer: evict the oldest snapshot (rel=0) */
  if (h->len == h->cap) {
    /* physical index of the oldest == start */
    if (h->buf[h->start]) {
      grid_destroy(h->buf[h->start]);
      h->buf[h->start] = NULL;
    }
    h->start = (h->start + 1) % h->cap;

    /* write at the logical end (rel = len-1) */
    size_t phys_end = pos_phys(h, h->len - 1);
    h->buf[phys_end] = snap;
    h->cur = h->len - 1;
    return true;
  }

  /* unreachable */
  grid_destroy(snap);
  return false;
}

bool history_can_back(const History *h) {
  return (h && h->len > 0 && h->cur > 0);
}

bool history_can_forward(const History *h) {
  return (h && h->len > 0 && (h->cur + 1) < h->len);
}

bool history_back(History *h) {
  if (!history_can_back(h)) return false;
  h->cur--;
  return true;
}

bool history_forward(History *h) {
  if (!history_can_forward(h)) return false;
  h->cur++;
  return true;
}
