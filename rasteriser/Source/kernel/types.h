

#define FOG 1
#define FOG_START 0.0
#define FOG_END 4.0
#define FOG_COLOR (float3)(0.529f, 0.808f, 0.922f)
// Also background colour

#define DOF 1
#define DOF_SAMPLES 5
#define DOF_KERNEL_SIZE (DOF_SAMPLES * 2) + 1
#define DOF_OFFSET 1
#define DOF_RANGE 2.0f


#ifdef IN_KERNEL
#define INT int
#define UINT unsigned int
#define FLOAT3 float3
#define FLOAT2 float2
#define FLOAT float
#define FLOAT4 float4
#else
#define INT cl_int
#define UINT cl_uint
#define FLOAT3 cl_float3
#define FLOAT2 cl_float2
#define FLOAT cl_float
#define FLOAT4 cl_float4
#endif


typedef struct __attribute__ ((packed))
{
    INT a;
    FLOAT3 b;
}st_foo;


typedef struct __attribute__ ((packed))
{
  INT dead;
  INT x;
  INT y;
  FLOAT zinv;
  FLOAT4 pos;
  FLOAT4 normal;
  FLOAT2 uv;
  FLOAT3 color;
}cl_pixel;


typedef struct __attribute__ ((packed))
{
  FLOAT4 pos;
  FLOAT3 power;
}cl_light;
