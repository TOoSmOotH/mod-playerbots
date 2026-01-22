#!/usr/bin/env python3
"""
Cave Detector Tool for mod-playerbots-betterquesting

This tool queries the AzerothCore world database to identify cave locations
by finding clusters of creatures/objects spawned significantly below surface level.

Usage:
    python cave_detector.py --host localhost --user root --password pass --database acore_world

Output:
    Generates SQL file with cave location data for the approach waypoints system.
"""

import argparse
import sys
from collections import defaultdict

try:
    import mysql.connector
except ImportError:
    print("Error: mysql-connector-python is required. Install with: pip install mysql-connector-python")
    sys.exit(1)


# Constants for cave detection
MIN_DEPTH_BELOW_SURFACE = 20.0  # Minimum depth below surface to consider "underground"
CLUSTER_RADIUS = 50.0           # Yards radius to group nearby spawns
MIN_SPAWNS_PER_CLUSTER = 3      # Minimum spawns to consider a valid cave cluster
GRID_SIZE = 100.0               # Grid cell size for surface Z estimation


def connect_to_database(host, user, password, database, port=3306):
    """Connect to the MySQL database."""
    try:
        conn = mysql.connector.connect(
            host=host,
            user=user,
            password=password,
            database=database,
            port=port
        )
        return conn
    except mysql.connector.Error as err:
        print(f"Error connecting to database: {err}")
        sys.exit(1)


def load_all_spawns(conn):
    """Load all creature spawns with their positions."""
    cursor = conn.cursor(dictionary=True)

    print("  Loading creature spawns...")
    query = """
    SELECT
        c.guid,
        c.id1 as entry,
        c.map,
        c.position_x as x,
        c.position_y as y,
        c.position_z as z,
        ct.name
    FROM creature c
    LEFT JOIN creature_template ct ON c.id1 = ct.entry
    WHERE c.map IN (0, 1, 530, 571)
    """

    cursor.execute(query)
    creatures = cursor.fetchall()
    print(f"    Loaded {len(creatures)} creature spawns")

    print("  Loading gameobject spawns...")
    query = """
    SELECT
        g.guid,
        g.id as entry,
        g.map,
        g.position_x as x,
        g.position_y as y,
        g.position_z as z,
        gt.name
    FROM gameobject g
    LEFT JOIN gameobject_template gt ON g.id = gt.entry
    WHERE g.map IN (0, 1, 530, 571)
    """

    cursor.execute(query)
    objects = cursor.fetchall()
    print(f"    Loaded {len(objects)} gameobject spawns")

    cursor.close()
    return creatures, objects


def build_surface_grid(spawns):
    """
    Build a grid of maximum Z values to estimate surface level.
    Uses the highest Z in each grid cell as the surface estimate.
    """
    grid = defaultdict(lambda: -10000.0)  # Default to very low

    for spawn in spawns:
        map_id = spawn['map']
        cell_x = int(spawn['x'] / GRID_SIZE)
        cell_y = int(spawn['y'] / GRID_SIZE)
        key = (map_id, cell_x, cell_y)

        if spawn['z'] > grid[key]:
            grid[key] = spawn['z']

    return grid


def get_surface_z(grid, map_id, x, y):
    """Get estimated surface Z for a position by checking nearby grid cells."""
    cell_x = int(x / GRID_SIZE)
    cell_y = int(y / GRID_SIZE)

    max_z = -10000.0

    # Check 3x3 grid of cells around the position
    for dx in [-1, 0, 1]:
        for dy in [-1, 0, 1]:
            key = (map_id, cell_x + dx, cell_y + dy)
            if grid[key] > max_z:
                max_z = grid[key]

    return max_z if max_z > -10000.0 else None


def find_underground_spawns(spawns, grid, min_depth=MIN_DEPTH_BELOW_SURFACE):
    """Find spawns that are significantly below the estimated surface level."""
    underground = []

    for spawn in spawns:
        surface_z = get_surface_z(grid, spawn['map'], spawn['x'], spawn['y'])
        if surface_z is None:
            continue

        depth = surface_z - spawn['z']
        if depth >= min_depth:
            spawn['surface_z_estimate'] = surface_z
            spawn['depth'] = depth
            underground.append(spawn)

    return underground


def cluster_spawns(spawns, cluster_radius=CLUSTER_RADIUS):
    """Group nearby spawns into clusters."""
    grid = defaultdict(list)
    cell_size = cluster_radius

    for spawn in spawns:
        map_id = spawn['map']
        cell_x = int(spawn['x'] / cell_size)
        cell_y = int(spawn['y'] / cell_size)
        grid[(map_id, cell_x, cell_y)].append(spawn)

    # Merge adjacent cells into clusters
    clusters = []
    processed = set()

    for key in grid:
        if key in processed:
            continue

        map_id, cell_x, cell_y = key
        cluster = list(grid[key])
        processed.add(key)

        # Check adjacent cells
        for dx in [-1, 0, 1]:
            for dy in [-1, 0, 1]:
                if dx == 0 and dy == 0:
                    continue
                adjacent_key = (map_id, cell_x + dx, cell_y + dy)
                if adjacent_key in grid and adjacent_key not in processed:
                    cluster.extend(grid[adjacent_key])
                    processed.add(adjacent_key)

        if len(cluster) >= MIN_SPAWNS_PER_CLUSTER:
            clusters.append(cluster)

    return clusters


def calculate_cluster_center(cluster):
    """Calculate the center point of a cluster."""
    if not cluster:
        return None

    avg_x = sum(s['x'] for s in cluster) / len(cluster)
    avg_y = sum(s['y'] for s in cluster) / len(cluster)
    avg_z = sum(s['z'] for s in cluster) / len(cluster)
    surface_z = max(s.get('surface_z_estimate', s['z']) for s in cluster)
    map_id = cluster[0]['map']

    return {
        'map': map_id,
        'x': avg_x,
        'y': avg_y,
        'z': avg_z,
        'surface_z': surface_z,
        'depth': surface_z - avg_z,
        'spawn_count': len(cluster),
        'sample_names': list(set(s.get('name', 'Unknown') for s in cluster[:5] if s.get('name')))
    }


def get_zone_for_position(conn, map_id, x, y):
    """Get the zone ID for a given position (approximate)."""
    cursor = conn.cursor(dictionary=True)

    query = """
    SELECT zoneId FROM creature
    WHERE map = %s
    AND ABS(position_x - %s) < 200
    AND ABS(position_y - %s) < 200
    AND zoneId > 0
    LIMIT 1
    """

    cursor.execute(query, (map_id, x, y))
    result = cursor.fetchone()
    cursor.close()

    return result['zoneId'] if result else 0


def generate_cave_sql(caves, output_file):
    """Generate SQL file with cave location data."""
    with open(output_file, 'w') as f:
        f.write("-- Cave locations detected by cave_detector.py\n")
        f.write("-- These are candidate cave entrance locations for manual review\n")
        f.write("-- Generated from AzerothCore world database creature/gameobject spawns\n\n")

        f.write("-- Cave candidates table (for review, not directly used by C++)\n")
        f.write("DROP TABLE IF EXISTS `playerbots_questing_cave_candidates`;\n")
        f.write("CREATE TABLE `playerbots_questing_cave_candidates` (\n")
        f.write("    `caveId` INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,\n")
        f.write("    `mapId` SMALLINT UNSIGNED NOT NULL,\n")
        f.write("    `zoneId` SMALLINT UNSIGNED NOT NULL DEFAULT 0,\n")
        f.write("    `centerX` FLOAT NOT NULL,\n")
        f.write("    `centerY` FLOAT NOT NULL,\n")
        f.write("    `caveZ` FLOAT NOT NULL COMMENT 'Average Z of underground spawns',\n")
        f.write("    `surfaceZ` FLOAT NOT NULL COMMENT 'Estimated surface Z',\n")
        f.write("    `depth` FLOAT NOT NULL COMMENT 'Depth below surface',\n")
        f.write("    `spawnCount` INT UNSIGNED NOT NULL COMMENT 'Number of underground spawns',\n")
        f.write("    `sampleNames` VARCHAR(255) DEFAULT NULL COMMENT 'Sample NPC/object names',\n")
        f.write("    `verified` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=unverified, 1=verified cave, 2=false positive',\n")
        f.write("    `entranceX` FLOAT DEFAULT NULL,\n")
        f.write("    `entranceY` FLOAT DEFAULT NULL,\n")
        f.write("    `entranceZ` FLOAT DEFAULT NULL,\n")
        f.write("    INDEX `idx_map_zone` (`mapId`, `zoneId`)\n")
        f.write(") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Cave candidates for approach waypoints';\n\n")

        f.write("-- Insert detected caves\n")
        for cave in caves:
            sample_names = ', '.join(cave['sample_names'][:3]) if cave['sample_names'] else 'Unknown'
            sample_names = sample_names.replace("'", "\\'")[:250]

            f.write(f"INSERT INTO `playerbots_questing_cave_candidates` ")
            f.write(f"(`mapId`, `zoneId`, `centerX`, `centerY`, `caveZ`, `surfaceZ`, `depth`, `spawnCount`, `sampleNames`) VALUES ")
            f.write(f"({cave['map']}, {cave.get('zone', 0)}, {cave['x']:.2f}, {cave['y']:.2f}, ")
            f.write(f"{cave['z']:.2f}, {cave['surface_z']:.2f}, {cave['depth']:.2f}, ")
            f.write(f"{cave['spawn_count']}, '{sample_names}');\n")

        f.write(f"\n-- Total caves detected: {len(caves)}\n")


def main():
    parser = argparse.ArgumentParser(description='Detect cave locations from AzerothCore world database')
    parser.add_argument('--host', default='localhost', help='MySQL host')
    parser.add_argument('--port', type=int, default=3306, help='MySQL port')
    parser.add_argument('--user', default='root', help='MySQL user')
    parser.add_argument('--password', default='', help='MySQL password')
    parser.add_argument('--database', default='acore_world', help='Database name')
    parser.add_argument('--output', default='detected_caves.sql', help='Output SQL file')
    parser.add_argument('--min-depth', type=float, default=MIN_DEPTH_BELOW_SURFACE,
                        help='Minimum depth below surface to consider underground')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')

    args = parser.parse_args()

    print("Connecting to database...")
    conn = connect_to_database(args.host, args.user, args.password, args.database, args.port)

    print("Loading all spawn data...")
    creatures, objects = load_all_spawns(conn)
    all_spawns = creatures + objects
    print(f"Total spawns loaded: {len(all_spawns)}")

    print("\nBuilding surface height grid...")
    surface_grid = build_surface_grid(all_spawns)
    print(f"  Grid cells: {len(surface_grid)}")

    print("\nFinding underground spawns...")
    underground = find_underground_spawns(all_spawns, surface_grid, args.min_depth)
    print(f"  Found {len(underground)} underground spawns")

    print("\nClustering spawns into cave locations...")
    clusters = cluster_spawns(underground)
    print(f"  Found {len(clusters)} cave clusters")

    # Calculate cluster centers and add zone info
    print("\nProcessing cave data...")
    caves = []
    for i, cluster in enumerate(clusters):
        center = calculate_cluster_center(cluster)
        if center:
            center['zone'] = get_zone_for_position(conn, center['map'], center['x'], center['y'])
            caves.append(center)

            if args.verbose:
                print(f"  Cave {i+1}: map={center['map']} zone={center['zone']} "
                      f"pos=({center['x']:.0f}, {center['y']:.0f}) "
                      f"depth={center['depth']:.0f} spawns={center['spawn_count']}")

    # Sort by map then by spawn count
    caves.sort(key=lambda c: (c['map'], -c['spawn_count']))

    print(f"\nGenerating SQL output: {args.output}")
    generate_cave_sql(caves, args.output)

    # Summary by map
    print("\nSummary by map:")
    map_counts = defaultdict(int)
    for cave in caves:
        map_counts[cave['map']] += 1

    map_names = {0: 'Eastern Kingdoms', 1: 'Kalimdor', 530: 'Outland', 571: 'Northrend'}
    for map_id, count in sorted(map_counts.items()):
        map_name = map_names.get(map_id, f'Map {map_id}')
        print(f"  {map_name}: {count} caves")

    conn.close()
    print("\nDone!")


if __name__ == '__main__':
    main()
