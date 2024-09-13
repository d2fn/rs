#version 430

#define EPSILON 0.5
#define TIME_STEP 1.0


layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

struct particle {
    vec2 position;
    vec2 v; // velocity
    vec2 a; // acceleration
    vec2 m; // mass
};

struct state {
    float G;
    float drag;
    int num_particles;
    int num_fixed_particles;
};

layout (std430, binding = 1) readonly restrict buffer gLayout1 {
    particle src[];
};

layout (std430, binding = 2) writeonly restrict buffer gLayout2 {
    particle dst[];
};

layout (std430, binding = 3) readonly restrict buffer gLayout3 {
    particle fixed_particles[];
};

layout (std430, binding = 4) readonly restrict buffer gLayout4 {
    state info;
};


void main() {
    uint i = gl_GlobalInvocationID.x;

    if (i < info.num_particles) {

        particle left = src[i];
        vec2 force = vec2(0,0);

        for (int j = 0; j < info.num_particles; j++) {
            if (i != j) {
                particle right = src[j];
                vec2 r = left.position - right.position;
                // vec2 r = right.position - left.position;
                float dist = sqrt(r.x*r.x + r.y*r.y);
                if (dist > EPSILON) {
                    vec2 dir = r / dist;
                    // vec2 dir = normalize(r);
                    float F = info.G * (left.m.x * right.m.x) / (dist * dist);
                    force += F * r / dist;// - 0.0001 * dir / (dist * dist);
                    // force -= 0.001 * (0.01 / (left.m.x + right.m.x)) * (1/dist) * dir;

                    // vec2 repulsion = -info.G * dir / (dist * dist);
                    // vec2 attraction = -1e-8f * (0.01 / (left.m.x + right.m.x)) * (1 / dist) * dir;
                    // force = repulsion - attraction;

                    // float fx = F * r.x / dist;
                    // float fy = F * r.y / dist;
                    // force.x += fx;
                    // force.y += fy;

                    /**
                    r = left.position - vec2(0,0);
                    dist = distance(r, vec2(0,0));
                    dir = r / dist;
                    F = info.G * (left.m.x * 100) / (dist * dist);
                    force += F * dir;
                    **/
                }
            }
        }

        for (int j = 0; j < info.num_fixed_particles; j++) {
            if (i != j) {
                particle right = fixed_particles[j];
                vec2 r = left.position - right.position;
                float dist = distance(left.position, right.position);
                if (dist < EPSILON) dist = EPSILON;
                vec2 dir = r / dist;
                float F = info.G * (left.m.x * right.m.x) / (dist * dist);
                force += F * dir;
            }
        }

        // F = ma => a = F/m
        vec2 new_position = left.position + left.v;

        vec2 a = force / (left.m.x);
        dst[i].position = new_position;
        dst[i].v = left.v + (TIME_STEP * left.a) - info.drag * left.v;
        dst[i].a = a;
        dst[i].m.x = left.m.x;

        // vec2 a = force / (left.m.x);
        /*
        dst[i].position = new_position;
        dst[i].v = (1.0 - info.drag) * (left.v + left.a);
        dst[i].a = a;
        dst[i].m.x = left.m.x;
        */
    }
}
