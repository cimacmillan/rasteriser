#include <glm/glm.hpp>
#include <vector>
#include "obj_loader.h"
#include "lodepng.h"
#include <cstdlib>


#define NOISE_SIZE 17
#define NOISE_MAG 0.1f
#define TERRAIN_POS vec4(-5, 0.5f, -5, 0)
#define TERRAIN_SCALE vec4(10, 4, 10, 1)

using glm::vec4;
using glm::vec3;
using std::vector;

struct Pixel {
  int dead;
  int x;
  int y;
  float zinv;
  vec4 pos;
  vec4 normal;
  vec3 color;
  vec2 uv;
};

struct Vertex {
  vec4 position;
  vec4 normal;
  vec3 color;
  vec2 uv;
  vec4 transformed;
};

struct Light {
  vec4 position;
  vec3 power;

  Light(vec4 position, vec3 power)
  : position(position),  power(power)
  {

  }

};

struct Texture {
  unsigned int width;
  unsigned int height;
  vector<unsigned char> image;
  int id;
};

struct Scene {
  vector<Triangle> triangles;
  vector<Light> lights;
  vec3 indirectLight;
  vector<Texture> textures;
};

int LoadTexture(vector <Texture> &inject_into, const char * filename) {
  Texture toAdd;
  lodepng::decode(toAdd.image, toAdd.width, toAdd.height, filename);
  toAdd.id = inject_into.size();
  inject_into.push_back(toAdd);
  return inject_into.size();
}

float random_float(){
  float r = static_cast <float> (std::rand()) / static_cast <float> (RAND_MAX);
  return r;
}

void DiamondSquare(float noise[NOISE_SIZE][NOISE_SIZE], int x1, int x2, int y1, int y2) {

  if(abs(x2 - x1 ) <= 1 || abs(y2 - y1) <= 1) return;

  int xmid = ((x1 + x2) / 2);
  int ymid = ((y1 + y2) / 2);

  float h1 = noise[x1][y1];
  float h2 = noise[x2][y1];
  float h3 = noise[x2][y2];
  float h4 = noise[x1][y2];

  float t = noise[xmid][y1];
  float b = noise[xmid][y2];
  float l = noise[x1][ymid];
  float r = noise[x2][ymid];
  float m = noise[xmid][ymid];

  noise[xmid][y1] = (h1 + h2 + t) / 3.0f;
  noise[xmid][y2] = (h3 + h4 + b) / 3.0f;
  noise[x1][ymid] = (h1 + h4 + l) / 3.0f;
  noise[x2][ymid] = (h2 + h3 + r) / 3.0f;
  noise[xmid][ymid] = (h1 + h2 + h3 + h4 + m) / 5.0f;

  DiamondSquare(noise, x1, xmid, y1, ymid);
  DiamondSquare(noise, xmid, x2, y1, ymid);
  DiamondSquare(noise, x1, xmid, ymid, y2);
  DiamondSquare(noise, xmid, x2, ymid, y2);

}

void GenerateTerrainNoise(float noise[NOISE_SIZE][NOISE_SIZE]) {
  for(int x = 0; x < NOISE_SIZE; x++) {
    for(int y = 0; y < NOISE_SIZE; y++) {
      noise[x][y] = random_float();
    }
  }
  DiamondSquare(noise, 0, NOISE_SIZE - 1, 0, NOISE_SIZE - 1);
  DiamondSquare(noise, 0, NOISE_SIZE - 1, 0, NOISE_SIZE - 1);
  DiamondSquare(noise, 0, NOISE_SIZE - 1, 0, NOISE_SIZE - 1);
}

void ConstructScene(Scene &scene){

  int tex_rock = LoadTexture(scene.textures, "./tex_a.png");
  int tex_blue = LoadTexture(scene.textures, "./tex_b.png");

  cout << tex_rock << " " << tex_blue << endl;

  LoadTestModel(scene.triangles);
  load_obj(scene.triangles, "./sphere.obj", vec4(-0.4f, 0.f, -2.0f, 0.f), vec4(0.4f, -0.4f, -0.4f, 1.0f), tex_rock);
  load_obj(scene.triangles, "./fox.obj", vec4(-0.4f, 1.0f, -2.0f, 0.f), vec4(0.01f, -0.01f, -0.01f, 1.0f), tex_blue);
  // load_obj(scene.triangles, "./sphere.obj", vec4(0.4f, 0.f, -2.0f, 0.f), vec4(0.3f, 0.3f, 0.3f, 1.0f), tex_blue);
  // load_obj(scene.triangles, "./test.obj");

  scene.indirectLight = 0.5f*vec3( 1, 1, 1 );
  scene.lights.push_back(Light(
      vec4(0,-0.5,-0.7, 1),
      14.f*vec3( 1, 1, 1 )
  ));


  // Triangle test(vec4(0, 0, -2, 1), vec4(1, 0, -2, 1), vec4(0, 1, -2, 1),
  //               vec4(0, 0, -1, 1), vec4(0, 0, -1, 1), vec4(0, 0, -1, 1),
  //               vec2(0, 0), vec2(1, 0), vec2(0, 1),
  //               tex_rock);
  // scene.triangles.push_back(test);
  scene.lights.push_back(Light(
      vec4(-0.9,0.5,-0.7, 1),
      8.f*vec3( 1, 0.1, 0.1 )
  ));

  scene.lights.push_back(Light(
      vec4(5,3, 5, 1),
      20.f*vec3( 1.0, 1.0, 1.0 )
  ));

  scene.lights.push_back(Light(
      vec4(0.8,0.5,-1, 1),
      8.f*vec3( 0.1, 0.0, 1.0 )
  ));

  float noise[NOISE_SIZE][NOISE_SIZE];
  GenerateTerrainNoise(noise);

  for(int x = 0; x < NOISE_SIZE - 1; x++) {
    for(int y = 0; y < NOISE_SIZE - 1; y++) {

      float x_grad = ((float)x) / ((float)NOISE_SIZE);
      float y_grad = ((float)y) / ((float)NOISE_SIZE);
      float x_grad_next = ((float)x + 1) / ((float)NOISE_SIZE);
      float y_grad_next = ((float)y + 1) / ((float)NOISE_SIZE);

      float h1 = noise[x][y];
      float h2 = noise[x + 1][y];
      float h3 = noise[x + 1][y + 1];
      float h4 = noise[x][y + 1];

      vec4 a = (vec4(x_grad, h1, y_grad, 1) * TERRAIN_SCALE) + TERRAIN_POS;
      vec4 b = (vec4(x_grad_next, h2, y_grad, 1) * TERRAIN_SCALE) + TERRAIN_POS;
      vec4 c = (vec4(x_grad_next, h3, y_grad_next, 1) * TERRAIN_SCALE) + TERRAIN_POS;
      vec4 d = (vec4(x_grad, h4, y_grad_next, 1) * TERRAIN_SCALE) + TERRAIN_POS;

      vec3 color = vec3(0, 1, 0);

      Triangle abc = Triangle( a, c, b, color );
      Triangle acd = Triangle( a, d, c, color );
      abc.culled = false;
      acd.culled = false;
      scene.triangles.push_back( abc );
      scene.triangles.push_back( acd );

    }
  }

}
