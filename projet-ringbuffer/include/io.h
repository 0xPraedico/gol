#ifndef IO_H
#define IO_H

#include <stdbool.h>
#include <stddef.h>

#include "grid.h"

/*
 * Format:
 *   width height
 *   then height lines, each with width chars in {'.','O'}
 *   trailing spaces/tabs are allowed.
 */
bool grid_load_from_file(const char *path, Grid *out, char *err, size_t errcap);
bool grid_save_to_file(const char *path, const Grid *g, char *err, size_t errcap);

#endif /* IO_H */
