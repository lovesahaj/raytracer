#ifndef CGR_COURSEWORK_CORE_SCENE_H
#define CGR_COURSEWORK_CORE_SCENE_H

#include "../Geometry/Cone.h"
#include "../Geometry/Cube.h"
#include "../Geometry/Cylinder.h"
#include "../Geometry/Plane.h"
#include "../Geometry/Sphere.h"
#include "../Geometry/Torus.h"
#include "../Math/Vector.h"
#include "Camera.h"
#include "Light.h"
#include <ostream>
#include <vector>

// Scene-level settings
struct SceneSettings {
  Color background_color{0.05, 0.05, 0.05};
  double background_strength{1.0};
  Color ambient_light{0.1, 0.1, 0.1};
  int frame_current{1};
  int frame_start{1};
  int frame_end{250};
  int fps{24};
  int max_bounces{12};
  int diffuse_bounces{4};
  int glossy_bounces{4};
  int transmission_bounces{12};
};

struct Scene {
  SceneSettings settings;
  std::vector<Camera> cameras;
  std::vector<Light> lights;
  std::vector<Sphere> spheres;
  std::vector<Cube> cubes;
  std::vector<Plane> planes;
  std::vector<Torus> toruses;
  std::vector<Cylinder> cylinders;
  std::vector<Cone> cones;
};

inline std::ostream &operator<<(std::ostream &os, const Scene &scene) {
  os << "Scene:\n";
  os << "  Cameras: " << scene.cameras.size() << "\n";
  for (const auto &cam : scene.cameras) {
    os << "    " << cam << "\n";
  }
  os << "  Lights: " << scene.lights.size() << "\n";
  for (const auto &light : scene.lights) {
    os << "    " << light << "\n";
  }
  os << "  Spheres: " << scene.spheres.size() << "\n";
  for (const auto &sphere : scene.spheres) {
    os << "    " << sphere << "\n";
  }
  os << "  Cubes: " << scene.cubes.size() << "\n";
  for (const auto &cube : scene.cubes) {
    os << "    " << cube << "\n";
  }
  os << "  Planes: " << scene.planes.size() << "\n";
  for (const auto &plane : scene.planes) {
    os << "    " << plane << "\n";
  }
  os << "  Toruses: " << scene.toruses.size() << "\n";
  for (const auto &torus : scene.toruses) {
    os << "    " << torus << "\n";
  }
  os << "  Cylinders: " << scene.cylinders.size() << "\n";
  for (const auto &cylinder : scene.cylinders) {
    os << "    " << cylinder << "\n";
  }
  os << "  Cones: " << scene.cones.size() << "\n";
  for (const auto &cone : scene.cones) {
    os << "    " << cone << "\n";
  }
  return os;
}

#endif // CGR_COURSEWORK_CORE_SCENE_H
