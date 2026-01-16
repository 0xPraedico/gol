#include "io.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static void set_err(char *err, size_t cap, const char *msg) {
  if (!err || cap == 0) {
    return;
  }
  (void)snprintf(err, cap, "%s", msg ? msg : "Erreur inconnue");
}

static void set_errf(char *err, size_t cap, const char *fmt, const char *detail) {
  if (!err || cap == 0) {
    return;
  }
  (void)snprintf(err, cap, fmt, detail ? detail : "");
}

bool grid_load_from_file(const char *path, Grid *out, char *err, size_t errcap) {
  if (!path || !out) {
    set_err(err, errcap, "Paramètres invalides");
    return false;
  }

  FILE *f = fopen(path, "r");
  if (!f) {
    set_errf(err, errcap, "Impossible d'ouvrir en lecture: %s", strerror(errno));
    return false;
  }

  char header[256];
  if (!fgets(header, (int)sizeof(header), f)) {
    fclose(f);
    set_err(err, errcap, "Fichier vide ou illisible");
    return false;
  }

  int w = 0, h = 0;
  if (sscanf(header, "%d %d", &w, &h) != 2 || w <= 0 || h <= 0) {
    fclose(f);
    set_err(err, errcap, "En-tête invalide (attendu: width height)");
    return false;
  }

  Grid tmp = {0};
  if (!grid_create(&tmp, w, h)) {
    fclose(f);
    set_err(err, errcap, "Allocation échouée pour la grille");
    return false;
  }

  for (int y = 0; y < h; y++) {
    int x = 0;
    while (x < w) {
      int c = fgetc(f);
      if (c == EOF) {
        grid_free(&tmp);
        fclose(f);
        set_err(err, errcap, "EOF prématuré lors de la lecture des cellules");
        return false;
      }
      if (c == '\r') {
        continue;
      }
      if (c == '\n') {
        grid_free(&tmp);
        fclose(f);
        set_err(err, errcap, "Ligne trop courte (pas assez de cellules)");
        return false;
      }
      if (c == '.' || c == 'O') {
        grid_set(&tmp, x, y, (c == 'O') ? 1u : 0u);
        x++;
        continue;
      }
      if (c == ' ' || c == '\t') {
        /* spaces allowed (including between cells) */
        continue;
      }
      grid_free(&tmp);
      fclose(f);
      set_err(err, errcap, "Caractère invalide (attendu '.' ou 'O')");
      return false;
    }

    /* Consume the rest of the line (spaces/tabs allowed) until '\n' */
    for (;;) {
      int c = fgetc(f);
      if (c == EOF) {
        /* EOF after the last line: ok */
        break;
      }
      if (c == '\r') {
        continue;
      }
      if (c == '\n') {
        break;
      }
      if (c == ' ' || c == '\t') {
        continue;
      }
      grid_free(&tmp);
      fclose(f);
      set_err(err, errcap, "Caractères en trop en fin de ligne");
      return false;
    }
  }

  fclose(f);

  grid_free(out);
  *out = tmp;
  return true;
}

bool grid_save_to_file(const char *path, const Grid *g, char *err, size_t errcap) {
  if (!path || !g || !g->cells) {
    set_err(err, errcap, "Paramètres invalides");
    return false;
  }

  FILE *f = fopen(path, "w");
  if (!f) {
    set_errf(err, errcap, "Impossible d'ouvrir en écriture: %s", strerror(errno));
    return false;
  }

  if (fprintf(f, "%d %d\n", g->w, g->h) < 0) {
    fclose(f);
    set_err(err, errcap, "Erreur d'écriture (header)");
    return false;
  }

  for (int y = 0; y < g->h; y++) {
    for (int x = 0; x < g->w; x++) {
      uint8_t v = grid_get(g, x, y);
      if (fputc(v ? 'O' : '.', f) == EOF) {
        fclose(f);
        set_err(err, errcap, "Erreur d'écriture (cellules)");
        return false;
      }
    }
    if (fputc('\n', f) == EOF) {
      fclose(f);
      set_err(err, errcap, "Erreur d'écriture (newline)");
      return false;
    }
  }

  if (fclose(f) != 0) {
    set_err(err, errcap, "Erreur lors de la fermeture du fichier");
    return false;
  }
  return true;
}
