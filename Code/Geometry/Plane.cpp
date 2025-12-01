#include "Plane.h"

#include <algorithm>

#include "Intersections.h"

// Plane intersection
bool intersect_plane(const Plane& plane, const Ray& ray, HitRecord& hit, double t_min,
                     double t_max) {
  // Need at least 3 points to define a plane
  if (plane.points.size() < 3) {
    return false;
  }

  // Compute plane normal using cross product of two edge vectors
  // v1 and v2 are two edges of the triangle formed by first 3 points
  Vec3 v1 = plane.points[1] - plane.points[0];
  Vec3 v2 = plane.points[2] - plane.points[0];
  Direction plane_normal = v1.cross(v2).norm();  // Normalize to unit length

  // Check if ray is parallel to plane (dot product near zero)
  double denom = plane_normal.dot(ray.direction);
  if (areSame(denom, 0.0)) {
    return false;  // Ray is parallel, no intersection
  }

  // Calculate intersection distance t using plane equation
  // t = ((point_on_plane - ray_origin) · normal) / (ray_direction · normal)
  double t = (plane.points[0] - ray.origin).dot(plane_normal) / denom;

  // Check if intersection is within valid range
  if (t < t_min || t > t_max) {
    return false;
  }

  // Calculate intersection point
  Point intersection_point = ray.origin + ray.direction * t;

  // Check if intersection point is within the bounds of the plane rectangle
  // For a rectangle defined by 4 points, we need to check if the point
  // is inside the quadrilateral

  // For simplicity, assume points define an axis-aligned or near-axis-aligned
  // rectangle Find the bounding box of all points and check if intersection is
  // within
  Point min_bound = plane.points[0];
  Point max_bound = plane.points[0];

  for (const auto& p : plane.points) {
    min_bound.x = std::min(min_bound.x, p.x);
    min_bound.y = std::min(min_bound.y, p.y);
    min_bound.z = std::min(min_bound.z, p.z);
    max_bound.x = std::max(max_bound.x, p.x);
    max_bound.y = std::max(max_bound.y, p.y);
    max_bound.z = std::max(max_bound.z, p.z);
  }

  // Add small tolerance to avoid floating-point precision issues at edges
  double tolerance = 1e-6;

  // Check if intersection point is within bounds (considering the dominant
  // axes) For a plane, one dimension will be nearly constant, so we check the
  // other two
  bool within_bounds = true;

  // Check each axis - if the range is non-zero, verify the point is within
  // bounds
  if (max_bound.x - min_bound.x > tolerance) {
    if (intersection_point.x < min_bound.x - tolerance ||
        intersection_point.x > max_bound.x + tolerance) {
      within_bounds = false;
    }
  }

  if (max_bound.y - min_bound.y > tolerance) {
    if (intersection_point.y < min_bound.y - tolerance ||
        intersection_point.y > max_bound.y + tolerance) {
      within_bounds = false;
    }
  }

  if (max_bound.z - min_bound.z > tolerance) {
    if (intersection_point.z < min_bound.z - tolerance ||
        intersection_point.z > max_bound.z + tolerance) {
      within_bounds = false;
    }
  }

  // If intersection point is outside bounds, return false
  if (!within_bounds) {
    return false;
  }

  // Fill in hit record with intersection data
  hit.t = t;
  hit.intersection_point = intersection_point;
  hit.set_face_normal(ray, plane_normal);

  // Set material properties
  hit.material = plane.material;
  hit.object_name = plane.name;

  // Calculate texture coordinates for plane
  // Use two edge vectors to create a local 2D coordinate system
  // We need the ACTUAL edge lengths (not normalized) to properly map UVs
  Vec3 edge1_full = plane.points[1] - plane.points[0];
  Vec3 edge2_full = plane.points[2] - plane.points[0];

  double edge1_length = edge1_full.length();
  double edge2_length = edge2_full.length();

  Vec3 edge1_norm = edge1_full / edge1_length;
  Vec3 edge2_norm = edge2_full / edge2_length;

  Vec3 local_pos = intersection_point - plane.points[0];

  // Project onto edge directions and normalize by edge lengths to get [0,1]
  // range
  hit.u = local_pos.dot(edge1_norm) / edge1_length;
  hit.v = local_pos.dot(edge2_norm) / edge2_length;

  // Calculate tangent space for normal mapping
  // Tangent follows edge1 direction (U)
  hit.tangent = edge1_norm;

  // Bitangent must be orthogonal to both normal and tangent
  // Use Gram-Schmidt orthogonalization: B = (N × T)
  // This ensures a proper orthonormal TBN basis
  hit.bitangent = hit.normal.cross(hit.tangent).norm();

  return true;
}

// Plane bounding box
BoundingBox get_plane_bounding_box(const Plane& plane) {
  if (plane.points.empty()) {
    return BoundingBox{Point(0, 0, 0), Point(0, 0, 0)};
  }

  // Start with first point as both min and max
  Point min_point = plane.points[0];
  Point max_point = plane.points[0];

  // Expand to contain all other points
  for (const auto& p : plane.points) {
    min_point.x = std::min(min_point.x, p.x);
    min_point.y = std::min(min_point.y, p.y);
    min_point.z = std::min(min_point.z, p.z);
    max_point.x = std::max(max_point.x, p.x);
    max_point.y = std::max(max_point.y, p.y);
    max_point.z = std::max(max_point.z, p.z);
  }

  BoundingBox object_bbox{min_point, max_point};

  if (plane.has_motion) {
    // Compute union of bounding boxes at start and end of motion
    Transform start_transform(plane.start_transform);
    Transform end_transform(plane.end_transform);

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
    return plane.cached_transform.transform_bbox(object_bbox);
  }
}
