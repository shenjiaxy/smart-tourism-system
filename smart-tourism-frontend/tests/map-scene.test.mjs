import assert from 'node:assert/strict'
import { readFile } from 'node:fs/promises'
import { buildMapScene } from '../src/utils/mapScene.ts'

const campus = {
  id: 1,
  name: '测试校园',
  type: 'campus',
  category: '综合大学',
  canvas_width: 1000,
  canvas_height: 700,
}

const scenic = {
  ...campus,
  id: 4,
  name: '测试景区',
  type: 'scenic',
  category: '自然风光',
}

const nodes = [
  { id: 1001, name: '东门', type: 'entrance', sub_type: 'gate', pos_x: 80, pos_y: 120 },
  { id: 1002, name: '教学楼', type: 'building', sub_type: 'teaching', pos_x: 420, pos_y: 260 },
  { id: 1003, name: '服务中心', type: 'facility', sub_type: 'service_center', pos_x: 860, pos_y: 590 },
]

const roads = Array.from({ length: 55 }, (_, index) => ({
  id: 5000 + index,
  from_node: nodes[index % nodes.length].id,
  to_node: nodes[(index + 1) % nodes.length].id,
  distance: 100 + index,
  congestion: 0.2 + (index % 5) * 0.1,
  ideal_speed: 1.4,
  transport: index % 3,
  is_bidirectional: true,
}))

const first = buildMapScene(campus, nodes, roads)
const repeated = buildMapScene(campus, nodes, roads)
const other = buildMapScene(scenic, nodes, roads)

assert.equal(first.signature, repeated.signature, 'one area should generate a stable scene')
assert.deepEqual(first.terrain, repeated.terrain, 'one area should keep the same terrain')
assert.notEqual(first.signature, other.signature, 'different areas should have different scene signatures')
assert.notDeepEqual(first.terrain, other.terrain, 'different areas should have different terrain')
assert.equal(first.roads.length, roads.length, 'scene should preserve every backend road')
assert.equal(first.nodes.length, nodes.length, 'scene should preserve every backend node')
assert.ok(first.bounds.minX < 80 && first.bounds.maxX > 860, 'bounds should pad backend node coordinates')
assert.ok(first.bounds.minY < 120 && first.bounds.maxY > 590, 'bounds should pad backend node coordinates')

const mapViewSource = await readFile(new URL('../src/views/MapView.vue', import.meta.url), 'utf8')
assert.doesNotMatch(mapViewSource, /graphRoads\.value\.slice\(0,\s*40\)/, 'MapView should not truncate roads')
assert.doesNotMatch(mapViewSource, /selectableNodes\.value\.slice\(0,\s*26\)/, 'MapView should not truncate nodes')
assert.match(mapViewSource, /buildMapScene/, 'MapView should render a generated backend scene')

console.log('map scene generation test passed')
