#include "Texture.h"

#include <cmath>
#include <fstream>

#include "../Utils/logger.h"

using Utils::Logger;

// Load a texture from file
bool TextureManager::load_texture(const std::string& filename) {
  // Check if already loaded
  // Check if already loaded (Reader lock)
  {
    std::shared_lock<std::shared_mutex> lock(mutex);
    if (textures.find(filename) != textures.end()) {
      return true;
    }
  }

  // Writer lock for loading
  std::unique_lock<std::shared_mutex> lock(mutex);

  // Double-check locking pattern
  if (textures.find(filename) != textures.end()) {
    return true;
  }

  try {
    // Replace .jpg extension with .ppm since our Image class only supports PPM
    std::string ppm_filename = filename;
    size_t jpg_pos = ppm_filename.find(".jpg");
    if (jpg_pos != std::string::npos) {
      ppm_filename.replace(jpg_pos, 4, ".ppm");
    }

    // Try loading from Textures directory relative to current directory
    std::string texture_path = "Textures/" + ppm_filename;

    // Check if file exists
    std::ifstream f(texture_path.c_str());
    if (!f.good()) {
      // Try parent directory (in case we are in build dir)
      texture_path = "../Textures/" + ppm_filename;
    }

    auto texture = std::make_shared<Image>(texture_path);

    // Check if image was successfully loaded (width and height > 0)
    if (texture->width == 0 || texture->height == 0) {
      Logger::instance()
          .Error()
          .Str("file", filename)
          .Msg("Failed to load texture (invalid dimensions)");
      return false;
    }

    textures[filename] = texture;
    Logger::instance()
        .Info()
        .Str("file", filename)
        .Str("path", texture_path)
        .Int("width", texture->width)
        .Int("height", texture->height)
        .Msg("Loaded texture");
    return true;
  } catch (const std::exception& e) {
    Logger::instance().Error().Str("file", filename).Err(e.what()).Msg("Failed to load texture");
    return false;
  }
}

// Sample texture at UV coordinates with wrapping
Color TextureManager::sample(const std::string& filename, double u, double v) {
  // 1. Check if texture exists
  // Reader lock
  std::shared_lock<std::shared_mutex> lock(mutex);

  auto it = textures.find(filename);
  if (it == textures.end()) {
    Logger::instance().Warn().Str("file", filename).Msg("Texture not found - returning debug pink");
    return Color(1.0, 0.0, 1.0);  // Debug Pink
  }

  static thread_local bool first_sample = true;
  if (first_sample) {
    Logger::instance()
        .Debug()
        .Str("file", filename)
        .Double("u", u)
        .Double("v", v)
        .Msg("First texture sample (thread)");
    first_sample = false;
  }

  const Image& texture = *(it->second);
  int w = texture.width;
  int h = texture.height;

  // 2. Clamp UV coordinates to [0, 1] (stretch/fit mode instead of repeat)
  // This prevents the texture from tiling - it will stretch to fit once
  u = std::max(0.0, std::min(1.0, u));
  v = std::max(0.0, std::min(1.0, v));

  // 3. Flip V (standard correction for image coordinates)
  v = 1.0 - v;

  // 4. Calculate exact position in pixel coordinates (e.g., 50.4, 20.7)
  // We subtract 0.5 because pixel centers are technically at 0.5, 1.5, etc.
  double x = u * w - 0.5;
  double y = v * h - 0.5;

  // 5. Get the integer coordinates of the top-left neighbor
  int x_floor = static_cast<int>(std::floor(x));
  int y_floor = static_cast<int>(std::floor(y));

  // 6. Calculate the fractional part (how far we are into the next pixel)
  // This will be our "weight" for mixing colors.
  double u_ratio = x - x_floor;
  double v_ratio = y - y_floor;

  // 7. Calculate coordinates of the 4 neighbors
  // We use modulo (%) to wrap around edges correctly
  int x0 = (x_floor % w + w) % w;  // Left
  int x1 = (x0 + 1) % w;           // Right
  int y0 = (y_floor % h + h) % h;  // Top
  int y1 = (y0 + 1) % h;           // Bottom

  // 8. Fetch the colors of the 4 neighbors
  Color c00 = texture.pixels[y0][x0];  // Top-Left
  Color c10 = texture.pixels[y0][x1];  // Top-Right
  Color c01 = texture.pixels[y1][x0];  // Bottom-Left
  Color c11 = texture.pixels[y1][x1];  // Bottom-Right

  // 9. Interpolate!
  // We perform Linear Interpolation (Lerp) on the top row, then the bottom row,
  // then interpolate between those two results.

  // Helper Lambda for Linear Interpolation
  auto lerp = [](const Color& a, const Color& b, double t) {
    return Color(a.r() + (b.r() - a.r()) * t, a.g() + (b.g() - a.g()) * t,
                 a.b() + (b.b() - a.b()) * t);
  };

  Color top_mix = lerp(c00, c10, u_ratio);
  Color bottom_mix = lerp(c01, c11, u_ratio);
  Color final_color = lerp(top_mix, bottom_mix, v_ratio);

  return final_color;
}

// Check if texture is loaded
bool TextureManager::has_texture(const std::string& filename) const {
  std::shared_lock<std::shared_mutex> lock(mutex);
  return textures.find(filename) != textures.end();
}
