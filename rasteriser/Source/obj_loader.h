#include <glm/glm.hpp>
#include <vector>
#include "TestModelH.h"

using namespace std;
using glm::vec4;
using glm::vec3;
using glm::vec2;


//http://www.opengl-tutorial.org/beginners-tutorials/tutorial-7-model-loading/
void load_obj(vector<Triangle> &inject_into, const char * path, vec4 position, vec4 scale, int tex_id) {

  cout << path << endl;

  vector<vec3> vertices;
  vector<vec2> uvs;
  vector<vec3> normals;

  FILE * file = fopen(path, "r");
  if( file == NULL ){
      printf("Impossible to open the file !\n");
  }


  while( 1 ){

      char lineHeader[128];
      // read the first word of the line
      int res = fscanf(file, "%s", lineHeader);
      if (res == EOF)
          break; // EOF = End Of File. Quit the loop.

      if ( strcmp( lineHeader, "v" ) == 0 ){

        glm::vec3 vertex;
        fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
        vertices.push_back(vertex);

      } else if ( strcmp( lineHeader, "vt" ) == 0 ){

        glm::vec2 uv;
        fscanf(file, "%f %f\n", &uv.x, &uv.y );
        uvs.push_back(uv);

      } else if ( strcmp( lineHeader, "vn" ) == 0 ){

        glm::vec3 normal;
        fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
        normals.push_back(normal);

      } else if ( strcmp( lineHeader, "f" ) == 0 ){
        std::string vertex1, vertex2, vertex3;
        unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
        int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
        if (matches != 9){
            printf("File can't be read by our simple parser : ( Try exporting with other options\n");
        }

        Triangle toAdd(
          (vec4(vertices[vertexIndex[0] - 1], 1.0) * scale) + position,
          (vec4(vertices[vertexIndex[1] - 1], 1.0) * scale) + position,
          (vec4(vertices[vertexIndex[2] - 1], 1.0) * scale) + position,
          vec4(normals[normalIndex[0] - 1] * vec3(1, -1, -1), 1.0),
          vec4(normals[normalIndex[1] - 1] * vec3(1, -1, -1), 1.0),
          vec4(normals[normalIndex[2] - 1] * vec3(1, -1, -1), 1.0),
          uvs[uvIndex[0] - 1],
          uvs[uvIndex[1] - 1],
          uvs[uvIndex[2] - 1],
          tex_id
        );

        inject_into.push_back(toAdd);

        // vertexIndices.push_back(vertexIndex[0]);
        // vertexIndices.push_back(vertexIndex[1]);
        // vertexIndices.push_back(vertexIndex[2]);
        // uvIndices    .push_back(uvIndex[0]);
        // uvIndices    .push_back(uvIndex[1]);
        // uvIndices    .push_back(uvIndex[2]);
        // normalIndices.push_back(normalIndex[0]);
        // normalIndices.push_back(normalIndex[1]);
        // normalIndices.push_back(normalIndex[2]);
      }


  }

  cout << "Vertices: " << vertices.size() << " UVS: " << uvs.size() << " Normals: " << normals.size() << endl;

  fclose(file);

}
