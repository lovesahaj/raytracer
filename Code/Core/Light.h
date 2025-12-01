#ifndef CGR_COURSEWORK_CORE_LIGHT_H
#define CGR_COURSEWORK_CORE_LIGHT_H

#include "../Math/Vector.h"
#include <ostream>
#include <string>

struct Light {
  std::string name;
  Point location;
  double intensity;
  Color color;

  // Light type: POINT, SUN, SPOT, AREA
  std::string light_type{"POINT"};

  // Spot light properties
  double spot_size{0.785398}; // 45 degrees in radians
  double spot_blend{0.15};

  // Area light properties
  std::string area_shape{"SQUARE"}; // SQUARE, RECTANGLE, DISK, ELLIPSE
  double area_size_x{1.0};
  double area_size_y{1.0};
  int samples{16};
  Direction normal{0.0, 0.0, -1.0}; // Direction for area lights

  // Directional light (sun) properties
  Direction direction{0.0, 0.0, -1.0};
  double angle{0.0};

  // Shadow properties (for distributed raytracing)
  bool cast_shadows{true};
  double shadow_soft_size{0.0};

  // Sample a point on the light surface (for soft shadows)
  // u, v are random numbers in [0,1]
  Point sample_point(double u, double v) const {
      if (light_type != "AREA") return location;

      // Build orthonormal basis from light normal
      Direction light_normal = normal;
      if (light_normal.length_squared() < 0.1) {
          light_normal = Direction(0, 0, -1); // Fallback
      }
      light_normal = light_normal.norm();

      // Create right and up vectors perpendicular to normal
      Direction light_right = (std::abs(light_normal.x) > 0.9)
          ? Direction(0, 1, 0)
          : Direction(1, 0, 0);
      light_right = light_normal.cross(light_right).norm();
      Direction light_up = light_normal.cross(light_right).norm();

      // Re-center randoms to [-0.5, 0.5]
      double ru = u - 0.5;
      double rv = v - 0.5;

      if (area_shape == "SQUARE" || area_shape == "RECTANGLE") {
          return location + light_right * (ru * area_size_x) + light_up * (rv * area_size_y);
      } else {
          // Disk - use uniform disk sampling
          double r = std::sqrt(u);
          double theta = 2.0 * 3.14159265359 * v;
          double x = r * std::cos(theta) * (area_size_x * 0.5);
          double y = r * std::sin(theta) * (area_size_y * 0.5);
          return location + light_right * x + light_up * y;
      }
  }
};

inline std::ostream &operator<<(std::ostream &os, const Light &light) {
  os << "Light '" << light.name << "': location=" << light.location
     << ", intensity=" << light.intensity << ", color=" << light.color;
  return os;
}

#endif // CGR_COURSEWORK_CORE_LIGHT_H
