#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "grid.h"
#include "history.h"
#include "life.h"

typedef struct BenchArgs {
  int width;
  int height;
  int steps;
  unsigned int seed;
  size_t history_cap;
} BenchArgs;

static void usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s --width W --height H --steps S --seed N --history-cap C\n",
          prog ? prog : "life_bench");
}

static bool parse_int(const char *s, int *out) {
  if (!s || !out) return false;
  char *end = NULL;
  long v = strtol(s, &end, 10);
  if (end == s || *end != '\0') return false;
  if (v < 0 || v > 2147483647L) return false;
  *out = (int)v;
  return true;
}

static bool parse_uint(const char *s, unsigned int *out) {
  if (!s || !out) return false;
  char *end = NULL;
  unsigned long v = strtoul(s, &end, 10);
  if (end == s || *end != '\0') return false;
  *out = (unsigned int)v;
  return true;
}

static bool parse_size(const char *s, size_t *out) {
  if (!s || !out) return false;
  char *end = NULL;
  unsigned long long v = strtoull(s, &end, 10);
  if (end == s || *end != '\0') return false;
  *out = (size_t)v;
  return true;
}

static bool parse_args(int argc, char **argv, BenchArgs *a) {
  if (!a) return false;
  a->width = 0;
  a->height = 0;
  a->steps = 0;
  a->seed = 1;
  a->history_cap = 512;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
      if (!parse_int(argv[++i], &a->width) || a->width < 1) return false;
    } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
      if (!parse_int(argv[++i], &a->height) || a->height < 1) return false;
    } else if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) {
      if (!parse_int(argv[++i], &a->steps) || a->steps < 1) return false;
    } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
      if (!parse_uint(argv[++i], &a->seed)) return false;
    } else if (strcmp(argv[i], "--history-cap") == 0 && i + 1 < argc) {
      if (!parse_size(argv[++i], &a->history_cap)) return false;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      usage(argv[0]);
      exit(0);
    } else {
      return false;
    }
  }
  return (a->width > 0 && a->height > 0 && a->steps > 0);
}

static uint64_t timespec_to_ns(const struct timespec *ts) {
  return (uint64_t)ts->tv_sec * 1000000000ull + (uint64_t)ts->tv_nsec;
}

static void fill_random(Grid *g, unsigned int seed) {
  if (!g || !g->cells) return;
  unsigned int s = seed;
  for (int y = 0; y < g->h; y++) {
    for (int x = 0; x < g->w; x++) {
      unsigned int r = (unsigned int)rand_r(&s);
      grid_set(g, x, y, (r % 4u == 0u) ? 1u : 0u);
    }
  }
}

int main(int argc, char **argv) {
  BenchArgs a;
  if (!parse_args(argc, argv, &a)) {
    usage(argv[0]);
    return 2;
  }

  Grid init = {0};
  if (!grid_create(&init, a.width, a.height)) {
    fprintf(stderr, "Allocation échouée (init)\n");
    return 1;
  }
  fill_random(&init, a.seed);

  History hist;
  if (!history_init(&hist, &init, a.history_cap)) {
    fprintf(stderr, "Init historique échouée\n");
    grid_free(&init);
    return 1;
  }
  grid_free(&init);

  Grid scratch_next = {0};

  struct timespec t0, t1;
  (void)clock_gettime(CLOCK_MONOTONIC, &t0);

  const int nav_period = 128;
  for (int i = 0; i < a.steps; i++) {
    Grid *cur = history_current(&hist);
    if (!cur) {
      fprintf(stderr, "Historique invalide\n");
      history_free(&hist);
      grid_free(&scratch_next);
      return 1;
    }

    if (scratch_next.cells == NULL || scratch_next.w != cur->w || scratch_next.h != cur->h) {
      grid_free(&scratch_next);
      if (!grid_create(&scratch_next, cur->w, cur->h)) {
        fprintf(stderr, "Allocation échouée (scratch)\n");
        history_free(&hist);
        return 1;
      }
    }

    life_step(cur, &scratch_next);
    if (!history_push(&hist, &scratch_next)) {
      fprintf(stderr, "history_push échoué\n");
      history_free(&hist);
      grid_free(&scratch_next);
      return 1;
    }

    if (nav_period > 0 && (i % nav_period) == 0) {
      (void)history_back(&hist);
      (void)history_forward(&hist);
    }
  }

  (void)clock_gettime(CLOCK_MONOTONIC, &t1);

  uint64_t dt_ns = timespec_to_ns(&t1) - timespec_to_ns(&t0);
  double total_s = (double)dt_ns / 1e9;
  double steps_per_s = (double)a.steps / total_s;
  double ns_per_step = (double)dt_ns / (double)a.steps;

  printf("RESULT impl=ring total_s=%.6f steps=%d steps_per_s=%.3f ns_per_step=%.1f width=%d height=%d seed=%u history_cap=%zu\n",
         total_s, a.steps, steps_per_s, ns_per_step, a.width, a.height, a.seed, a.history_cap);

  history_free(&hist);
  grid_free(&scratch_next);
  return 0;
}

