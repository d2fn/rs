#include "rs_tween.h"
#include "stdlib.h"
#include "float.h"
#include "math.h"


rs_tween* rs_make_tween(float value) {
    rs_tween* t = malloc(sizeof(rs_tween));
    t->value = value;
    t->vel = 0;
    t->accel = 0;
    t->force = 0;
    t->mass = 1;
    t->damping = DAMPING;
    t->attraction = ATTRACTION;
    t->prev = FLT_MAX;
    t->targeting = 0;
    t->target = value;
    return t;
}

void rs_free_tween(rs_tween* t) {
    free(t);
}

void rs_tween_target(rs_tween* t, float target) {
    t->targeting = 1;
    t->target = target;
}

void rs_tween_set(rs_tween* t, float value) {
    t->targeting = 0;
    t->target = value;
    t->value = value;
}

void rs_tween_update(rs_tween* t) {
    if (t->targeting > 0) {
        t->force += t->attraction * (t->target - t->value);
    }

    t->accel = t->force / t->mass;
    t->vel = (t->vel + t->accel) * t->damping;
    t->value += t->vel;

    t->force = 0;

    if (fabs(t->value - t->prev) < EPSILON) {
        t->value = t->target;
        return;
    }

    t->prev = t->value;
}

float rs_tween_poll(rs_tween* t) {
    return t->value;
}

float rs_tween_poll_target(rs_tween* t) {
    return t->target;
}
