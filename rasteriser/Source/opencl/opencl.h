#define __CL_ENABLE_EXCEPTIONS

#include "./lib/cl.hpp"
#include "./lib/util.hpp" // OpenCL utility library
#include "./lib/err_code.h"
#include "../kernel/types.h"

#define SOURCE_RENDERER "./Source/kernel/shader_pixel.cl"

#ifndef DEVICE
#define DEVICE CL_DEVICE_TYPE_DEFAULT
#endif

cl_float3 pattern = (cl_float3){0.529f, 0.808f, 0.922f};
cl_uint blank_write_buffer[SCREEN_WIDTH * SCREEN_HEIGHT];
cl_float3 blank_screen_buffer[SCREEN_WIDTH * SCREEN_HEIGHT];
cl_float blank_depth_buffer[SCREEN_WIDTH * SCREEN_HEIGHT];

// std::vector<cl_float3> buffer_read(SCREEN_WIDTH * SCREEN_HEIGHT);

struct ocl {

    cl::Device device;
    cl::Context context;
    cl::CommandQueue queue;

    cl::Program renderer;

    cl::Kernel helloWorld;
    cl::Kernel DrawPolygon;
    cl::Kernel ClearScreen;
    cl::Kernel PostProcess;
    cl::Kernel PostProcessInit;
    cl::Kernel PrePass;

    cl::Buffer screen_write;
    cl::Buffer depth_buffer;
    cl::Buffer light_buffer;
    cl::Buffer write_buffer;
    cl::Buffer temp_screen;

    cl::Buffer kernel_sin;
    cl::Buffer kernel_regular;

    vector<cl::Buffer> texture_buffers;

};



void initBlankBuffers() {
  for(int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
    blank_screen_buffer[i] = pattern;
    blank_write_buffer[i] = 0;
    blank_depth_buffer[i] = 0;
  }
}

void CLRegisterTextures(ocl &opencl, vector<Texture> textures) {
  opencl.texture_buffers.push_back(cl::Buffer(opencl.context, CL_MEM_READ_ONLY, sizeof(cl_float4) * 8 * 8)); //Empty texture
  for (int i = 0; i < textures.size(); i++) {
    Texture texture = textures[i];
    cl_float4* data = (cl_float4*)malloc(sizeof(cl_float4) * texture.width * texture.height);
    for(int read = 0; read < texture.width * texture.height * 4; read += 4) {
      unsigned char r = texture.image[read];
      unsigned char g = texture.image[read + 1];
      unsigned char b = texture.image[read + 2];
      unsigned char a = texture.image[read + 3];

      float red_f = (((float)r) / 255.0f);
      float green_f = (((float)g) / 255.0f);
      float blue_f = (((float)b) / 255.0f);
      float a_f = (((float)a) / 255.0f);

      int index = read / 4;

      data[index] = (cl_float4){red_f, green_f, blue_f, a_f};
    }

    cout << "writing textures" << endl;

    cl::Buffer texture_buffer = cl::Buffer(opencl.context, CL_MEM_READ_ONLY, sizeof(cl_float4) * texture.width * texture.height);
    opencl.queue.enqueueWriteBuffer(texture_buffer, true, 0, sizeof(cl_float4) * texture.width * texture.height, data);
    opencl.texture_buffers.push_back(texture_buffer);
    free(data);
  }
}

cl_pixel toClPixel(Pixel pixel) {
  cl_pixel pix = {
    pixel.dead,
    pixel.x,
    pixel.y,
    pixel.zinv,
    (cl_float4){pixel.pos.x, pixel.pos.y, pixel.pos.z, pixel.pos.w},
    (cl_float4){pixel.normal.x, pixel.normal.y, pixel.normal.z, pixel.normal.w},
    (cl_float2){pixel.uv.x, pixel.uv.y},
    (cl_float3){pixel.color.x, pixel.color.y, pixel.color.z}
  };
  return pix;
}

cl_light toClLight(Light light) {
  cl_light cllight = {
    (cl_float4){light.position.x, light.position.y, light.position.z, light.position.w},
    (cl_float3){light.power.x, light.power.y, light.power.z},
  };
  return cllight;
}

void CLClearScreen(ocl &opencl) {
  opencl.queue.enqueueWriteBuffer(opencl.screen_write, false, 0, (SCREEN_WIDTH * SCREEN_HEIGHT) * sizeof(cl_float3), &blank_screen_buffer);
  opencl.queue.enqueueWriteBuffer(opencl.depth_buffer, false, 0, (SCREEN_WIDTH * SCREEN_HEIGHT) * sizeof(cl_float), &blank_depth_buffer);
  opencl.queue.enqueueWriteBuffer(opencl.write_buffer, false, 0, (SCREEN_WIDTH * SCREEN_HEIGHT) * sizeof(cl_uint), &blank_write_buffer);
}

void CLCopyToSDL(ocl &opencl, screen* screen) {
  opencl.queue.enqueueReadBuffer(
    opencl.write_buffer, true, 0, sizeof(cl_uint) * SCREEN_WIDTH * SCREEN_HEIGHT, screen->buffer
  );
}

void CLRegisterLights(ocl &opencl, vector<Light> lights){
  vector<cl_light> cl_lights(lights.size());
  for(int i = 0; i < lights.size(); i++){
    cl_lights[i] = toClLight(lights[i]);
  }

  opencl.queue.enqueueWriteBuffer(opencl.light_buffer, false, 0, lights.size() * sizeof(cl_light), cl_lights.data());
}

void CLDrawPolygon(ocl &opencl, Pixel vertexPixels[3], Scene& scene, int tex_id){
  // cl::Buffer light_buffer(opencl.context, begin(cl_lights), end(cl_lights), true);

  int minX = min(vertexPixels[0].x, min(vertexPixels[1].x, vertexPixels[2].x));
  int maxX = max(vertexPixels[0].x, max(vertexPixels[1].x, vertexPixels[2].x));
  int minY = min(vertexPixels[0].y, min(vertexPixels[1].y, vertexPixels[2].y));
  int maxY = max(vertexPixels[0].y, max(vertexPixels[1].y, vertexPixels[2].y));
  if(minX < 0) minX = 0;
  if(minY < 0) minY = 0;
  if(maxX >= SCREEN_WIDTH) maxX = SCREEN_WIDTH - 1;
  if(maxY >= SCREEN_HEIGHT) maxY = SCREEN_HEIGHT - 1;
  int width = maxX - minX;
  int height = maxY - minY;
  if(width == 0 || height == 0) return;

  cl_pixel a = toClPixel(vertexPixels[0]);
  cl_pixel b = toClPixel(vertexPixels[1]);
  cl_pixel c = toClPixel(vertexPixels[2]);

  opencl.DrawPolygon.setArg(0,opencl.screen_write);
  opencl.DrawPolygon.setArg(1,opencl.depth_buffer);
  opencl.DrawPolygon.setArg(2,a);
  opencl.DrawPolygon.setArg(3,b);
  opencl.DrawPolygon.setArg(4,c);
  opencl.DrawPolygon.setArg(5,SCREEN_WIDTH);

  opencl.DrawPolygon.setArg(6,minX);
  opencl.DrawPolygon.setArg(7,minY);

  opencl.DrawPolygon.setArg(8,opencl.light_buffer);
  opencl.DrawPolygon.setArg(9,(int)scene.lights.size());

  cl_float3 indirectLight = (cl_float3){scene.indirectLight.x, scene.indirectLight.y, scene.indirectLight.z};
  opencl.DrawPolygon.setArg(10,indirectLight);

  opencl.DrawPolygon.setArg(11, tex_id);
  opencl.DrawPolygon.setArg(12, opencl.texture_buffers[tex_id]);
  opencl.DrawPolygon.setArg(13, scene.textures[tex_id - 1].width);
  opencl.DrawPolygon.setArg(14, scene.textures[tex_id - 1].height);

  opencl.queue.enqueueNDRangeKernel(opencl.DrawPolygon,cl::NullRange,cl::NDRange(width, height),cl::NullRange);

}

void CLPostProcess(ocl &opencl) {

  opencl.PrePass.setArg(0,opencl.screen_write);
  opencl.PrePass.setArg(1,opencl.temp_screen);
  opencl.PrePass.setArg(2,opencl.depth_buffer);
  opencl.PrePass.setArg(3,opencl.kernel_sin);
  opencl.PrePass.setArg(4,opencl.kernel_regular);
  opencl.PrePass.setArg(5, 1);
  opencl.queue.enqueueNDRangeKernel(opencl.PrePass,cl::NullRange,cl::NDRange(SCREEN_WIDTH, SCREEN_HEIGHT),cl::NullRange);
  opencl.PrePass.setArg(0,opencl.temp_screen);
  opencl.PrePass.setArg(1,opencl.screen_write);
  opencl.PrePass.setArg(5, 0);
  opencl.queue.enqueueNDRangeKernel(opencl.PrePass,cl::NullRange,cl::NDRange(SCREEN_WIDTH, SCREEN_HEIGHT),cl::NullRange);

  opencl.PostProcess.setArg(0,opencl.screen_write);
  opencl.PostProcess.setArg(1,opencl.depth_buffer);
  opencl.PostProcess.setArg(2,opencl.write_buffer);
  opencl.queue.enqueueNDRangeKernel(opencl.PostProcess,cl::NullRange,cl::NDRange(SCREEN_WIDTH, SCREEN_HEIGHT),cl::NullRange);

}

void CLPostProcessInit(ocl &opencl) {

    opencl.PostProcessInit.setArg(0,opencl.kernel_sin);
    opencl.PostProcessInit.setArg(1,opencl.kernel_regular);
    opencl.queue.enqueueNDRangeKernel(opencl.PostProcessInit,cl::NullRange,cl::NDRange(1),cl::NullRange);
}

void MakeKernels(ocl &opencl, Scene &scene) {
  opencl.renderer = cl::Program(opencl.context, util::loadProgram(SOURCE_RENDERER));
  opencl.renderer.build();

  opencl.DrawPolygon = cl::Kernel(opencl.renderer,"DrawPolygon");
  opencl.PostProcess = cl::Kernel(opencl.renderer,"PostProcess");
  opencl.PostProcessInit = cl::Kernel(opencl.renderer,"PostProcessInit");
  opencl.PrePass = cl::Kernel(opencl.renderer,"PrePass");

  opencl.screen_write = cl::Buffer(opencl.context, CL_MEM_READ_WRITE, sizeof(cl_float3) * SCREEN_WIDTH * SCREEN_HEIGHT);
  opencl.temp_screen = cl::Buffer(opencl.context, CL_MEM_READ_WRITE, sizeof(cl_float3) * SCREEN_WIDTH * SCREEN_HEIGHT);
  opencl.depth_buffer = cl::Buffer(opencl.context, CL_MEM_WRITE_ONLY, sizeof(cl_float) * SCREEN_WIDTH * SCREEN_HEIGHT);
  opencl.light_buffer = cl::Buffer(opencl.context, CL_MEM_WRITE_ONLY, sizeof(cl_light) * scene.lights.size());
  opencl.write_buffer = cl::Buffer(opencl.context, CL_MEM_WRITE_ONLY, sizeof(cl_uint) * SCREEN_WIDTH * SCREEN_HEIGHT);

  opencl.kernel_sin = cl::Buffer(opencl.context, CL_MEM_READ_WRITE, sizeof(cl_float) * DOF_KERNEL_SIZE);
  opencl.kernel_regular = cl::Buffer(opencl.context, CL_MEM_READ_WRITE, sizeof(cl_float) * DOF_KERNEL_SIZE);

}

void InitOpenCL(ocl &opencl, Scene &scene){
    try
    {
    	// Create a context

        opencl.device = cl::Device::getDefault();
        opencl.context = cl::Context(opencl.device);
        opencl.queue = cl::CommandQueue(opencl.context);

        string device_name;
        opencl.device.getInfo(CL_DEVICE_NAME, &device_name);
        cout << "Device Chosen: " << device_name << endl;

        MakeKernels(opencl, scene);
        initBlankBuffers();

    }
    catch (cl::Error err) {
      if (err.err() == CL_BUILD_PROGRAM_FAILURE)
      {
        // Get the build log for the first device
        std::string log = opencl.renderer.getBuildInfo<CL_PROGRAM_BUILD_LOG>(opencl.device);
        std::cerr << log << std::endl;
      }

        std::cout << "Exception\n";
        std::cerr
            << "ERROR: "
            << err.what()
            << "("
            << err_code(err.err())
           << ")"
           << std::endl;
    }
}
