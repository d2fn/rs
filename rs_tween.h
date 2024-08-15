#ifndef RS_TWEEN_H
#define RS_TWEEN_H

#include "rs_types.h"

#define DAMPING 0.5
#define ATTRACTION 0.2
#define EPSILON 0.0001

rs_tween* rs_make_tween(float value);
void rs_free_tween(rs_tween* t);
void rs_tween_target(rs_tween* t, float target);
void rs_tween_set(rs_tween* t, float value);
void rs_tween_update(rs_tween* t);
float rs_tween_poll(rs_tween* t);
float rs_tween_poll_target(rs_tween* t);

/*
package com.d2fn.sumi.sketch;

 * This class is for simple easeIn / easeOut functionality. Based on springs.
 *
 * @author Ben Fry
 *
public class Integrator {

    static final float DAMPING = 0.5f; // formerly 0.9f
    static final float ATTRACTION = 0.2f; // formerly 0.1f

    public float value = 0;
    public float vel = 0;
    public float accel = 0;
    public float force = 0;
    public float mass = 1;

    public float damping;
    public float attraction;
    public boolean targeting;
    public float target;

    public float prev = Float.MAX_VALUE;
    public float epsilon = 0.0001f;

    public Integrator() {
        this(0, DAMPING, ATTRACTION);
    }

    public Integrator(float value) {
        this(value, DAMPING, ATTRACTION);
    }

    public Integrator(float value, float damping, float attraction) {
        this.value = value;
        this.damping = damping;
        this.attraction = attraction;
        set(value);
    }

    public void set(float v) {
        value = v;
        target = value;
        noTarget();
    }

    public void set(double v) {
        value = (float) v;
        target = value;
        noTarget();
    }

     * @return Update for next time step. Returns true if actually updated, false if no longer changing.
     *
    public boolean update() {
        if (targeting) {
            force += attraction * (target - value);
        }

        accel = force / mass;
        vel = (vel + accel) * damping;
        value += vel;

        force = 0;

        if (Math.abs(value - prev) < epsilon) {
            value = target;
            return false;
        }
        prev = value;
        return true;
    }

    public void target(float t) {
        targeting = true;
        target = t;
    }

    public void target(double d) {
        target((float) d);
    }

    public void noTarget() {
        targeting = false;
    }

    public float get() {
        return value;
    }
}

*/

#endif
