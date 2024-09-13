#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <string.h>

#include "raylib.h"
#include "rlgl.h"

// raygui
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "raymath.h"

#include "rs.h"

#define WIDTH                         (1920)
#define HEIGHT                        (1080)
#define PIXEL_SIZE                       (1)
#define FPS                           (3000)
#define WORLD_WIDTH              ((1<<11)+1)
#define WORLD_HEIGHT             ((1<<11)+1)
#define WORLD_CENTER_X     (((1<<11)+1)/2.0)
#define WORLD_CENTER_Y     (((1<<11)+1)/2.0)

#define GRAVITY                       (1e-4)
#define MASS_HI                        (100)
#define MASS_LO                          (1)
#define DRAG                          (0.01)

#define COLOR_MODE_DARK                  (0)
#define COLOR_MODE_LIGHT                 (1)

#define COLOR_TYPE_BW                    (0)
#define COLOR_TYPE_COLOR                 (1)
#define COLOR_TYPE_HALFTONE              (2)

typedef struct {
    Vector2 ul;
    Vector2 lr;
} bb;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    Vector2 mass;
} particle;

typedef struct {
    void* data;
    unsigned int size;
    unsigned int ptr;
} circular_buffer;

typedef struct {
    float G;
    float drag;
    int num_particles;
    int num_fixed_particles;
} state;

#define MAX_PARTICLES 50000

Rectangle player = { 0, 0, 20, 20 };
Camera2D camera = { 0 };
state info = { GRAVITY, DRAG, 0, 0};
particle* src_particles;
particle* fixed_particles;
Vector2* positions;
particle* dst_particles;
Image targetImage;
int generating_points = 0;

unsigned int compute_forces_program;
unsigned int ssboA;
unsigned int ssboB;
unsigned int ssboF;
unsigned int info_buffer;

//
// layout_name: controls initialization
//----------------------------------------------------------------------------------
float GravitySliderValue = GRAVITY;
float DragSliderValue = 0.01f;
int ColorMode = 0;
int ColorType = 0;
float ParticleScaleSliderValue = 0.03f;
float ReplicationRateSliderValue = 0.0f;
bool ShouldReplicate = false;
//----------------------------------------------------------------------------------

float min(float a, float b, float c, float d) {
    float m = a;
    if (b < m) m = b;
    if (c < m) m = c;
    if (d < m) m = d;
    return m;
}

float max(float a, float b, float c, float d) {
    float m = a;
    if (b > m) m = b;
    if (c > m) m = c;
    if (d > m) m = d;
    return m;
}

float brightness(Color c) {

    int cmax = c.r > c.g ? c.r : c.g;
    if (c.b > cmax) {
        cmax = c.b;
    }

    return (float)cmax / 255.0F;
}

Color sample_color(const Image image, Vector2 world) {
    float scale = 5;
    int imagex = (int)round(image.width/2.0 + (world.x * scale));
    int imagey = (int)round(image.height/2.0 + (world.y * scale));
    if (imagex >= image.width) return BLANK;
    if (imagex < 0) return BLANK;
    if (imagey >= image.height) return BLANK;
    if (imagey < 0) return BLANK;
    Color c = GetImageColor(image, imagex, imagey);
    return c;
}

float compute_charge_mass_at_point(const Image image, Vector2 world) {
    Color c = sample_color(image, world);
    float b = brightness(c);
    if (ColorMode == COLOR_MODE_LIGHT) {
        b = 1.0 - b;
    }
    return rs_remap(b, 0.0, 1.0, MASS_LO, MASS_HI);
}

void setup_camera() {
    camera.target = (Vector2){ player.x, player.y };
    camera.offset = (Vector2){ WIDTH/2.0, HEIGHT/2.0 };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
}

void add_particle(Vector2 worldPosition) {
    int i = info.num_particles;
    if (i < MAX_PARTICLES) {
        Vector2 p = (Vector2) { worldPosition.x + GetRandomValue(-5, 5), worldPosition.y + GetRandomValue(-5, 5) };
        float charge_mass = compute_charge_mass_at_point(targetImage, p);
        if (charge_mass > 0) {
            src_particles[i].position = p;
            src_particles[i].velocity = (Vector2) { 0, 0 };
            src_particles[i].acceleration = (Vector2) { 0, 0 };
            src_particles[i].mass.x = charge_mass;
            info.num_particles++;
            if (info.num_particles > MAX_PARTICLES) {
                info.num_particles = MAX_PARTICLES;
            }
        }
    }
}

void process_input() {

    if (IsKeyDown(KEY_RIGHT)) {
        player.x += 2;
    }
    if (IsKeyDown(KEY_LEFT)) {
        player.x -= 2;
    }
    if (IsKeyDown(KEY_UP)) {
        player.y -= 2;
    }
    if (IsKeyDown(KEY_DOWN)) {
        player.y += 2;
    }

    camera.target = (Vector2){ player.x + player.width/2.0, player.y + player.height/2.0 };

    if (IsKeyDown(KEY_A)) {
        camera.rotation--;
    }
    if (IsKeyDown(KEY_S)) {
        camera.rotation++;
    }

    if (IsKeyDown(KEY_SPACE)) {
        generating_points = !generating_points;
    }

    if (IsMouseButtonDown(0) || generating_points) {
        Vector2 mousePosition = (Vector2) { GetMouseX(), GetMouseY() };
        if (mousePosition.x > 300) {
            Vector2 worldPosition = GetScreenToWorld2D(mousePosition, camera);
            add_particle(worldPosition);
        }
        /**
        if (Vector2Distance(worldPosition, (Vector2){0,0}) < 325) {
            int i = info.num_particles;
            if (i < MAX_PARTICLES) {
                Vector2 p = (Vector2) { worldPosition.x + GetRandomValue(-5, 5), worldPosition.y + GetRandomValue(-5, 5) };
                float gamma = compute_charge_mass_at_point(targetImage, src_particles[i].position);
                if (gamma > 0) {
                    src_particles[i].position = p;
                    src_particles[i].mass.x = gamma;
                    src_particles[i].velocity = (Vector2) { 0, 0 };
                    src_particles[i].acceleration = (Vector2) { 0, 0 };
                    src_particles[i].mass.x = gamma;
                    info.num_particles++;
                    if (info.num_particles > MAX_PARTICLES) info.num_particles = MAX_PARTICLES;
                }
            }
        }
        **/
    }

    if (camera.rotation >  40) camera.rotation =  40;
    if (camera.rotation < -40) camera.rotation = -40;

    // Camera zoom controls
    
    camera.zoom += ((float)GetMouseWheelMove()*0.05f);

    // Camera reset (zoom and rotation)
    if (IsKeyPressed(KEY_R))
    {
        camera.zoom = 1.0f;
        camera.rotation = 0.0f;
    }
}

void compute_particle_forces() {

    rlUpdateShaderBuffer(ssboA, src_particles, MAX_PARTICLES * sizeof(particle), 0);
    rlUpdateShaderBuffer(info_buffer, &info, sizeof(state), 0);

    rlEnableShader(compute_forces_program);
    rlBindShaderBuffer(ssboA, 1);
    rlBindShaderBuffer(ssboB, 2);
    rlBindShaderBuffer(ssboF, 3);
    rlBindShaderBuffer(info_buffer, 4);
    rlComputeShaderDispatch(info.num_particles, 1, 1);
    rlDisableShader();

    rlReadShaderBuffer(ssboB, src_particles, sizeof(particle) * MAX_PARTICLES, 0);
    rlReadShaderBuffer(ssboA, dst_particles, sizeof(particle) * MAX_PARTICLES, 0);

    unsigned int tmp = ssboA;
    ssboA = ssboB;
    ssboB = tmp;
}

void replicate_particles(int n) {
    for(int i = 0; i < n; i++) {
        int j = GetRandomValue(0, info.num_particles);
        add_particle(src_particles[j].position);
    }
}

void process_updates() {
    compute_particle_forces();
    if (ShouldReplicate) {
        replicate_particles((int)round(ReplicationRateSliderValue));
    }
}

void render_particles() {
}

int main() {

    InitWindow(WIDTH, HEIGHT, "RS");
    SetTargetFPS(FPS);

    setup_camera();

    char *compute_forces_code = LoadFileText("resources/compute_forces.glsl");
    unsigned int compute_forces_shader = rlCompileShader(compute_forces_code, RL_COMPUTE_SHADER);
    compute_forces_program = rlLoadComputeShaderProgram(compute_forces_shader);
    UnloadFileText(compute_forces_code);

    src_particles = malloc(MAX_PARTICLES * sizeof(particle));
    dst_particles = malloc(MAX_PARTICLES * sizeof(particle));
    fixed_particles = malloc(MAX_PARTICLES * sizeof(particle));
    positions = malloc(MAX_PARTICLES * sizeof(Vector2));
    memcpy(dst_particles, src_particles, sizeof(particle) * MAX_PARTICLES);

    float r = 350;
    int i = 0;
    for(float theta = 0; theta < 2*PI; theta += PI/256.0) {
        fixed_particles[i].position = (Vector2) { r * cos(theta), r * sin(theta) };
        fixed_particles[i].mass = (Vector2) { 10000, 0 };
        info.num_fixed_particles++;
        i++;
    }

    ssboA = rlLoadShaderBuffer(sizeof(particle) * MAX_PARTICLES, src_particles, RL_DYNAMIC_COPY);
    ssboB = rlLoadShaderBuffer(sizeof(particle) * MAX_PARTICLES, dst_particles, RL_DYNAMIC_COPY);
    ssboF = rlLoadShaderBuffer(sizeof(particle) * MAX_PARTICLES, fixed_particles, RL_DYNAMIC_COPY);
    info_buffer = rlLoadShaderBuffer(sizeof(state), &info, RL_DYNAMIC_COPY);

    /*Shader circle_shader = LoadShader("resources/world2cam_vs.glsl", "resources/circle_fs.glsl");*/
    Shader circle_shader = LoadShader(0, "resources/circle_fs.glsl");
    Shader electric_field_shader = LoadShader(0, "resources/electric_field_fs.glsl");

    // Create a white texture of the size of the window to update 
    // each pixel of the window using the fragment shader: golRenderShader
    Image whiteImage = GenImageColor(1000, 1000, BLUE);
    Texture whiteTex = LoadTextureFromImage(whiteImage);
    UnloadImage(whiteImage);

    targetImage = LoadImage("resources/Rape_of_Prosepina.png");
    /*gammaField = LoadImage("resources/gl.png");*/

    GuiLoadStyle("resources/styles/jungle/style_jungle.rgs");

    while (!WindowShouldClose()) {

        process_input();

        BeginDrawing();

        if (ColorMode == COLOR_MODE_DARK) {
            ClearBackground(BLACK);
        }
        else {
            ClearBackground(WHITE);
        }

        /*
        Matrix view = MatrixInvert(GetCameraMatrix2D(camera));
        SetShaderValueMatrix(electric_field_shader, GetShaderLocation(electric_field_shader, "view"), view);
        SetShaderValue(electric_field_shader, GetShaderLocation(electric_field_shader, "numParticles"), &info.num_particles, SHADER_UNIFORM_INT);

        rlEnableShader(electric_field_shader.id);
        rlBindShaderBuffer(ssboA, 1);
        rlDisableShader();

        BeginShaderMode(electric_field_shader);
            DrawRectangle(0, 0, WIDTH, HEIGHT, BLANK);
        EndShaderMode();
        */

        /*for(int i = 0; i < 100; i++) {*/
        /*}*/

        BeginMode2D(camera);
        for (int i = 0; i < info.num_particles; i++) {
            particle p = src_particles[i];
            src_particles[i].mass.x = compute_charge_mass_at_point(targetImage, p.position);
            /*
            Vector2 v = src_particles[i].velocity;
            float speed = sqrtf(v.x*v.x+v.y*v.y);
            float cspeed = rs_remap(speed, 0, 0.2, 0, 255);
            if (cspeed > 255) cspeed = 255;
            */
            Color c;
            if (ColorType == COLOR_TYPE_BW) {
                c = (ColorMode == COLOR_MODE_DARK) ? WHITE : BLACK;
            }
            else {
                c = sample_color(targetImage, p.position);
            }
            /*float r = sqrtf(ParticleScaleSliderValue * p.mass.x/PI);*/
            float r = ParticleScaleSliderValue * p.mass.x;
            /*p.mass.x = brightness(c);*/
            /*float r = p.mass.x / 10;*/
            /*float r = 1;*/
            DrawCircleV(p.position, r, c);
        }
        EndMode2D();
        

        /*DrawRectangle( 10, 10, 250, 150, Fade(SKYBLUE, 0.5f));*/
        /*DrawRectangleLines( 10, 10, 250, 150, BLUE);*/
        /**/
        /*DrawText("Free 2d camera controls:", 20, 20, 10, BLACK);*/
        /*DrawText("- Right/Left to move Offset", 40, 40, 10, DARKGRAY);*/
        /*DrawText("- Mouse Wheel to Zoom in-out", 40, 60, 10, DARKGRAY);*/
        /*DrawText("- A / S to Rotate", 40, 80, 10, DARKGRAY);*/
        /*DrawText("- R to reset Zoom and Rotation", 40, 100, 10, DARKGRAY);*/
        /*char particle_msg[100];*/
        /*sprintf(particle_msg, "- Click and drag to add particles (%d)", info.num_particles);*/
        /*DrawText(particle_msg, 40, 120, 10, DARKGRAY);*/
        //
        // raygui: controls drawing
        //----------------------------------------------------------------------------------
        GuiGroupBox((Rectangle){ 24, 24, 304, 184 }, "Physics");
        GuiSliderBar((Rectangle){ 104, 48, 208, 24 }, "Gravity", NULL, &GravitySliderValue, 1e-6f, 1e-2f);
        GuiSliderBar((Rectangle){ 104, 80, 208, 24 }, "Drag", NULL, &DragSliderValue, 0.0f, 0.2f);
        GuiToggle((Rectangle){ 40, 128, 272, 24 }, "Start Replicating", &ShouldReplicate);
        GuiSliderBar((Rectangle){ 152, 160, 160, 24 }, "Replication Rate", NULL, &ReplicationRateSliderValue, 1, 10);

        GuiGroupBox((Rectangle){ 24, 232, 304, 152 }, "Theme");
        GuiToggleGroup((Rectangle){ 40, 256, 136, 24 }, "Dark;Light", &ColorMode);
        GuiToggleGroup((Rectangle){ 40, 296, 88, 24 }, "B&W;Color;Halftone", &ColorType);
        GuiSliderBar((Rectangle){ 80, 336, 232, 24 }, "Scale", NULL, &ParticleScaleSliderValue, 0.02, 0.1f);
        //----------------------------------------------------------------------------------

        info.G = GravitySliderValue;
        info.drag = DragSliderValue;

        //----------------------------------------------------------------------------------

        EndDrawing();

        process_updates();
    }

    rlUnloadShaderBuffer(ssboA);
    rlUnloadShaderBuffer(ssboB);
    rlUnloadShaderBuffer(info_buffer);
    rlUnloadShaderProgram(compute_forces_program);
    UnloadTexture(whiteTex);            // Unload white texture
    UnloadShader(circle_shader);      // Unload rendering fragment shader
    
    CloseWindow();
    return 0;
}
