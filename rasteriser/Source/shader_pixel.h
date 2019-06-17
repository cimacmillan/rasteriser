

vec3 getLighting (const Pixel &p, vec3 &pixel_pos, Scene &scene, vec4 &normal) {
  vec3 lighting_sum = vec3(0, 0, 0);
  for(int i = 0; i < (int)scene.lights.size(); i++) {
    Light light = scene.lights[i];

    vec3 light_position = vec3(light.position);
    vec3 difference = (light_position - pixel_pos);
    float distance_squared = (difference.x * difference.x) + (difference.y * difference.y) + (difference.z * difference.z);
    float dotProduct = max(dot(vec3(normal), normalize(difference)), 0);
    lighting_sum += (light.power * dotProduct) / (4 * 3.14159f * distance_squared);
  }
  return lighting_sum;
}

vec3 PixelShader( screen* screen, Scene &scene, const Pixel& p) {

  if (p.dead == 1) return vec3(0, 0, 0);

  vec3 pos = vec3(p.pos / p.zinv);
  vec4 normal = p.normal / p.zinv;
  vec3 color = p.color / p.zinv;

  vec3 lighting_sum = getLighting(p, pos, scene, normal);
  vec3 reflected = (lighting_sum + scene.indirectLight) * color;

  return reflected;

}
