#include "Camera.h"

#include "../Math/Random.h"
#include "../Utils/logger.h"

using Utils::Logger;

// Generate a ray from camera through pixel (x, y) with sub-pixel precision
// x and y are in pixel coordinates (0 to width, 0 to height)
Ray Camera::get_ray(double x, double y, int width, int height, double time) const {
  // Log first ray generation for debugging
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
  // Step 1: Pixel coordinates to Normalized Device Coordinates (NDC)
  // NDC range: x in [0, 1], y in [0, 1]
  double ndc_x = x / width;
  double ndc_y = y / height;

  // Step 2: Construct camera coordinate system
  // w points opposite to gaze direction (camera looks down -w)
  Direction w = -gaze_direction.norm();
  // u points to the right (cross product of up and w)
  Direction u = up_direction.cross(w).norm();
  // v points up (cross product of w and u)
  Direction v = w.cross(u).norm();

  // Step 3: Calculate viewport dimensions in world space
  // The focal length and sensor are in mm from Blender, but we need world-space
  // dimensions We treat focal_length as the distance to image plane in world
  // units (NOT mm) The sensor dimensions determine the aspect ratio and
  // relative scale
  double aspect_ratio = sensor_width / sensor_height;

  // Use focal length as image plane distance (treat as world units, typically
  // ~1.0) Scale sensor dimensions to maintain proper FOV Standard approach:
  // viewport height = 2 * tan(vfov/2) * distance For now, scale sensor by a
  // factor to convert from mm to reasonable world scale
  double scale_factor = 0.001;  // Convert mm to world units (1mm = 0.001 units)

  double viewport_w = sensor_width * scale_factor;
  double viewport_h = sensor_height * scale_factor;

  // Convert NDC to viewport coordinates (centered at origin)
  // Flip y because image coordinates go down, but camera y goes up
  double viewport_x = (ndc_x - 0.5) * viewport_w;
  double viewport_y = (0.5 - ndc_y) * viewport_h;

  // Image plane distance in world units
  double image_plane_dist = focal_length * scale_factor;

  // Step 4: Calculate point on image plane in world space
  // Image plane is in the -w direction (along gaze) at distance focal_length
  Point image_point = location - w * image_plane_dist + u * viewport_x + v * viewport_y;

  // Step 5: Ray from camera origin towards image plane point
  Point ray_origin = location;
  Direction ray_direction = (image_point - location).norm();

  // Depth of Field (Thin Lens Model)
  if (dof_enabled && aperture_fstop > 0.0) {
    // 1. Calculate focal point (where the ray hits the focus plane)
    // The focus plane is perpendicular to gaze_direction at distance focus_distance
    // We need to find where the pinhole ray intersects this plane
    //
    // Plane equation: dot(point - plane_center, plane_normal) = 0
    // where plane_center = location + gaze_direction * focus_distance
    // and plane_normal = gaze_direction
    //
    // Ray equation: point = ray_origin + ray_direction * t
    // Solving for t: t = focus_distance / dot(gaze_direction, ray_direction)

    double t = focus_distance / gaze_direction.dot(ray_direction);
    Point focus_point = ray_origin + ray_direction * t;

    // 2. Sample point on lens aperture
    // Aperture diameter = focal_length / f_stop
    // But focal_length here is in mm, need world units?
    // Let's use a simpler aperture radius parameter derived from f-stop
    // aperture_radius = (focal_length * scale_factor) / (2 * fstop)
    double aperture_radius = (focal_length * scale_factor) / (2.0 * aperture_fstop);

    // Sample disk
    double r1 = random_double();
    double r2 = random_double();
    double r = aperture_radius * std::sqrt(r1);
    double theta = 2.0 * M_PI * r2;

    double lens_x = r * std::cos(theta);
    double lens_y = r * std::sin(theta);

    Point lens_point = location + u * lens_x + v * lens_y;

    // 3. New ray from lens point to focus point
    ray_origin = lens_point;
    ray_direction = (focus_point - lens_point).norm();
  }

  return Ray(ray_origin, ray_direction, time);
}
