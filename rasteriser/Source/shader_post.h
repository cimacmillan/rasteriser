#include <stdlib.h>

#define FOG 0
#define FOG_START 0.0
#define FOG_END 5.0

#define DOF 1
#define DOF_SAMPLES 3
#define DOF_KERNEL_SIZE (DOF_SAMPLES * 2) + 1
#define DOF_OFFSET 2
#define DOF_RANGE 1.0f
float *SIN_MULTIPLIER;
float *REGULAR_MULTIPLIER;
vec3 temp_colour[SCREEN_WIDTH][SCREEN_HEIGHT];

const vec3 FOG_COLOR(1.0f, 1.0f, 1.0f);

void PostShaderInit(){

  SIN_MULTIPLIER = (float*)malloc(sizeof(float) * DOF_KERNEL_SIZE);
  REGULAR_MULTIPLIER = (float*)malloc(sizeof(float) * DOF_KERNEL_SIZE);

  float sum = 0;
  for(int i = 0; i < ((DOF_SAMPLES * 2) + 1); i++) {
    float grad = (((float)i) + 0.5f) / (DOF_KERNEL_SIZE);
    float rad = (grad - 0.5f) * 3.1415;
    float cosVal = cosf(rad);
    SIN_MULTIPLIER[i] = cosVal;
    sum += cosVal;
  }
  for(int i = 0; i < DOF_KERNEL_SIZE; i++) SIN_MULTIPLIER[i] /= sum;
  for(int i = 0; i < DOF_KERNEL_SIZE; i++) REGULAR_MULTIPLIER[i] = 0.0f;

  REGULAR_MULTIPLIER[DOF_SAMPLES] = 1.0;


}

void PostShaderPost() {
  free(SIN_MULTIPLIER);
  free(REGULAR_MULTIPLIER);
}

vec3 getBackgroundColor() {
  if(FOG) {
    return FOG_COLOR;
  }
  return vec3(0, 0, 0);
}

float getFogGamma(float depth) {
  if(FOG == 0){
    return 0;
  }
  return max((depth - FOG_START) / (FOG_END - FOG_START), 0);;
}

vec3 sin_kernel(vec3 (&colour_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], int x, int y, float gamma, bool vertical) {

  vec3 avg(0, 0, 0);
  int Start = - (DOF_SAMPLES * DOF_OFFSET) + (y * vertical) + (x * !vertical);
  int End = + (DOF_SAMPLES * DOF_OFFSET) + (y * vertical) + (x * !vertical);
  int maximum = ((SCREEN_HEIGHT * vertical) + (SCREEN_WIDTH * !vertical)) - 1;
  Start = max(Start, 0);
  End = min(End, maximum);

  for(int i = Start; i < End; i += DOF_OFFSET) {

    int coord = (x * !vertical) + (y * vertical);

    int sample_index = (i - coord + (DOF_SAMPLES * DOF_OFFSET)) / DOF_OFFSET;
    float multiplier = (SIN_MULTIPLIER[sample_index] * gamma) + (REGULAR_MULTIPLIER[sample_index] * (1.0f - gamma));

    int x_cord = (x * vertical) + (i * !vertical);
    int y_cord = (y * !vertical) + (i * vertical);

    avg += (colour_buffer[x_cord][y_cord] * multiplier);
  }
  return avg;

}

vec3 PostShaderPixel(screen* screen, Scene &scene, const Pixel& p, float (&depth_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], Pixel (&pixel_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], vec3 (&colour_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], int xPixel, int yPixel) {

  //BACKGROUND
  if (p.dead == 1) return getBackgroundColor();

  float depth = 1.0 / depth_buffer[xPixel][yPixel];
  float fog_gamma = getFogGamma(depth);

  vec3 final_color = (fog_gamma * FOG_COLOR) + ((1 - fog_gamma) * colour_buffer[xPixel][yPixel]);

  return final_color;

}

void PostShader(screen* screen, Scene &scene, float (&depth_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], Pixel (&pixel_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], vec3 (&colour_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT]) {


  float target_depth = 1.0f / depth_buffer[SCREEN_WIDTH/2][SCREEN_HEIGHT/2];

  if(DOF == 1) {

    for (int x = 0; x < SCREEN_WIDTH; x++) {
      for (int y = 0; y < SCREEN_HEIGHT; y++) {

        float depth = 1.0f / depth_buffer[x][y];
        float gamma = min(abs(depth - target_depth) / DOF_RANGE, 1.0f);

        temp_colour[x][y] = sin_kernel(colour_buffer, x, y, gamma, false);
        // PutPixelSDL( screen, x, y, PostShaderPixel(screen, scene, pixel_buffer[x][y], depth_buffer, pixel_buffer, colour_buffer, x, y));
      }
    }

  }

  for (int x = 0; x < SCREEN_WIDTH; x++) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      // PutPixelSDK
      if(DOF == 1) {
        float depth = 1.0f / depth_buffer[x][y];
        float gamma = min(abs(depth - target_depth) / DOF_RANGE, 1.0f);

        colour_buffer[x][y] = sin_kernel(temp_colour, x, y, gamma, true);
      }

      PutPixelSDL( screen, x, y, PostShaderPixel(screen, scene, pixel_buffer[x][y], depth_buffer, pixel_buffer, colour_buffer, x, y));
    }
  }
}
