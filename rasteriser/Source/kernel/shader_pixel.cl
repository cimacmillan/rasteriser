#define IN_KERNEL
#include "./Source/kernel/types.h"
#include "./Source/kernel/shader_post.cl"

void InterpolateBarycentricPixel(cl_pixel a, cl_pixel b, cl_pixel c, float a1, float a2, float a3, cl_pixel *result) {

  result->zinv = (a.zinv * a1) + (b.zinv * a2) + (c.zinv * a3);
  result->pos = (a.pos * a1) + (b.pos * a2) + (c.pos * a3);
  result->normal = (a.normal * a1) + (b.normal * a2) + (c.normal * a3);
  result->color = (a.color * a1) + (b.color * a2) + (c.color * a3);
  result->uv = (a.uv * a1) + (b.uv * a2) + (c.uv * a3);

}

float3 PixelShader(__global cl_light * lights, int light_num, cl_pixel toDraw, float3 indirectLight, int tex_id, __global float4 * texture, int texture_width, int texture_height){

  float3 pos = (toDraw.pos / toDraw.zinv).xyz;
  float3 normal = (toDraw.normal / toDraw.zinv).xyz;

  float4 color;

  if(tex_id == 0){
    color = (float4)((toDraw.color / toDraw.zinv).xyz, 1.0f);
  } else {
    float2 uv = (toDraw.uv / toDraw.zinv);
    int xPos = (int)(uv.x * ((float)texture_width));
    int yPos = (int)(uv.y * ((float)texture_height));
    int index = xPos + (yPos * texture_width);
    color = texture[index];
    if(color.w == 0.0f){
      return (float3)(-1, -1, -1);
    }
  }

  float3 lighting_sum = (float3)(0, 0, 0);
  for(int i = 0; i < light_num; i++) {
    cl_light light = lights[i];
    float3 light_position = (light.pos).xyz;
    float3 difference = (light_position - pos);
    float distance_squared = (difference.x * difference.x) + (difference.y * difference.y) + (difference.z * difference.z);
    float dotProduct = max((float)dot(normal, normalize(difference)), (float)0);
    lighting_sum += (light.power * dotProduct) / (4 * 3.14159f * distance_squared);
  }

  return (lighting_sum + indirectLight) * color.xyz;
}

__kernel void DrawPolygon(__global float3 * screen, __global float * depth_buffer,
                          cl_pixel a, cl_pixel b, cl_pixel c,
                          int screen_width, int offset_x, int offset_y,
                           __global cl_light * lights, int light_num, float3 indirectLight,
                           int tex_id, __global float4 * texture, int texture_width, int texture_height){

  // if(tex_id != 0) {
  //   printf("%f %f %f %f %f %f\n", a.uv.x, a.uv.y, b.uv.x, b.uv.y , c.uv.x, c.uv.y);
  // }

  int x_pos = get_global_id(0) + offset_x;
  int y_pos = get_global_id(1) + offset_y;
  int index = x_pos + (screen_width * y_pos);

  float edge_first = ((x_pos - a.x) * (b.y - a.y)) - ((y_pos - a.y) * (b.x - a.x));
  float edge_second = ((x_pos - b.x) * (c.y - b.y)) - ((y_pos - b.y) * (c.x - b.x));
  float edge_third= ((x_pos - c.x) * (a.y - c.y)) - ((y_pos - c.y) * (a.x - c.x));

  const bool within_triangle = (edge_first <= 0 && edge_second <= 0 && edge_third <= 0) || (edge_first >= 0 && edge_second >= 0 && edge_third >= 0);

  if(!within_triangle) return;

  const float overall_area = (edge_first + edge_second + edge_third);
  const float area_first = edge_first / overall_area;
  const float area_second = edge_second / overall_area;
  const float area_third = edge_third / overall_area;


  cl_pixel toDraw;
  toDraw.x = x_pos;
  toDraw.y = y_pos;
  InterpolateBarycentricPixel(a, b, c, area_second, area_third, area_first, &toDraw);

  if (toDraw.zinv > depth_buffer[index] ) {
      // screen[index] = toDraw.color / toDraw.zinv;;
      float3 color = PixelShader(lights, light_num, toDraw, indirectLight, tex_id, texture, texture_width, texture_height);
      if(color.x < 0) return;
      depth_buffer[index] = toDraw.zinv;
      screen[index] = color;
      // PutPixelSDL( screen, x, y, PixelShader(screen, scene, toDraw));
  }

}

// __kernel void ClearScreen(__global float3 * screen, __global float * depth_buffer) {
//   int x_pos = get_global_id(0);
//   int y_pos = get_global_id(1);
//   int x_size = get_global_size(0);
//   int y_size = get_global_size(1);
//   int index = x_pos + (x_size * y_pos);
//
//   screen[index] = float3(0.0, 0.0, 0.0);
//   depth_buffer[index] = 0.0;
//
// }
//
//
// __kernel void helloWorld(__global float3 * screen, __global float * depth_buffer, st_foo foo){
//
//   int group_x = get_group_id(0);
//   int group_y = get_group_id(1);
//   int local_x = get_local_id(0);
//   int local_y = get_local_id(1);
//   int x_pos = get_global_id(0);
//   int y_pos = get_global_id(1);
//   int x_size = get_global_size(0);
//   int y_size = get_global_size(1);
//
//   int index = x_pos + (x_size * y_pos);
//
//   screen[index] = float3(1.0, 0.0, 0.0);
//
//     // printf("Hello World from Kernel! (%f)\n", depth_buffer[index]);
// }
