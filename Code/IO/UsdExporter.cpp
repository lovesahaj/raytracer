#include "UsdExporter.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "../Utils/logger.h"

using Utils::Logger;

// Helper to convert radians to degrees
static double to_degrees(double radians) { return radians * 180.0 / M_PI; }

// Helper to sanitize USD prim names
// Replaces invalid characters (like '.') with underscores
static std::string sanitize_usd_name(const std::string &name) {
  std::string result = name;
  for (char &c : result) {
    if (!isalnum(c) && c != '_') {
      c = '_';
    }
  }
  // Ensure it doesn't start with a number
  if (!result.empty() && isdigit(result[0])) {
    result = "_" + result;
  }
  return result;
}

// Helper to write color
static void write_color(std::ofstream &out, const std::string &name,
                        const Color &c) {
  out << "        color3f " << name << " = (" << c.x << ", " << c.y << ", "
      << c.z << ")\n";
}

// Helper to write material
static void write_material(std::ofstream &out, const std::string &mat_name,
                           const Material &material) {
  std::string sanitized_name = sanitize_usd_name(mat_name);
  out << "    def Material \"" << sanitized_name << "\"\n";
  out << "    {\n";
  out << "        token outputs:surface.connect = <" << sanitized_name
      << "/PBRShader.outputs:surface>\n";
  out << "        def Shader \"PBRShader\"\n";
  out << "        {\n";
  out << "            uniform token info:id = \"UsdPreviewSurface\"\n";
  write_color(out, "inputs:diffuseColor", material.diffuse_color);
  write_color(out, "inputs:emissiveColor", material.emission_color);
  out << "            float inputs:roughness = "
      << (1.0 - material.shininess / 1000.0) << "\n"; // Approximate
  out << "            float inputs:metallic = " << material.reflectivity
      << "\n";
  out << "            float inputs:opacity = " << (1.0 - material.transparency)
      << "\n";
  out << "            float inputs:ior = " << material.refractive_index << "\n";
  out << "            token outputs:surface\n";
  out << "        }\n";
  out << "    }\n";
}

void export_scene_to_usd(const Scene &scene, const std::string &filename) {
  std::ofstream out(filename);
  if (!out) {
    Logger::instance().Error().Str("file", filename).Msg("Failed to open file for writing");
    return;
  }

  // Header
  out << "#usda 1.0\n";
  out << "(\n";
  out << "    defaultPrim = \"Scene\"\n";
  out << "    upAxis = \"Z\"\n";
  out << "    metersPerUnit = 1.0\n";
  out << ")\n\n";

  out << "def Xform \"Scene\"\n";
  out << "{\n";

  // --- Materials ---
  // We will create unique materials for each object or share them.
  // For simplicity, let's embed materials or create a "Materials" scope.
  out << "    def Scope \"Materials\"\n";
  out << "    {\n";

  // Spheres
  for (const auto &sphere : scene.spheres) {
    write_material(out, "Mat_" + sphere.name, sphere.material);
  }
  // Cubes
  for (const auto &cube : scene.cubes) {
    write_material(out, "Mat_" + cube.name, cube.material);
  }
  // Planes
  for (const auto &plane : scene.planes) {
    write_material(out, "Mat_" + plane.name, plane.material);
  }
  out << "    }\n\n";

  // --- Cameras ---
  for (const auto &cam : scene.cameras) {
    out << "    def Camera \"" << sanitize_usd_name(cam.name) << "\"\n";
    out << "    {\n";
    out << "        double3 xformOp:translate = (" << cam.location.x << ", "
        << cam.location.y << ", " << cam.location.z << ")\n";

    // Camera orientation in USD is -Z forward, +Y up.
    // Our camera has 'gaze' and 'up'. We need to compute the rotation matrix
    // and extract Euler angles or simply use LookAt if USD supported it
    // directly as a transform (it doesn't easily). For now, we'll skip complex
    // rotation export for cameras to keep it simple, or just export the
    // translation. Ideally, we would compute the basis vectors and convert to
    // quaternion or euler.

    out << "        float2 clippingRange = (" << cam.clip_start << ", "
        << cam.clip_end << ")\n";
    out << "        float focalLength = " << cam.focal_length << "\n";
    out << "        float horizontalAperture = " << cam.sensor_width << "\n";
    out << "        float verticalAperture = " << cam.sensor_height << "\n";
    out << "        token[] xformOpOrder = [\"xformOp:translate\"]\n";
    out << "    }\n";
  }

  // --- Lights ---
  for (const auto &light : scene.lights) {
    out << "    def SphereLight \"" << sanitize_usd_name(light.name) << "\"\n";
    out << "    {\n";
    out << "        double3 xformOp:translate = (" << light.location.x << ", "
        << light.location.y << ", " << light.location.z << ")\n";
    out << "        float intensity = " << light.intensity << "\n";
    write_color(out, "inputs:color", light.color);
    out << "        float radius = 0.1\n"; // Arbitrary visual size
    out << "        token[] xformOpOrder = [\"xformOp:translate\"]\n";
    out << "    }\n";
  }

  // --- Spheres ---
  for (const auto &sphere : scene.spheres) {
    if (!sphere.visible)
      continue;
    std::string name = sanitize_usd_name(sphere.name);
    out << "    def Sphere \"" << name << "\"\n";
    out << "    {\n";
    out << "        double3 xformOp:translate = (" << sphere.location.x << ", "
        << sphere.location.y << ", " << sphere.location.z << ")\n";
    out << "        float3 xformOp:rotateXYZ = ("
        << to_degrees(sphere.rotation.x) << ", "
        << to_degrees(sphere.rotation.y) << ", "
        << to_degrees(sphere.rotation.z) << ")\n";
    out << "        float3 xformOp:scale = (" << sphere.scale.x << ", "
        << sphere.scale.y << ", " << sphere.scale.z << ")\n";
    out << "        token[] xformOpOrder = [\"xformOp:translate\", "
           "\"xformOp:rotateXYZ\", "
           "\"xformOp:scale\"]\n";
    out << "        double radius = 1.0\n";
    out << "        rel material:binding = <../Materials/Mat_" << name << ">\n";
    out << "    }\n";
  }

  // --- Cubes ---
  for (const auto &cube : scene.cubes) {
    if (!cube.visible)
      continue;
    std::string name = sanitize_usd_name(cube.name);
    out << "    def Cube \"" << name << "\"\n";
    out << "    {\n";
    out << "        double3 xformOp:translate = (" << cube.translation.x << ", "
        << cube.translation.y << ", " << cube.translation.z << ")\n";
    out << "        float3 xformOp:rotateXYZ = (" << to_degrees(cube.rotation.x)
        << ", " << to_degrees(cube.rotation.y) << ", "
        << to_degrees(cube.rotation.z) << ")\n";
    out << "        float3 xformOp:scale = (" << cube.scale.x << ", "
        << cube.scale.y << ", " << cube.scale.z << ")\n";
    out << "        token[] xformOpOrder = [\"xformOp:translate\", "
           "\"xformOp:rotateXYZ\", "
           "\"xformOp:scale\"]\n";
    out << "        double size = 1.0\n";
    out << "        rel material:binding = <../Materials/Mat_" << name << ">\n";
    out << "    }\n";
  }

  // --- Planes ---
  for (const auto &plane : scene.planes) {
    if (!plane.visible)
      continue;
    std::string name = sanitize_usd_name(plane.name);
    // Export plane as a simple Mesh (quad)
    out << "    def Mesh \"" << name << "\"\n";
    out << "    {\n";

    out << "        point3f[] points = [";
    for (size_t i = 0; i < plane.points.size(); ++i) {
      out << "(" << plane.points[i].x << ", " << plane.points[i].y << ", "
          << plane.points[i].z << ")";
      if (i < plane.points.size() - 1)
        out << ", ";
    }
    out << "]\n";

    out << "        int[] faceVertexCounts = [" << plane.points.size() << "]\n";

    out << "        int[] faceVertexIndices = [";
    for (size_t i = 0; i < plane.points.size(); ++i) {
      out << i;
      if (i < plane.points.size() - 1)
        out << ", ";
    }
    out << "]\n";

    out << "        rel material:binding = <../Materials/Mat_" << name << ">\n";
    out << "    }\n";
  }

  out << "}\n"; // End Scene
  out.close();
  Logger::instance().Info().Str("file", filename).Msg("Exported scene to USD");
}