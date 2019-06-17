
//Drawing
vector<Pixel> LEFT_PIXELS;
vector<Pixel> RIGHT_PIXELS;

void StandardDrawRows ( screen* screen, const vector<Pixel>& leftPixels, const vector<Pixel>& rightPixels, float (&depth_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], Pixel (&pixel_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], vec3 (&colour_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], Scene &scene) {

  for(int y = 0; y < (int)min(leftPixels.size(), rightPixels.size()); y++) {

    int x1 = leftPixels[y].x;
    int x2 = rightPixels[y].x;
    int yPos = leftPixels[y].y;

    if(yPos < 0 || yPos >= screen->height) {
      continue;
    }

    int rowLength = (x2 - x1) + 1;
    vector<Pixel> row(rowLength);
    InterpolatePixel(leftPixels[y], rightPixels[y], row);

    for(int i = 0; i < rowLength; i++){

      Pixel toDraw = row[i];
      int xPos = toDraw.x;
      int yPos = toDraw.y;

      if(xPos < 0 || xPos >= screen->width ) {
        continue;
      }

      if (toDraw.zinv > depth_buffer[xPos][yPos] ) {
          depth_buffer[xPos][yPos] = toDraw.zinv;
          pixel_buffer[xPos][yPos] = toDraw;
          colour_buffer[xPos][yPos] = PixelShader(screen, scene, toDraw);
          // PutPixelSDL( screen, xPos, yPos, PixelShader(screen, scene, toDraw));
      }

    }

  }

}


void StandardDrawPolygon ( screen* screen, vector<Pixel> &vertexPixels, float (&depth_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], Pixel (&pixel_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], vec3 (&colour_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], Scene &scene) {
  ComputePolygonRows( vertexPixels, LEFT_PIXELS, RIGHT_PIXELS );
  StandardDrawRows( screen, LEFT_PIXELS, RIGHT_PIXELS, depth_buffer, pixel_buffer, colour_buffer, scene);
}


void BarycentricDrawPolygon ( screen* screen, vector<Pixel> &vertexPixels, float (&depth_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], Pixel (&pixel_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], vec3 (&colour_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], Scene &scene) {

  int minX = min(vertexPixels[0].x, min(vertexPixels[1].x, vertexPixels[2].x));
  int maxX = max(vertexPixels[0].x, max(vertexPixels[1].x, vertexPixels[2].x));
  int minY = min(vertexPixels[0].y, min(vertexPixels[1].y, vertexPixels[2].y));
  int maxY = max(vertexPixels[0].y, max(vertexPixels[1].y, vertexPixels[2].y));

  if (minX < 0) minX = 0;
  if (minY < 0) minY = 0;
  if (maxX > SCREEN_WIDTH) maxX = SCREEN_WIDTH;
  if (maxY > SCREEN_HEIGHT) maxY = SCREEN_HEIGHT;

  const float edge_first_step_x = (vertexPixels[1].y - vertexPixels[0].y);
  const float edge_second_step_x = (vertexPixels[2].y - vertexPixels[1].y);
  const float edge_third_step_x = (vertexPixels[0].y - vertexPixels[2].y);

  const float edge_first_step_y =  - (vertexPixels[1].x - vertexPixels[0].x);
  const float edge_second_step_y =  - (vertexPixels[2].x - vertexPixels[1].x);
  const float edge_third_step_y =  - (vertexPixels[0].x - vertexPixels[2].x);

  float edge_first_y_offset = - ((-edge_first_step_y) * (minY - vertexPixels[0].y));
  float edge_second_y_offset = - ((-edge_second_step_y) * (minY - vertexPixels[1].y));
  float edge_third_y_offset = - ((-edge_third_step_y) * (minY - vertexPixels[2].y));

  for (int y = minY; y < maxY; y++) {

    float edge_first = ((minX - vertexPixels[0].x) * (edge_first_step_x)) + edge_first_y_offset;
    float edge_second = ((minX - vertexPixels[1].x) * (edge_second_step_x)) + edge_second_y_offset;
    float edge_third = ((minX - vertexPixels[2].x) * (edge_third_step_x)) + edge_third_y_offset;

    for (int x = minX; x < maxX; x++) {

      const bool within_triangle = (edge_first <= 0 && edge_second <= 0 && edge_third <= 0) || (edge_first >= 0 && edge_second >= 0 && edge_third >= 0);

      if (within_triangle) {

        const float overall_area = (edge_first + edge_second + edge_third);

        const float area_first = edge_first / overall_area;
        const float area_second = edge_second / overall_area;
        const float area_third = edge_third / overall_area;

        Pixel toDraw;
        toDraw.x = x;
        toDraw.y = y;
        InterpolateBarycentricPixel(vertexPixels[0], vertexPixels[1], vertexPixels[2], area_second, area_third, area_first, toDraw);

        if (toDraw.zinv > depth_buffer[x][y] ) {
            depth_buffer[x][y] = toDraw.zinv;
            pixel_buffer[x][y] = toDraw;
            colour_buffer[x][y] = PixelShader(screen, scene, toDraw);
            // PutPixelSDL( screen, x, y, PixelShader(screen, scene, toDraw));
        }

      }

      edge_first += edge_first_step_x;
      edge_second += edge_second_step_x;
      edge_third += edge_third_step_x;

    }

    edge_first_y_offset += edge_first_step_y;
    edge_second_y_offset += edge_second_step_y;
    edge_third_y_offset += edge_third_step_y;
  }

}


void DrawPolygon(screen* screen, vector<Pixel> &vertexPixels, float (&depth_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], Pixel (&pixel_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], vec3 (&colour_buffer)[SCREEN_WIDTH][SCREEN_HEIGHT], Scene &scene) {
  BarycentricDrawPolygon(screen, vertexPixels, depth_buffer, pixel_buffer, colour_buffer, scene);
}
