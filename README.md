# CGR Raytracer

A C++ raytracer implementation with Blender integration for computer graphics rendering.

## Features

- Whitted-style recursive raytracing
- BVH acceleration structure for optimized rendering
- Multiple geometric primitives (sphere, cube, plane, torus, cylinder, cone)
- Composite BRDF shading (Lambertian diffuse + Blinn-Phong specular)
- Hard and soft shadows
- Reflection and refraction with Snell's law
- Texture mapping with UV coordinates
- Antialiasing with multi-sampling
- Glossy reflections
- Motion blur
- Depth of field
- Blender scene exporter

## Requirements

- C++20 compatible compiler (g++)
- OpenMP (libomp on macOS)
- Blender (optional, for scene creation)

## Building

Build the raytracer:
```bash
make
```

Clean build artifacts:
```bash
make clean
```

Debug build:
```bash
make debug
```

## Usage

Basic usage:
```bash
./raytracer --scene ASCII/Test1.txt --output Output/result.ppm
```

With custom settings:
```bash
./raytracer --scene ASCII/Test1.txt \
  --output Output/result.ppm \
  --resolution 1920 1080 \
  --samples 16 \
  --max-depth 5
```

Available options:
- `--scene <file>` - Input ASCII scene file
- `--output <file>` - Output PPM file
- `--resolution <w> <h>` - Image resolution
- `--samples <n>` - Antialiasing samples per pixel
- `--max-depth <n>` - Maximum ray recursion depth
- `--soft-shadows <n>` - Enable soft shadows with n samples
- `--glossy-reflection <n>` - Enable glossy reflections with n samples
- `--motion-blur <n>` - Enable motion blur with n temporal samples
- `--depth-of-field <a> <d>` - Enable DOF with aperture and focal distance
- `--light-intensity <f>` - Global light intensity multiplier (default: 0.2)
- `--ambient-light <f>` - Ambient light factor (default: 1.0)

## Makefile Targets

Run raytracer with default scene:
```bash
make run
```

Quick test renders:
```bash
make test-quick
```

High quality test renders:
```bash
make test-all
```

Export Blender scene:
```bash
make export BLEND_FILE=scene.blend
```

Full workflow (export, build, render):
```bash
make workflow
```

## Project Structure

```
Code/
├── Core/       - Main application, camera, scene, config
├── Math/       - Vector, transform, quaternion utilities
├── Geometry/   - Shape primitives and BVH acceleration
├── IO/         - Image I/O, texture loading, scene parsing
└── Render/     - Raytracing engine
ASCII/          - Scene description files
Blend/          - Blender files and export scripts
Output/         - Rendered images
```

## License

Academic project for computer graphics course.
