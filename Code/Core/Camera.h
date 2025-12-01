#ifndef CGR_COURSEWORK_CORE_CAMERA_H
#define CGR_COURSEWORK_CORE_CAMERA_H

#include <ostream>
#include <string>

#include "../Math/Vector.h"
#include "Ray.h"

struct Camera {
  std::string name;
  Point location;
  Direction gaze_direction;
  Direction up_direction;
  double focal_length;
  double sensor_width;
  double sensor_height;
  int film_resolution_x;
  int film_resolution_y;

  // Depth of field settings (for lens effects)
  bool dof_enabled{false};
  double focus_distance{10.0};
  double aperture_fstop{2.8};
  int aperture_blades{0};

  // Camera type and clipping
  std::string camera_type{"PERSP"}; // PERSP, ORTHO, PANO
  double clip_start{0.1};
  double clip_end{100.0};

  // Generate a ray for pixel (x, y)
  Ray get_ray(double x, double y, int width, int height,
              double time = 0.5) const;
};

inline std::ostream &operator<<(std::ostream &os, const Camera &cam) {
  os << "Camera '" << cam.name << "': location=" << cam.location
     << ", gaze=" << cam.gaze_direction
     << ", resolution=" << cam.film_resolution_x << "x"
     << cam.film_resolution_y;
  return os;
}

#endif // CGR_COURSEWORK_CORE_CAMERA_H
