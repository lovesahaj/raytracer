"""Blender scene exporter for CGR raytracer - COMPREHENSIVE VERSION

Exports Blender scene objects (cameras, lights, shapes) to a custom text format
that can be parsed by the C++ raytracer. Uses transformation-based approach where
shapes are exported as unit objects with separate translation, rotation, and scale.

Enhanced to export ALL possible scene data that might be needed for coursework.

Usage:
    blender scene.blend --background --python Export_Enhanced.py

Output:
    ASCII/scene_name.txt
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))

import bpy
import mathutils
from models import (
    Camera,
    Color,
    Cone,
    Cube,
    Cylinder,
    Direction,
    Light,
    Material,
    Plane,
    Point,
    Sphere,
    Torus,
)


def export_material(obj) -> Material:
    """Extract material properties from Blender object."""
    material = Material()

    if not obj.material_slots or not obj.material_slots[0].material:
        if hasattr(obj, "color") and (
            obj.color[0] != 1.0 or obj.color[1] != 1.0 or obj.color[2] != 1.0
        ):
            material.diffuse_color = Color(obj.color[0], obj.color[1], obj.color[2])
            print(
                f"  {obj.name}: Using object color RGB({obj.color[0]:.3f}, {obj.color[1]:.3f}, {obj.color[2]:.3f})"
            )
        else:
            print(f"  {obj.name}: No material assigned (using default gray)")
        return material

    mat = obj.material_slots[0].material

    if mat.use_nodes:
        nodes = mat.node_tree.nodes
        principled = None
        diffuse_bsdf = None
        glossy_bsdf = None
        mix_shader = None
        emission_node = None
        glass_bsdf = None
        refract_bsdf = None

        for node in nodes:
            if node.type == "BSDF_PRINCIPLED":
                principled = node
                break
            elif node.type == "BSDF_DIFFUSE":
                diffuse_bsdf = node
            elif node.type == "BSDF_GLOSSY":
                glossy_bsdf = node
            elif node.type == "MIX_SHADER":
                mix_shader = node
            elif node.type == "EMISSION":
                emission_node = node
            elif node.type == "BSDF_GLASS":
                glass_bsdf = node
            elif node.type == "BSDF_REFRACTION":
                refract_bsdf = node

    def find_texture_node(socket):
        if not socket.is_linked:
            return None
        node = socket.links[0].from_node
        visited = set()

        def traverse(n):
            if n in visited:
                return None
            visited.add(n)

            if n.type == "TEX_IMAGE":
                return n

            if n.type in [
                "MIX_RGB",
                "MIX_SHADER",
                "ADD_SHADER",
                "BSDF_DIFFUSE",
                "BSDF_GLOSSY",
            ]:
                for input_name in ["Color", "Color1", "Color2", "Base Color", "Shader"]:
                    if input_name in n.inputs and n.inputs[input_name].is_linked:
                        res = traverse(n.inputs[input_name].links[0].from_node)
                        if res:
                            return res
            elif n.type in [
                "VALTORGB",
                "GAMMA",
                "HUE_SAT",
                "BRIGHTCONTRAST",
                "INVERT",
                "CURVES_RGB",
                "MATH",
                "VECT_MATH",
            ]:
                for input_socket in n.inputs:
                    if (
                        input_socket.type in ["RGBA", "VECTOR", "VALUE"]
                        and input_socket.is_linked
                    ):
                        res = traverse(input_socket.links[0].from_node)
                        if res:
                            return res
            return None

        return traverse(node)

    if mat.use_nodes:
        nodes = mat.node_tree.nodes
        principled = None
        diffuse_bsdf = None
        glossy_bsdf = None
        mix_shader = None
        emission_node = None
        glass_bsdf = None
        refract_bsdf = None

        for node in nodes:
            if node.type == "BSDF_PRINCIPLED":
                principled = node
                break
            elif node.type == "BSDF_DIFFUSE":
                diffuse_bsdf = node
            elif node.type == "BSDF_GLOSSY":
                glossy_bsdf = node
            elif node.type == "MIX_SHADER":
                mix_shader = node
            elif node.type == "EMISSION":
                emission_node = node
            elif node.type == "BSDF_GLASS":
                glass_bsdf = node
            elif node.type == "BSDF_REFRACTION":
                refract_bsdf = node

        if principled:
            base_color = principled.inputs["Base Color"].default_value
            material.diffuse_color = Color(base_color[0], base_color[1], base_color[2])
            material.ambient_color = Color(
                base_color[0] * 0.1, base_color[1] * 0.1, base_color[2] * 0.1
            )

            specular = (
                principled.inputs["Specular IOR Level"].default_value
                if "Specular IOR Level" in principled.inputs
                else 0.5
            )
            material.specular_color = Color(specular, specular, specular)

            roughness = principled.inputs["Roughness"].default_value
            # Use a smoother, non-linear mapping for shininess to get softer highlights
            # roughness 0.0 -> shininess ~80 (glossy but not super sharp)
            # roughness 0.5 -> shininess ~20 (moderate)
            # roughness 1.0 -> shininess ~1 (very rough)
            material.shininess = max(1.0, pow(1.0 - roughness, 2.5) * 120.0)
            material.glossiness = 1.0 - roughness  # Export glossiness

            if "Metallic" in principled.inputs:
                metallic = principled.inputs["Metallic"].default_value
                material.reflectivity = metallic

            transmission_found = False
            for trans_name in ["Transmission Weight", "Transmission"]:
                if trans_name in principled.inputs:
                    transmission = principled.inputs[trans_name].default_value
                    material.transparency = transmission
                    transmission_found = True
                    break

            if not transmission_found and "Alpha" in principled.inputs:
                alpha = principled.inputs["Alpha"].default_value
                if alpha < 1.0:
                    material.transparency = 1.0 - alpha
                else:
                    material.transparency = 0.0

            if "IOR" in principled.inputs:
                material.refractive_index = principled.inputs["IOR"].default_value

            if (
                "Emission Color" in principled.inputs
                and "Emission Strength" in principled.inputs
            ):
                emission_color = principled.inputs["Emission Color"].default_value
                emission_strength = principled.inputs["Emission Strength"].default_value
                if emission_strength > 0.0:
                    material.emission_color = Color(
                        emission_color[0], emission_color[1], emission_color[2]
                    )
                    material.emission_strength = emission_strength

            if "Subsurface Weight" in principled.inputs:
                subsurface = principled.inputs["Subsurface Weight"].default_value
                if subsurface > 0.0:
                    material.subsurface = subsurface

            if "Sheen Weight" in principled.inputs:
                sheen = principled.inputs["Sheen Weight"].default_value
                if sheen > 0.0:
                    material.sheen = sheen

            if "Coat Weight" in principled.inputs:
                clearcoat = principled.inputs["Coat Weight"].default_value
                if clearcoat > 0.0:
                    material.clearcoat = clearcoat
                    if "Coat Roughness" in principled.inputs:
                        material.clearcoat_roughness = principled.inputs[
                            "Coat Roughness"
                        ].default_value

            # Texture handling
            tex_node = find_texture_node(principled.inputs["Base Color"])
            if tex_node and tex_node.image:
                image_path = bpy.path.abspath(tex_node.image.filepath)
                if image_path:
                    material.texture_file = os.path.basename(image_path)
                    material.has_texture = True

        elif diffuse_bsdf and glossy_bsdf and mix_shader:
            diffuse_color = diffuse_bsdf.inputs["Color"].default_value

            tex_node = find_texture_node(diffuse_bsdf.inputs["Color"])
            if tex_node and tex_node.image:
                image_path = bpy.path.abspath(tex_node.image.filepath)
                if image_path:
                    material.texture_file = os.path.basename(image_path)
                    material.has_texture = True
                    material.diffuse_color = Color(1.0, 1.0, 1.0)
            else:
                material.diffuse_color = Color(
                    diffuse_color[0], diffuse_color[1], diffuse_color[2]
                )

            material.specular_color = Color(1.0, 1.0, 1.0)
            material.ambient_color = Color(
                material.diffuse_color.x * 0.1,
                material.diffuse_color.y * 0.1,
                material.diffuse_color.z * 0.1,
            )

            if "Roughness" in glossy_bsdf.inputs:
                roughness = glossy_bsdf.inputs["Roughness"].default_value
                material.shininess = max(1.0, pow(1.0 - roughness, 2.5) * 120.0)
                material.glossiness = 1.0 - roughness

            if "Fac" in mix_shader.inputs:
                mix_fac = mix_shader.inputs["Fac"].default_value
                material.reflectivity = mix_fac * 0.15
            else:
                material.reflectivity = 0.05

        elif glass_bsdf:
            color = glass_bsdf.inputs["Color"].default_value
            material.diffuse_color = Color(color[0], color[1], color[2])
            material.transparency = 1.0
            if "IOR" in glass_bsdf.inputs:
                material.refractive_index = glass_bsdf.inputs["IOR"].default_value
            if "Roughness" in glass_bsdf.inputs:
                roughness = glass_bsdf.inputs["Roughness"].default_value
                material.shininess = max(1.0, pow(1.0 - roughness, 2.5) * 120.0)
                material.glossiness = 1.0 - roughness
            material.ambient_color = Color(
                color[0] * 0.1, color[1] * 0.1, color[2] * 0.1
            )
            material.specular_color = Color(1.0, 1.0, 1.0)

        elif refract_bsdf:
            color = refract_bsdf.inputs["Color"].default_value
            material.diffuse_color = Color(color[0], color[1], color[2])
            material.transparency = 1.0
            if "IOR" in refract_bsdf.inputs:
                material.refractive_index = refract_bsdf.inputs["IOR"].default_value
            if "Roughness" in refract_bsdf.inputs:
                roughness = refract_bsdf.inputs["Roughness"].default_value
                material.shininess = max(1.0, pow(1.0 - roughness, 2.5) * 120.0)
                material.glossiness = 1.0 - roughness
            material.ambient_color = Color(
                color[0] * 0.1, color[1] * 0.1, color[2] * 0.1
            )
            material.specular_color = Color(1.0, 1.0, 1.0)

        if emission_node:
            emission_color = emission_node.inputs["Color"].default_value
            emission_strength = emission_node.inputs["Strength"].default_value
            material.emission_color = Color(
                emission_color[0], emission_color[1], emission_color[2]
            )
            material.emission_strength = emission_strength

        for node in nodes:
            if node.type == "NORMAL_MAP" and node.inputs["Color"].is_linked:
                link = node.inputs["Color"].links[0]
                if link.from_node.type == "TEX_IMAGE" and link.from_node.image:
                    normal_path = bpy.path.abspath(link.from_node.image.filepath)
                    material.normal_map = os.path.basename(normal_path)
                break
        for node in nodes:
            if node.type == "BUMP" and node.inputs["Height"].is_linked:
                link = node.inputs["Height"].links[0]
                if link.from_node.type == "TEX_IMAGE" and link.from_node.image:
                    bump_path = bpy.path.abspath(link.from_node.image.filepath)
                    material.bump_map = os.path.basename(bump_path)
                    if "Strength" in node.inputs:
                        material.bump_strength = node.inputs["Strength"].default_value
                break

    else:
        material.diffuse_color = Color(
            mat.diffuse_color[0], mat.diffuse_color[1], mat.diffuse_color[2]
        )
        material.specular_color = Color(
            mat.specular_color[0], mat.specular_color[1], mat.specular_color[2]
        )
        material.shininess = mat.specular_intensity * 128.0
        material.glossiness = 0.0

    return material


def get_motion_data(obj):
    """Check for animation and return transform matrices at t=0 and t=1."""
    if not obj.animation_data or not obj.animation_data.action:
        return False, None, None

    scene = bpy.context.scene
    current_frame = scene.frame_current

    matrix_t0 = obj.matrix_world.copy()

    try:
        scene.frame_set(current_frame + 1)
        matrix_t1 = obj.matrix_world.copy()
        return True, matrix_t0, matrix_t1
    finally:
        scene.frame_set(current_frame)


def export_camera(cam_obj) -> Camera:
    cam = cam_obj.data
    scene = bpy.context.scene
    matrix = cam_obj.matrix_world
    gaze = Direction.from_vector(
        matrix @ mathutils.Vector((0, 0, -1)) - matrix.translation
    )
    up = Direction.from_vector(
        matrix @ mathutils.Vector((0, 1, 0)) - matrix.translation
    )

    camera = Camera(
        name=cam_obj.name,
        location=Point.from_vector(cam_obj.location),
        gaze_direction=gaze,
        up_direction=up,
        focal_length=cam.lens,
        sensor_width=cam.sensor_width,
        sensor_height=cam.sensor_height,
        film_resolution_x=scene.render.resolution_x,
        film_resolution_y=scene.render.resolution_y,
    )
    camera.dof_enabled = cam.dof.use_dof if hasattr(cam, "dof") else False

    if hasattr(cam, "dof") and cam.dof.focus_object:
        focus_obj = cam.dof.focus_object
        camera.focus_distance = (cam_obj.location - focus_obj.location).length
        print(
            f"  Camera '{cam_obj.name}' focusing on '{focus_obj.name}' at distance {camera.focus_distance:.2f}"
        )
    elif hasattr(cam, "dof"):
        camera.focus_distance = cam.dof.focus_distance
    else:
        camera.focus_distance = 10.0

    camera.aperture_fstop = cam.dof.aperture_fstop if hasattr(cam, "dof") else 2.8
    camera.aperture_blades = cam.dof.aperture_blades if hasattr(cam, "dof") else 0
    camera.camera_type = cam.type
    camera.clip_start = cam.clip_start
    camera.clip_end = cam.clip_end
    return camera


def export_light(light_obj) -> Light:
    light = light_obj.data
    light_data = Light(
        name=light_obj.name,
        location=Point.from_vector(light_obj.location),
        intensity=light.energy,
        color=Color(light.color.r, light.color.g, light.color.b),
    )
    light_data.light_type = light.type

    if light.type == "SPOT":
        light_data.spot_size = light.spot_size
        light_data.spot_blend = light.spot_blend

    if light.type == "AREA":
        light_data.area_shape = light.shape
        light_data.area_size_x = light.size
        light_data.area_size_y = (
            light.size_y if light.shape in ["RECTANGLE", "ELLIPSE"] else light.size
        )

        matrix = light_obj.matrix_world
        direction = matrix @ mathutils.Vector((0, 0, -1)) - matrix.translation
        light_data.normal = Direction.from_vector(direction)

        if "samples" in light_obj:
            light_data.samples = light_obj["samples"]
        else:
            light_data.samples = 16

    if light.type == "SUN":
        matrix = light_obj.matrix_world
        direction = matrix @ mathutils.Vector((0, 0, -1)) - matrix.translation
        light_data.direction = Direction.from_vector(direction)
        light_data.angle = light.angle if hasattr(light, "angle") else 0.0

    if hasattr(light, "use_shadow"):
        light_data.cast_shadows = light.use_shadow
    if hasattr(light, "shadow_soft_size"):
        light_data.shadow_soft_size = light.shadow_soft_size

    return light_data


def export_shape(obj, ShapeClass):
    """Generic export for shapes with motion data."""
    mat = export_material(obj)

    base_scale = obj.matrix_world.to_scale()
    dims = obj.dimensions

    if ShapeClass in [Sphere, Cube]:
        sx = (dims.x / 2.0) * (-1.0 if base_scale.x < 0 else 1.0)
        sy = (dims.y / 2.0) * (-1.0 if base_scale.y < 0 else 1.0)
        sz = (dims.z / 2.0) * (-1.0 if base_scale.z < 0 else 1.0)

        has_motion, matrix_t0, matrix_t1 = get_motion_data(obj)

        if ShapeClass == Sphere:
            shape = Sphere(
                name=obj.name,
                location=Point.from_vector(obj.matrix_world.translation),
                rotation=Point.from_vector(obj.matrix_world.to_euler()),
                scale=Point(sx, sy, sz),
                material=mat,
            )
        elif ShapeClass == Cube:
            shape = Cube(
                name=obj.name,
                translation=Point.from_vector(obj.matrix_world.translation),
                rotation=Point.from_vector(obj.matrix_world.to_euler()),
                scale=Point(sx, sy, sz),
                material=mat,
            )

        if has_motion:
            shape.motion_blur = True
            shape.matrix_t0 = matrix_t0
            shape.matrix_t1 = matrix_t1

    elif ShapeClass == Plane:
        mesh = obj.data
        points = [Point.from_vector(obj.matrix_world @ v.co) for v in mesh.vertices]
        has_motion, matrix_t0, matrix_t1 = get_motion_data(obj)
        shape = Plane(name=obj.name, points=points, material=mat)
        if has_motion:
            shape.motion_blur = True
            shape.matrix_t0 = matrix_t0
            shape.matrix_t1 = matrix_t1

    elif ShapeClass == Torus:
        location, _, scale = obj.matrix_world.decompose()
        s_x = scale.x if scale.x != 0 else 1.0
        s_z = scale.z if scale.z != 0 else 1.0
        raw_height = obj.dimensions.z / s_z
        raw_width = obj.dimensions.x / s_x
        calc_minor = raw_height / 2.0
        calc_major = (raw_width / 2.0) - calc_minor
        if calc_major <= 0:
            calc_major = 0.1

        has_motion, matrix_t0, matrix_t1 = get_motion_data(obj)
        shape = Torus(
            name=obj.name,
            location=Point.from_vector(location),
            rotation=Point.from_vector(obj.matrix_world.to_euler()),
            scale=Point.from_vector(scale),
            major_radius=calc_major,
            minor_radius=calc_minor,
            material=mat,
        )
        if has_motion:
            shape.motion_blur = True
            shape.matrix_t0 = matrix_t0
            shape.matrix_t1 = matrix_t1

    elif ShapeClass in [Cylinder, Cone]:
        location, _, scale = obj.matrix_world.decompose()
        s_x = scale.x if scale.x != 0 else 1.0
        s_z = scale.z if scale.z != 0 else 1.0
        calc_radius = (obj.dimensions.x / 2.0) / s_x
        calc_depth = obj.dimensions.z / s_z

        has_motion, matrix_t0, matrix_t1 = get_motion_data(obj)
        shape = ShapeClass(
            name=obj.name,
            location=Point.from_vector(location),
            rotation=Point.from_vector(obj.matrix_world.to_euler()),
            scale=Point.from_vector(scale),
            radius=calc_radius,
            depth=calc_depth,
            material=mat,
        )
        if has_motion:
            shape.motion_blur = True
            shape.matrix_t0 = matrix_t0
            shape.matrix_t1 = matrix_t1

    shape.visible = not obj.hide_render
    return shape


def write_material(f, material: Material):
    f.write(
        f"material_diffuse {material.diffuse_color.x} {material.diffuse_color.y} {material.diffuse_color.z}\n"
    )
    f.write(
        f"material_specular {material.specular_color.x} {material.specular_color.y} {material.specular_color.z}\n"
    )
    f.write(
        f"material_ambient {material.ambient_color.x} {material.ambient_color.y} {material.ambient_color.z}\n"
    )
    f.write(f"material_shininess {material.shininess}\n")
    f.write(f"material_glossiness {material.glossiness}\n")
    f.write(f"material_reflectivity {material.reflectivity}\n")
    f.write(f"material_transparency {material.transparency}\n")
    f.write(f"material_refractive_index {material.refractive_index}\n")

    if hasattr(material, "emission_strength") and material.emission_strength > 0.0:
        f.write(
            f"material_emission {material.emission_color.x} {material.emission_color.y} {material.emission_color.z}\n"
        )
        f.write(f"material_emission_strength {material.emission_strength}\n")

    if hasattr(material, "subsurface") and material.subsurface > 0.0:
        f.write(f"material_subsurface {material.subsurface}\n")
    if hasattr(material, "sheen") and material.sheen > 0.0:
        f.write(f"material_sheen {material.sheen}\n")
    if hasattr(material, "clearcoat") and material.clearcoat > 0.0:
        f.write(f"material_clearcoat {material.clearcoat}\n")
        if hasattr(material, "clearcoat_roughness"):
            f.write(f"material_clearcoat_roughness {material.clearcoat_roughness}\n")

    if material.has_texture:
        f.write(f"material_texture {material.texture_file}\n")
    if hasattr(material, "normal_map") and material.normal_map:
        f.write(f"material_normal_map {material.normal_map}\n")
    if hasattr(material, "bump_map") and material.bump_map:
        f.write(f"material_bump_map {material.bump_map}\n")
        if hasattr(material, "bump_strength"):
            f.write(f"material_bump_strength {material.bump_strength}\n")


def export_to_text(scene_data, filepath):
    with open(filepath, "w") as f:
        scene = bpy.context.scene
        f.write("SCENE_SETTINGS\n")
        if scene.world and scene.world.use_nodes:
            for node in scene.world.node_tree.nodes:
                if node.type == "BACKGROUND":
                    bg_color = node.inputs["Color"].default_value
                    bg_strength = node.inputs["Strength"].default_value
                    f.write(
                        f"background_color {bg_color[0]} {bg_color[1]} {bg_color[2]}\n"
                    )
                    f.write(f"background_strength {bg_strength}\n")
                    break
        else:
            f.write("background_color 0.05 0.05 0.05\n")
            f.write("background_strength 1.0\n")

        if scene.world:
            f.write("ambient_light 0.1 0.1 0.1\n")
        f.write(f"frame_current {scene.frame_current}\n")
        f.write(f"frame_start {scene.frame_start}\n")
        f.write(f"frame_end {scene.frame_end}\n")
        f.write(f"fps {scene.render.fps}\n")
        f.write("max_bounces 12\n")
        f.write("diffuse_bounces 4\n")
        f.write("glossy_bounces 4\n")
        f.write("transmission_bounces 12\n")
        f.write("\n")

        f.write(f"CAMERAS {len(scene_data['cameras'])}\n")
        for cam in scene_data["cameras"]:
            f.write(f"name {cam.name}\n")
            f.write(f"location {cam.location.x} {cam.location.y} {cam.location.z}\n")
            f.write(
                f"gaze {cam.gaze_direction.x} {cam.gaze_direction.y} {cam.gaze_direction.z}\n"
            )
            f.write(
                f"up {cam.up_direction.x} {cam.up_direction.y} {cam.up_direction.z}\n"
            )
            f.write(f"focal {cam.focal_length}\n")
            f.write(f"sensor {cam.sensor_width} {cam.sensor_height}\n")
            f.write(f"resolution {cam.film_resolution_x} {cam.film_resolution_y}\n")
            if hasattr(cam, "dof_enabled"):
                f.write(f"dof_enabled {1 if cam.dof_enabled else 0}\n")
                f.write(f"focus_distance {cam.focus_distance}\n")
                f.write(f"aperture_fstop {cam.aperture_fstop}\n")
                f.write(f"aperture_blades {cam.aperture_blades}\n")
            if hasattr(cam, "camera_type"):
                f.write(f"camera_type {cam.camera_type}\n")
            if hasattr(cam, "clip_start"):
                f.write(f"clip_start {cam.clip_start}\n")
                f.write(f"clip_end {cam.clip_end}\n")

        f.write(f"LIGHTS {len(scene_data['lights'])}\n")
        for light in scene_data["lights"]:
            f.write(f"name {light.name}\n")
            f.write(
                f"location {light.location.x} {light.location.y} {light.location.z}\n"
            )
            f.write(f"intensity {light.intensity}\n")
            f.write(f"color {light.color.x} {light.color.y} {light.color.z}\n")
            f.write(f"light_type {light.light_type}\n")

            if light.light_type == "SPOT":
                f.write(f"spot_size {light.spot_size}\n")
                f.write(f"spot_blend {light.spot_blend}\n")
            if light.light_type == "AREA":
                f.write(f"area_shape {light.area_shape}\n")
                f.write(f"area_size {light.area_size_x} {light.area_size_y}\n")
                # Export area light normal and samples
                if hasattr(light, "normal") and light.normal:
                    f.write(
                        f"normal {light.normal.x} {light.normal.y} {light.normal.z}\n"
                    )
                if hasattr(light, "samples"):
                    f.write(f"samples {light.samples}\n")
            if light.light_type == "SUN":
                f.write(
                    f"direction {light.direction.x} {light.direction.y} {light.direction.z}\n"
                )
                f.write(f"angle {light.angle}\n")

            f.write(f"cast_shadows {1 if light.cast_shadows else 0}\n")
            f.write(f"shadow_soft_size {light.shadow_soft_size}\n")

        def write_shape_data(shape, has_pos=False, has_trans=False):
            f.write(f"name {shape.name}\n")
            if has_pos:
                f.write(
                    f"location {shape.location.x} {shape.location.y} {shape.location.z}\n"
                )
            if has_trans:
                f.write(
                    f"translation {shape.translation.x} {shape.translation.y} {shape.translation.z}\n"
                )
            if hasattr(shape, "rotation"):
                f.write(
                    f"rotation {shape.rotation.x} {shape.rotation.y} {shape.rotation.z}\n"
                )
            if hasattr(shape, "scale"):
                f.write(f"scale {shape.scale.x} {shape.scale.y} {shape.scale.z}\n")
            if hasattr(shape, "radius"):
                f.write(f"radius {shape.radius}\n")
            if hasattr(shape, "depth"):
                f.write(f"depth {shape.depth}\n")
            if hasattr(shape, "major_radius"):
                f.write(f"major_radius {shape.major_radius}\n")
            if hasattr(shape, "minor_radius"):
                f.write(f"minor_radius {shape.minor_radius}\n")
            if hasattr(shape, "points"):
                f.write(f"points {len(shape.points)}\n")
                for pt in shape.points:
                    f.write(f"{pt.x} {pt.y} {pt.z}\n")

            if (
                shape.motion_blur
                and hasattr(shape, "matrix_t0")
                and hasattr(shape, "matrix_t1")
            ):
                f.write("motion_blur 1\n")
                f.write("matrix_t0\n")
                for row in range(4):
                    f.write(
                        f"{shape.matrix_t0[row][0]} {shape.matrix_t0[row][1]} {shape.matrix_t0[row][2]} {shape.matrix_t0[row][3]}\n"
                    )
                f.write("matrix_t1\n")
                for row in range(4):
                    f.write(
                        f"{shape.matrix_t1[row][0]} {shape.matrix_t1[row][1]} {shape.matrix_t1[row][2]} {shape.matrix_t1[row][3]}\n"
                    )

            f.write(f"visible {1 if shape.visible else 0}\n")
            write_material(f, shape.material)

        f.write(f"SPHERES {len(scene_data['spheres'])}\n")
        for s in scene_data["spheres"]:
            write_shape_data(s, has_pos=True)

        f.write(f"CUBES {len(scene_data['cubes'])}\n")
        for c in scene_data["cubes"]:
            write_shape_data(c, has_trans=True)

        f.write(f"PLANES {len(scene_data['planes'])}\n")
        for p in scene_data["planes"]:
            write_shape_data(p)

        f.write(f"TORUSES {len(scene_data['toruses'])}\n")
        for t in scene_data["toruses"]:
            write_shape_data(t, has_pos=True)

        f.write(f"CYLINDERS {len(scene_data['cylinders'])}\n")
        for c in scene_data["cylinders"]:
            write_shape_data(c, has_pos=True)

        f.write(f"CONES {len(scene_data['cones'])}\n")
        for c in scene_data["cones"]:
            write_shape_data(c, has_pos=True)


def export_scene():
    scene_data = {
        "cameras": [],
        "lights": [],
        "spheres": [],
        "cubes": [],
        "planes": [],
        "toruses": [],
        "cylinders": [],
        "cones": [],
    }
    for obj in bpy.data.objects:
        if obj.type == "CAMERA":
            scene_data["cameras"].append(export_camera(obj))
        elif obj.type == "LIGHT":
            scene_data["lights"].append(export_light(obj))
        elif obj.type == "MESH":
            obj_name = obj.name.lower()
            mesh_name = obj.data.name.lower()
            obj_type = None
            for t in ["sphere", "cube", "plane", "torus", "cylinder", "cone"]:
                if t in obj_name:
                    obj_type = t
                    break
            if not obj_type:
                for t in ["sphere", "cube", "plane", "torus", "cylinder", "cone"]:
                    if t in mesh_name:
                        obj_type = t
                        break

            if obj_type == "sphere":
                scene_data["spheres"].append(export_shape(obj, Sphere))
            elif obj_type == "cube":
                scene_data["cubes"].append(export_shape(obj, Cube))
            elif obj_type == "plane":
                scene_data["planes"].append(export_shape(obj, Plane))
            elif obj_type == "torus":
                scene_data["toruses"].append(export_shape(obj, Torus))
            elif obj_type == "cylinder":
                scene_data["cylinders"].append(export_shape(obj, Cylinder))
            elif obj_type == "cone":
                scene_data["cones"].append(export_shape(obj, Cone))

    return scene_data


def main():
    log_path = os.path.join(os.path.dirname(__file__), "export_log.txt")
    import sys

    log_file = open(log_path, "w")
    original_stdout = sys.stdout

    class DualWriter:
        def __init__(self, f1, f2):
            self.f1, self.f2 = f1, f2

        def write(self, t):
            self.f1.write(t)
            self.f2.write(t)

        def flush(self):
            self.f1.flush()
            self.f2.flush()

    sys.stdout = DualWriter(original_stdout, log_file)

    scene_data = export_scene()
    blend_filepath = bpy.data.filepath
    scene_name = os.path.splitext(os.path.basename(blend_filepath))[0]
    output_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "ASCII")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, f"{scene_name}.txt")
    export_to_text(scene_data, output_path)

    sys.stdout = original_stdout
    log_file.close()


if __name__ == "__main__":
    main()
