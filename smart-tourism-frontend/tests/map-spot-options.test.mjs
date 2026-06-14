import assert from 'node:assert/strict'
import { readFile } from 'node:fs/promises'

const mapViewUrl = new URL('../src/views/MapView.vue', import.meta.url)
const mapViewSource = await readFile(mapViewUrl, 'utf8')

let mergeMapSpotOptions
try {
  ;({ mergeMapSpotOptions } = await import('../src/utils/mapSpotOptions.ts'))
} catch {
  mergeMapSpotOptions = undefined
}

assert.equal(
  typeof mergeMapSpotOptions,
  'function',
  'map options should provide a merge helper',
)

const demo = { id: 900001, name: '北湖书院示范校园' }
const backendOptions = [
  { id: 1, name: '北京邮电大学' },
  { id: 2, name: '北京大学' },
  { id: 900001, name: '重复示范项' },
]

assert.deepEqual(
  mergeMapSpotOptions(demo, backendOptions),
  [demo, backendOptions[0], backendOptions[1]],
  'backend map options should remain visible without duplicating the demo option',
)

assert.match(
  mapViewSource,
  /onMounted\(async \(\) => \{[\s\S]*?await loadSpotOptions\(\)/,
  'MapView should load backend spot options when mounted',
)

console.log('map spot option initialization test passed')
