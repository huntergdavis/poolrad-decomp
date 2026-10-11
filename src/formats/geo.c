#include "poolrad/formats/geo.h"

#include <string.h>

int pr_geo_parse(const uint8_t *data, size_t size, pr_geo_map *out) {
    if (!data || !out || size != PR_GEO_SIZE) return -1;
    memcpy(out->bytes, data, PR_GEO_SIZE);
    return 0;
}

static int tile_index(const pr_geo_map *map, unsigned x, unsigned y,
                      uint8_t *value) {
    return !map || !value || x >= PR_GEO_WIDTH || y >= PR_GEO_WIDTH ? -1 : 0;
}

int pr_geo_wall(const pr_geo_map *map, unsigned x, unsigned y,
                pr_geo_direction direction, uint8_t *value) {
    uint8_t packed;
    if (tile_index(map, x, y, value) || (unsigned)direction > PR_GEO_WEST) return -1;
    packed = map->bytes[2 + y * PR_GEO_WIDTH + x +
                        (direction >= PR_GEO_SOUTH ? 256 : 0)];
    *value = (uint8_t)((direction % 2 ? packed : packed >> 4) & 15);
    return 0;
}

int pr_geo_event(const pr_geo_map *map, unsigned x, unsigned y, uint8_t *value) {
    if (tile_index(map, x, y, value)) return -1;
    *value = map->bytes[514 + y * PR_GEO_WIDTH + x];
    return 0;
}

int pr_geo_door(const pr_geo_map *map, unsigned x, unsigned y,
                pr_geo_direction direction, uint8_t *value) {
    if (tile_index(map, x, y, value) || (unsigned)direction > PR_GEO_WEST) return -1;
    *value = (uint8_t)((map->bytes[770 + y * PR_GEO_WIDTH + x] >>
                        (direction * 2)) & 3);
    return 0;
}
