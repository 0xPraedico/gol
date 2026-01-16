#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "grid.h"
#include "history.h"
#include "io.h"
#include "life.h"
#include "ui_sdl.h"

typedef struct Args {
  const char *input_path;
  const char *output_path;
  int steps; /* if >0 and output provided: batch mode without UI */
  int w;
  int h;
  size_t history_cap; /* ring: max capacity (0 => internal default) */
} Args;

static void usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s [--input FILE] [--output FILE] [--steps N] [--w W --h H] [--history-cap N]\n",
          prog ? prog : "life");
}

static bool parse_int(const char *s, int *out) {
  if (!s || !out) return false;
  char *end = NULL;
  long v = strtol(s, &end, 10);
  if (end == s || *end != '\0') return false;
  if (v < -2147483647L || v > 2147483647L) return false;
  *out = (int)v;
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

static bool parse_args(int argc, char **argv, Args *a) {
  if (!a) return false;
  a->input_path = NULL;
  a->output_path = NULL;
  a->steps = 0;
  a->w = 0;
  a->h = 0;
  a->history_cap = 512;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
      a->input_path = argv[++i];
    } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
      a->output_path = argv[++i];
    } else if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) {
      if (!parse_int(argv[++i], &a->steps) || a->steps < 0) return false;
    } else if (strcmp(argv[i], "--w") == 0 && i + 1 < argc) {
      if (!parse_int(argv[++i], &a->w) || a->w < 1) return false;
    } else if (strcmp(argv[i], "--h") == 0 && i + 1 < argc) {
      if (!parse_int(argv[++i], &a->h) || a->h < 1) return false;
    } else if (strcmp(argv[i], "--history-cap") == 0 && i + 1 < argc) {
      if (!parse_size(argv[++i], &a->history_cap)) return false;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      usage(argv[0]);
      exit(0);
    } else {
      return false;
    }
  }
  return true;
}

static void prompt_size(int *w, int *h, int def_w, int def_h) {
  if (w) *w = def_w;
  if (h) *h = def_h;

  char buf[256];
  fprintf(stdout, "Taille de la grille (width height) [%d %d]: ", def_w, def_h);
  fflush(stdout);
  if (!fgets(buf, (int)sizeof(buf), stdin)) {
    return;
  }
  int ww = 0, hh = 0;
  if (sscanf(buf, "%d %d", &ww, &hh) == 2 && ww > 0 && hh > 0) {
    if (w) *w = ww;
    if (h) *h = hh;
  }
}

static void prompt_path(const char *label, char *out, size_t outcap, const char *def) {
  if (!out || outcap == 0) return;
  out[0] = '\0';
  char buf[512];
  fprintf(stdout, "%s [%s]: ", label, def ? def : "");
  fflush(stdout);
  if (!fgets(buf, (int)sizeof(buf), stdin)) {
    snprintf(out, outcap, "%s", def ? def : "output.txt");
    return;
  }
  size_t n = strlen(buf);
  if (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[n - 1] = '\0';
  if (buf[0] == '\0') {
    snprintf(out, outcap, "%s", def ? def : "output.txt");
  } else {
    snprintf(out, outcap, "%s", buf);
  }
}

static bool do_step_and_push(History *hist, Grid *scratch_next) {
  Grid *cur = history_current(hist);
  if (!cur || !scratch_next) return false;
  if (scratch_next->cells == NULL || scratch_next->w != cur->w || scratch_next->h != cur->h) {
    grid_free(scratch_next);
    if (!grid_create(scratch_next, cur->w, cur->h)) return false;
  }
  life_step(cur, scratch_next);
  return history_push(hist, scratch_next);
}

int main(int argc, char **argv) {
  Args args;
  if (!parse_args(argc, argv, &args)) {
    usage(argv[0]);
    return 2;
  }

  fprintf(stdout,
          "Contrôles: [Espace]=play/pause, N=step, B=back, F=forward, S=save, R=resize, Q/Echap=quit\n");
  fflush(stdout);

  Grid g0 = {0};
  char err[256];

  if (args.input_path) {
    if (!grid_load_from_file(args.input_path, &g0, err, sizeof(err))) {
      fprintf(stderr, "Erreur chargement '%s': %s\n", args.input_path, err);
      return 1;
    }
  } else {
    int w = (args.w > 0) ? args.w : 50;
    int h = (args.h > 0) ? args.h : 30;
    if (args.w <= 0 || args.h <= 0) {
      prompt_size(&w, &h, w, h);
    }
    if (!grid_create(&g0, w, h)) {
      fprintf(stderr, "Allocation grille échouée (%d x %d)\n", w, h);
      return 1;
    }
  }

  /* Batch mode: --steps N and --output PATH => compute without SDL then save. */
  if (args.steps > 0 && args.output_path) {
    Grid cur = {0};
    Grid next = {0};
    if (!grid_create(&cur, g0.w, g0.h) || !grid_create(&next, g0.w, g0.h)) {
      fprintf(stderr, "Allocation échouée (batch)\n");
      grid_free(&g0);
      grid_free(&cur);
      grid_free(&next);
      return 1;
    }
    memcpy(cur.cells, g0.cells, (size_t)g0.w * (size_t)g0.h);

    for (int i = 0; i < args.steps; i++) {
      life_step(&cur, &next);
      grid_swap(&cur, &next);
    }
    if (!grid_save_to_file(args.output_path, &cur, err, sizeof(err))) {
      fprintf(stderr, "Erreur sauvegarde '%s': %s\n", args.output_path, err);
      grid_free(&g0);
      grid_free(&cur);
      grid_free(&next);
      return 1;
    }
    grid_free(&g0);
    grid_free(&cur);
    grid_free(&next);
    return 0;
  }

  History hist;
  if (!history_init(&hist, &g0, args.history_cap)) {
    fprintf(stderr, "Init historique échouée\n");
    grid_free(&g0);
    return 1;
  }
  grid_free(&g0);

  Grid scratch_next = {0};
  bool playing = (args.input_path != NULL);
  uint32_t last_tick = 0;
  const uint32_t step_ms = 120;

  UiSdl ui;
  if (!ui_init(&ui, "Jeu de la vie (Conway) - ring buffer", 960, 640)) {
    fprintf(stderr, "Init SDL échouée\n");
    history_free(&hist);
    return 1;
  }

  bool quit = false;
  while (!quit) {
    UiAction act = ui_poll_action(&quit);
    if (act == UI_ACT_QUIT) {
      quit = true;
    } else if (act == UI_ACT_TOGGLE_PLAY) {
      playing = !playing;
    } else if (act == UI_ACT_STEP) {
      playing = false;
      if (!do_step_and_push(&hist, &scratch_next)) {
        fprintf(stderr, "Step: échec (allocation/historique)\n");
      }
    } else if (act == UI_ACT_BACK) {
      playing = false;
      (void)history_back(&hist);
    } else if (act == UI_ACT_FORWARD) {
      playing = false;
      (void)history_forward(&hist);
    } else if (act == UI_ACT_SAVE) {
      playing = false;
      char path[512];
      const char *def = args.output_path ? args.output_path : "output.txt";
      prompt_path("Chemin de sauvegarde", path, sizeof(path), def);
      if (!grid_save_to_file(path, history_current_const(&hist), err, sizeof(err))) {
        fprintf(stderr, "Sauvegarde échouée: %s\n", err);
      } else {
        fprintf(stdout, "Sauvegardé: %s\n", path);
      }
    } else if (act == UI_ACT_RESIZE) {
      playing = false;
      int nw = 0, nh = 0;
      prompt_size(&nw, &nh, history_current(&hist)->w, history_current(&hist)->h);
      Grid *cur = history_current(&hist);
      Grid *resized = grid_clone(cur);
      if (!resized) {
        fprintf(stderr, "Resize: clone échoué\n");
      } else if (!grid_resize(resized, nw, nh)) {
        fprintf(stderr, "Resize: realloc échoué (%d x %d)\n", nw, nh);
        grid_destroy(resized);
      } else {
        History new_hist;
        if (!history_init(&new_hist, resized, args.history_cap)) {
          fprintf(stderr, "Resize: init historique échouée\n");
        } else {
          history_free(&hist);
          hist = new_hist;
          grid_free(&scratch_next);
          fprintf(stdout, "Resize OK -> %d x %d (historique réinitialisé)\n", nw, nh);
          fflush(stdout);
        }
        grid_destroy(resized);
      }
    }

    if (playing) {
      uint32_t now = SDL_GetTicks();
      if (now - last_tick >= step_ms) {
        last_tick = now;
        if (!do_step_and_push(&hist, &scratch_next)) {
          fprintf(stderr, "Play: step échoué (allocation/historique)\n");
          playing = false;
        }
      }
    }

    ui_render_grid(&ui, history_current_const(&hist));
    SDL_Delay(10);
  }

  ui_shutdown(&ui);
  grid_free(&scratch_next);
  history_free(&hist);
  return 0;
}

