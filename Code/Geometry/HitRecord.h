#ifndef CGR_COURSEWORK_GEOMETRY_HIT_RECORD_H
#define CGR_COURSEWORK_GEOMETRY_HIT_RECORD_H

#include "../Core/Material.h"
#include "../Core/Ray.h"
#include "../Math/Vector.h"

struct HitRecord {
  Point intersection_point;
  Direction normal;
  double t;        // distance along ray
  bool front_face; // did ray hit from outside?

  // Material properties (for Module 3)
  Material material;
  std::string object_name;

  // Texture coordinates (for texture mapping)
  double u{0.0};
  double v{0.0};

  // Tangent space vectors for normal mapping
  Direction tangent{1, 0, 0};   // T vector (along U direction)
  Direction bitangent{0, 1, 0}; // B vector (along V direction)

  // Helper method to set the normal direction based on ray direction
  // Ensures normal always points against the ray (outward)
  void set_face_normal(const Ray &ray, const Direction &outward_normal) {
    front_face = ray.direction.dot(outward_normal) < 0;
    normal = front_face ? outward_normal : -outward_normal;
  }
};

#endif // CGR_COURSEWORK_GEOMETRY_HIT_RECORD_H
