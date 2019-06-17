# Rasteriser
Rasteriser made for 3rd year graphics project.

Features include:
  + Very basic generated terrain using Diamond-Square
  + Multiple lights
  + Barycentric coordinates for triangle rendering
  + .OBJ loader
  + Interpolated normals for smooth shading
  + Basic texture mapping, with basic alpha (if alpha 0, then ignore)
  + Basic fog effect
  + Depth of field effect using horizontal and vertical pass of a 1D convolution, circular-ish using Sin function
  + OpenCL GPU pixel shader and post process shader
  + Face culling
  + Full clipping vertex shader using Sutherland–Hodgman algorithm