#include <iostream>
#include <SDL.h>
#include <stdint.h>
#include <string>

// #include "shader_pixel.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 256

#include "SDLauxiliary.h"
#include "scene.h"
#include "shader_vertex.h"
#include "rasmath.h"
#include "shader_pixel.h"
#include "rasteriser.h"
#include "shader_post.h"

#include "./opencl/opencl.h"

using namespace std;
using glm::vec2;
using glm::ivec2;
using glm::vec3;
using glm::mat3;
using glm::vec4;
using glm::mat4;

#define FULLSCREEN_MODE false
#define TRANSLATION_SPEED 1
#define ROTATION_SPEED 1

#define FACE_CULLING 1

float current_tick_count = 0;
float current_millis = 0;

//Scene
Scene scene;

//Shader
VertexShader vertex_shader(SCREEN_WIDTH, SCREEN_HEIGHT);

float depth_buffer[SCREEN_WIDTH][SCREEN_HEIGHT];
Pixel pixel_buffer[SCREEN_WIDTH][SCREEN_HEIGHT];
vec3 colour_buffer[SCREEN_WIDTH][SCREEN_HEIGHT];
const vec3 BLACK(0, 0, 0);

//Transformation
float f = ((float)SCREEN_HEIGHT / 2);
vec2 center_translation(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
vec4 cameraPos(0, 0, -3.0, 1.0);
vec4 cameraRot(0, 0, 0, 0); //Pitch, yaw, roll
mat4 cameraMatrix;

//OpenCL

ocl opencl;


/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Init();
bool Update();
void Draw(screen* screen);

int main( int argc, char* argv[] )
{

  screen *screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE );

  Init();
  while ( Update())
    {
      Draw(screen);
      SDL_Renderframe(screen);
    }

  SDL_SaveImage( screen, "screenshot.bmp" );
  PostShaderPost();
  KillSDL(screen);
  return 0;
}

void Init() {
  ConstructScene(scene);
  InitOpenCL(opencl, scene);
  CLPostProcessInit(opencl);
  PostShaderInit();
  CLRegisterTextures(opencl, scene.textures);
}


void ClearDepthBuffer() {
  for( int y=0; y<SCREEN_HEIGHT; ++y ) {
    for( int x=0; x<SCREEN_WIDTH; ++x ) {
      depth_buffer[x][y] = 0;
      pixel_buffer[x][y].dead = 1;
      colour_buffer[x][y] = BLACK;
    }
  }
}

/*Place your drawing here*/
void Draw (screen* screen) {

  /* Clear buffer */
  // memset(screen->buffer, 0, screen->height*screen->width*sizeof(uint32_t));
  // ClearDepthBuffer();
  CLClearScreen(opencl);
  CLRegisterLights(opencl, scene.lights);

  cameraMatrix = TransformationMatrix(-cameraPos, cameraRot);

  for (int i = 0; i < (int)scene.triangles.size(); i++) {

    if (FACE_CULLING && scene.triangles[i].culled == true) {
        float dot = glm::dot(vec3(scene.triangles[i].culling_pos - cameraPos), vec3(scene.triangles[i].culling_normal));
        if(dot >= 0) continue;
    }

    vector<Vertex> vertices(3);
    vertices[0].position = scene.triangles[i].v0;
    vertices[1].position = scene.triangles[i].v1;
    vertices[2].position = scene.triangles[i].v2;
    vertices[0].transformed = cameraMatrix * scene.triangles[i].v0;
    vertices[1].transformed = cameraMatrix * scene.triangles[i].v1;
    vertices[2].transformed = cameraMatrix * scene.triangles[i].v2;
    vertices[0].normal = scene.triangles[i].normal0;
    vertices[1].normal = scene.triangles[i].normal1;
    vertices[2].normal = scene.triangles[i].normal2;
    vertices[0].color = scene.triangles[i].color;
    vertices[1].color = scene.triangles[i].color;
    vertices[2].color = scene.triangles[i].color;
    vertices[0].uv = scene.triangles[i].uv0;
    vertices[1].uv = scene.triangles[i].uv1;
    vertices[2].uv = scene.triangles[i].uv2;


    vector<Pixel> vertexPixels;
    vertex_shader.compute(vertices, vertexPixels);
    for(int p = 0; p < (int)(vertexPixels.size() - 2); p++) {
      Pixel pixels[3] = {vertexPixels[0], vertexPixels[p + 1], vertexPixels[p + 2]};
      CLDrawPolygon(opencl, pixels, scene, scene.triangles[i].tex_id);
    }
    // vector<Pixel> vertexPixels(3);
    // for( int i=0; i<3; ++i )
    //    vertex_shader.compute( vertices[i], cameraMatrix, vertexPixels[i]);


    // CLDrawPolygon(opencl, vertexPixels, scene, scene.triangles[i].tex_id);
    // DrawPolygon(screen, vertexPixels, depth_buffer, pixel_buffer, colour_buffer, scene);
  }

  // PostShader(screen, scene, depth_buffer, pixel_buffer, colour_buffer);

  CLPostProcess(opencl);
  CLCopyToSDL(opencl, screen);
}

/*Place updates of parameters here*/
bool Update() {
  static int t = SDL_GetTicks();
  /* Compute frame time */
  int t2 = SDL_GetTicks();
  float dt = float(t2-t);
  float sub_fps = 1000.0 / dt;
  t = t2;
  current_tick_count++;
  current_millis += dt;

  if(current_millis > 1000.0) {
    printf("FPS: %f\n", current_tick_count);
    current_tick_count = 0;
    current_millis = 0;
  }

  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if( e.type == SDL_QUIT )
    {
      return false;
    }
  }

  float dampening = (sub_fps);
  float translationSpeed = TRANSLATION_SPEED / dampening;
  float rotationSpeed = ROTATION_SPEED / dampening;

  const Uint8* keystate = SDL_GetKeyboardState(NULL);

  if (keystate[SDL_SCANCODE_ESCAPE]){
    return false;
  }

  mat4 pitch_matrix = glm::rotate(cameraRot.x, vec3(1.f, 0.f, 0.f));
  mat4 yaw_matrix = glm::rotate(cameraRot.y, vec3(0.f, 1.f, 0.f));
  mat4 move_matrix = yaw_matrix * pitch_matrix;

  vec4 forward = move_matrix * vec4(0, 0, translationSpeed, 0);
  vec4 back = move_matrix * vec4(0, 0, -translationSpeed, 0);
  vec4 left = move_matrix * vec4(-translationSpeed, 0, 0, 0);
  vec4 right = move_matrix * vec4(translationSpeed, 0, 0, 0);
  vec4 top = move_matrix * vec4(0, -translationSpeed, 0, 0);
  vec4 bottom = move_matrix * vec4(0, translationSpeed, 0, 0);


  // vec4 rot_left =
  // vec4 rot_right = vec4(cos(cameraRot.y), 0, sin(cameraRot.y)) * rotation_speed;
  vec4 rot_up = vec4(cos(cameraRot.y), 0, sin(cameraRot.y), 0) * rotationSpeed;
  // vec4 rot_down = vec4(cos(cameraRot.y), 0, sin(cameraRot.y), 0) * -rotationSpeed;

  if (keystate[SDL_SCANCODE_W]) cameraPos += forward;
  if (keystate[SDL_SCANCODE_S]) cameraPos += back;
  if (keystate[SDL_SCANCODE_D]) cameraPos += right;
  if (keystate[SDL_SCANCODE_A]) cameraPos += left;
  if (keystate[SDL_SCANCODE_SPACE]) cameraPos += top;
  if (keystate[SDL_SCANCODE_LSHIFT]) cameraPos += bottom;

  if (keystate[SDL_SCANCODE_LEFT]) cameraRot.y -= rotationSpeed;
  if (keystate[SDL_SCANCODE_RIGHT]) cameraRot.y += rotationSpeed;
  if (keystate[SDL_SCANCODE_UP]) cameraRot.x += rotationSpeed;
  if (keystate[SDL_SCANCODE_DOWN]) cameraRot.x -= rotationSpeed;
  if (keystate[SDL_SCANCODE_O]) cameraRot.z -= rotationSpeed;
  if (keystate[SDL_SCANCODE_P]) cameraRot.z += rotationSpeed;

  if (keystate[SDL_SCANCODE_I]) scene.lights[0].position.z += translationSpeed;
  if (keystate[SDL_SCANCODE_K]) scene.lights[0].position.z -= translationSpeed;
  if (keystate[SDL_SCANCODE_L]) scene.lights[0].position.x += translationSpeed;
  if (keystate[SDL_SCANCODE_J]) scene.lights[0].position.x -= translationSpeed;
  if (keystate[SDL_SCANCODE_M]) scene.lights[0].position.y += translationSpeed;
  if (keystate[SDL_SCANCODE_U]) scene.lights[0].position.y -= translationSpeed;

  return true;

}

/*
• Shadows
• Loading of general models • Textures
• Normal mapping
• Parallax mapping
• optimizations
• level-of-detail
• hierarchical models
• efficient data-structures
• more advanced clipping
• etc.
*/
