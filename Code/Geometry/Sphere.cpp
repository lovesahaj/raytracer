#include "Sphere.h"
#include "../Math/Transform.h"
#include "Intersections.h"
#include <cmath>

bool intersect_sphere(const Sphere &sphere, const Ray &ray, HitRecord &hit,
                      double t_min, double t_max) {
  Transform transform;
  if (sphere.has_motion) {
    Mat4 current_matrix = Mat4::interpolate(sphere.start_transform, sphere.end_transform, ray.time);
    transform = Transform(current_matrix);
  } else {
    transform = sphere.cached_transform;
  }

  Ray object_ray = transform.world_to_object_ray(ray);

  Vec3 oc = object_ray.origin;

  double a = object_ray.direction.length_squared();
  double half_b = oc.dot(object_ray.direction);
  double c = oc.length_squared() - 1.0;

  double discriminant = half_b * half_b - a * c;
  if (discriminant < 0) {
    return false;
  }

  double sqrtd = std::sqrt(discriminant);

  double root = (-half_b - sqrtd) / a;
  if (root < t_min || root > t_max) {
    root = (-half_b + sqrtd) / a;
    if (root < t_min || root > t_max) {
      return false;
    }
  }

  Point object_hit_point = object_ray.origin + object_ray.direction * root;
  Direction object_normal = object_hit_point;

  hit.intersection_point = transform.object_to_world_point(object_hit_point);

  // Calculate world-space t from world-space hit point to handle non-uniform scaling
  Vec3 world_offset = hit.intersection_point - ray.origin;
  hit.t = world_offset.length() / ray.direction.length();

  Direction world_normal = transform.object_to_world_normal(object_normal);
  hit.set_face_normal(ray, world_normal);

  hit.material = sphere.material;
  hit.object_name = sphere.name;

  double theta = std::acos(object_hit_point.z);
  double phi = std::atan2(object_hit_point.y, object_hit_point.x);

  hit.u = (phi + M_PI) / (2.0 * M_PI);
  hit.v = 1.0 - (theta / M_PI);

  Direction object_tangent(-object_hit_point.y, object_hit_point.x, 0.0);

  Direction object_bitangent(-object_hit_point.z * object_hit_point.x,
                             -object_hit_point.z * object_hit_point.y,
                             object_hit_point.x * object_hit_point.x +
                                 object_hit_point.y * object_hit_point.y);

  if (object_tangent.length_squared() < 1e-6) {
    object_tangent = Direction(1, 0, 0);
    object_bitangent = Direction(0, 1, 0);
  }

  hit.tangent = transform.object_to_world_direction(object_tangent).norm();
  hit.bitangent = transform.object_to_world_direction(object_bitangent).norm();

  hit.tangent = (hit.tangent - hit.normal * hit.tangent.dot(hit.normal)).norm();

  hit.bitangent = hit.normal.cross(hit.tangent).norm();

  return true;
}

BoundingBox get_sphere_bounding_box(const Sphere &sphere) {
  BoundingBox object_bbox{Point(-1, -1, -1), Point(1, 1, 1)};

  if (sphere.has_motion) {
    Transform start_transform(sphere.start_transform);
    Transform end_transform(sphere.end_transform);

    BoundingBox box_start = start_transform.transform_bbox(object_bbox);
    BoundingBox box_end = end_transform.transform_bbox(object_bbox);

    BoundingBox result;
    result.min.x = std::min(box_start.min.x, box_end.min.x);
    result.min.y = std::min(box_start.min.y, box_end.min.y);
    result.min.z = std::min(box_start.min.z, box_end.min.z);
    result.max.x = std::max(box_start.max.x, box_end.max.x);
    result.max.y = std::max(box_start.max.y, box_end.max.y);
    result.max.z = std::max(box_start.max.z, box_end.max.z);
    return result;
  } else {
    return sphere.cached_transform.transform_bbox(object_bbox);
  }
}
