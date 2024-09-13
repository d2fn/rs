#version 430

#define KE 8.99

struct particle {
    vec2 position;
    vec2 v; // velocity
    vec2 a; // acceleration
    vec2 m; // mass
};

in vec2 fragPosition;
out vec4 fragColor;

layout(std430, binding = 1) readonly buffer particleLayout
{
    particle particles[];
};

uniform mat4 view;
uniform int numParticles;

// Function to linearly interpolate between two colors
vec3 lerp(vec3 c1, vec3 c2, float t) {
    return c1 + t * (c2 - c1);
}

vec3 mapToROYGBIV(float x) {
    // Define the ROYGBIV colors as vec3 (RGB) values
    vec3 red = vec3(1.0, 0.0, 0.0);
    vec3 orange = vec3(1.0, 0.5, 0.0);
    vec3 yellow = vec3(1.0, 1.0, 0.0);
    vec3 green = vec3(0.0, 1.0, 0.0);
    vec3 blue = vec3(0.0, 0.0, 1.0);
    vec3 indigo = vec3(0.29, 0.0, 0.51);
    vec3 violet = vec3(0.56, 0.0, 1.0);

    // Map the input value to the corresponding color range and interpolate
    if (x < 0.1667) {
        return lerp(red, orange, x / 0.1667);
    } else if (x < 0.3333) {
        return lerp(orange, yellow, (x - 0.1667) / 0.1667);
    } else if (x < 0.5) {
        return lerp(yellow, green, (x - 0.3333) / 0.1667);
    } else if (x < 0.6667) {
        return lerp(green, blue, (x - 0.5) / 0.1667);
    } else if (x < 0.8333) {
        return lerp(blue, indigo, (x - 0.6667) / 0.1667);
    } else {
        return lerp(indigo, violet, (x - 0.8333) / 0.1667);
    }
}

void main() {

    // Get the screen position of the current fragment
    vec4 screenPos = vec4(gl_FragCoord.xy, 0.0, 1.0);
    vec4 worldPos = view * screenPos;
    worldPos.y *= -1;
    // worldPos.y += 20;

    if (numParticles > 0) {
        float efield = 0.0;
        for(int i = 0; i < numParticles; i++) {
            vec2 p = particles[i].position;
            float mass = particles[i].m.x;
            vec2 fieldPoint = worldPos.xy - p;
            float dist = distance(worldPos.xy, p);
            // vec2 dir = fieldPoint / length(fieldPoint);
            if (dist > 0.01) {
                efield += (KE * mass) / (dist * dist);
            }

            // vec2 efield = magnitude * dir; 

            // float mass = p.m.x;
            // if(distance(p.position, worldPos.xy) < mass) {
            //     fragColor = vec4(1, 0, 1, 1);
            //     return;
            // }
        }
        fragColor = vec4(mapToROYGBIV(efield/3.0), 1.0);
        return;
    }
    discard;
}
