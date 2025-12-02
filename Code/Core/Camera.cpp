#include "Camera.h"

#include "../Math/Random.h"
#include "../Utils/logger.h"

using Utils::Logger;

Ray Camera::get_ray(double x, double y, int width, int height, double time) const {
  static thread_local bool first_ray = true;
  if (first_ray) {
    Logger::instance()
        .Debug()
        .Double("focal_length", focal_length)
        .Double("sensor_width", sensor_width)
        .Double("sensor_height", sensor_height)
        .Int("width", width)
        .Int("height", height)
        .Msg("Generating first ray for camera");
    first_ray = false;
  }

  double ndc_x = x / width;
  double ndc_y = y / height;

  // Construct camera coordinate system (right-handed)
  Direction w = -gaze_direction.norm();
  Direction u = up_direction.cross(w).norm();
  Direction v = w.cross(u).norm();

  // CRITICAL: Use resolution aspect ratio to match output dimensions
  // Sensor dimensions (e.g., 36mm x 24mm = 3:2) may differ from output resolution (e.g., 1920x1080 = 16:9)
  double resolution_aspect_ratio = static_cast<double>(width) / height;
  double scale_factor = 0.001;  // Convert mm to world units

  double viewport_h = sensor_height * scale_factor;
  double viewport_w = viewport_h * resolution_aspect_ratio;

  double viewport_x = (ndc_x - 0.5) * viewport_w;
  double viewport_y = (0.5 - ndc_y) * viewport_h;

  double image_plane_dist = focal_length * scale_factor;
  Point image_point = location - w * image_plane_dist + u * viewport_x + v * viewport_y;

  Point ray_origin = location;
  Direction ray_direction = (image_point - location).norm();

  // Depth of Field (Thin Lens Model)
  if (dof_enabled && aperture_fstop > 0.0) {
    // Calculate where pinhole ray intersects focus plane
    double t = focus_distance / gaze_direction.dot(ray_direction);
    Point focus_point = ray_origin + ray_direction * t;

    // Sample random point on lens aperture
    double aperture_radius = (focal_length * scale_factor) / (2.0 * aperture_fstop);
    double r1 = random_double();
    double r2 = random_double();
    double r = aperture_radius * std::sqrt(r1);
    double theta = 2.0 * M_PI * r2;

    Point lens_point = location + u * (r * std::cos(theta)) + v * (r * std::sin(theta));

    ray_origin = lens_point;
    ray_direction = (focus_point - lens_point).norm();
  }

  return Ray(ray_origin, ray_direction, time);
}
