# Data-Driven Scenic Maps Design

## Goal

Render every scenic area and campus from its own backend graph data so that
different areas have visibly different layouts, roads, points of interest, and
supporting terrain.

## Data Boundary

The backend already provides:

- scenic area metadata, including type and canvas dimensions;
- nodes with coordinates, names, types, and sub-types;
- roads with endpoints, transport type, distance, and congestion.

It does not provide real geographic boundaries, lakes, vegetation polygons, or
map tiles. The frontend must therefore treat the graph as authoritative and
generate only decorative terrain around it. Generated terrain must be
deterministic for one area and different across area IDs.

## Architecture

Create a pure `mapScene` utility that accepts the selected spot, graph nodes,
and graph roads. It returns a deterministic scene containing:

- bounds derived from backend nodes and canvas dimensions;
- area-specific palette and terrain polygons;
- road drawing styles derived from backend road transport and congestion;
- all backend nodes and roads without arbitrary list truncation.

`MapView.vue` remains responsible for canvas interaction and route planning. It
uses the scene utility for visual data and keeps the existing handcrafted demo
map only for area `900001`.

## Rendering Rules

- Campus areas use cooler green and blue palettes; scenic areas use warmer
  natural palettes.
- Terrain clusters are seeded by area ID and biased toward actual node
  clusters, so the background supports rather than replaces backend geometry.
- Roads are drawn from every backend road. Transport type controls width and
  casing; congestion changes tone subtly.
- Junctions stay visually quiet. Entrances, buildings, and facilities use
  distinct symbols and all selectable nodes remain available.
- Labels are density-aware so all data is rendered without making the initial
  view unreadable.

## Failure Handling

If graph data is empty, show an empty map state. If scene generation cannot
derive useful bounds, fall back to the selected area's canvas dimensions.

## Verification

- Unit tests prove deterministic output for one area and different output for
  different area IDs or graph layouts.
- Unit tests prove all roads and selectable nodes are retained.
- Production build must pass.
- Browser checks compare at least three areas and confirm different scene
  signatures, node counts, and road counts with no console errors.
