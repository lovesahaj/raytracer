#ifndef CGR_COURSEWORK_CORE_CONFIG_H
#define CGR_COURSEWORK_CORE_CONFIG_H

#include <string>

/**
 * Global configuration for the raytracer
 * Centralized place for all rendering parameters, quality settings, and
 * constants
 */
struct RenderConfig {
  // ===== RENDERING QUALITY =====

  // Antialiasing samples per pixel (1 = no AA, 4-16 = good quality)
  int aa_samples = 4;

  // Shadow samples for soft shadows (1 = hard shadows, 16-64 = soft shadows)
  int shadow_samples = 16;

  // Maximum ray recursion depth (reflection/refraction bounces)
  // Note: Glass/transparent objects typically need 8-12+ bounces
  int max_ray_depth = 12;

  // ===== TONE MAPPING =====

  // Tone mapping mode: "none", "reinhard", "exposure", "aces"
  // Set to "none" to match Blender's output (Blender applies its own color management)
  std::string tone_mapping_mode = "none";

  // Exposure value for exposure tone mapping (only used if mode = "exposure")
  double exposure = 1.0;

  // Reinhard white point (only used if mode = "reinhard")
  double reinhard_white_point = 10.0;

  // ===== GAMMA CORRECTION =====

  // Gamma value for final output (2.2 is standard for sRGB)
  double gamma = 2.2;

  // Enable gamma correction (should almost always be true)
  bool enable_gamma_correction = true;

  // ===== LIGHTING =====

  // Global light intensity multiplier
  // Blender uses different units, so we scale down
  double light_intensity_factor = 0.2;

  // Ambient lighting factor (0.0 = no ambient, 1.0 = full ambient)
  double ambient_factor = 1.0;

  // ===== RAY OFFSETTING (Prevent self-intersection) =====

  // Base epsilon for ray offsetting
  double ray_offset_epsilon = 0.001;

  // Use adaptive epsilon (scales with distance from origin)
  bool use_adaptive_epsilon = true;

  // Adaptive epsilon scale factor (multiplier per unit distance)
  double adaptive_epsilon_scale = 0.0001;

  // ===== MATERIAL PROPERTIES =====

  // Default material properties when none specified
  struct {
    double diffuse_r = 0.8;
    double diffuse_g = 0.8;
    double diffuse_b = 0.8;
    double specular = 0.5;
    double shininess = 32.0;
    double reflectivity = 0.0;
    double transparency = 0.0;
    double refractive_index = 1.0;
  } default_material;

  // ===== FRESNEL REFLECTANCE =====

  // Enable Schlick's approximation for Fresnel effect on glass
  bool enable_fresnel = true;

  // ===== TEXTURE SAMPLING =====

  // Enable texture mapping
  bool enable_textures = true;

  // Texture filtering: "nearest", "bilinear"
  std::string texture_filter = "bilinear";

  // Enable normal mapping
  bool enable_normal_maps = true;

  // Enable bump mapping
  bool enable_bump_maps = true;

  // ===== SHADOW SETTINGS =====

  // Enable shadows globally
  bool enable_shadows = true;

  // Shadow transparency (allow light through transparent objects)
  bool enable_shadow_transparency = true;

  // ===== PERFORMANCE =====

  // Enable BVH acceleration structure
  bool enable_bvh = true;

  // Enable OpenMP parallelization
  bool enable_parallel = true;

  // Number of threads (0 = auto-detect)
  int num_threads = 0;

  // Print performance statistics
  bool print_stats = true;

  // Print detailed debug information
  bool debug_mode = false;

  // Log level: "debug", "info", "warn", "error", "fatal"
  std::string log_level = "info";

  // ===== OUTPUT =====

  // Output format: "ppm", "png", "both"
  std::string output_format = "both";

  // Automatically convert PPM to PNG using ImageMagick
  bool auto_convert_to_png = true;

  // ===== ADVANCED RENDERING =====

  // Enable distributed ray tracing features
  bool enable_distributed_raytracing = true;

  // Depth of field parameters (if camera.dof_enabled)
  int dof_samples = 32;

  // Motion blur samples (if object has motion)
  int motion_blur_samples = 16;

  // Enable/disable motion blur globally
  bool enable_motion_blur = true;

  // Glossy reflection samples
  int glossy_samples = 0;

  // Aperture size (0.0 = pinhole)
  double lens_aperture = 0.0;

  // Focal distance
  double lens_focal_distance = 10.0;

  // Flag to track if DOF was explicitly set via command line
  bool dof_flag_set = false;

  // ===== SCENE & OUTPUT =====

  // Input scene file
  std::string scene_file;

  // Output file path
  std::string output_file = "output.ppm";

  // Override resolution (0 = use scene settings)
  int override_width = 0;
  int override_height = 0;

  // ===== GLASS RENDERING =====

  // Pure glass threshold (transparency >= this value skips diffuse shading)
  double pure_glass_threshold = 0.99;

  // Beer's law absorption strength for colored glass
  double glass_absorption_strength = 0.1;

  // ===== EMISSION =====

  // Emission strength threshold for pure emissive objects
  // Objects above this threshold skip Blinn-Phong shading
  double pure_emission_threshold = 4.0;
};

// Global configuration instance
extern RenderConfig g_config;

#endif  // CGR_COURSEWORK_CORE_CONFIG_H
