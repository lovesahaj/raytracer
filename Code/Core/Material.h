#ifndef CGR_COURSEWORK_CORE_MATERIAL_H
#define CGR_COURSEWORK_CORE_MATERIAL_H

#include "../Math/Vector.h"
#include <string>

struct Material {
  Color diffuse_color{0.8, 0.8, 0.8};  // Base color (kd)
  Color specular_color{1.0, 1.0, 1.0}; // Specular highlight color (ks)
  Color ambient_color{0.1, 0.1, 0.1};  // Ambient color (ka)
  double shininess{32.0};              // Phong exponent
  double glossiness{0.0};              // 0.0 = mirror, 1.0 = diffuse
  double reflectivity{0.0};            // Mirror reflection coefficient (kr)
  double transparency{0.0};            // Transmission coefficient (kt)
  double refractive_index{1.0};        // Index of refraction (eta)
  std::string texture_file;            // Path to texture file (optional)
  bool has_texture{false};             // Whether this material uses a texture

  // Emission (for glowing objects)
  Color emission_color{0.0, 0.0, 0.0};
  double emission_strength{0.0};

  // Advanced PBR properties
  double subsurface{0.0}; // Subsurface scattering
  double sheen{0.0};      // Fabric-like reflection
  double clearcoat{0.0};  // Clear coat layer
  double clearcoat_roughness{0.0};

  // Additional texture maps
  std::string normal_map;
  std::string bump_map;
  double bump_strength{1.0};

  // Evaluate BRDF (Blinn-Phong)
  Color eval(const Direction& view_dir, const Direction& light_dir, const Direction& normal, const Color& albedo) const {
      // Diffuse using provided albedo (which might be texture)
      Color brdf = albedo;

      // Specular (Blinn-Phong)
      Direction halfway = (light_dir + view_dir).norm();
      double n_dot_h = std::max(0.0, normal.dot(halfway));
      if (n_dot_h > 0.0) {
          double spec = std::pow(n_dot_h, shininess);
          brdf = brdf + specular_color * spec;
      }
      
      return brdf;
  }
};

#endif // CGR_COURSEWORK_CORE_MATERIAL_H
