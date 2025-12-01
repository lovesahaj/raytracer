#ifndef CGR_COURSEWORK_GEOMETRY_SHAPE_H
#define CGR_COURSEWORK_GEOMETRY_SHAPE_H

#include "../Core/Material.h"
#include "../Math/Transform.h"
#include <string>

struct Shape {
  std::string name;
  std::string type;
  Material material;  // Material properties for shading
  bool visible{true}; // Visibility flag

  // Motion blur data (Part 3.1) - Transform-based system
  bool has_motion{false};
  Mat4 start_transform; // Transform at t=0
  Mat4 end_transform;   // Transform at t=1

  // Cached transform (precomputed to avoid recalculation in hot path)
  Transform cached_transform;
};

#endif // CGR_COURSEWORK_GEOMETRY_SHAPE_H
