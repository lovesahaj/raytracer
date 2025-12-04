#include "Cylinder.h"
#include "../Math/Transform.h"
#include "Intersections.h"
#include <algorithm>
#include <cmath>

bool intersect_cylinder(const Cylinder &cylinder, const Ray &ray,
                        HitRecord &hit, double t_min, double t_max) {
  Transform transform;
  if (cylinder.has_motion) {
    Mat4 current_matrix = Mat4::interpolate(cylinder.start_transform, cylinder.end_transform, ray.time);
    transform = Transform(current_matrix);
  } else {
    transform = cylinder.cached_transform;
  }
  Ray r = transform.world_to_object_ray(ray);

  double radius = cylinder.radius;
  double half_depth = cylinder.depth / 2.0;

  double a = r.direction.x * r.direction.x + r.direction.y * r.direction.y;
  double b = 2.0 * (r.origin.x * r.direction.x + r.origin.y * r.direction.y);
  double c =
      r.origin.x * r.origin.x + r.origin.y * r.origin.y - radius * radius;

  double t_near = t_max;
  bool hit_something = false;
  Direction normal;
  Point hit_p;
  double u = 0, v = 0;

  if (std::abs(a) > 1e-6) {
    double discriminant = b * b - 4 * a * c;
    if (discriminant >= 0) {
      double sqrt_d = std::sqrt(discriminant);
      double t1 = (-b - sqrt_d) / (2 * a);
      double t2 = (-b + sqrt_d) / (2 * a);

      auto check_cylinder_hit = [&](double t) {
        if (t >= t_min && t < t_near) {
          double z = r.origin.z + t * r.direction.z;
          if (z >= -half_depth && z <= half_depth) {
            t_near = t;
            hit_something = true;
            hit_p = r.origin + r.direction * t;
            normal = Direction(hit_p.x / radius, hit_p.y / radius, 0.0);
            double phi = std::atan2(hit_p.y, hit_p.x);
            u = (phi + M_PI) / (2.0 * M_PI);
            v = (z + half_depth) / cylinder.depth;
          }
        }
      };

      check_cylinder_hit(t1);
      check_cylinder_hit(t2);
    }
  }

  if (std::abs(r.direction.z) > 1e-6) {
    double t_cap_top = (half_depth - r.origin.z) / r.direction.z;
    if (t_cap_top >= t_min && t_cap_top < t_near) {
      double x = r.origin.x + t_cap_top * r.direction.x;
      double y = r.origin.y + t_cap_top * r.direction.y;
      if (x * x + y * y <= radius * radius) {
        t_near = t_cap_top;
        hit_something = true;
        hit_p = r.origin + r.direction * t_cap_top;
        normal = Direction(0, 0, 1);
        u = (x / radius + 1.0) * 0.5;
        v = (y / radius + 1.0) * 0.5;
      }
    }

    double t_cap_bottom = (-half_depth - r.origin.z) / r.direction.z;
    if (t_cap_bottom >= t_min && t_cap_bottom < t_near) {
      double x = r.origin.x + t_cap_bottom * r.direction.x;
      double y = r.origin.y + t_cap_bottom * r.direction.y;
      if (x * x + y * y <= radius * radius) {
        t_near = t_cap_bottom;
        hit_something = true;
        hit_p = r.origin + r.direction * t_cap_bottom;
        normal = Direction(0, 0, -1);
        u = (x / radius + 1.0) * 0.5;
        v = (y / radius + 1.0) * 0.5;
      }
    }
  }

  if (!hit_something)
    return false;

  hit.intersection_point = transform.object_to_world_point(hit_p);
  Vec3 world_offset = hit.intersection_point - ray.origin;
  hit.t = world_offset.length() / ray.direction.length();

  hit.set_face_normal(ray, transform.object_to_world_normal(normal));
  hit.material = cylinder.material;
  hit.object_name = cylinder.name;
  hit.u = u;
  hit.v = v;

  Direction object_tangent, object_bitangent;
  if (std::abs(normal.z) > 0.9) {
    object_tangent = Vec3(1, 0, 0);
    object_bitangent = Vec3(0, 1, 0);
  } else {
    object_tangent = Vec3(-hit_p.y, hit_p.x, 0).norm();
    object_bitangent = Vec3(0, 0, 1);
  }
  hit.tangent = transform.object_to_world_direction(object_tangent).norm();
  hit.bitangent = transform.object_to_world_direction(object_bitangent).norm();
  hit.tangent = (hit.tangent - hit.normal * hit.tangent.dot(hit.normal)).norm();
  hit.bitangent = hit.normal.cross(hit.tangent).norm();

  return true;
}

BoundingBox get_cylinder_bounding_box(const Cylinder &cylinder) {
  double r = cylinder.radius;
  double h = cylinder.depth / 2.0;
  BoundingBox object_bbox{Point(-r, -r, -h), Point(r, r, h)};

  if (cylinder.has_motion) {
    Transform start_transform(cylinder.start_transform);
    Transform end_transform(cylinder.end_transform);

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
    return cylinder.cached_transform.transform_bbox(object_bbox);
  }
}
