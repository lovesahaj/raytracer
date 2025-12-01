import dataclasses
import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))

print(sys.path)

from pathlib import Path
from typing import List

from draw_objects import draw_cylinder
from models import Cylinder, Direction, Point


@dataclasses.dataclass
class Ray:
    name: str
    origin: Point
    direction: Direction


def parse_value(keyword, value: str):
    if keyword == "name":
        return value
    elif keyword == "origin":
        return Point(*(map(float, value.split(" ", maxsplit=3))))
    elif keyword == "direction":
        return Direction(*(map(float, value.split(" ", maxsplit=3))))


def read_txt_to_rays(filename: Path) -> List[Ray]:
    rays = []
    with open(filename, "r") as file:
        keyword, count = file.readline().strip().split(" ", maxsplit=1)
        assert keyword == "ray"

        for _ in range(int(count)):
            ray_data = {}
            for __ in range(3):
                keyword, value = file.readline().strip().split(" ", maxsplit=1)
                ray_data[keyword] = parse_value(keyword, value)

            rays.append(Ray(**ray_data))

    return rays


def ray_to_cylinder(rays: List[Ray]) -> List[Cylinder]:
    cylinders = []
    for ray in rays:
        # Get magnitude of direction vector
        magnitude = ray.direction.mod()

        # Handle zero-length direction vectors
        if magnitude < 1e-10:
            raise ValueError(f"Ray '{ray.name}' has zero-length direction vector")

        # Calculate spherical coordinates with numerical stability
        phi = math.atan2(ray.direction.y, ray.direction.x)

        # Clamp to [-1, 1] to avoid numerical errors in acos
        cos_theta = max(-1.0, min(1.0, ray.direction.z / magnitude))
        theta = math.acos(cos_theta)

        # Calculate cylinder location: offset by half depth along ray direction
        # so cylinder starts at origin and extends in ray direction
        depth = 100
        half_depth = depth / 2
        normalized_dir = Point(
            ray.direction.x / magnitude,
            ray.direction.y / magnitude,
            ray.direction.z / magnitude,
        )

        location = Point(
            ray.origin.x + normalized_dir.x * half_depth,
            ray.origin.y + normalized_dir.y * half_depth,
            ray.origin.z + normalized_dir.z * half_depth,
        )

        cylinders.append(
            Cylinder(
                name=ray.name,
                radius=0.1,
                depth=depth,
                location=location,
                rotation=Point(0.0, theta, phi),
            ),
        )

    return cylinders


def main():
    rays = read_txt_to_rays(
    filename=Path(__file__).parent.parent / "data" / "gen_rays.txt"
    )
    print(rays)

    cylinders = ray_to_cylinder(rays)
    for cylinder in cylinders:
        draw_cylinder(cylinder)


if __name__ == "__main__":
    main()
