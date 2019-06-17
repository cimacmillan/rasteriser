#include <glm/glm.hpp>
using glm::vec2;
using glm::mat4;

#define FULL_CLIPPING 0

class VertexShader {

  int screen_width;
  int screen_height;
  float focal_point;
  mat4 projection_matrix;

  vec4 zNearP = vec4(0, 0, 0.1f, 0);
  vec4 zNearN =  vec4(0, 0, 1.0f, 0);
  vec4 zFarP = vec4(0, 0, 5.0f, 0);
  vec4 zFarN = vec4(0, 0, -1.0f, 0);

  vec4 xLeftP;
  vec4 xLeftN;
  vec4 xRightP;
  vec4 xRightN;
  vec4 topP;
  vec4 topN;
  vec4 bottomP;
  vec4 bottomN;

public:
  VertexShader(int screen_width, int screen_height)
  : screen_width(screen_width), screen_height(screen_height)
  {
   this->focal_point = ((float)screen_height / 2);
   this->projection_matrix = mat4(0.0f);
   this->projection_matrix[0][0] = 1.0f;
   this->projection_matrix[1][1] = 1.0f;
   this->projection_matrix[2][2] = 1.0f;
   this->projection_matrix[2][3] = 1.0f / this->focal_point;

   float aspect_ratio = ((float)screen_width) / ((float)screen_height);

   this->xLeftP = vec4(-aspect_ratio, 0, 1.0f, 0);
   this->xLeftN = glm::normalize(vec4(1.0f, 0, aspect_ratio, 0));
   this->xRightP = vec4(aspect_ratio, 0, 1.0f, 0);
   this->xRightN = glm::normalize(vec4(-1.0f, 0, aspect_ratio, 0));

   this->topP = vec4(0, -1.0f, 1.0f, 0);
   this->topN = glm::normalize(vec4(0, 1.0f, 1.0f, 0));
   this->bottomP = vec4(0, 1.0f, 1.0f, 0);
   this->bottomN = glm::normalize(vec4(0.0f, -1.0f, 1.0f, 0));
 }

  vec4 Projection(vec4 vector) {
    return (focal_point / vector.z) * vector;
  }

  void compute_pixel(Vertex& v, Pixel& projPos) {
    vec4 trans = projection_matrix * v.transformed;
    vec4 proj = Projection(trans) + vec4(screen_width / 2, screen_height / 2, 1, 1);
    projPos.x = (int)proj.x;
    projPos.y = (int)proj.y;
    projPos.zinv = (1.0 / trans.z);
    projPos.pos = v.position * projPos.zinv;
    projPos.normal = v.normal * projPos.zinv;
    projPos.color = v.color * projPos.zinv;
    projPos.uv = v.uv * projPos.zinv;
  }

  //If 1, b - if 0, a
  Vertex tween(Vertex &a, Vertex &b, float t) {
    Vertex toAdd;
    float tprime = 1.0f - t;
    toAdd.position = (t * b.position) + (tprime * a.position);
    toAdd.transformed = (t * b.transformed) + (tprime * a.transformed);
    toAdd.normal = (t * b.normal) + (tprime * a.normal);
    toAdd.color = (t * b.color) + (tprime * a.color);
    toAdd.uv = (t * b.uv) + (tprime * a.uv);
    return toAdd;
  }

  //Return false if both break plane
  void clip_to_plane(Vertex &vertex_a, Vertex &vertex_b, vector<Vertex> &triangles, vec4 plane_point, vec4 plane_normal) {

    vec4 a = vertex_a.transformed;
    vec4 b = vertex_b.transformed;

    float d1 = glm::dot((a - plane_point), plane_normal);
    float d2 = glm::dot((b - plane_point), plane_normal);

    float t = d1 / (d1 - d2);

    if(d1 > 0) {
      if(d2 < 0){
        triangles.push_back(tween(vertex_a, vertex_b, t));
      }
      triangles.push_back(vertex_a);
    } else if (d2 > 0) {
      triangles.push_back(tween(vertex_a, vertex_b, t));
    }

  }

  //Sutherland–Hodgman algorithm
  void clip_triangles_to_plane(vector<Vertex> &vertices, vec4 plane_point, vec4 plane_normal) {

    vector<Vertex> resulting;
    int size = (int)vertices.size();

    for(int i = 0; i < size; i++ ) {
        Vertex current_vert = vertices[i];
        Vertex previous_vert = vertices[(i + size - 1) % size];
        clip_to_plane(current_vert, previous_vert, resulting, plane_point, plane_normal);
     }

     vertices.clear();
     vertices.insert( vertices.end(), resulting.begin(), resulting.end() );
  }

  void compute(vector<Vertex> &vertices, vector<Pixel> &pixels) {

    clip_triangles_to_plane(vertices, this->zNearP, this->zNearN);
    clip_triangles_to_plane(vertices, this->zFarP, this->zFarN);
    if(FULL_CLIPPING == 1) {
      clip_triangles_to_plane(vertices, this->xLeftP, this->xLeftN);
      clip_triangles_to_plane(vertices, this->xRightP, this->xRightN);
      clip_triangles_to_plane(vertices, this->bottomP, this->bottomN);
      clip_triangles_to_plane(vertices, this->topP, this->topN);
    }

    for( int i=0; i < (int)vertices.size(); i++ ) {
      Pixel pixel;
      Vertex vertex = vertices[i];
      compute_pixel( vertex, pixel);
      pixels.push_back(pixel);

     }

  }

};
