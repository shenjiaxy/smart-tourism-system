import type { MapNode, Road, Spot } from '@/types'

export type MapSceneBounds = {
  minX: number
  minY: number
  maxX: number
  maxY: number
}

export type MapTerrainShape = {
  kind: 'water' | 'green' | 'district'
  points: Array<[number, number]>
}

export type MapScenePalette = {
  background: string
  grid: string
  water: string
  waterStroke: string
  green: string
  greenStroke: string
  district: string
  districtStroke: string
}

export type MapScene = {
  signature: string
  bounds: MapSceneBounds
  palette: MapScenePalette
  terrain: MapTerrainShape[]
  nodes: MapNode[]
  roads: Road[]
}

const campusPalettes: MapScenePalette[] = [
  {
    background: '#f4f6f1',
    grid: 'rgba(76, 96, 86, 0.035)',
    water: '#cfe6ed',
    waterStroke: '#b7d5df',
    green: '#dce9d5',
    greenStroke: '#c5d9bc',
    district: '#ecefe7',
    districtStroke: '#d9ded2',
  },
  {
    background: '#f6f4ee',
    grid: 'rgba(92, 82, 64, 0.035)',
    water: '#d5e7ee',
    waterStroke: '#bdd7e0',
    green: '#e1ead4',
    greenStroke: '#ccd9bc',
    district: '#eee9df',
    districtStroke: '#ddd5c7',
  },
  {
    background: '#f1f5f4',
    grid: 'rgba(58, 89, 94, 0.035)',
    water: '#cbe3ed',
    waterStroke: '#afd1de',
    green: '#d7e7dc',
    greenStroke: '#bed5c5',
    district: '#e5ece9',
    districtStroke: '#cfdbd6',
  },
]

const scenicPalettes: MapScenePalette[] = [
  {
    background: '#f7f4eb',
    grid: 'rgba(104, 88, 58, 0.032)',
    water: '#c9e4ef',
    waterStroke: '#acd3e2',
    green: '#d9e8c8',
    greenStroke: '#c1d7ad',
    district: '#efe8d8',
    districtStroke: '#ded2ba',
  },
  {
    background: '#f3f2e9',
    grid: 'rgba(76, 88, 55, 0.034)',
    water: '#d0e5e8',
    waterStroke: '#b4d2d6',
    green: '#dbe5c7',
    greenStroke: '#c4d2aa',
    district: '#ebe8d9',
    districtStroke: '#d7d2bd',
  },
  {
    background: '#f6f1e8',
    grid: 'rgba(105, 78, 55, 0.032)',
    water: '#cce1ed',
    waterStroke: '#b0ccdc',
    green: '#dce7ce',
    greenStroke: '#c5d5b3',
    district: '#efe4d7',
    districtStroke: '#dfcdbd',
  },
]

function hashNumber(value: number, hash: number) {
  let next = hash ^ Math.round(value * 100)
  next = Math.imul(next ^ (next >>> 16), 0x45d9f3b)
  next = Math.imul(next ^ (next >>> 16), 0x45d9f3b)
  return (next ^ (next >>> 16)) >>> 0
}

function sceneSeed(spot: Spot, nodes: MapNode[], roads: Road[]) {
  let seed = hashNumber(spot.id, 0x811c9dc5)
  for (const node of nodes) {
    seed = hashNumber(node.id, seed)
    seed = hashNumber(node.pos_x, seed)
    seed = hashNumber(node.pos_y, seed)
  }
  for (const road of roads) {
    seed = hashNumber(road.id, seed)
    seed = hashNumber(road.from_node + road.to_node, seed)
  }
  return seed >>> 0
}

function mulberry32(seed: number) {
  let value = seed >>> 0
  return () => {
    value += 0x6d2b79f5
    let result = value
    result = Math.imul(result ^ (result >>> 15), result | 1)
    result ^= result + Math.imul(result ^ (result >>> 7), result | 61)
    return ((result ^ (result >>> 14)) >>> 0) / 4294967296
  }
}

function clamp(value: number, min: number, max: number) {
  return Math.max(min, Math.min(max, value))
}

function calculateBounds(spot: Spot, nodes: MapNode[]): MapSceneBounds {
  const canvasWidth = spot.canvas_width || 1000
  const canvasHeight = spot.canvas_height || 700
  if (nodes.length === 0) {
    return { minX: 0, minY: 0, maxX: canvasWidth, maxY: canvasHeight }
  }

  const xs = nodes.map(node => node.pos_x)
  const ys = nodes.map(node => node.pos_y)
  const minX = Math.min(...xs)
  const maxX = Math.max(...xs)
  const minY = Math.min(...ys)
  const maxY = Math.max(...ys)
  const paddingX = Math.max(50, (maxX - minX) * 0.08)
  const paddingY = Math.max(45, (maxY - minY) * 0.1)

  return {
    minX: Math.min(0, minX - paddingX),
    minY: Math.min(0, minY - paddingY),
    maxX: Math.max(canvasWidth, maxX + paddingX),
    maxY: Math.max(canvasHeight, maxY + paddingY),
  }
}

function generateTerrain(
  spot: Spot,
  nodes: MapNode[],
  bounds: MapSceneBounds,
  seed: number,
): MapTerrainShape[] {
  const random = mulberry32(seed)
  const width = bounds.maxX - bounds.minX
  const height = bounds.maxY - bounds.minY
  const scenic = spot.type === 'scenic'
  const shapeCount = scenic ? 8 : 6
  const shapes: MapTerrainShape[] = []

  for (let index = 0; index < shapeCount; index++) {
    const anchor = nodes.length ? nodes[Math.floor(random() * nodes.length)] : undefined
    const centerX = clamp(
      (anchor?.pos_x ?? bounds.minX + random() * width) + (random() - 0.5) * width * 0.28,
      bounds.minX + width * 0.08,
      bounds.maxX - width * 0.08,
    )
    const centerY = clamp(
      (anchor?.pos_y ?? bounds.minY + random() * height) + (random() - 0.5) * height * 0.28,
      bounds.minY + height * 0.08,
      bounds.maxY - height * 0.08,
    )
    const radiusX = width * (0.08 + random() * 0.13)
    const radiusY = height * (0.07 + random() * 0.13)
    const pointCount = 7 + Math.floor(random() * 4)
    const points: Array<[number, number]> = []

    for (let pointIndex = 0; pointIndex < pointCount; pointIndex++) {
      const angle = (pointIndex / pointCount) * Math.PI * 2
      const wobble = 0.72 + random() * 0.42
      points.push([
        Math.round(clamp(centerX + Math.cos(angle) * radiusX * wobble, bounds.minX, bounds.maxX)),
        Math.round(clamp(centerY + Math.sin(angle) * radiusY * wobble, bounds.minY, bounds.maxY)),
      ])
    }

    const kind: MapTerrainShape['kind'] = scenic
      ? (index % 3 === 0 ? 'water' : index % 3 === 1 ? 'green' : 'district')
      : (index === 0 && seed % 2 === 0 ? 'water' : index % 2 === 0 ? 'district' : 'green')
    shapes.push({ kind, points })
  }

  return shapes
}

export function buildMapScene(spot: Spot, nodes: MapNode[], roads: Road[]): MapScene {
  const seed = sceneSeed(spot, nodes, roads)
  const bounds = calculateBounds(spot, nodes)
  const palettes = spot.type === 'scenic' ? scenicPalettes : campusPalettes

  return {
    signature: `${spot.id}-${seed.toString(16).padStart(8, '0')}`,
    bounds,
    palette: palettes[seed % palettes.length],
    terrain: generateTerrain(spot, nodes, bounds, seed),
    nodes: [...nodes],
    roads: [...roads],
  }
}
