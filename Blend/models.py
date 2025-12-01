"""Data models for CGR raytracer scene export.

Defines dataclasses for all scene elements with comprehensive properties
to support all coursework features including distributed raytracing,
lens effects, and advanced materials.
"""

from dataclasses import dataclass, field
from typing import List, Optional

import mathutils


@dataclass
class Point:
    """3D point in space."""

    x: float
    y: float
    z: float

    @classmethod
    def from_vector(cls, vec):
        """Create Point from Blender Vector."""
        return cls(vec.x, vec.y, vec.z)


@dataclass
class Direction:
    """3D direction vector (normalized)."""

    x: float
    y: float
    z: float

    @classmethod
    def from_vector(cls, vec):
        """Create Direction from Blender Vector (normalized)."""
        normalized = vec.normalized()
        return cls(normalized.x, normalized.y, normalized.z)


@dataclass
class Color:
    """RGB color."""

    x: float  # Red
    y: float  # Green
    z: float  # Blue

    def __init__(self, r: float, g: float, b: float):
        self.x = r
        self.y = g
        self.z = b


@dataclass
class Material:
    """Material properties for ray tracing."""

    # Basic Blinn-Phong properties
    diffuse_color: Color = field(default_factory=lambda: Color(0.5, 0.5, 0.5))
    specular_color: Color = field(default_factory=lambda: Color(1.0, 1.0, 1.0))
    ambient_color: Color = field(default_factory=lambda: Color(0.05, 0.05, 0.05))
    shininess: float = 32.0
    glossiness: float = 0.0  # 0.0 = mirror, 1.0 = diffuse (1-roughness)

    # Advanced material properties
    reflectivity: float = 0.0
    transparency: float = 0.0
    refractive_index: float = 1.0

    # Emission (for glowing objects)
    emission_color: Color = field(default_factory=lambda: Color(0.0, 0.0, 0.0))
    emission_strength: float = 0.0

    # Advanced PBR properties
    subsurface: float = 0.0  # Subsurface scattering
    sheen: float = 0.0  # Fabric-like reflection
    clearcoat: float = 0.0  # Clear coat layer
    clearcoat_roughness: float = 0.0

    # Texture mapping
    has_texture: bool = False
    texture_file: str = ""
    normal_map: str = ""
    bump_map: str = ""
    bump_strength: float = 1.0


@dataclass
class Camera:
    """Camera properties."""

    name: str
    location: Point
    gaze_direction: Direction
    up_direction: Direction
    focal_length: float
    sensor_width: float
    sensor_height: float
    film_resolution_x: int
    film_resolution_y: int

    # Depth of field settings (for lens effects)
    dof_enabled: bool = False
    focus_distance: float = 10.0
    aperture_fstop: float = 2.8
    aperture_blades: int = 0

    # Camera type and clipping
    camera_type: str = "PERSP"  # PERSP, ORTHO, PANO
    clip_start: float = 0.1
    clip_end: float = 100.0


@dataclass
class Light:
    """Light source properties."""

    name: str
    location: Point
    intensity: float
    color: Color

    # Light type: POINT, SUN, SPOT, AREA
    light_type: str = "POINT"

    # Spot light properties
    spot_size: float = 0.785398  # 45 degrees in radians
    spot_blend: float = 0.15

    # Area light properties
    area_shape: str = "SQUARE"  # SQUARE, RECTANGLE, DISK, ELLIPSE
    area_size_x: float = 1.0
    area_size_y: float = 1.0
    samples: int = 16
    normal: Optional[Direction] = None

    # Directional light (sun) properties
    direction: Optional[Direction] = None
    angle: float = 0.0

    # Shadow properties (for distributed raytracing)
    cast_shadows: bool = True
    shadow_soft_size: float = 0.0


@dataclass
class Sphere:
    """Sphere primitive."""

    name: str
    location: Point
    scale: Point  # Non-uniform scale
    rotation: Point  # Euler angles in radians
    material: Material
    visible: bool = True
    # Motion blur
    motion_blur: bool = False
    position_t1: Optional[Point] = None


@dataclass
class Cube:
    """Cube primitive."""

    name: str
    translation: Point
    rotation: Point  # Euler angles in radians
    scale: Point  # Non-uniform scale
    material: Material
    visible: bool = True
    # Motion blur
    motion_blur: bool = False
    position_t1: Optional[Point] = None


@dataclass
class Plane:
    """Plane primitive (defined by vertices)."""

    name: str
    points: List[Point]
    material: Material
    visible: bool = True
    # Motion blur
    motion_blur: bool = False
    position_t1: Optional[Point] = None


@dataclass
class Torus:
    """Torus primitive."""

    name: str
    location: Point
    rotation: Point  # Euler angles in radians
    scale: Point  # NEW: Non-uniform scale
    major_radius: float
    minor_radius: float
    material: Material
    visible: bool = True
    # Motion blur
    motion_blur: bool = False
    position_t1: Optional[Point] = None


@dataclass
class Cylinder:
    """Cylinder primitive."""

    name: str
    location: Point
    rotation: Point  # Euler angles in radians
    scale: Point  # NEW: Non-uniform scale
    radius: float
    depth: float
    material: Material
    visible: bool = True
    # Motion blur
    motion_blur: bool = False
    position_t1: Optional[Point] = None


@dataclass
class Cone:
    """Cone primitive."""

    name: str
    location: Point
    rotation: Point  # Euler angles in radians
    scale: Point  # NEW: Non-uniform scale
    radius: float
    depth: float
    material: Material
    visible: bool = True
    # Motion blur
    motion_blur: bool = False
    position_t1: Optional[Point] = None

