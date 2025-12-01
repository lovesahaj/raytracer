#ifndef CGR_COURSEWORK_RENDER_RAYTRACER_H
#define CGR_COURSEWORK_RENDER_RAYTRACER_H

#include "../Core/Config.h"
#include "../Core/Scene.h"
#include "../Geometry/HitRecord.h"
#include "../IO/Image.h"
#include "../Math/Vector.h"
#include <memory>

// Forward declaration for BVH
struct BVHNode;

/**
 * Raytracer class encapsulates the core rendering logic.
 * It manages the BVH, handles ray-scene intersections, and computes shading.
 */
class Raytracer {
public:
  // Constructor: Initializes the raytracer with a scene
  Raytracer(const Scene &scene);

  // Destructor
  ~Raytracer();

  // Main render method
  Image render(const Camera &camera, int width, int height,
               int samples_per_pixel);

private:
  const Scene &scene;
  std::unique_ptr<BVHNode> bvh_root;

  // Preload textures for the scene
  void preload_textures();

  // Core recursive trace function
  Color trace(const Ray &ray, int depth);

  // Shading function (Blinn-Phong)
  Color shade(const HitRecord &hit, const Direction &view_dir);

  // Shadow computation (Soft shadows)
  double compute_shadow(const Point &point, const Direction &normal,
                        const Light &light);

  // Helper for area light sampling
  Point sample_area_light(const Light &light);

  // Helper for random numbers
  double random_double();
};

// Global functions for backward compatibility or simple usage
// (Can be deprecated later)
Image render_scene_bvh_antialiased(const Scene &scene, const Camera &camera,
                                   int width, int height, int samples_per_pixel,
                                   int shadow_samples);

#endif // CGR_COURSEWORK_RENDER_RAYTRACER_H