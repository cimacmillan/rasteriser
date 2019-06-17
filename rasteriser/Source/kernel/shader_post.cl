


// #define DOF 1
// #define DOF_SAMPLES 3
// #define DOF_KERNEL_SIZE (DOF_SAMPLES * 2) + 1
// #define DOF_OFFSET 2
// #define DOF_RANGE 1.0f

float getFogGamma(float depth) {
  if(FOG == 0){
    return 0;
  }
  return max((float)((depth - FOG_START) / (FOG_END - FOG_START)), (float)0);;
}

void PutPixelSDL(__global uint * write_buffer, int index, float3 colour)
{
  uint r = (uint)(clamp( 255*colour.x, 0.f, 255.f ) );
  uint g = (uint)(clamp( 255*colour.y, 0.f, 255.f ) );
  uint b = (uint)(clamp( 255*colour.z, 0.f, 255.f ) );

  write_buffer[index] = (128<<24) + (r<<16) + (g<<8) + b;
}

__kernel void PostProcess(__global float3 * screen, __global float * depth_buffer, __global uint * write_buffer){
    int x_pos = get_global_id(0);
    int y_pos = get_global_id(1);
    int x_size = get_global_size(0);
    int index = x_pos + (y_pos * x_size);

    // Background, no render
    if (depth_buffer[index] == 0) {
        PutPixelSDL(write_buffer, index, FOG_COLOR);
        return;
    }

    float depth = 1.0f / depth_buffer[index];

    float fog_gamma = getFogGamma(depth);
    float3 final_color = (fog_gamma * FOG_COLOR) + ((1 - fog_gamma) * screen[index]);

    PutPixelSDL(write_buffer, index, final_color);
    // PutPixelSDL(write_buffer, index, screen[index]);
}

float3 sin_kernel(__global float3 * screen, int x, int y, int screen_width, int screen_height, float gamma, int vertical, __global float * kernel_sin, __global float * kernel_regular, __global float * depth_buffer) {

  float3 avg = (float3)(0);
  int Start = - (DOF_SAMPLES * DOF_OFFSET) + (y * vertical) + (x * !vertical);
  int End = + (DOF_SAMPLES * DOF_OFFSET) + (y * vertical) + (x * !vertical);
  int maximum = ((screen_height * vertical) + (screen_width * !vertical)) - 1;
  Start = max(Start, 0);
  End = min(End, maximum);

  for(int i = Start; i < End; i += DOF_OFFSET) {

    int coord = (x * !vertical) + (y * vertical);

    int sample_index = (i - coord + (DOF_SAMPLES * DOF_OFFSET)) / DOF_OFFSET;
    float multiplier = (kernel_sin[sample_index] * gamma) + (kernel_regular[sample_index] * (1.0f - gamma));

    int x_cord = (x * vertical) + (i * !vertical);
    int y_cord = (y * !vertical) + (i * vertical);

    int index = (x_cord + (y_cord * screen_width));

    if(depth_buffer[index] == 0){
      avg += (FOG_COLOR * multiplier);
    } else {
      avg += (screen[index] * multiplier);
    }
  }
  return avg;

}

__kernel void PrePass(__global float3 * screen, __global float3 * temp_screen, __global float * depth_buffer, __global float * kernel_sin, __global float * kernel_regular, int vertical) {

  if(DOF == 0) return;

  int x_pos = get_global_id(0);
  int y_pos = get_global_id(1);
  int x_size = get_global_size(0);
  int y_size = get_global_size(1);
  int index = x_pos + (y_pos * x_size);
  int target_index = (x_size / 2) + ((y_size / 2) * x_size);

  float target_depth;
  if (depth_buffer[target_index] == 0){
    target_depth = 100000.0f;
  }else{
    target_depth = 1.0f / depth_buffer[target_index];
  }

  float depth = 1.0f / depth_buffer[index];
  float gamma = min((float)(fabs((float)(depth - target_depth)) / DOF_RANGE), 1.0f);

  temp_screen[index] = sin_kernel(screen, x_pos, y_pos, x_size, y_size, gamma, vertical, kernel_sin, kernel_regular, depth_buffer);
}

__kernel void PostProcessInit(__global float * kernel_sin, __global float * kernel_regular){
  float sum = 0;
  for(int i = 0; i < DOF_KERNEL_SIZE; i++) {
    float grad = (((float)i) + 0.5f) / (DOF_KERNEL_SIZE);
    float rad = (grad - 0.5f) * 3.1415;
    float cosVal = cos(rad);
    kernel_sin[i] = cosVal;
    sum += cosVal;
  }
  for(int i = 0; i < DOF_KERNEL_SIZE; i++) kernel_sin[i] /= sum;
  for(int i = 0; i < DOF_KERNEL_SIZE; i++) kernel_regular[i] = 0.0f;
  kernel_regular[DOF_SAMPLES] = 1.0;
}
