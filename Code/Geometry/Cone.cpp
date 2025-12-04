#include "Cone.h"

#include <algorithm>
#include <cmath>

#include "../Math/Transform.h"
#include "Intersections.h"

// Constants for optimization
static constexpr double INV_2PI = 0.5 / M_PI;
static constexpr double EPSILON = 1e-6;

bool intersect_cone(const Cone& cone, const Ray& ray, HitRecord& hit, double t_min, double t_max) {
  Transform transform;
  if (cone.has_motion) {
    Mat4 current_matrix = Mat4::interpolate(cone.start_transform, cone.end_transform, ray.time);
    transform = Transform(current_matrix);
  } else {
    transform = cone.cached_transform;
  }
  Ray r = transform.world_to_object_ray(ray);

  const double radius = cone.radius;
  const double height = cone.depth;
  const double half_depth = height * 0.5;

  const double ox = r.origin.x, oy = r.origin.y, oz = r.origin.z;
  const double dx = r.direction.x, dy = r.direction.y, dz = r.direction.z;

  double a_cyl = dx * dx + dy * dy;
  double b_cyl = ox * dx + oy * dy;
  double c_cyl = ox * ox + oy * oy - radius * radius;

  if (c_cyl > 0 && b_cyl > 0 && a_cyl > EPSILON) return false;

  if (a_cyl > EPSILON) {
    double disc_cyl = b_cyl * b_cyl - a_cyl * c_cyl;
    if (disc_cyl < 0) return false;
  }

  double t_z_min, t_z_max;
  if (std::abs(dz) > EPSILON) {
    double inv_dz = 1.0 / dz;
    t_z_min = (-half_depth - oz) * inv_dz;
    t_z_max = (half_depth - oz) * inv_dz;
    if (t_z_min > t_z_max) std::swap(t_z_min, t_z_max);
    if (t_z_max < t_min || t_z_min > t_max) return false;
  } else if (oz < -half_depth || oz > half_depth) {
    return false;
  }

  const double k = radius / height;
  const double k2 = k * k;
  const double z_tip = half_depth;
  const double z_term_origin = z_tip - oz;

  const double ox2_oy2 = ox * ox + oy * oy;
  const double dx2_dy2 = dx * dx + dy * dy;
  const double ox_dx_oy_dy = ox * dx + oy * dy;

  double a = dx2_dy2 - k2 * dz * dz;
  double b = 2.0 * (ox_dx_oy_dy + k2 * z_term_origin * dz);
  double c = ox2_oy2 - k2 * z_term_origin * z_term_origin;

  double t_near = t_max;
  bool hit_something = false;
  Direction normal;
  Point hit_p;
  double u = 0, v = 0;

  if (std::abs(a) > EPSILON) {
    double discriminant = b * b - 4.0 * a * c;
    if (discriminant >= 0) {
      double sqrt_d = std::sqrt(discriminant);
      double inv_2a = 0.5 / a;
      double t1 = (-b - sqrt_d) * inv_2a;
      double t2 = (-b + sqrt_d) * inv_2a;

      if (t1 > t2) std::swap(t1, t2);

      if (t1 >= t_min && t1 <= t_max) {
        double z = oz + t1 * dz;
        if (z >= -half_depth && z <= half_depth) {
          t_near = t1;
          hit_something = true;
          hit_p = r.origin + r.direction * t1;
          double z_diff = z_tip - hit_p.z;
          normal = Direction(hit_p.x, hit_p.y, k2 * z_diff).norm();
          double phi = std::atan2(hit_p.y, hit_p.x);
          u = (phi + M_PI) * INV_2PI;
          v = (z + half_depth) / height;
        }
      }

      if (!hit_something && t2 >= t_min && t2 < t_near) {
        double z = oz + t2 * dz;
        if (z >= -half_depth && z <= half_depth) {
          t_near = t2;
          hit_something = true;
          hit_p = r.origin + r.direction * t2;
          double z_diff = z_tip - hit_p.z;
          normal = Direction(hit_p.x, hit_p.y, k2 * z_diff).norm();
          double phi = std::atan2(hit_p.y, hit_p.x);
          u = (phi + M_PI) * INV_2PI;
          v = (z + half_depth) / height;
        }
      }
    }
  }

  if (std::abs(dz) > EPSILON) {
    double inv_dz = 1.0 / dz;
    double t_cap = (-half_depth - oz) * inv_dz;
    
    if (t_cap >= t_min && t_cap < t_near) {
      double x = ox + t_cap * dx;
      double y = oy + t_cap * dy;
      double r_sq = x * x + y * y;
      double radius_sq = radius * radius;
      
      if (r_sq <= radius_sq) {
        t_near = t_cap;
        hit_something = true;
        hit_p = Point(x, y, -half_depth);
        normal = Direction(0, 0, -1);
        
        double inv_radius = 1.0 / radius;
        u = (x * inv_radius + 1.0) * 0.5;
        v = (y * inv_radius + 1.0) * 0.5;
      }
    }
  }

  if (!hit_something) return false;

  hit.intersection_point = transform.object_to_world_point(hit_p);
  
  Vec3 world_offset = hit.intersection_point - ray.origin;
  double dir_len_sq = ray.direction.length_squared();
  hit.t = std::sqrt(world_offset.length_squared() / dir_len_sq);
  
  hit.set_face_normal(ray, transform.object_to_world_normal(normal));
  hit.material = cone.material;
  hit.object_name = cone.name;
  hit.u = u;
  hit.v = v;

  Direction object_tangent, object_bitangent;
  double nz_abs = std::abs(normal.z);
  if (nz_abs > 0.9) {
    object_tangent = Direction(1, 0, 0);
    object_bitangent = Direction(0, 1, 0);
  } else {
    object_tangent = Direction(-hit_p.y, hit_p.x, 0);
    double len_sq = object_tangent.length_squared();
    if (len_sq > EPSILON) {
      double inv_len = 1.0 / std::sqrt(len_sq);
      object_tangent = object_tangent * inv_len;
    } else {
      object_tangent = Direction(1, 0, 0);
    }
    object_bitangent = normal.cross(object_tangent);
  }
  
  hit.tangent = transform.object_to_world_direction(object_tangent).norm();
  hit.bitangent = transform.object_to_world_direction(object_bitangent).norm();
  hit.tangent = (hit.tangent - hit.normal * hit.tangent.dot(hit.normal)).norm();
  hit.bitangent = hit.normal.cross(hit.tangent).norm();

  return true;
}

BoundingBox get_cone_bounding_box(const Cone& cone) {
  double r = cone.radius;
  double h = cone.depth / 2.0;
  BoundingBox object_bbox{Point(-r, -r, -h), Point(r, r, h)};

  if (cone.has_motion) {
    Transform start_transform(cone.start_transform);
    Transform end_transform(cone.end_transform);

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
    return cone.cached_transform.transform_bbox(object_bbox);
  }
}