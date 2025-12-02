#include <fstream>
#include <iostream>
#include <sstream>

#include "../Core/Scene.h"
#include "../Utils/logger.h"
#include "SceneUtils.h"

using Utils::Logger;

bool check_stream(const std::istringstream &iss, const std::string &context) {
  if (iss.fail()) {
    Logger::instance()
        .Error()
        .Str("context", context)
        .Msg("Error parsing stream");
    return false;
  }
  return true;
}

Material parse_material(std::ifstream &file) {
  Material material;
  std::string line, keyword;

  Logger::instance().Debug().Msg("Parsing material properties");

  while (true) {
    std::streampos pos = file.tellg();
    if (!std::getline(file, line)) {
      break;
    }

    std::istringstream iss(line);
    iss >> keyword;

    if (keyword == "material_diffuse") {
      iss >> material.diffuse_color.x >> material.diffuse_color.y >>
          material.diffuse_color.z;
      check_stream(iss, "material_diffuse");
    } else if (keyword == "material_specular") {
      iss >> material.specular_color.x >> material.specular_color.y >>
          material.specular_color.z;
      check_stream(iss, "material_specular");
    } else if (keyword == "material_ambient") {
      iss >> material.ambient_color.x >> material.ambient_color.y >>
          material.ambient_color.z;
      check_stream(iss, "material_ambient");
    } else if (keyword == "material_shininess") {
      iss >> material.shininess;
    } else if (keyword == "material_glossiness") {
      iss >> material.glossiness;
    } else if (keyword == "material_reflectivity") {
      iss >> material.reflectivity;
    } else if (keyword == "material_transparency") {
      iss >> material.transparency;
    } else if (keyword == "material_refractive_index") {
      iss >> material.refractive_index;
    } else if (keyword == "material_texture") {
      std::getline(iss, material.texture_file);
      size_t start = material.texture_file.find_first_not_of(" \t");
      if (start != std::string::npos) {
        material.texture_file = material.texture_file.substr(start);
      } else {
        material.texture_file = "";
      }
      material.has_texture = true;
    } else if (keyword == "material_emission") {
      iss >> material.emission_color.x >> material.emission_color.y >>
          material.emission_color.z;
    } else if (keyword == "material_emission_strength") {
      iss >> material.emission_strength;
    } else if (keyword == "material_subsurface") {
      iss >> material.subsurface;
    } else if (keyword == "material_sheen") {
      iss >> material.sheen;
    } else if (keyword == "material_clearcoat") {
      iss >> material.clearcoat;
    } else if (keyword == "material_clearcoat_roughness") {
      iss >> material.clearcoat_roughness;
    } else if (keyword == "material_normal_map") {
      std::getline(iss, material.normal_map);
      size_t start = material.normal_map.find_first_not_of(" \t");
      if (start != std::string::npos) {
        material.normal_map = material.normal_map.substr(start);
      } else {
        material.normal_map = "";
      }
    } else if (keyword == "material_bump_map") {
      std::getline(iss, material.bump_map);
      size_t start = material.bump_map.find_first_not_of(" \t");
      if (start != std::string::npos) {
        material.bump_map = material.bump_map.substr(start);
      } else {
        material.bump_map = "";
      }
    } else if (keyword == "material_bump_strength") {
      iss >> material.bump_strength;
    } else {
      // Not a material property, rewind
      file.seekg(pos);
      break;
    }
  }

  return material;
}

Scene load_scene(const std::string &filepath) {
  Scene scene;
  std::ifstream file(filepath);

  if (!file.is_open()) {
    Logger::instance()
        .Error()
        .Str("file", filepath)
        .Msg("Failed to open scene file");
    return scene;
  }

  Logger::instance().Info().Str("path", filepath).Msg("Loading scene...");

  std::string line, keyword;

  while (std::getline(file, line)) {
    std::istringstream iss(line);
    iss >> keyword;

    if (keyword == "SCENE_SETTINGS") {
      // Parse scene-level settings
      while (std::getline(file, line)) {
        std::istringstream settings_iss(line);
        std::string setting_key;
        settings_iss >> setting_key;

        if (setting_key == "background_color") {
          settings_iss >> scene.settings.background_color.x >>
              scene.settings.background_color.y >>
              scene.settings.background_color.z;
        } else if (setting_key == "background_strength") {
          settings_iss >> scene.settings.background_strength;
        } else if (setting_key == "ambient_light") {
          settings_iss >> scene.settings.ambient_light.x >>
              scene.settings.ambient_light.y >> scene.settings.ambient_light.z;
        } else if (setting_key == "frame_current") {
          settings_iss >> scene.settings.frame_current;
        } else if (setting_key == "frame_start") {
          settings_iss >> scene.settings.frame_start;
        } else if (setting_key == "frame_end") {
          settings_iss >> scene.settings.frame_end;
        } else if (setting_key == "fps") {
          settings_iss >> scene.settings.fps;
        } else if (setting_key == "max_bounces") {
          settings_iss >> scene.settings.max_bounces;
        } else if (setting_key == "diffuse_bounces") {
          settings_iss >> scene.settings.diffuse_bounces;
        } else if (setting_key == "glossy_bounces") {
          settings_iss >> scene.settings.glossy_bounces;
        } else if (setting_key == "transmission_bounces") {
          settings_iss >> scene.settings.transmission_bounces;
        } else if (setting_key == "CAMERAS" || setting_key == "LIGHTS" ||
                   setting_key == "SPHERES" || setting_key == "CUBES" ||
                   setting_key == "PLANES" || setting_key == "TORUSES" ||
                   setting_key == "CYLINDERS" || setting_key == "CONES") {
          // Hit the next section, put the line back
          file.seekg(-(line.length() + 1), std::ios_base::cur);
          break;
        }
      }
    } else if (keyword == "CAMERAS") {
      int count;
      iss >> count;
      scene.cameras.reserve(count);

      Logger::instance().Info().Int("count", count).Msg("Loading cameras");

      for (int i = 0; i < count; ++i) {
        Camera cam;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword;
        std::getline(iss, cam.name);
        size_t start = cam.name.find_first_not_of(" \t");
        if (start != std::string::npos) {
          cam.name = cam.name.substr(start);
        } else {
          cam.name = "";
        }

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cam.location.x >> cam.location.y >> cam.location.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cam.gaze_direction.x >> cam.gaze_direction.y >>
            cam.gaze_direction.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cam.up_direction.x >> cam.up_direction.y >>
            cam.up_direction.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cam.focal_length;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cam.sensor_width >> cam.sensor_height;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cam.film_resolution_x >> cam.film_resolution_y;

        // Parse optional depth of field and camera type settings
        while (true) {
          std::streampos pos = file.tellg();
          if (!std::getline(file, line)) {
            break;
          }

          iss = std::istringstream(line);
          iss >> keyword;

          if (keyword == "dof_enabled") {
            int enabled;
            iss >> enabled;
            cam.dof_enabled = (enabled != 0);
          } else if (keyword == "focus_distance") {
            iss >> cam.focus_distance;
          } else if (keyword == "aperture_fstop") {
            iss >> cam.aperture_fstop;
          } else if (keyword == "aperture_blades") {
            iss >> cam.aperture_blades;
          } else if (keyword == "camera_type") {
            std::getline(iss, cam.camera_type);
            cam.camera_type = cam.camera_type.substr(1); // Remove leading space
          } else if (keyword == "clip_start") {
            iss >> cam.clip_start;
          } else if (keyword == "clip_end") {
            iss >> cam.clip_end;
          } else {
            // Not a camera property, rewind
            file.seekg(pos);
            break;
          }
        }

        Logger::instance()
            .Debug()
            .Str("name", cam.name)
            .Double("focal_length", cam.focal_length)
            .Int("res_x", cam.film_resolution_x)
            .Int("res_y", cam.film_resolution_y)
            .Msg("Camera loaded");

        scene.cameras.push_back(cam);
      }
    } else if (keyword == "LIGHTS") {
      int count;
      iss >> count;
      scene.lights.reserve(count);

      Logger::instance().Info().Int("count", count).Msg("Loading lights");

      for (int i = 0; i < count; ++i) {
        Light light;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword;
        std::getline(iss, light.name);
        size_t start = light.name.find_first_not_of(" \t");
        if (start != std::string::npos) {
          light.name = light.name.substr(start);
        } else {
          light.name = "";
        }

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> light.location.x >> light.location.y >>
            light.location.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> light.intensity;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> light.color.x >> light.color.y >> light.color.z;

        // Parse optional light type and properties
        while (true) {
          std::streampos pos = file.tellg();
          if (!std::getline(file, line)) {
            break;
          }

          iss = std::istringstream(line);
          iss >> keyword;

          if (keyword == "light_type") {
            std::getline(iss, light.light_type);
            light.light_type =
                light.light_type.substr(1); // Remove leading space
          } else if (keyword == "spot_size") {
            iss >> light.spot_size;
          } else if (keyword == "spot_blend") {
            iss >> light.spot_blend;
          } else if (keyword == "area_shape") {
            std::getline(iss, light.area_shape);
            light.area_shape =
                light.area_shape.substr(1); // Remove leading space
          } else if (keyword == "area_size") {
            iss >> light.area_size_x >> light.area_size_y;
          } else if (keyword == "direction") {
            iss >> light.direction.x >> light.direction.y >> light.direction.z;
          } else if (keyword == "angle") {
            iss >> light.angle;
          } else if (keyword == "cast_shadows") {
            int shadows;
            iss >> shadows;
            light.cast_shadows = (shadows != 0);
          } else if (keyword == "shadow_soft_size") {
            iss >> light.shadow_soft_size;
          } else if (keyword == "samples") {
            iss >> light.samples;
          } else if (keyword == "normal") {
            iss >> light.normal.x >> light.normal.y >> light.normal.z;
          } else {
            // Not a light property, rewind
            file.seekg(pos);
            break;
          }
        }

        scene.lights.push_back(light);
      }
    } else if (keyword == "SPHERES") {
      int count;
      iss >> count;
      scene.spheres.reserve(count);

      for (int i = 0; i < count; ++i) {
        Sphere sphere;
        sphere.type = "sphere";

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword;
        std::getline(iss, sphere.name);
        size_t start = sphere.name.find_first_not_of(" \t");
        if (start != std::string::npos) {
          sphere.name = sphere.name.substr(start);
        } else {
          sphere.name = "";
        }

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> sphere.location.x >> sphere.location.y >>
            sphere.location.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> sphere.rotation.x >> sphere.rotation.y >>
            sphere.rotation.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> sphere.scale.x >> sphere.scale.y >> sphere.scale.z;

        // Check for visibility flag
        // Check for optional properties (visibility, motion blur)
        while (true) {
          std::streampos pos = file.tellg();
          if (!std::getline(file, line))
            break;

          iss = std::istringstream(line);
          iss >> keyword;
          if (keyword == "visible") {
            int vis;
            iss >> vis;
            sphere.visible = (vis != 0);
          } else if (keyword == "motion_blur") {
            int mb;
            iss >> mb;
            sphere.has_motion = (mb != 0);
          } else if (keyword == "matrix_t0") {
            // Read 4x4 matrix (row-major)
            for (int row = 0; row < 4; row++) {
              if (!std::getline(file, line))
                throw std::runtime_error("Failed to read matrix_t0 row");
              std::istringstream row_iss(line);
              row_iss >> sphere.start_transform.m[row][0] >>
                  sphere.start_transform.m[row][1] >>
                  sphere.start_transform.m[row][2] >>
                  sphere.start_transform.m[row][3];
            }
          } else if (keyword == "matrix_t1") {
            // Read 4x4 matrix (row-major)
            for (int row = 0; row < 4; row++) {
              if (!std::getline(file, line))
                throw std::runtime_error("Failed to read matrix_t1 row");
              std::istringstream row_iss(line);
              row_iss >> sphere.end_transform.m[row][0] >>
                  sphere.end_transform.m[row][1] >>
                  sphere.end_transform.m[row][2] >>
                  sphere.end_transform.m[row][3];
            }
          } else {
            // Not an optional property, rewind
            file.seekg(pos);
            break;
          }
        }

        // Parse material properties
        sphere.material = parse_material(file);

        scene.spheres.push_back(sphere);
      }
    } else if (keyword == "CUBES") {
      int count;
      iss >> count;
      scene.cubes.reserve(count);

      for (int i = 0; i < count; ++i) {
        Cube cube;
        cube.type = "cube";

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword;
        std::getline(iss, cube.name);
        size_t start = cube.name.find_first_not_of(" \t");
        if (start != std::string::npos) {
          cube.name = cube.name.substr(start);
        } else {
          cube.name = "";
        }

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cube.translation.x >> cube.translation.y >>
            cube.translation.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cube.rotation.x >> cube.rotation.y >> cube.rotation.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cube.scale.x >> cube.scale.y >> cube.scale.z;

        // Check for optional properties
        while (true) {
          std::streampos pos = file.tellg();
          if (!std::getline(file, line))
            break;

          iss = std::istringstream(line);
          iss >> keyword;
          if (keyword == "visible") {
            int vis;
            iss >> vis;
            cube.visible = (vis != 0);
          } else if (keyword == "motion_blur") {
            int mb;
            iss >> mb;
            cube.has_motion = (mb != 0);
          } else if (keyword == "matrix_t0") {
            // Read 4x4 matrix (row-major)
            for (int row = 0; row < 4; row++) {
              if (!std::getline(file, line))
                throw std::runtime_error("Failed to read matrix_t0 row");
              std::istringstream row_iss(line);
              row_iss >> cube.start_transform.m[row][0] >>
                  cube.start_transform.m[row][1] >>
                  cube.start_transform.m[row][2] >>
                  cube.start_transform.m[row][3];
            }
          } else if (keyword == "matrix_t1") {
            // Read 4x4 matrix (row-major)
            for (int row = 0; row < 4; row++) {
              if (!std::getline(file, line))
                throw std::runtime_error("Failed to read matrix_t1 row");
              std::istringstream row_iss(line);
              row_iss >> cube.end_transform.m[row][0] >>
                  cube.end_transform.m[row][1] >>
                  cube.end_transform.m[row][2] >> cube.end_transform.m[row][3];
            }
          } else {
            file.seekg(pos);
            break;
          }
        }

        // Parse material properties
        cube.material = parse_material(file);

        scene.cubes.push_back(cube);
      }
    } else if (keyword == "PLANES") {
      int count;
      iss >> count;
      scene.planes.reserve(count);

      for (int i = 0; i < count; ++i) {
        Plane plane;
        plane.type = "plane";

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword;
        std::getline(iss, plane.name);
        size_t start = plane.name.find_first_not_of(" \t");
        if (start != std::string::npos) {
          plane.name = plane.name.substr(start);
        } else {
          plane.name = "";
        }

        std::getline(file, line);
        iss = std::istringstream(line);
        int point_count;
        iss >> keyword >> point_count;

        plane.points.reserve(point_count);
        for (int j = 0; j < point_count; ++j) {
          Point pt;
          std::getline(file, line);
          iss = std::istringstream(line);
          iss >> pt.x >> pt.y >> pt.z;
          plane.points.push_back(pt);
        }

        // Check for optional properties
        while (true) {
          std::streampos pos = file.tellg();
          if (!std::getline(file, line))
            break;

          iss = std::istringstream(line);
          iss >> keyword;
          if (keyword == "visible") {
            int vis;
            iss >> vis;
            plane.visible = (vis != 0);
          } else if (keyword == "motion_blur") {
            int mb;
            iss >> mb;
            plane.has_motion = (mb != 0);
          } else if (keyword == "matrix_t0") {
            // Read 4x4 matrix (row-major)
            for (int row = 0; row < 4; row++) {
              if (!std::getline(file, line))
                throw std::runtime_error("Failed to read matrix_t0 row");
              std::istringstream row_iss(line);
              row_iss >> plane.start_transform.m[row][0] >>
                  plane.start_transform.m[row][1] >>
                  plane.start_transform.m[row][2] >>
                  plane.start_transform.m[row][3];
            }
          } else if (keyword == "matrix_t1") {
            // Read 4x4 matrix (row-major)
            for (int row = 0; row < 4; row++) {
              if (!std::getline(file, line))
                throw std::runtime_error("Failed to read matrix_t1 row");
              std::istringstream row_iss(line);
              row_iss >> plane.end_transform.m[row][0] >>
                  plane.end_transform.m[row][1] >>
                  plane.end_transform.m[row][2] >>
                  plane.end_transform.m[row][3];
            }
          } else {
            file.seekg(pos);
            break;
          }
        }

        // Parse material properties
        plane.material = parse_material(file);

        scene.planes.push_back(plane);
      }
    } else if (keyword == "TORUSES") {
      int count;
      iss >> count;
      scene.toruses.reserve(count);

      for (int i = 0; i < count; ++i) {
        Torus torus;
        torus.type = "torus";

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword;
        std::getline(iss, torus.name);
        size_t start = torus.name.find_first_not_of(" \t");
        if (start != std::string::npos) {
          torus.name = torus.name.substr(start);
        } else {
          torus.name = "";
        }

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> torus.location.x >> torus.location.y >>
            torus.location.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> torus.rotation.x >> torus.rotation.y >>
            torus.rotation.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> torus.scale.x >> torus.scale.y >> torus.scale.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> torus.major_radius;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> torus.minor_radius;

        // Check for optional properties
        while (true) {
          std::streampos pos = file.tellg();
          if (!std::getline(file, line))
            break;

          iss = std::istringstream(line);
          iss >> keyword;
          if (keyword == "visible") {
            int vis;
            iss >> vis;
            torus.visible = (vis != 0);
          } else if (keyword == "motion_blur") {
            int mb;
            iss >> mb;
            torus.has_motion = (mb != 0);
          } else if (keyword == "matrix_t0") {
            // Read 4x4 matrix (row-major)
            for (int row = 0; row < 4; row++) {
              if (!std::getline(file, line))
                throw std::runtime_error("Failed to read matrix_t0 row");
              std::istringstream row_iss(line);
              row_iss >> torus.start_transform.m[row][0] >>
                  torus.start_transform.m[row][1] >>
                  torus.start_transform.m[row][2] >>
                  torus.start_transform.m[row][3];
            }
          } else if (keyword == "matrix_t1") {
            // Read 4x4 matrix (row-major)
            for (int row = 0; row < 4; row++) {
              if (!std::getline(file, line))
                throw std::runtime_error("Failed to read matrix_t1 row");
              std::istringstream row_iss(line);
              row_iss >> torus.end_transform.m[row][0] >>
                  torus.end_transform.m[row][1] >>
                  torus.end_transform.m[row][2] >>
                  torus.end_transform.m[row][3];
            }
          } else {
            file.seekg(pos);
            break;
          }
        }

        // Parse material properties
        torus.material = parse_material(file);

        scene.toruses.push_back(torus);
      }
    } else if (keyword == "CYLINDERS") {
      int count;
      iss >> count;
      scene.cylinders.reserve(count);

      for (int i = 0; i < count; ++i) {
        Cylinder cylinder;
        cylinder.type = "cylinder";

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword;
        std::getline(iss, cylinder.name);
        size_t start = cylinder.name.find_first_not_of(" \t");
        if (start != std::string::npos) {
          cylinder.name = cylinder.name.substr(start);
        } else {
          cylinder.name = "";
        }

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cylinder.location.x >> cylinder.location.y >>
            cylinder.location.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cylinder.rotation.x >> cylinder.rotation.y >>
            cylinder.rotation.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cylinder.scale.x >> cylinder.scale.y >>
            cylinder.scale.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cylinder.radius;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cylinder.depth;

        // Check for optional properties
        while (true) {
          std::streampos pos = file.tellg();
          if (!std::getline(file, line))
            break;

          iss = std::istringstream(line);
          iss >> keyword;
          if (keyword == "visible") {
            int vis;
            iss >> vis;
            cylinder.visible = (vis != 0);
          } else if (keyword == "motion_blur") {
            int mb;
            iss >> mb;
            cylinder.has_motion = (mb != 0);
          } else if (keyword == "matrix_t0") {
            // Read 4x4 matrix (row-major)
            for (int row = 0; row < 4; row++) {
              if (!std::getline(file, line))
                throw std::runtime_error("Failed to read matrix_t0 row");
              std::istringstream row_iss(line);
              row_iss >> cylinder.start_transform.m[row][0] >>
                  cylinder.start_transform.m[row][1] >>
                  cylinder.start_transform.m[row][2] >>
                  cylinder.start_transform.m[row][3];
            }
          } else if (keyword == "matrix_t1") {
            // Read 4x4 matrix (row-major)
            for (int row = 0; row < 4; row++) {
              if (!std::getline(file, line))
                throw std::runtime_error("Failed to read matrix_t1 row");
              std::istringstream row_iss(line);
              row_iss >> cylinder.end_transform.m[row][0] >>
                  cylinder.end_transform.m[row][1] >>
                  cylinder.end_transform.m[row][2] >>
                  cylinder.end_transform.m[row][3];
            }
          } else {
            file.seekg(pos);
            break;
          }
        }

        // Parse material properties
        cylinder.material = parse_material(file);

        scene.cylinders.push_back(cylinder);
      }
    } else if (keyword == "CONES") {
      int count;
      iss >> count;
      scene.cones.reserve(count);

      for (int i = 0; i < count; ++i) {
        Cone cone;
        cone.type = "cone";

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword;
        std::getline(iss, cone.name);
        size_t start = cone.name.find_first_not_of(" \t");
        if (start != std::string::npos) {
          cone.name = cone.name.substr(start);
        } else {
          cone.name = "";
        }

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cone.location.x >> cone.location.y >> cone.location.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cone.rotation.x >> cone.rotation.y >> cone.rotation.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cone.scale.x >> cone.scale.y >> cone.scale.z;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cone.radius;

        std::getline(file, line);
        iss = std::istringstream(line);
        iss >> keyword >> cone.depth;

        // Check for optional properties
        while (true) {
          std::streampos pos = file.tellg();
          if (!std::getline(file, line))
            break;

          iss = std::istringstream(line);
          iss >> keyword;
          if (keyword == "visible") {
            int vis;
            iss >> vis;
            cone.visible = (vis != 0);
          } else if (keyword == "motion_blur") {
            int mb;
            iss >> mb;
            cone.has_motion = (mb != 0);
          } else if (keyword == "matrix_t0") {
            // Read 4x4 matrix (row-major)
            for (int row = 0; row < 4; row++) {
              if (!std::getline(file, line))
                throw std::runtime_error("Failed to read matrix_t0 row");
              std::istringstream row_iss(line);
              row_iss >> cone.start_transform.m[row][0] >>
                  cone.start_transform.m[row][1] >>
                  cone.start_transform.m[row][2] >>
                  cone.start_transform.m[row][3];
            }
          } else if (keyword == "matrix_t1") {
            // Read 4x4 matrix (row-major)
            for (int row = 0; row < 4; row++) {
              if (!std::getline(file, line))
                throw std::runtime_error("Failed to read matrix_t1 row");
              std::istringstream row_iss(line);
              row_iss >> cone.end_transform.m[row][0] >>
                  cone.end_transform.m[row][1] >>
                  cone.end_transform.m[row][2] >> cone.end_transform.m[row][3];
            }
          } else {
            file.seekg(pos);
            break;
          }
        }

        // Parse material properties
        cone.material = parse_material(file);

        scene.cones.push_back(cone);
      }
    }
  }

  // Precompute transforms for all shapes to avoid recalculation in hot path
  Logger::instance().Info().Msg("Precomputing transforms...");
  for (auto &sphere : scene.spheres) {
    sphere.cached_transform = Transform::from_trs_nonuniform(
        sphere.location, sphere.rotation, sphere.scale);
  }
  for (auto &cube : scene.cubes) {
    cube.cached_transform = Transform::from_trs_nonuniform(
        cube.translation, cube.rotation, cube.scale);
  }
  for (auto &torus : scene.toruses) {
    torus.cached_transform = Transform::from_trs_nonuniform(
        torus.location, torus.rotation, torus.scale);
  }
  for (auto &cylinder : scene.cylinders) {
    cylinder.cached_transform = Transform::from_trs_nonuniform(
        cylinder.location, cylinder.rotation, cylinder.scale);
  }
  for (auto &cone : scene.cones) {
    cone.cached_transform = Transform::from_trs_nonuniform(
        cone.location, cone.rotation, cone.scale);
  }

  Logger::instance()
      .Info()
      .Int("cameras", scene.cameras.size())
      .Int("lights", scene.lights.size())
      .Int("spheres", scene.spheres.size())
      .Int("cubes", scene.cubes.size())
      .Int("planes", scene.planes.size())
      .Int("toruses", scene.toruses.size())
      .Int("cylinders", scene.cylinders.size())
      .Int("cones", scene.cones.size())
      .Msg("Scene loaded successfully");

  return scene;
}