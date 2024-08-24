#include "rs_geometry.h"
#include <math.h>

int rs_bb_contains_point(const rs_bb* bb, const rs_point* p) {
    return
        p->x >= bb->ul.x && p->x <= bb->lr.x &&
        p->y >= bb->ul.y && p->y <= bb->lr.y;
}

float rs_point_distance(const rs_point* a, const rs_point* b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    return sqrt(dx * dx + dy * dy);
}

int rs_collision_test(const rs_bb* a, const rs_bb* b) {

    // Check if one box is completely to the left of the other
    if (a->lr.x < b->ul.x || b->lr.x < a->ul.x) {
        return 0;
    }

    // Check if one box is completely below the other
    if (a->ul.y > b->lr.y || b->lr.y > a->ul.y) {
        return 0;
    }

    return 1;
}
