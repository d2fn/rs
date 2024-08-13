#ifndef RS_GRAPHICS_H
#define RS_GRAPHICS_H

typedef struct {
    float x, y;
} rs_point;

typedef struct {
    rs_point ul;
    rs_point lr;
} rs_bb;

int rs_bb_contains_point(const rs_bb* bb, const rs_point* p);
float rs_point_distance(const rs_point* a, const rs_point* b);
int rs_collision_test(const rs_bb* a, const rs_bb* b);

#endif
