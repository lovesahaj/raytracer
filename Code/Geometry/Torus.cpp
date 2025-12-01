#include "Torus.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "../Math/Transform.h"
#include "Intersections.h"

// --- HIGH PRECISION SOLVER HELPERS ---
// We use 'long double' for internal calculations to prevent the "swirled" artifacts
// caused by catastrophic cancellation in the quartic discriminant.

static const long double PEPS = 1e-10L; // High precision epsilon

// Helper to solve quadratic equation x^2 + bx + c = 0
// Returns number of real roots
static inline int solve_quadratic(long double b, long double c, double roots[2]) {
  long double disc = b * b - 4.0L * c;

  // Aggressive clamping: treat slightly negative discriminants as 0 (tangent case)
  if (disc < -PEPS) return 0;
  if (disc < 0) disc = 0;

  if (disc == 0) {
    roots[0] = (double)(-0.5L * b);
    return 1;
  }

  long double q = -0.5L * (b + (b > 0 ? std::sqrt(disc) : -std::sqrt(disc)));
  roots[0] = (double)q;
  roots[1] = (double)(c / q);
  return 2;
}

// Helper to solve cubic equation x^3 + ax^2 + bx + c = 0
// Returns number of real roots
static inline int solve_cubic(long double a, long double b, long double c, double roots[3]) {
  long double const ONE_THIRD = 1.0L / 3.0L;
  long double sq_a = a * a;
  long double p = b - ONE_THIRD * sq_a;
  long double q = a * (2.0L / 27.0L * sq_a - ONE_THIRD * b) + c;
  long double cube_p = p * p * p;
  long double D = q * q + 4.0L / 27.0L * cube_p;

  if (std::abs(D) < PEPS) D = 0;

  if (D > 0) {
    // One real root
    long double sqrt_D = std::sqrt(D);
    long double u = std::cbrt(-0.5L * q + 0.5L * sqrt_D);
    long double v = std::cbrt(-0.5L * q - 0.5L * sqrt_D);
    roots[0] = (double)(u + v - ONE_THIRD * a);
    return 1;
  } else if (D == 0) {
    // Three real roots, at least two equal
    long double u_val = std::cbrt(-0.5L * q);
    roots[0] = (double)(2.0L * u_val - ONE_THIRD * a);
    roots[1] = (double)(-u_val - ONE_THIRD * a);
    return 2;
  } else {
    // Three distinct real roots (casus irreducibilis)
    // This is where standard float often fails and creates holes
    // CRITICAL FIX: Use std::clamp to prevent acos from receiving values > 1.0 due to numerical noise
    long double acos_arg = -0.5L * q / std::sqrt(-cube_p / 27.0L);
    acos_arg = std::clamp(acos_arg, -1.0L, 1.0L);
    long double phi = std::acos(acos_arg);
    long double r = 2.0L * std::sqrt(-p / 3.0L);
    roots[0] = (double)(r * std::cos(phi * ONE_THIRD) - ONE_THIRD * a);
    roots[1] = (double)(r * std::cos((phi + 2.0L * M_PI) * ONE_THIRD) - ONE_THIRD * a);
    roots[2] = (double)(r * std::cos((phi + 4.0L * M_PI) * ONE_THIRD) - ONE_THIRD * a);
    return 3;
  }
}

// Helper to solve quartic equation (input: c[0] + c[1]*x + c[2]*x^2 + c[3]*x^3 + c[4]*x^4 = 0)
// Returns number of real roots found, stores them in roots[] array
static inline int solve_quartic(const double* __restrict c_in, double* __restrict s) {
  // Convert inputs to long double immediately for high precision
  long double c[5];
  for (int i = 0; i < 5; i++) c[i] = (long double)c_in[i];

  // Normalize so leading coefficient is 1
  if (std::abs(c[4]) < PEPS) return 0; // Not a quartic

  long double inv_a = 1.0L / c[4];
  long double A = c[3] * inv_a;
  long double B = c[2] * inv_a;
  long double C = c[1] * inv_a;
  long double D = c[0] * inv_a;

  // Depressed quartic: y^4 + py^2 + qy + r = 0
  // Substitution: x = y - A/4
  long double sq_A = A * A;
  long double p = -0.375L * sq_A + B;
  long double q = 0.125L * sq_A * A - 0.5L * A * B + C;
  long double r = -0.01171875L * sq_A * sq_A + 0.0625L * sq_A * B - 0.25L * A * C + D;

  int num_roots = 0;

  if (std::abs(q) < PEPS) {
    // Biquadratic case: y^4 + py^2 + r = 0
    double quad_roots[2];
    int n = solve_quadratic(p, r, quad_roots);
    for (int i = 0; i < n; i++) {
      if (quad_roots[i] >= 0) {
        long double y = std::sqrt((long double)quad_roots[i]);
        s[num_roots++] = (double)(y - 0.25L * A);
        s[num_roots++] = (double)(-y - 0.25L * A);
      }
    }
  } else {
    // General quartic - use Ferrari's method
    // Resolvent cubic: z^3 + 2pz^2 + (p^2 - 4r)z - q^2 = 0
    double cubic_roots[3];
    long double rc_b = 2.0L * p;
    long double rc_c = p * p - 4.0L * r;
    long double rc_d = -q * q;

    solve_cubic(rc_b, rc_c, rc_d, cubic_roots);

    // Pick a valid z (any real root works)
    long double z = cubic_roots[0];

    // FIX: Avoid negative sqrt due to numerical noise
    // If z is slightly negative due to precision, clamp to 0 (tangent case)
    if (z < 0 && z > -1e-5L) z = 0;

    long double sqrt_z = std::sqrt(std::max(0.0L, z));

    // Two quadratics from Ferrari split:
    // y^2 - sqrt(z)*y + (p/2 + z/2 - q/(2*sqrt(z))) = 0
    // y^2 + sqrt(z)*y + (p/2 + z/2 + q/(2*sqrt(z))) = 0

    long double r1 = 0.5L * (p + z + (std::abs(sqrt_z) > PEPS ? q / sqrt_z : 0));
    long double r2 = 0.5L * (p + z - (std::abs(sqrt_z) > PEPS ? q / sqrt_z : 0));

    double q_roots[2];
    int n1 = solve_quadratic(-sqrt_z, r1, q_roots);
    for (int i = 0; i < n1; i++) s[num_roots++] = q_roots[i] - 0.25 * (double)A;

    int n2 = solve_quadratic(sqrt_z, r2, q_roots);
    for (int i = 0; i < n2; i++) s[num_roots++] = q_roots[i] - 0.25 * (double)A;
  }

  return num_roots;
}

bool intersect_torus(const Torus& torus, const Ray& ray, HitRecord& hit, double t_min,
                     double t_max) {
  // Handle motion blur by interpolating transforms
  Transform transform;
  if (torus.has_motion) {
    // Interpolate between start and end transforms based on ray time
    Mat4 current_matrix = Mat4::interpolate(torus.start_transform, torus.end_transform, ray.time);
    transform = Transform(current_matrix);
  } else {
    // Use precomputed cached transform
    transform = torus.cached_transform;
  }

  Ray r = transform.world_to_object_ray(ray);

  // CRITICAL FIX 1: Normalize direction for coefficient stability
  // We must track the scaling factor to convert back to world t later
  double dir_length = r.direction.length();
  r.direction = r.direction.norm();

  double R = torus.major_radius;
  double r_tube = torus.minor_radius;

  // OPTIMIZATION: Cache repeated calculations
  const double R_sq = R * R;
  const double r_tube_sq = r_tube * r_tube;
  const double four_R_sq = 4.0 * R_sq;

  // OPTIMIZATION: Early rejection - bounding sphere test
  // Outer bounding sphere has radius R + r_tube
  double total_r = R + r_tube;
  double oc_len_sq = r.origin.dot(r.origin);
  double b_sphere = r.origin.dot(r.direction);
  double c_sphere = oc_len_sq - total_r * total_r;

  // If ray misses outer bounding sphere, no intersection possible
  if (c_sphere > 0 && b_sphere > 0) return false;
  double disc_sphere = b_sphere * b_sphere - c_sphere;
  if (disc_sphere < 0) return false;

  double oz = r.origin.z;
  double dz = r.direction.z;

  double alpha = 1.0;  // Since direction is normalized
  double beta = 2 * r.origin.dot(r.direction);
  double gamma = r.origin.dot(r.origin) - r_tube_sq - R_sq;

  // Quartic coefficients
  double c[5];
  c[4] = 1.0;       // alpha * alpha
  c[3] = 2 * beta;  // 2 * alpha * beta
  c[2] = beta * beta + 2 * gamma + four_R_sq * dz * dz;
  c[1] = 2 * beta * gamma + 2 * four_R_sq * oz * dz;
  c[0] = gamma * gamma + four_R_sq * (oz * oz - r_tube_sq);

  double roots[4];
  int num_roots = solve_quartic(c, roots);

  // Find nearest valid root
  double t_initial = t_max * dir_length;  // Scale t_max to local space
  bool hit_something = false;

  // Filter roots
  double current_t = t_initial;
  for (int i = 0; i < num_roots; i++) {
    if (roots[i] >= t_min * dir_length && roots[i] < current_t) {
      current_t = roots[i];
      hit_something = true;
    }
  }

  if (!hit_something) return false;

  // CRITICAL FIX 2: Newton-Raphson Refinement (OPTIMIZED - Adaptive)
  // The analytic root is noisy. We must refine it to snap to the surface.
  // T(x,y,z) = (x^2 + y^2 + z^2 + R^2 - r^2)^2 - 4R^2(x^2 + y^2) = 0
  double t_refined = current_t;
  for (int i = 0; i < 3; i++) {  // Reduced from 5 - most converge in 1-2 iterations
    Point p = r.origin + r.direction * t_refined;
    double sum_sq = p.x * p.x + p.y * p.y + p.z * p.z;
    double xy_sq = p.x * p.x + p.y * p.y;
    double term = sum_sq + R_sq - r_tube_sq;

    // Value of implicit function
    double val = term * term - four_R_sq * xy_sq;

    // Early exit if already on surface (tight tolerance)
    if (std::abs(val) < 1e-10) break;

    // Gradient (Normal) at p - optimized using common subexpression
    // df/dx = 4 * term * x - 8 * R^2 * x
    // df/dy = 4 * term * y - 8 * R^2 * y
    // df/dz = 4 * term * z
    double common = 4.0 * term;
    double df_dx = common * p.x - 2 * four_R_sq * p.x;
    double df_dy = common * p.y - 2 * four_R_sq * p.y;
    double df_dz = common * p.z;

    // Derivative with respect to t: dot(Gradient, RayDir)
    double derivative = df_dx * r.direction.x + df_dy * r.direction.y + df_dz * r.direction.z;

    // Newton step: t_new = t - f(t) / f'(t)
    if (std::abs(derivative) < 1e-8) break;
    double step = val / derivative;
    t_refined -= step;

    // Slightly relaxed convergence tolerance
    if (std::abs(step) < 1e-6) break;
  }

  // Check if refined t is valid
  if (t_refined < t_min * dir_length || t_refined > t_max * dir_length) return false;

  // -- End Refinement --

  Point hit_p = r.origin + r.direction * t_refined;

  // Recalculate Normal with high precision point (using cached R_sq)
  double rho = std::sqrt(hit_p.x * hit_p.x + hit_p.y * hit_p.y);
  double rho_safe = std::max(rho, 1e-10);
  double radial_factor = (rho - R) / rho_safe;

  double nx = hit_p.x * radial_factor;
  double ny = hit_p.y * radial_factor;
  double nz = hit_p.z;

  Direction normal = Direction(nx, ny, nz).norm();

  // UV mapping (Same as before)
  double phi = std::atan2(hit_p.y, hit_p.x);
  double u = (phi + M_PI) / (2.0 * M_PI);
  double theta = std::atan2(hit_p.z, rho - R);
  double v = (theta + M_PI) / (2.0 * M_PI);

  // Final Output Generation
  hit.intersection_point = transform.object_to_world_point(hit_p);

  // Convert local t back to world t correctly (independent of scale)
  Vec3 world_offset = hit.intersection_point - ray.origin;
  hit.t = world_offset.length();  // Assuming normalized world ray direction

  hit.set_face_normal(ray, transform.object_to_world_normal(normal));
  hit.material = torus.material;
  hit.object_name = torus.name;
  hit.u = u;
  hit.v = v;

  // Tangent space
  Direction object_tangent(-hit_p.y, hit_p.x, 0);
  if (object_tangent.length_squared() < 1e-6) object_tangent = Vec3(1, 0, 0);
  object_tangent = object_tangent.norm();

  Direction object_bitangent = normal.cross(object_tangent).norm();

  hit.tangent = transform.object_to_world_direction(object_tangent).norm();
  hit.bitangent = transform.object_to_world_direction(object_bitangent).norm();
  hit.tangent = (hit.tangent - hit.normal * hit.tangent.dot(hit.normal)).norm();
  hit.bitangent = hit.normal.cross(hit.tangent).norm();

  return true;
}

BoundingBox get_torus_bounding_box(const Torus& torus) {
  // FIX: Use dimensions from file
  double R = torus.major_radius;
  double r = torus.minor_radius;
  double total_r = R + r;

  BoundingBox object_bbox{Point(-total_r, -total_r, -r), Point(total_r, total_r, r)};

  if (torus.has_motion) {
    // Compute union of bounding boxes at start and end of motion
    Transform start_transform(torus.start_transform);
    Transform end_transform(torus.end_transform);

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
    return torus.cached_transform.transform_bbox(object_bbox);
  }
}
