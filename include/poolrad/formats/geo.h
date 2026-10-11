#ifndef POOLRAD_FORMATS_GEO_H
#define POOLRAD_FORMATS_GEO_H

#include <stddef.h>
#include <stdint.h>

#define PR_GEO_WIDTH 16u
#define PR_GEO_SIZE 1026u

typedef enum { PR_GEO_NORTH = 0, PR_GEO_EAST, PR_GEO_SOUTH, PR_GEO_WEST } pr_geo_direction;
typedef struct { uint8_t bytes[PR_GEO_SIZE]; } pr_geo_map;

/* Copies one decoded 16x16 map. The first two bytes remain opaque. */
int pr_geo_parse(const uint8_t *data, size_t size, pr_geo_map *out);
/* Return 0 on success, -1 for an invalid coordinate, direction or pointer. */
int pr_geo_wall(const pr_geo_map *map, unsigned x, unsigned y,
                pr_geo_direction direction, uint8_t *value);
int pr_geo_event(const pr_geo_map *map, unsigned x, unsigned y, uint8_t *value);
int pr_geo_door(const pr_geo_map *map, unsigned x, unsigned y,
                pr_geo_direction direction, uint8_t *value);

#endif
