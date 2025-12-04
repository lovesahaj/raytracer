//
// Created by Lovesahaj on 10/10/25.
//

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "../IO/Image.h"
#include "../IO/SceneUtils.h"
#include "../Render/Raytracer.h"
#include "../Utils/logger.h"
#include "Config.h"
#include "Scene.h"

using Utils::Logger;

void print_help(const char *program_name) {
  std::cout
      << "Usage: " << program_name << " [options]\n"
      << "Options:\n"
      << "  --scene <filename>       Input ASCII scene file\n"
      << "  --output <filename>      Specify output PPM file (default: "
      << g_config.output_file << ")\n"
      << "  --resolution <w> <h>     Set output image resolution (overrides "
         "scene)\n"
      << "  --samples <n>            Number of AA samples per pixel (default: "
      << g_config.aa_samples << ")\n"
      << "  --max-depth <n>          Maximum ray recursion depth (default: "
      << g_config.max_ray_depth << ")\n"
      << "  --enable-textures        Enable texture mapping (default)\n"
      << "  --disable-textures       Disable texture mapping\n"
      << "  --soft-shadows <n>       Enable soft shadows with n samples\n"
      << "  --glossy-reflection <n>  Enable glossy reflections with n samples\n"
      << "  --motion-blur <n>        Enable motion blur with n temporal samples (0 to disable)\n"
      << "  --disable-motion-blur    Disable motion blur completely\n"
      << "  --depth-of-field <a> <d> Enable DOF with aperture f-stop a and focal distance d\n"
      << "  --disable-dof            Disable depth of field\n"
      << "  --light-intensity <f>    Global light intensity multiplier (default: "
      << g_config.light_intensity_factor << ")\n"
      << "  --ambient-light <f>      Ambient light factor (default: "
      << g_config.ambient_factor << ")\n"
      << "  --threads <n>            Number of rendering threads\n"
      << "  --log-level <level>      Set log level (debug, info, warn, error)\n"
      << "  --help                   Display this help message\n";
}

void parse_arguments(int argc, char *argv[]) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "--help") {
      print_help(argv[0]);
      exit(0);
    } else if ((arg == "--scene" || arg == "-s") && i + 1 < argc) {
      g_config.scene_file = argv[++i];
    } else if ((arg == "--output" || arg == "-o") && i + 1 < argc) {
      g_config.output_file = argv[++i];
    } else if (arg == "--resolution" && i + 2 < argc) {
      g_config.override_width = std::atoi(argv[++i]);
      g_config.override_height = std::atoi(argv[++i]);
    } else if ((arg == "-w" || arg == "-W") && i + 1 < argc) {
      // Width only
      g_config.override_width = std::atoi(argv[++i]);
    } else if (arg == "-H" && i + 1 < argc) {
      // Height only (capital H to avoid conflict with -h/--help)
      g_config.override_height = std::atoi(argv[++i]);
    } else if (arg == "--samples" && i + 1 < argc) {
      g_config.aa_samples = std::atoi(argv[++i]);
      if (g_config.aa_samples < 1)
        g_config.aa_samples = 1;
    } else if (arg == "--max-depth" && i + 1 < argc) {
      g_config.max_ray_depth = std::atoi(argv[++i]);
    } else if (arg == "--enable-textures") {
      g_config.enable_textures = true;
    } else if (arg == "--disable-textures") {
      g_config.enable_textures = false;
    } else if (arg == "--soft-shadows" && i + 1 < argc) {
      g_config.shadow_samples = std::atoi(argv[++i]);
    } else if (arg == "--glossy-reflection" && i + 1 < argc) {
      g_config.glossy_samples = std::atoi(argv[++i]);
    } else if (arg == "--motion-blur" && i + 1 < argc) {
      g_config.motion_blur_samples = std::atoi(argv[++i]);
      g_config.enable_motion_blur = (g_config.motion_blur_samples > 0);
    } else if (arg == "--disable-motion-blur") {
      g_config.enable_motion_blur = false;
    } else if (arg == "--depth-of-field" && i + 2 < argc) {
      g_config.lens_aperture = std::atof(argv[++i]);
      g_config.lens_focal_distance = std::atof(argv[++i]);
      g_config.dof_flag_set = true;
    } else if (arg == "--disable-dof") {
      g_config.lens_aperture = 0.0;
      g_config.dof_flag_set = true;
    } else if (arg == "--light-intensity" && i + 1 < argc) {
      g_config.light_intensity_factor = std::atof(argv[++i]);
    } else if (arg == "--ambient-light" && i + 1 < argc) {
      g_config.ambient_factor = std::atof(argv[++i]);
    } else if (arg == "--threads" && i + 1 < argc) {
      g_config.num_threads = std::atoi(argv[++i]);
    } else if (arg == "--log-level" && i + 1 < argc) {
      g_config.log_level = argv[++i];
    } else {
      Logger::instance().Error().Str("arg", arg).Msg("Unknown argument");
      print_help(argv[0]);
      exit(1);
    }
  }
}

// Main entry point for raytracer
int main(int argc, char *argv[]) {
  if (argc <= 1) {
    print_help(argv[0]);
    return 0;
  }

  parse_arguments(argc, argv);

  if (g_config.scene_file.empty()) {
    Logger::instance().Error().Msg("No scene file specified");
    print_help(argv[0]);
    return 1;
  }

  // Set log level
  if (g_config.log_level == "debug")
    Logger::instance().SetLevel(Utils::Level::Debug);
  else if (g_config.log_level == "info")
    Logger::instance().SetLevel(Utils::Level::Info);
  else if (g_config.log_level == "warn")
    Logger::instance().SetLevel(Utils::Level::Warn);
  else if (g_config.log_level == "error")
    Logger::instance().SetLevel(Utils::Level::Error);
  else if (g_config.log_level == "fatal")
    Logger::instance().SetLevel(Utils::Level::Fatal);

  // Load scene
  std::string scene_path = g_config.scene_file;
  // Handle relative paths assuming they might be in ASCII folder if not found?
  // For now, strictly follow the provided path or maybe prepend ./ASCII/ if
  // it's just a name? The previous code did some magic, let's be more explicit
  // but fallback if needed could be nice. But the spec says "--scene <filename>
  // - Input ASCII scene file". I will assume the user provides the correct
  // path.

  // However, if it doesn't exist, we could check ./ASCII/
  // keeping it simple for now: use provided path.

  Scene scene = load_scene(scene_path);
  Logger::instance().Info().Str("path", scene_path).Msg("Loaded scene");
  Logger::instance().Info() << scene;

  if (scene.cameras.empty()) {
    Logger::instance().Error().Msg("Scene must contain at least one camera");
    return 1;
  }

  // Make a copy of the camera so we can override settings
  Camera render_camera = scene.cameras[0];

  // Determine resolution
  int width = render_camera.film_resolution_x;
  int height = render_camera.film_resolution_y;

  if (g_config.override_width > 0 && g_config.override_height > 0) {
    width = g_config.override_width;
    height = g_config.override_height;
    Logger::instance()
        .Info()
        .Int("width", width)
        .Int("height", height)
        .Msg("Overriding resolution");
  }

  // Override depth-of-field settings if flags were provided
  if (g_config.dof_flag_set) {
    if (g_config.lens_aperture > 0.0) {
      render_camera.dof_enabled = true;
      // Convert lens_aperture to f-stop
      // If user provides aperture directly, we use it as f-stop
      render_camera.aperture_fstop = g_config.lens_aperture;
      render_camera.focus_distance = g_config.lens_focal_distance;
      Logger::instance()
          .Info()
          .Double("aperture_fstop", render_camera.aperture_fstop)
          .Double("focus_distance", render_camera.focus_distance)
          .Msg("Overriding depth-of-field from command-line");
    } else {
      render_camera.dof_enabled = false;
      Logger::instance()
          .Info()
          .Msg("Disabling depth-of-field from command-line");
    }
  }

  // Print full configuration before rendering
  Logger::instance()
      .Info()
      .Int("width", width)
      .Int("height", height)
      .Int("aa_samples", g_config.aa_samples)
      .Int("shadow_samples", g_config.shadow_samples)
      .Int("glossy_samples", g_config.glossy_samples)
      .Int("motion_blur_samples", g_config.motion_blur_samples)
      .Int("dof_samples", g_config.dof_samples)
      .Double("lens_aperture", g_config.lens_aperture)
      .Double("lens_focal_distance", g_config.lens_focal_distance)
      .Int("max_ray_depth", g_config.max_ray_depth)
      .Bool("textures", g_config.enable_textures)
      .Str("threads",
           (g_config.num_threads == 0 ? "Auto"
                                      : std::to_string(g_config.num_threads)))
      .Msg("Render Configuration");

  // Render using global config (wrapper syncs to g_config)
  Image output = render_scene_bvh_antialiased(scene, render_camera, width,
                                              height, g_config.aa_samples,
                                              g_config.shadow_samples);

  output.write(g_config.output_file);
  Logger::instance()
      .Info()
      .Str("path", g_config.output_file)
      .Msg("Image saved");

  return 0;
}
