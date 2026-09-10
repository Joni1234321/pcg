#!/usr/bin/env python3
"""Converts assets/rail/scenario_source into assets/rail/scenarios_base.

scenario_source/<name>.json        map definition: image, crop, projection, countries
scenario_source/<name>.<ext>       relief location map image (downloaded by fetch)
scenario_source/<name>_cities.json cities with coordinates, classes and population by year (downloaded by fetch)
scenario_source/city_classes.json  Wikidata classes counted as city or town (downloaded by fetch)

scenarios_base/<name>.txt                elevation hex grid
scenarios_base/<name>_cities_<year>.txt  cities present that decade, one "city x y name" line each (axial hex)

The game only ever sees hex coordinates; projections and lat/lon live here and in scenario_source.

Run from the repo root:
    python3 tools/rail_scenario_base.py fetch [name ...]   download images and Wikidata cities
    python3 tools/rail_scenario_base.py [name ...]         convert

The elevation heuristic mirrors ElevationFromImage in src/8_rail/g_rail_scenarios.cppm.
Images are Wikimedia Commons relief location maps (CC BY-SA); the projections mirror
the matching Wikipedia Module:Location map/data pages.
"""
import json
import math
import subprocess
import sys
import time
from pathlib import Path

from PIL import Image

SOURCE_DIR = Path("assets/rail/scenario_source")
BASE_DIR = Path("assets/rail/scenarios_base")
USER_AGENT = "pcg-dev/1.0 (joni1234@hotmail.dk)"
ELEVATION_MIN = -128
ELEVATION_MAX = 127
DECADES = range(1830, 2030, 10)
SQRT3 = math.sqrt(3)
AXIAL_NEIGHBOURS = [(1, 0), (1, -1), (0, -1), (-1, 0), (-1, 1), (0, 1)]


def pixel_to_elevation(r, g, b):
    if b >= r + 30 and b >= g:
        luminance = (r * 299 + g * 587 + b * 114) // 1000
        return min(max(-1 - (220 - luminance) * 127 // 100, ELEVATION_MIN), -1)
    return min(max((r - 140) * ELEVATION_MAX // 105, 0), ELEVATION_MAX)


def inside_polygon(x, y, polygon):
    inside = False
    for (ax, ay), (bx, by) in zip(polygon, polygon[1:] + polygon[:1]):
        if (ay > y) != (by > y) and x < ax + (y - ay) * (bx - ax) / (by - ay):
            inside = not inside
    return inside


def project(projection, lat, lon):
    """lat/lon -> fraction of the full image"""
    p = projection["params"]
    if projection["type"] == "equirect":
        top, bottom, left, right = p
        return (lon - left) / (right - left), (top - lat) / (top - bottom)
    if projection["type"] == "laea":
        lat0, lon0, sx, sy, ox, oy = p
        phi, phi0, dl = math.radians(lat), math.radians(lat0), math.radians(lon - lon0)
        k = math.sqrt(2 / (1 + math.sin(phi) * math.sin(phi0) + math.cos(phi) * math.cos(phi0) * math.cos(dl)))
        return (ox + sx * math.cos(phi) * math.sin(dl) * k) / 100, (oy - sy * (math.cos(phi0) * math.sin(phi) - math.sin(phi0) * math.cos(phi) * math.cos(dl)) * k) / 100
    cx, cy, sx, sy, a, b, n, lon0, wrap = p
    rho = a - b * lat
    theta = n * ((lon + 360 if wrap and lon < 0 else lon) - lon0)
    return (cx + sx * rho * math.sin(theta)) / 100, (cy + sy * rho * math.cos(theta)) / 100


def hex_round(axial_x, axial_y):
    cube_z = -axial_x - axial_y
    rx, ry, rz = round(axial_x), round(axial_y), round(cube_z)
    dx, dy, dz = abs(rx - axial_x), abs(ry - axial_y), abs(rz - cube_z)
    if dx > dy and dx > dz:
        rx = -ry - rz
    elif dy > dz:
        ry = -rx - rz
    return rx, ry


def sparql(query):
    for attempt in range(4):
        result = subprocess.run(["curl", "-s", "-m", "170", "-A", USER_AGENT, "-H", "Accept: application/sparql-results+json", "--data-urlencode", "query=" + query, "https://query.wikidata.org/sparql"], capture_output=True, text=True).stdout
        try:
            return json.loads(result)["results"]["bindings"]
        except (json.JSONDecodeError, KeyError):
            print(f"  wikidata retry {attempt + 1}: {result[:80]!r}")
            time.sleep(10 * (attempt + 1))
    raise RuntimeError("wikidata query failed")


def fetch(name, definition):
    image_path = SOURCE_DIR / definition["image_file"]
    if not image_path.exists():
        subprocess.run(["curl", "-s", "-L", "--fail", "-m", "180", "-A", USER_AGENT, "-o", str(image_path), definition["image"]], check=True)
        print(f"{name}: downloaded {image_path}")
    settlement_classes = {row["class"]["value"].rsplit("/", 1)[1] for row in sparql("SELECT ?class WHERE { ?class wdt:P279* wd:Q486972 }")}
    city_classes = {row["class"]["value"].rsplit("/", 1)[1] for row in sparql("SELECT ?class WHERE { ?class wdt:P279* wd:Q7930989 }")}
    (SOURCE_DIR / "city_classes.json").write_text(json.dumps(sorted(city_classes)))
    cities = {}
    for country in definition["countries"]:
        rows = sparql(f"""SELECT ?city ?cityLabel ?coord ?class WHERE {{
            ?city wdt:P17 wd:{country} ; wdt:P31 ?class ; wdt:P625 ?coord ; wdt:P1082 ?pop . FILTER(?pop >= 10000)
            SERVICE wikibase:label {{ bd:serviceParam wikibase:language "en" . }} }}""")
        for row in rows:
            class_id = row["class"]["value"].rsplit("/", 1)[1]
            city_id = row["city"]["value"].rsplit("/", 1)[1]
            if class_id not in settlement_classes or row["cityLabel"]["value"] == city_id:
                continue
            city = cities.setdefault(city_id, {"id": city_id, "name": row["cityLabel"]["value"], "lat": 0.0, "lon": 0.0, "classes": [], "populations": {}})
            lon, lat = row["coord"]["value"].removeprefix("Point(").removesuffix(")").split()
            city["lat"], city["lon"] = float(lat), float(lon)
            city["classes"].append(class_id)
        print(f"{name}: {country} -> {len(cities)} settlements so far")
    ids = sorted(cities)
    for start in range(0, len(ids), 300):
        values = " ".join("wd:" + city_id for city_id in ids[start:start + 300])
        for row in sparql(f"SELECT ?city ?pop ?year WHERE {{ VALUES ?city {{ {values} }} ?city p:P1082 ?ps . ?ps ps:P1082 ?pop ; pq:P585 ?date . BIND(YEAR(?date) AS ?year) }}"):
            if "year" not in row:
                continue
            populations = cities[row["city"]["value"].rsplit("/", 1)[1]]["populations"]
            year = row["year"]["value"]
            populations[year] = max(populations.get(year, 0), int(float(row["pop"]["value"])))
    (SOURCE_DIR / f"{name}_cities.json").write_text(json.dumps(sorted(cities.values(), key=lambda city: city["name"]), indent=1, ensure_ascii=False))
    print(f"{name}: wrote {len(cities)} cities")


def population_at(populations, decade):
    years = sorted(populations)
    if not years:
        return None
    if decade < years[0]:
        return populations[years[0]] if years[0] - decade <= 10 else None
    if decade >= years[-1]:
        return populations[years[-1]]
    for earlier, later in zip(years, years[1:]):
        if earlier <= decade <= later:
            t = (decade - earlier) / (later - earlier)
            return populations[earlier] + (populations[later] - populations[earlier]) * t
    return None


def convert(name, definition):
    image = Image.open(SOURCE_DIR / definition["image_file"]).convert("RGB")
    crop = definition.get("crop") or [0.0, 0.0, 1.0, 1.0]
    sea_polygons = definition.get("sea_polygons", [])
    cropped = image.crop((int(crop[0] * image.width), int(crop[1] * image.height), int(crop[2] * image.width), int(crop[3] * image.height)))
    width = definition["columns"]
    world_width = width * SQRT3
    height = round(world_width * cropped.height / cropped.width / 1.5) + 1
    world_height = (height - 1) * 1.5

    def image_fraction(world_x, world_y):
        u = min(max(world_x / world_width, 0.0), 1.0)
        v = min(max(world_y / world_height, 0.0), 1.0)
        return crop[0] + u * (crop[2] - crop[0]), crop[1] + v * (crop[3] - crop[1])

    elevation = []
    for y in range(height):
        for x_offset in range(width):
            fx, fy = image_fraction(SQRT3 * (x_offset + (0.5 if y & 1 else 0.0)), 1.5 * y)
            if any(inside_polygon(fx, fy, polygon) for polygon in sea_polygons):
                elevation.append(ELEVATION_MIN)
                continue
            elevation.append(pixel_to_elevation(*image.getpixel((int(fx * (image.width - 1)), int(fy * (image.height - 1))))))
    BASE_DIR.mkdir(parents=True, exist_ok=True)
    rows = [" ".join(str(value) for value in elevation[y * width:(y + 1) * width]) for y in range(height)]
    projection = definition["projection"]
    (BASE_DIR / f"{name}.txt").write_text(f"{width} {height}\n" + "\n".join(rows) + "\n")

    def contains(axial):
        y = axial[1]
        x_offset = axial[0] + y // 2
        return 0 <= y < height and 0 <= x_offset < width

    def is_land(axial):
        return elevation[axial[0] + axial[1] // 2 + axial[1] * width] >= 0

    def city_axial(lat, lon):
        fx, fy = project(projection, lat, lon)
        u = (fx - crop[0]) / (crop[2] - crop[0])
        v = (fy - crop[1]) / (crop[3] - crop[1])
        if not (0.0 <= u < 1.0 and 0.0 <= v < 1.0):
            return None
        axial_y = v * world_height / 1.5
        axial = hex_round(u * world_width / SQRT3 - axial_y * 0.5, axial_y)
        if not contains(axial):
            return None
        if is_land(axial):
            return axial
        ring = [axial]
        for _ in range(3):
            ring = [(a[0] + dx, a[1] + dy) for a in ring for dx, dy in AXIAL_NEIGHBOURS]
            land = [a for a in ring if contains(a) and is_land(a)]
            if land:
                return min(land, key=lambda a: (a[0] - axial[0]) ** 2 + (a[1] - axial[1]) ** 2)
        return None

    cities_path = SOURCE_DIR / f"{name}_cities.json"
    city_classes = set(json.loads((SOURCE_DIR / "city_classes.json").read_text()))
    cities = [city for city in json.loads(cities_path.read_text()) if any(class_id in city_classes for class_id in city["classes"])] if cities_path.exists() else []
    placed = [(city, city_axial(city["lat"], city["lon"])) for city in cities]
    placed = [(city, axial) for city, axial in placed if axial is not None]
    written = 0
    for decade in DECADES:
        by_hex = {}
        for city, axial in placed:
            population = population_at({int(year): pop for year, pop in city["populations"].items()}, decade)
            if population is None or population < definition["population_min"]:
                continue
            if axial not in by_hex or by_hex[axial][2] < population:
                by_hex[axial] = (city["name"], city, population)
        if not by_hex:
            continue
        lines = [f"city {axial[0]} {axial[1]} {city_name}" for axial, (city_name, _, _) in sorted(by_hex.items(), key=lambda item: -item[1][2])]
        (BASE_DIR / f"{name}_cities_{decade}.txt").write_text("\n".join(lines) + "\n")
        written += 1
    print(f"{name}: {width}x{height}, {len(placed)}/{len(cities)} cities on map, {written} decade files")


if __name__ == "__main__":
    arguments = sys.argv[1:]
    fetching = bool(arguments) and arguments[0] == "fetch"
    selected = set(arguments[1:] if fetching else arguments)
    for path in sorted(SOURCE_DIR.glob("*.json")):
        name = path.stem
        if name.endswith("_cities") or name == "city_classes" or (selected and name not in selected):
            continue
        definition = json.loads(path.read_text())
        (fetch if fetching else convert)(name, definition)
