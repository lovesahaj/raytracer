#include "Sphere.h"
#include "../Math/Transform.h"
#include "Intersections.h"
#include <cmath>

// Sphere intersection
bool intersect_sphere(const Sphere &sphere, const Ray &ray, HitRecord &hit,
                      double t_min, double t_max) {
  // Handle motion blur by interpolating transforms
  Transform transform;
  if (sphere.has_motion) {
    // Interpolate between start and end transforms based on ray time
    Mat4 current_matrix = Mat4::interpolate(sphere.start_transform, sphere.end_transform, ray.time);
    transform = Transform(current_matrix);
  } else {
    // Use precomputed cached transform
    transform = sphere.cached_transform;
  }

  // Transform ray from world space to object space
  // This allows us to always intersect with a simple unit sphere
  Ray object_ray = transform.world_to_object_ray(ray);

  // Step 3: Intersect with unit sphere (radius=1) at origin in object space
  // Vector from ray origin to sphere center (which is at origin)
  Vec3 oc = object_ray.origin;

  // Solve quadratic equation: at^2 + 2bt + c = 0
  // where ray P(t) = O + tD intersects sphere |P|^2 = 1
  double a = object_ray.direction.length_squared();
  double half_b = oc.dot(object_ray.direction);
  double c = oc.length_squared() - 1.0; // Unit sphere has radius = 1

  // Check discriminant to see if ray intersects sphere
  double discriminant = half_b * half_b - a * c;
  if (discriminant < 0) {
    return false; // No intersection
  }

  double sqrtd = std::sqrt(discriminant);

  // Find nearest intersection point within valid t range
  double root = (-half_b - sqrtd) / a; // Try near intersection first
  if (root < t_min || root > t_max) {
    root = (-half_b + sqrtd) / a; // Try far intersection
    if (root < t_min || root > t_max) {
      return false; // No valid intersection in range
    }
  }

  // Step 4: Compute hit point and normal in object space
  Point object_hit_point = object_ray.origin + object_ray.direction * root;
  // For unit sphere at origin, the position vector IS the outward normal
  Direction object_normal = object_hit_point;

  // Step 5: Transform results back to world space
  hit.intersection_point = transform.object_to_world_point(object_hit_point);

  // CRITICAL FIX: Calculate world-space t from world-space hit point
  // Using object-space t is WRONG when there's non-uniform scaling
  // because the ray direction gets scaled differently along each axis
  Vec3 world_offset = hit.intersection_point - ray.origin;
  hit.t = world_offset.length() / ray.direction.length();

  Direction world_normal = transform.object_to_world_normal(object_normal);
  hit.set_face_normal(ray, world_normal);

  // Step 6: Set material properties
  hit.material = sphere.material;
  hit.object_name = sphere.name;

  // Step 7: Calculate texture coordinates (spherical mapping)
  // Use Z-up coordinate system to match Blender/Standard conventions
  // Poles at +Z and -Z.
  double theta = std::acos(object_hit_point.z); // Polar angle from +Z (0 to PI)
  double phi = std::atan2(object_hit_point.y,
                          object_hit_point.x); // Azimuthal angle ( -PI to PI)

  hit.u = (phi + M_PI) / (2.0 * M_PI); // Map [-PI, PI] to [0, 1]
  hit.v = 1.0 - (theta / M_PI);        // Map [0, PI] to [1, 0] (1 at Top/+Z)

  // Step 8: Calculate tangent space for normal mapping
  // Parametric definition:
  // x = sin(theta) * cos(phi)
  // y = sin(theta) * sin(phi)
  // z = cos(theta)

  // Tangent (∂p/∂φ):
  // dx/dphi = -sin(theta)sin(phi) = -y
  // dy/dphi =  sin(theta)cos(phi) =  x
  // dz/dphi = 0
  Direction object_tangent(-object_hit_point.y, object_hit_point.x, 0.0);

  // Bitangent (∂p/∂θ):
  // Instead of deriving from sphericals (singularities at poles), use cross
  // product B = N x T (for RH system). N = (x, y, z). T = (-y, x, 0). Bx = y*0
  // - z*x = -zx By = z*(-y) - x*0 = -zy Bz = x*x - y*(-y) = x^2 + y^2
  Direction object_bitangent(-object_hit_point.z * object_hit_point.x,
                             -object_hit_point.z * object_hit_point.y,
                             object_hit_point.x * object_hit_point.x +
                                 object_hit_point.y * object_hit_point.y);

  // Handle poles (where x=0, y=0)
  if (object_tangent.length_squared() < 1e-6) {
    object_tangent = Direction(1, 0, 0);
    object_bitangent = Direction(0, 1, 0);
  }

  // Transform tangent and bitangent to world space using standard direction
  // transform
  hit.tangent = transform.object_to_world_direction(object_tangent).norm();
  hit.bitangent = transform.object_to_world_direction(object_bitangent).norm();

  // Gram-Schmidt orthogonalization to ensure orthogonal TBN basis
  // 1. Re-orthogonalize Tangent with respect to Normal
  hit.tangent = (hit.tangent - hit.normal * hit.tangent.dot(hit.normal)).norm();

  // 2. Re-calculate Bitangent to be orthogonal to both Normal and Tangent
  hit.bitangent = hit.normal.cross(hit.tangent).norm();

  return true;
}

// Sphere bounding box
BoundingBox get_sphere_bounding_box(const Sphere &sphere) {
  // Unit sphere has bounding box from (-1,-1,-1) to (1,1,1) in object space
  BoundingBox object_bbox{Point(-1, -1, -1), Point(1, 1, 1)};

  if (sphere.has_motion) {
    // Compute union of bounding boxes at start and end of motion
    Transform start_transform(sphere.start_transform);
    Transform end_transform(sphere.end_transform);

    BoundingBox box_start = start_transform.transform_bbox(object_bbox);
    BoundingBox box_end = end_transform.transform_bbox(object_bbox);

    // Return union of both boxes
    BoundingBox result;
    result.min.x = std::min(box_start.min.x, box_end.min.x);
    result.min.y = std::min(box_start.min.y, box_end.min.y);
    result.min.z = std::min(box_start.min.z, box_end.min.z);
    result.max.x = std::max(box_start.max.x, box_end.max.x);
    result.max.y = std::max(box_start.max.y, box_end.max.y);
    result.max.z = std::max(box_start.max.z, box_end.max.z);
    return result;
  } else {
    // Use precomputed cached transform
    return sphere.cached_transform.transform_bbox(object_bbox);
  }
}
