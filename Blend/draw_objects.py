import os
import sys

sys.path.insert(0, os.path.dirname(__file__))

import bpy
from models import Cylinder


def draw_cylinder(cylinder: Cylinder) -> bpy.types.Object:
    """Create a cylinder mesh in Blender from a Cylinder dataclass."""
    # Add cylinder primitive with specified parameters
    bpy.ops.mesh.primitive_cylinder_add(
        radius=cylinder.radius,
        depth=cylinder.depth,
        location=(cylinder.location.x, cylinder.location.y, cylinder.location.z),
        rotation=(cylinder.rotation.x, cylinder.rotation.y, cylinder.rotation.z),
    )

    # Get reference to the newly created cylinder
    cyl_obj = bpy.context.active_object
    cyl_obj.name = cylinder.name

    return cyl_obj
