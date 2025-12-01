#include "Image.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../Utils/logger.h"

using Utils::Logger;

// Constructor for creating empty image with specified dimensions
Image::Image(int height, int width, int max_value, std::string magic_number) {
  this->width = width;
  this->height = height;
  this->max_value = max_value;
  this->magic_number = magic_number;
  pixels.resize(height, std::vector<Pixel>(width));
}

// Constructor for reading image from file
Image::Image(const std::string &filepath) {
  std::ifstream input_file;
  input_file.open(filepath, std::ios::binary);

  if (!input_file) {
    Logger::instance().Error().Str("path", filepath).Msg("Error: Could not open file");
    return;
  }

  input_file >> magic_number;

  if (magic_number != "P3" && magic_number != "P6") {
    Logger::instance().Error().Str("format", magic_number).Msg("Error: Invalid PPM format");
    return;
  }

  width = read_next_int(input_file);
  height = read_next_int(input_file);
  max_value = read_next_int(input_file);

  pixels.resize(height, std::vector<Pixel>(width));

  if (magic_number == "P3") {
    read_ascii_pixels(input_file);
  } else {
    read_binary_pixels(input_file);
  }

  input_file.close();
}

// Write image to file
void Image::write(const std::string &filepath) const {
  std::ofstream output_file;

  if (magic_number == "P6") {
    output_file.open(filepath, std::ios::out | std::ios::binary);
  } else {
    output_file.open(filepath, std::ios::out);
  }

  if (!output_file) {
    Logger::instance().Error().Str("path", filepath).Msg("Error: Could not open file for writing");
    return;
  }

  if (magic_number == "P6") {
    write_binary(output_file);
  } else {
    write_ascii(output_file);
  }

  output_file.close();
}

// Print image information
std::ostream &operator<<(std::ostream &os, const Image &img) {
  os << "Image Information:\n";
  os << "  Format: " << img.magic_number << "\n";
  os << "  Dimensions: " << img.width << "x" << img.height << "\n";
  os << "  Max Value: " << img.max_value << "\n";
  os << "  Total Pixels: " << (img.width * img.height) << "\n";
  return os;
}

// Read next integer from file, skipping comments
int Image::read_next_int(std::ifstream &file) {
  std::string line;
  int value;

  while (true) {
    file >> value;

    if (file.good()) {
      file >> std::ws;

      if (file.peek() == '#') {
        std::getline(file, line);
      }

      return value;
    }

    file.clear();
    std::getline(file, line);

    if (line[0] != '#') {
      std::istringstream iss(line);
      iss >> value;
      return value;
    }
  }
}

// Convert byte (0-max_value) to double (0.0-1.0)
double Image::byte_to_double(const int byte_val) const {
  return static_cast<double>(byte_val) / max_value;
}

// Convert double (0.0-1.0) to byte (0-max_value) with clamping
unsigned char Image::double_to_byte(double val) const {
  // Clamp to [0.0, 1.0] range
  val = std::clamp(val, 0.0, 1.0);
  // Convert to byte range with proper rounding
  return static_cast<unsigned char>(std::lround(val * max_value));
}

// Read pixel data in ASCII format (P3)
void Image::read_ascii_pixels(std::ifstream &file) {
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      int r, g, b;
      file >> r >> g >> b;

      pixels[i][j].r() = byte_to_double(r);
      pixels[i][j].g() = byte_to_double(g);
      pixels[i][j].b() = byte_to_double(b);
    }
  }
}

// Read pixel data in binary format (P6)
void Image::read_binary_pixels(std::ifstream &file) {
  file >> std::ws;

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      unsigned char r, g, b;
      file.read(reinterpret_cast<char *>(&r), 1);
      file.read(reinterpret_cast<char *>(&g), 1);
      file.read(reinterpret_cast<char *>(&b), 1);

      pixels[i][j].r() = byte_to_double(r);
      pixels[i][j].g() = byte_to_double(g);
      pixels[i][j].b() = byte_to_double(b);
    }
  }
}

// Write image in binary format (P6)
void Image::write_binary(std::ofstream &file) const {
  // Write header
  file << magic_number << "\n";
  file << width << " " << height << "\n";
  file << max_value << "\n";

  // Write pixel data as raw bytes
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      unsigned char r = double_to_byte(pixels[i][j].r());
      unsigned char g = double_to_byte(pixels[i][j].g());
      unsigned char b = double_to_byte(pixels[i][j].b());

      file.write(reinterpret_cast<const char *>(&r), 1);
      file.write(reinterpret_cast<const char *>(&g), 1);
      file.write(reinterpret_cast<const char *>(&b), 1);
    }
  }
}

// Write image in ASCII format (P3)
void Image::write_ascii(std::ofstream &file) const {
  // Write header
  file << magic_number << "\n";
  file << width << " " << height << "\n";
  file << max_value << "\n";

  // Write pixel data as ASCII
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      file << static_cast<int>(double_to_byte(pixels[i][j].r())) << " ";
      file << static_cast<int>(double_to_byte(pixels[i][j].g())) << " ";
      file << static_cast<int>(double_to_byte(pixels[i][j].b())) << " ";

      // Add newline every 5 pixels for readability
      if ((j + 1) % 5 == 0) {
        file << "\n";
      }
    }
    file << "\n";
  }
}