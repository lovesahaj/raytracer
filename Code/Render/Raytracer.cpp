#include "Raytracer.h"

#include <omp.h>

#include <fstream>
#include <random>
#include <thread>

// Platform-specific headers for memory profiling
#ifdef __APPLE__
#include <sys/resource.h>
#endif

#include "../Core/Config.h"
#include "../Geometry/Bvh.h"
#include "../IO/Texture.h"
#include "../Utils/logger.h"
#include "../Utils/tqdm.hpp"

using Utils::Logger;

// FIX: Use atomic for thread-safe intersection test counting
// Note: This is still extern-declared in Bvh.cpp as thread_local, but we'll
// switch to atomic For now, keep thread_local for compatibility but document
// the issue
thread_local long long g_intersection_tests = 0;

// Global texture manager (static to persist)
static TextureManager g_texture_manager;

// Constructor
Raytracer::Raytracer(const Scene& scene) : scene(scene) {
  // Build BVH on initialization
  Logger::instance().Info().Msg("Initializing Raytracer and building BVH...");

  int total_objects = scene.spheres.size() + scene.cubes.size() + scene.planes.size() +
                      scene.toruses.size() + scene.cylinders.size() + scene.cones.size();

  std::vector<int> all_objects;
  all_objects.reserve(total_objects);
  for (int i = 0; i < total_objects; i++) {
    all_objects.push_back(i);
  }

  auto bvh_start = std::chrono::high_resolution_clock::now();
  bvh_root = build_bvh(all_objects, scene);
  auto bvh_end = std::chrono::high_resolution_clock::now();
  auto bvh_build_time = std::chrono::duration_cast<std::chrono::milliseconds>(bvh_end - bvh_start);

  Logger::instance()
      .Info()
      .Int("duration_ms", bvh_build_time.count())
      .Msg("BVH construction complete!");

  if (bvh_root) {
    BVHStats stats = get_bvh_stats(bvh_root.get());
    Logger::instance()
        .Info()
        .Int("nodes", stats.node_count)
        .Int("leaves", stats.leaf_count)
        .Int("depth", stats.max_depth)
        .Msg("BVH Stats");
  }

  // DEBUG: Print BVH root bounding box
  if (bvh_root) {
    Logger::instance()
        .Debug()
        .Double("min_x", bvh_root->bbox.min.x)
        .Double("min_y", bvh_root->bbox.min.y)
        .Double("min_z", bvh_root->bbox.min.z)
        .Double("max_x", bvh_root->bbox.max.x)
        .Double("max_y", bvh_root->bbox.max.y)
        .Double("max_z", bvh_root->bbox.max.z)
        .Msg("BVH root bbox");
  }

  preload_textures();
}

Raytracer::~Raytracer() = default;

// Helper function for random number generation
double Raytracer::random_double() {
  // FIX: Use random_device to seed each thread differently
  // The seed is computed once per thread, combining random_device with thread
  // hash
  static thread_local std::mt19937 generator(
      std::random_device{}() ^ std::hash<std::thread::id>{}(std::this_thread::get_id()));
  static thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
  return distribution(generator);
}

void Raytracer::preload_textures() {
  // Same implementation as before, but using member scene
  Logger::instance().Info().Msg("Preloading textures...");
  int loaded_count = 0;

  auto check_and_load = [&](const std::string& path) {
    if (!path.empty() && !g_texture_manager.has_texture(path)) {
      if (g_texture_manager.load_texture(path)) {
        loaded_count++;
      }
    }
  };

  for (const auto& sphere : scene.spheres) {
    if (sphere.material.has_texture) check_and_load(sphere.material.texture_file);
    check_and_load(sphere.material.normal_map);
    check_and_load(sphere.material.bump_map);
  }
  for (const auto& cube : scene.cubes) {
    if (cube.material.has_texture) check_and_load(cube.material.texture_file);
    check_and_load(cube.material.normal_map);
    check_and_load(cube.material.bump_map);
  }
  for (const auto& plane : scene.planes) {
    if (plane.material.has_texture) check_and_load(plane.material.texture_file);
    check_and_load(plane.material.normal_map);
    check_and_load(plane.material.bump_map);
  }
  // Add other shapes if needed
  Logger::instance().Info().Int("count", loaded_count).Msg("Preloaded textures");
}

Point Raytracer::sample_area_light(const Light& light) {
  if (light.light_type != "AREA") {
    return light.location;
  }

  double u = random_double() - 0.5;
  double v = random_double() - 0.5;

  // Construct basis from light normal
  // Assuming light.normal is the direction the light faces
  Direction normal = light.normal;
  if (normal.length_squared() < 0.1) normal = Direction(0, 0, -1);  // Fallback
  normal = normal.norm();

  // Create orthonormal basis
  Direction light_right = (std::abs(normal.x) > 0.9) ? Direction(0, 1, 0) : Direction(1, 0, 0);
  light_right = normal.cross(light_right).norm();
  Direction light_up = normal.cross(light_right).norm();

  if (light.area_shape == "SQUARE" || light.area_shape == "RECTANGLE") {
    Vec3 offset = light_right * (u * light.area_size_x) + light_up * (v * light.area_size_y);
    return light.location + offset;
  } else {
    // Disk
    double r = std::sqrt(random_double());
    double theta = 2.0 * M_PI * random_double();
    double x = r * std::cos(theta) * light.area_size_x;  // Use size_x as radius?
    // Usually disk uses size_x/2 for radius
    double radius_x = light.area_size_x / 2.0;
    double radius_y = light.area_size_y / 2.0;

    x = r * std::cos(theta) * radius_x;
    double y = r * std::sin(theta) * radius_y;

    return light.location + light_right * x + light_up * y;
  }
}

// External helper functions from Bvh.cpp/Raytracer.cpp original need to be
// accessible We need to check if intersect_bvh is declared in Bvh.h or
// Raytracer.h. It is declared in Bvh.h. Wait, Raytracer.cpp previously
// implemented intersect_scene_bvh, shadow_attenuation_bvh, etc. We need to
// reimplement them as member functions.

double Raytracer::compute_shadow(const Point& point, const Direction& normal, const Light& light) {
  int samples = (light.light_type == "AREA") ? std::max(1, light.samples) : 1;
  // Override with global config if set higher, or just use global config?
  // TODO.md says "enable soft shadows with sample count" via CLI.
  // If CLI sets shadow_samples > 0, we should probably use it.
  if (g_config.shadow_samples > 0 && light.light_type == "AREA") {
    samples = g_config.shadow_samples;
  }

  double total_attenuation = 0.0;
  int sqrt_samples = static_cast<int>(std::sqrt(samples));
  int actual_samples = sqrt_samples * sqrt_samples;

  // Fallback for non-square numbers or if samples < 4
  if (actual_samples == 0) {
    sqrt_samples = 1;
    actual_samples = 1;
  }

  for (int i = 0; i < sqrt_samples; i++) {
    for (int j = 0; j < sqrt_samples; j++) {
      // Stratified sampling: jitter within grid cell
      double u = (i + random_double()) / sqrt_samples;
      double v = (j + random_double()) / sqrt_samples;

      // Use Light class method
      Point light_pos = light.sample_point(u, v);

      Vec3 to_light = light_pos - point;
      double dist = to_light.length();
      Direction dir = to_light / dist;

      // Adaptive epsilon for shadow rays (prevents torus self-intersection)
      double shadow_epsilon = g_config.ray_offset_epsilon;
      if (g_config.use_adaptive_epsilon) {
        shadow_epsilon += point.length() * g_config.adaptive_epsilon_scale;
      }

      Ray shadow_ray(point + normal * shadow_epsilon, dir);

      double attenuation = 1.0;
      double current_t = shadow_epsilon;

      // Manual traversal similar to original shadow_attenuation_bvh_at_point
      // But we need intersect_bvh from Bvh.h
      while (current_t < dist) {
        HitRecord hit;
        Ray shadow_ray_step(point + dir * current_t, dir);
        double closest_t = dist - current_t;  // Limit search to light distance

        // Use global/external intersect_bvh from Bvh.h
        if (intersect_bvh(shadow_ray_step, bvh_root.get(), scene, hit, shadow_epsilon, closest_t)) {
          // Hit something
          if (hit.t < (dist - current_t)) {
            if (hit.material.transparency > 0.0) {
              double opacity = 1.0 - hit.material.transparency;
              attenuation *= (1.0 - opacity);
              current_t += hit.t + shadow_epsilon;
              if (attenuation < 0.01) break;  // Fully occluded
            } else {
              attenuation = 0.0;
              break;
            }
          } else {
            break;
          }
        } else {
          break;
        }
      }
      total_attenuation += (1.0 - attenuation);  // 1.0 = full shadow
    }
  }
  return total_attenuation / actual_samples;
}

// Independent helper functions for reflection/refraction/fresnel/normal map
// (These can be static members or private helpers)

static Direction reflect_dir(const Direction& incident, const Direction& normal) {
  return incident - normal * 2.0 * incident.dot(normal);
}

static double schlick_fresnel(double cosine, double eta_ratio) {
  // Schlick's approximation for Fresnel reflectance
  // eta_ratio = n1/n2 (ratio of refractive indices)
  // For air->glass: eta_ratio = 1.0/1.5 = 0.6667
  // For glass->air: eta_ratio = 1.5/1.0 = 1.5
  double r0 = (eta_ratio - 1.0) / (eta_ratio + 1.0);
  r0 = r0 * r0;
  return r0 + (1.0 - r0) * std::pow((1.0 - cosine), 5.0);
}

static Direction apply_normal_map(const HitRecord& hit, TextureManager& tm) {
  if (!hit.material.normal_map.empty() && tm.has_texture(hit.material.normal_map)) {
    Color ns = tm.sample(hit.material.normal_map, hit.u, hit.v);
    Vec3 tan_normal(ns.r() * 2.0 - 1.0, ns.g() * 2.0 - 1.0, ns.b() * 2.0 - 1.0);
    tan_normal.x *= hit.material.bump_strength;
    tan_normal.y *= hit.material.bump_strength;
    tan_normal = tan_normal.norm();
    return (hit.tangent * tan_normal.x + hit.bitangent * tan_normal.y + hit.normal * tan_normal.z)
        .norm();
  } else if (!hit.material.bump_map.empty() && tm.has_texture(hit.material.bump_map)) {
    // Simplified bump mapping logic
    double delta = 0.001;
    auto get_h = [&](double u, double v) {
      Color c = tm.sample(hit.material.bump_map, u, v);
      return 0.299 * c.r() + 0.587 * c.g() + 0.114 * c.b();
    };
    double h_c = get_h(hit.u, hit.v);
    double dU = (get_h(hit.u + delta, hit.v) - h_c) / delta;
    double dV = (get_h(hit.u, hit.v + delta) - h_c) / delta;
    double scale = 10.0 * hit.material.bump_strength;
    return (hit.normal - hit.tangent * dU * scale - hit.bitangent * dV * scale).norm();
  }
  return hit.normal;
}

Color Raytracer::shade(const HitRecord& hit, const Direction& view_dir) {
  Direction shading_normal = apply_normal_map(hit, g_texture_manager);

  Color base_color = hit.material.diffuse_color;
  Color ambient_color = hit.material.ambient_color;

  if (hit.material.has_texture && !hit.material.texture_file.empty()) {
    if (g_texture_manager.has_texture(hit.material.texture_file)) {
      Color tex = g_texture_manager.sample(hit.material.texture_file, hit.u, hit.v);
      // Note: PPM textures are already in linear space, no gamma decode needed
      base_color = tex * hit.material.diffuse_color;
      ambient_color = tex * hit.material.ambient_color;
    }
  }

  Color ambient = ambient_color * g_config.ambient_factor;
  Color total_light(0, 0, 0);

  for (const auto& light : scene.lights) {
    double shadow = compute_shadow(hit.intersection_point, hit.normal, light);
    if (shadow >= 1.0) continue;

    Vec3 to_light = light.location - hit.intersection_point;
    double dist = to_light.length();
    Direction L = to_light / dist;

    Color L_in = light.color * (light.intensity * g_config.light_intensity_factor / (dist * dist));
    L_in = L_in * (1.0 - shadow);

    double n_dot_l = std::max(0.0, shading_normal.dot(L));
    if (n_dot_l > 0.0) {
      // Use Material class method for BRDF with texture-modulated color
      Color brdf_val = hit.material.eval(view_dir, L, shading_normal, base_color);

      total_light += Color(brdf_val.x * L_in.x * n_dot_l, brdf_val.y * L_in.y * n_dot_l,
                           brdf_val.z * L_in.z * n_dot_l);
    }
  }

  // Debug log for shading
  if (g_config.log_level == "debug" && std::abs(view_dir.x) < 0.01 && std::abs(view_dir.y) < 0.01) {
    Logger::instance()
        .Debug()
        .Double("ambient_r", ambient.x)
        .Double("ambient_g", ambient.y)
        .Double("ambient_b", ambient.z)
        .Double("light_r", total_light.x)
        .Double("light_g", total_light.y)
        .Double("light_b", total_light.z)
        .Msg("Shade result");
  }

  return ambient + total_light;
}

Color Raytracer::trace(const Ray& ray, int depth) {
  if (depth >= g_config.max_ray_depth) return Color(0, 0, 0);

  HitRecord hit;
  // FIX: Use numeric_limits instead of 1e30 for proper max value
  double t_max = std::numeric_limits<double>::max();

  // FIX: Reduce t_min from 0.001 to 1e-5 to prevent missing valid intersections
  // on implicit surfaces like torus. The larger epsilon was causing "shattered"
  // artifacts where valid surface hits were being skipped.
  if (intersect_bvh(ray, bvh_root.get(), scene, hit, 1e-5, t_max)) {
    Direction view_dir = -ray.direction;

    // Skip shading for pure glass (pure transparent dielectrics)
    bool is_pure_glass = (hit.material.transparency >= g_config.pure_glass_threshold);
    Color color = is_pure_glass ? Color(0, 0, 0) : shade(hit, view_dir);

    Direction shading_normal = apply_normal_map(hit, g_texture_manager);

    // Reflection
    if (hit.material.reflectivity > 0.0) {
      Direction r = reflect_dir(ray.direction, shading_normal);

      // Adaptive epsilon to prevent self-intersection
      // Torus and complex geometry need larger offsets
      double epsilon = g_config.ray_offset_epsilon;
      if (g_config.use_adaptive_epsilon) {
        double dist_from_origin = hit.intersection_point.length();
        epsilon += dist_from_origin * g_config.adaptive_epsilon_scale;
      }

      // Glossy reflection: glossiness = 1 - roughness (from Blender)
      // glossiness = 1.0 -> smooth/mirror (narrow cone, high exponent)
      // glossiness = 0.0 -> rough/matte (wide cone, low exponent)
      int samples = 1;
      if (g_config.glossy_samples > 1 && hit.material.glossiness < 0.94) {
        // Limit branching to first 2 bounces to prevent exponential explosion
        if (depth < 2) {
          samples = g_config.glossy_samples;
        }
      }

      Color reflection_accum(0, 0, 0);

      if (samples == 1 || hit.material.glossiness > 0.94) {
        // Perfect mirror reflection (glossiness >= 0.94, i.e., roughness <= 0.06)
        // This matches Blender's behavior for near-mirror surfaces
        Ray reflect_ray(hit.intersection_point + hit.normal * epsilon, r, ray.time);
        reflection_accum = trace(reflect_ray, depth + 1);
      } else {
        // Build ONB around reflection direction
        Vec3 w = r;
        Vec3 u = (std::abs(w.x) > 0.1 ? Vec3(0, 1, 0) : Vec3(1, 0, 0)).cross(w).norm();
        Vec3 v = w.cross(u);

        // Use exponential curve for exponent to get sharper reflections
        // glossiness 1.0 -> exponent ~10000 (very sharp)
        // glossiness 0.8 -> exponent ~100 (somewhat glossy)
        // glossiness 0.5 -> exponent ~3 (quite rough)
        // glossiness 0.0 -> exponent ~1 (completely diffuse)
        double exponent = std::pow(10.0, hit.material.glossiness * 4.0);

        for (int i = 0; i < samples; ++i) {
          double r1 = random_double();
          double r2 = random_double();

          // Importance sampling with power cosine distribution
          double cos_theta = std::pow(r1, 1.0 / (exponent + 1.0));
          double sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));
          double phi = 2.0 * M_PI * r2;

          Vec3 local_dir(sin_theta * std::cos(phi), sin_theta * std::sin(phi), cos_theta);

          Vec3 sample_dir = (u * local_dir.x + v * local_dir.y + w * local_dir.z).norm();

          // Check if sample is below surface
          if (sample_dir.dot(hit.normal) < 0) {
            sample_dir = r;  // Fallback to ideal reflection
          }

          Ray reflect_ray(hit.intersection_point + hit.normal * epsilon, sample_dir, ray.time);
          reflection_accum += trace(reflect_ray, depth + 1);
        }
        reflection_accum /= samples;
      }

      // PBR Metal/Dielectric workflow:
      // Metals (high reflectivity, no transparency) have colored reflections
      // Dielectrics (glossy plastic, paint) have white/neutral reflections
      // Lowered threshold from 0.8 to 0.5 to match Blender's metallic export
      // (Metallic=0.8 in Blender exports as reflectivity=0.72)

      // Sample texture for metal tinting (same as in shade())
      Color reflection_tint = hit.material.diffuse_color;
      if (hit.material.has_texture && !hit.material.texture_file.empty()) {
        if (g_texture_manager.has_texture(hit.material.texture_file)) {
          Color tex = g_texture_manager.sample(hit.material.texture_file, hit.u, hit.v);
          // Note: PPM textures are already in linear space, no gamma decode needed
          reflection_tint = tex * hit.material.diffuse_color;
        }
      }

      bool is_metal = (hit.material.reflectivity > 0.5 && hit.material.transparency < 0.1);

      if (is_metal) {
        // Metal: tint reflection by TEXTURED base color (gold, copper, etc.)
        reflection_accum = reflection_accum * reflection_tint;
      }
      // Else: dielectric keeps white reflections

      color =
          color * (1.0 - hit.material.reflectivity) + reflection_accum * hit.material.reflectivity;
    }

    // Transparency (refraction with Fresnel)
    if (hit.material.transparency > 0.0) {
      // Calculate eta ratio: n1/n2
      // front_face=true: ray entering object (air->material), eta = 1.0/n
      // front_face=false: ray exiting object (material->air), eta = n/1.0
      double eta =
          hit.front_face ? (1.0 / hit.material.refractive_index) : hit.material.refractive_index;

      // CRITICAL: hit.normal is ALREADY oriented against the ray by
      // set_face_normal() Do NOT flip it again! Trust the HitRecord.
      Direction norm = hit.normal;

      // Adaptive epsilon for glass
      double epsilon = g_config.ray_offset_epsilon;
      if (g_config.use_adaptive_epsilon) {
        double dist_from_origin = hit.intersection_point.length();
        epsilon += dist_from_origin * g_config.adaptive_epsilon_scale;
      }

      double cos_theta = std::abs(ray.direction.dot(norm));
      // Pass eta_ratio (not absolute IOR) to Schlick approximation
      double fresnel = schlick_fresnel(cos_theta, eta);

      Direction r_out_perp = (ray.direction + norm * cos_theta) * eta;
      double disc = 1.0 - r_out_perp.length_squared();

      Color refract_col(0, 0, 0);
      Color reflect_col(0, 0, 0);

      // Calculate reflection color (Fresnel reflection)
      Direction ref_dir = reflect_dir(ray.direction, norm);
      Ray ref_ray(hit.intersection_point + norm * epsilon, ref_dir, ray.time);
      reflect_col = trace(ref_ray, depth + 1);

      if (disc >= 0) {
        // Refraction is possible
        Direction r_out_para = norm * (-std::sqrt(std::max(0.0, disc)));
        Ray refract_ray(hit.intersection_point - norm * epsilon, r_out_perp + r_out_para, ray.time);
        refract_col = trace(refract_ray, depth + 1);

        Color combined = reflect_col * fresnel + refract_col * (1.0 - fresnel);

        // Simple mixing with local color if needed, or just replace
        if (hit.material.transparency >= 0.99) {
          return combined;
        }
        color = color * (1.0 - hit.material.transparency) + combined * hit.material.transparency;
      } else {
        // TIR
        color = color * (1.0 - hit.material.transparency) + reflect_col * hit.material.transparency;
      }
    }

    return color + hit.material.emission_color * hit.material.emission_strength;
  }

  if (g_config.log_level == "debug") {  // Original debug check was `if (debug)`
    Logger::instance().Debug().Msg("Missed object");
  }
  return scene.settings.background_color * scene.settings.background_strength;
}

Image Raytracer::render(const Camera& camera, int width, int height, int samples_per_pixel) {
  Image output(height, width, 255, "P3");
  g_intersection_tests = 0;

  // Set optimal thread count if not already set
  // FIX: Removed arbitrary cap - let hardware use all cores or respect
  // OMP_NUM_THREADS
  if (getenv("OMP_NUM_THREADS") == nullptr) {
    int optimal_threads = std::thread::hardware_concurrency();
    omp_set_num_threads(optimal_threads);
    Logger::instance().Info().Int("threads", optimal_threads).Msg("Auto-configured OpenMP threads");
  } else {
    Logger::instance().Info().Int("threads", omp_get_max_threads()).Msg("Using OMP_NUM_THREADS");
  }

  Logger::instance()
      .Info()
      .Int("width", width)
      .Int("height", height)
      .Int("samples", samples_per_pixel)
      .Msg("Rendering started");

  // Memory profiling: get initial memory usage
  auto get_memory_mb = []() -> double {
#ifdef __APPLE__
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss / 1024.0 / 1024.0;  // Convert to MB
#else
    std::ifstream stat_stream("/proc/self/stat", std::ios_base::in);
    std::string dummy;
    long rss = 0;
    for (int i = 0; i < 23; i++) stat_stream >> dummy;
    stat_stream >> rss;
    return rss * 4096.0 / 1024.0 / 1024.0;  // Convert pages to MB
#endif
  };

  double initial_memory = get_memory_mb();
  Logger::instance().Info().Double("mb", initial_memory).Msg("Initial memory");

  auto start_time = std::chrono::high_resolution_clock::now();

  tqdm::ProgressBar bar(height);

  // Open CSV file for row timing
  std::ofstream csv_file("Output/render_row_times.csv");
  if (csv_file.is_open()) {
    csv_file << "Row,Time(ms)\n";
  }

// Changed from schedule(dynamic) to schedule(guided) for better load balancing
// with less overhead. Guided starts with large chunks and reduces over time.
#pragma omp parallel for schedule(guided, 1)
  for (int y = 0; y < height; y++) {
    auto row_start = std::chrono::high_resolution_clock::now();

    for (int x = 0; x < width; x++) {
      Color pixel_color(0, 0, 0);

      for (int s = 0; s < samples_per_pixel; s++) {
        double u = x + random_double();
        double v = y + random_double();

        // Sample random time for motion blur [0, 1]
        double time = random_double();

        // Use Camera class method!
        Ray ray = camera.get_ray(u, v, width, height, time);
        pixel_color += trace(ray, 0);
      }
      pixel_color /= samples_per_pixel;

      // Apply tone mapping based on config
      if (g_config.tone_mapping_mode == "reinhard") {
        // Reinhard tone mapping
        pixel_color.x = pixel_color.x / (1.0 + pixel_color.x);
        pixel_color.y = pixel_color.y / (1.0 + pixel_color.y);
        pixel_color.z = pixel_color.z / (1.0 + pixel_color.z);
      } else if (g_config.tone_mapping_mode == "exposure") {
        // Simple exposure tone mapping
        pixel_color *= g_config.exposure;
        // Clamp to [0, 1]
        pixel_color.x = std::min(1.0, pixel_color.x);
        pixel_color.y = std::min(1.0, pixel_color.y);
        pixel_color.z = std::min(1.0, pixel_color.z);
      }
      // else: "none" - no tone mapping

      // Gamma correction (converts linear to sRGB)
      if (g_config.enable_gamma_correction) {
        const double gamma_inv = 1.0 / g_config.gamma;
        pixel_color.x = std::pow(pixel_color.x, gamma_inv);
        pixel_color.y = std::pow(pixel_color.y, gamma_inv);
        pixel_color.z = std::pow(pixel_color.z, gamma_inv);
      }

      output.pixels[y][x] = pixel_color;
    }

    auto row_end = std::chrono::high_resolution_clock::now();
    auto row_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(row_end - row_start).count();

// FIX: Protect progress bar update and CSV write with critical section
#pragma omp critical(progress_bar)
    {
      if (csv_file.is_open()) {
        csv_file << y << "," << row_duration << "\n";
        csv_file.flush();
      }
      bar.update();
    }
  }
  bar.finish();
  if (csv_file.is_open()) {
    csv_file.close();
  }

  // Post-render profiling
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  double final_memory = get_memory_mb();
  double memory_delta = final_memory - initial_memory;

  Logger::instance().Info().Msg("=== Render Statistics ===");
  Logger::instance().Info().Double("seconds", duration.count() / 1000.0).Msg("Render time");
  Logger::instance().Info().Double("mb", final_memory).Msg("Final memory");
  Logger::instance().Info().Double("mb_delta", memory_delta).Msg("Memory delta");
  Logger::instance()
      .Info()
      .Double("ms_per_row", duration.count() / (double)height)
      .Msg("Average time per row");

  return output;
}

// Wrapper for Main.cpp compatibility
// Main.cpp sets g_config before calling this, so we just use the parameters
// passed
Image render_scene_bvh_antialiased(const Scene& scene, const Camera& camera, int width, int height,
                                   int samples_per_pixel, int shadow_samples) {
  // Note: Main.cpp has already set g_config.aa_samples and
  // g_config.shadow_samples This wrapper just passes the values to render() -
  // DO NOT overwrite g_config here The trace() function uses
  // g_config.shadow_samples internally

  Raytracer rt(scene);
  return rt.render(camera, width, height, samples_per_pixel);
}
