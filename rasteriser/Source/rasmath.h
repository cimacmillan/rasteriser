#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <stdint.h>
#include <vector>

using namespace std;
using glm::vec2;
using glm::vec3;
using glm::mat3;
using glm::vec4;
using glm::mat4;
using glm::ivec2;

void InterpolateBarycentricPixel(Pixel a, Pixel b, Pixel c, float a1, float a2, float a3, Pixel& result) {

  result.zinv = (a.zinv * a1) + (b.zinv * a2) + (c.zinv * a3);
  result.pos = (a.pos * a1) + (b.pos * a2) + (c.pos * a3);
  result.normal = (a.normal * a1) + (b.normal * a2) + (c.normal * a3);
  result.color = (a.color * a1) + (b.color * a2) + (c.color * a3);

}

void InterpolatePixel ( Pixel a, Pixel b, vector<Pixel>& result ) {
  int N = result.size();

  float xStep = ((float)(b.x-a.x)) / float(max(N-1,1));
  float xCurrent = (float)a.x;

  float yStep = ((float)(b.y-a.y)) / float(max(N-1,1));
  float yCurrent = (float)a.y;

  float zStep = ((float)(b.zinv-a.zinv)) / float(max(N-1,1));
  float zCurrent = (float)a.zinv;

  vec4 posStep = (b.pos - a.pos) / float(max(N-1,1));
  vec4 posCurrent = a.pos;

  for( int i=0; i<N; ++i )
  {
     result[i].x = round(xCurrent);
     result[i].y = round(yCurrent);
     result[i].zinv = zCurrent;
     result[i].pos = posCurrent;

     xCurrent += xStep;
     yCurrent += yStep;
     zCurrent += zStep;
     posCurrent += posStep;
  }
}

void ComputePolygonRows (const vector<Pixel>& vertexPixels, vector<Pixel>& leftPixels, vector<Pixel>& rightPixels ) {

  //TODO make this use arbitrary vertex amount
  int minY = min(vertexPixels[0].y, min(vertexPixels[1].y, vertexPixels[2].y));
  int maxY = max(vertexPixels[0].y, max(vertexPixels[1].y, vertexPixels[2].y));
  int rows = (maxY - minY) + 1;

  leftPixels.resize(rows);
  rightPixels.resize(rows);

  for( int i=0; i<rows; ++i ) {
     leftPixels[i].x  = +numeric_limits<int>::max();
     rightPixels[i].x = -numeric_limits<int>::max();
     leftPixels[i].y = i + minY;
     rightPixels[i].y = i + minY;
  }


  for( int i=0; i< (int)vertexPixels.size(); ++i ) {

    int j = (i+1)%vertexPixels.size(); // The next vertex
    Pixel a = vertexPixels[i];
    Pixel b = vertexPixels[j];
    ivec2 delta = glm::abs(ivec2(a.x - b.x, a.y - b.y));
    int pixels = glm::max( delta.x, delta.y ) + 1;
    vector<Pixel> line( pixels );
    InterpolatePixel( a, b, line );

    for (int pix = 0; pix < pixels; pix++){
      Pixel interp = line[pix];
      int rowPos = interp.y - minY;

      if (interp.x < leftPixels[rowPos].x) {
        leftPixels[rowPos] = interp;
      }
      if (interp.x > rightPixels[rowPos].x) {
        rightPixels[rowPos] = interp;
      }

    }

  }

}

mat4 RotationMatrix(vec4 R) {
  mat4 pitch_matrix = glm::rotate(R.x, vec3(-1.f, 0.f, 0.f));
  mat4 yaw_matrix = glm::rotate(R.y, vec3(0.f, -1.f, 0.f));
  // mat4 roll_matrix = glm::rotate(R.z, vec3(0.f, 0.f, 1.f));
  return pitch_matrix * yaw_matrix;
}


mat4 TransformationMatrix(vec4 T, vec4 R) {
  mat4 translation = glm::translate(vec3(T.x, T.y, T.z));
  return (RotationMatrix(R) * translation);

}


float max(float a, float b) {
  if (a > b) {
    return a;
  }
  return b;
}
