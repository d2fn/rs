#version 430

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

struct particle {
    vec2 position;
    vec2 v; // velocity
    vec2 a; // acceleration
    vec2 m; // mass
};

struct scene_info {
    float G;
    int num_particles;
};

layout (std430, binding = 1) readonly restrict buffer gLayout1 {
    particle src[];
};

layout (std430, binding = 2) writeonly restrict buffer gLayout2 {
    particle dst[];
};

layout (std430, binding = 3) readonly restrict buffer gLayout3 {
    scene_info info;
};

void main() {
    uint i = gl_GlobalInvocationID.x;

    if (i < info.num_particles) {

        particle left = src[i];
        vec2 force = vec2(0,0);

        for (int j = 0; j < 100; j++) {
            if (i != j) {
                particle right = src[j];
                vec2 r = left.position - right.position;
                float dist = distance(left.position, right.position);
                if (dist < 2) dist = 2;
                vec2 dir = r / dist;
                float F = info.G * (left.m.x * right.m.x) / (dist * dist);
                force += F * dir;

                /**
                r = left.position - vec2(0,0);
                dist = distance(r, vec2(0,0));
                if (dist < 2) dist = 2;
                dir = r / dist;
                F = info.G * (left.m.x * 100) / (dist * dist);
                force += F * dir;
                **/
            }
        }

        // F = ma => a = F/m
        vec2 a = force / left.m.x;
        dst[i].position = left.position + left.v;
        dst[i].v = 0.9 * (left.v + left.a);
        dst[i].a = a;
        dst[i].m = left.m;
    }
}
