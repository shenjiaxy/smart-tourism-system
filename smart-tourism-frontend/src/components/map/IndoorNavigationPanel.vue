<template>
  <div class="indoor-layout">
    <aside class="indoor-panel">
      <div class="indoor-head">
        <p class="editorial-kicker">Indoor navigation</p>
        <h2>楼内导航</h2>
        <p>选择建筑与房间，规划大门、电梯、楼层和走廊路径。</p>
      </div>

      <div class="indoor-control">
        <label>景区 / 校园</label>
        <select v-model="areaId" @change="loadBuildings">
          <option v-for="area in areas" :key="area.id" :value="area.id">{{ area.name }}</option>
        </select>
      </div>

      <div class="indoor-control">
        <label>建筑</label>
        <select v-model="buildingId" @change="loadGraph">
          <option value="">请选择有室内数据的建筑</option>
          <option v-for="building in buildings" :key="building.id" :value="building.id">
            {{ building.name }} · {{ building.total_floors }}层
          </option>
        </select>
      </div>

      <div class="indoor-control">
        <label>规划策略</label>
        <div class="indoor-segmented">
          <button :class="{ active: strategy === 'distance' }" @click="strategy = 'distance'">最短距离</button>
          <button :class="{ active: strategy === 'time' }" @click="strategy = 'time'">最短时间</button>
        </div>
      </div>

      <div class="indoor-control">
        <label>起点</label>
        <select v-model="fromNodeId">
          <option value="">请选择起点</option>
          <option v-for="node in nodes" :key="node.id" :value="node.id">
            {{ node.floor }}F · {{ node.name }}
          </option>
        </select>
      </div>

      <div class="indoor-control">
        <label>终点</label>
        <select v-model="toNodeId">
          <option value="">请选择房间或设施</option>
          <option v-for="node in destinationNodes" :key="node.id" :value="node.id">
            {{ node.floor }}F · {{ node.name }}
          </option>
        </select>
      </div>

      <div class="indoor-actions">
        <button class="indoor-primary" :disabled="!canPlan || loading" @click="planRoute">
          {{ loading ? '规划中...' : '生成室内路线' }}
        </button>
        <button @click="resetRoute">清除路线</button>
      </div>

      <div v-if="route" class="indoor-result">
        <div class="indoor-metrics">
          <div><span>距离</span><strong>{{ Math.round(route.distance) }}m</strong></div>
          <div><span>预计</span><strong>{{ Math.max(1, Math.ceil(route.time / 60)) }}min</strong></div>
          <div><span>跨层</span><strong>{{ route.floor_changes }}次</strong></div>
        </div>
        <ol>
          <li v-for="(step, index) in route.steps" :key="`${index}-${step}`">{{ step }}</li>
        </ol>
      </div>
    </aside>

    <section class="indoor-stage">
      <div class="floor-tabs">
        <button
          v-for="floor in floors"
          :key="floor"
          :class="{ active: activeFloor === floor, routed: routeFloors.includes(floor) }"
          @click="activeFloor = floor; draw()"
        >
          {{ floor }}F
        </button>
      </div>
      <canvas ref="canvasRef" class="indoor-canvas" />
      <div v-if="!nodes.length" class="indoor-empty">
        <Building2 :size="44" />
        <strong>请选择建筑</strong>
        <span>楼层、房间和路线会显示在这里</span>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, onMounted, onUnmounted, ref, watch } from 'vue'
import { Building2 } from 'lucide-vue-next'
import { ElMessage } from 'element-plus'
import { getIndoorBuildings, getIndoorGraph, planIndoorRoute } from '@/api/route'
import type { IndoorBuilding, IndoorNode, IndoorRoad, IndoorRouteResult, Spot } from '@/types'

const props = defineProps<{
  areas: Spot[]
  initialAreaId?: number | string
}>()

const areaId = ref<number | string>(props.initialAreaId || '')
const buildingId = ref<number | string>('')
const buildings = ref<IndoorBuilding[]>([])
const nodes = ref<IndoorNode[]>([])
const roads = ref<IndoorRoad[]>([])
const fromNodeId = ref<number | string>('')
const toNodeId = ref<number | string>('')
const strategy = ref<'distance' | 'time'>('distance')
const activeFloor = ref(1)
const route = ref<IndoorRouteResult | null>(null)
const loading = ref(false)
const canvasRef = ref<HTMLCanvasElement | null>(null)
let resizeObserver: ResizeObserver | null = null

const floors = computed(() => [...new Set(nodes.value.map(node => node.floor))].sort((a, b) => a - b))
const routeFloors = computed(() => route.value ? [...new Set(route.value.floors)] : [])
const destinationNodes = computed(() => nodes.value.filter(node =>
  node.type === 'room' || node.type === 'facility' || node.type === 'entrance'
))
const canPlan = computed(() => Boolean(buildingId.value && fromNodeId.value && toNodeId.value))

watch(() => props.initialAreaId, value => {
  if (!areaId.value && value) {
    areaId.value = value
    loadBuildings()
  }
})
watch([nodes, roads, route, activeFloor], () => nextTick(draw), { deep: true })

async function loadBuildings() {
  buildings.value = []
  buildingId.value = ''
  nodes.value = []
  roads.value = []
  route.value = null
  if (!areaId.value) return
  const res = await getIndoorBuildings(Number(areaId.value))
  buildings.value = res.data || []
  if (buildings.value[0]) {
    buildingId.value = buildings.value[0].id
    await loadGraph()
  }
}

async function loadGraph() {
  resetRoute()
  if (!buildingId.value) return
  const res = await getIndoorGraph(Number(buildingId.value))
  nodes.value = res.data?.nodes || []
  roads.value = res.data?.roads || []
  activeFloor.value = floors.value[0] || 1
  const entrance = nodes.value.find(node => node.floor === 1 && node.type === 'entrance') || nodes.value[0]
  const room = [...nodes.value].reverse().find(node => node.type === 'room')
  fromNodeId.value = entrance?.id || ''
  toNodeId.value = room?.id || ''
  await nextTick()
  resizeCanvas()
}

async function planRoute() {
  if (!canPlan.value) return
  loading.value = true
  try {
    const res = await planIndoorRoute({
      building_id: Number(buildingId.value),
      from_node: Number(fromNodeId.value),
      to_node: Number(toNodeId.value),
      strategy: strategy.value,
    })
    if (!res.data?.found) {
      ElMessage.warning(res.data?.message || '未找到可达路线')
      return
    }
    route.value = res.data
    activeFloor.value = route.value.floors[0] || activeFloor.value
  } finally {
    loading.value = false
  }
}

function resetRoute() {
  route.value = null
}

function resizeCanvas() {
  const canvas = canvasRef.value
  const parent = canvas?.parentElement
  if (!canvas || !parent) return
  const rect = parent.getBoundingClientRect()
  const dpr = window.devicePixelRatio || 1
  canvas.width = Math.max(1, Math.floor(rect.width * dpr))
  canvas.height = Math.max(1, Math.floor(rect.height * dpr))
  canvas.style.width = `${Math.floor(rect.width)}px`
  canvas.style.height = `${Math.floor(rect.height)}px`
  canvas.getContext('2d')?.setTransform(dpr, 0, 0, dpr, 0, 0)
  draw()
}

function draw() {
  const canvas = canvasRef.value
  const ctx = canvas?.getContext('2d')
  if (!canvas || !ctx) return
  const rect = canvas.getBoundingClientRect()
  ctx.clearRect(0, 0, rect.width, rect.height)
  ctx.fillStyle = '#f7f8f5'
  ctx.fillRect(0, 0, rect.width, rect.height)
  if (!nodes.value.length) return

  const floorNodes = nodes.value.filter(node => node.floor === activeFloor.value)
  const minX = Math.min(...floorNodes.map(node => node.pos_x), 0)
  const maxX = Math.max(...floorNodes.map(node => node.pos_x), 1)
  const minY = Math.min(...floorNodes.map(node => node.pos_y), 0)
  const maxY = Math.max(...floorNodes.map(node => node.pos_y), 1)
  const padding = 80
  const scale = Math.min(
    (rect.width - padding * 2) / Math.max(1, maxX - minX),
    (rect.height - padding * 2) / Math.max(1, maxY - minY),
  )
  const point = (node: IndoorNode): [number, number] => [
    padding + (node.pos_x - minX) * scale,
    padding + (node.pos_y - minY) * scale,
  ]
  const nodeMap = new Map(nodes.value.map(node => [node.id, node]))
  const routeEdges = new Set<string>()
  if (route.value) {
    for (let i = 0; i + 1 < route.value.path.length; i++) {
      routeEdges.add(`${route.value.path[i]}-${route.value.path[i + 1]}`)
      routeEdges.add(`${route.value.path[i + 1]}-${route.value.path[i]}`)
    }
  }

  for (const road of roads.value) {
    const from = nodeMap.get(road.from_node)
    const to = nodeMap.get(road.to_node)
    if (!from || !to || from.floor !== activeFloor.value || to.floor !== activeFloor.value) continue
    const [x1, y1] = point(from)
    const [x2, y2] = point(to)
    const highlighted = routeEdges.has(`${from.id}-${to.id}`)
    ctx.strokeStyle = highlighted ? '#1f7a4d' : '#c8cfca'
    ctx.lineWidth = highlighted ? 6 : 2
    ctx.beginPath()
    ctx.moveTo(x1, y1)
    ctx.lineTo(x2, y2)
    ctx.stroke()
  }

  for (const node of floorNodes) {
    const [x, y] = point(node)
    const selected = route.value?.path.includes(node.id) || Number(fromNodeId.value) === node.id || Number(toNodeId.value) === node.id
    ctx.fillStyle = selected ? '#1f7a4d' : node.type === 'room' ? '#ffffff' : '#e8efe9'
    ctx.strokeStyle = selected ? '#155c39' : '#8fa297'
    ctx.lineWidth = selected ? 3 : 1.5
    ctx.beginPath()
    ctx.arc(x, y, node.type === 'room' ? 10 : 13, 0, Math.PI * 2)
    ctx.fill()
    ctx.stroke()
    ctx.fillStyle = '#24342b'
    ctx.font = '12px sans-serif'
    ctx.textAlign = 'center'
    ctx.fillText(node.name, x, y - 18)
  }

  ctx.fillStyle = '#506158'
  ctx.font = '700 13px sans-serif'
  ctx.textAlign = 'left'
  ctx.fillText(`${activeFloor.value}F 平面图`, 24, 34)
}

onMounted(async () => {
  if (!areaId.value && props.areas[0]) areaId.value = props.areas[0].id
  if (areaId.value) await loadBuildings()
  if (canvasRef.value?.parentElement) {
    resizeObserver = new ResizeObserver(resizeCanvas)
    resizeObserver.observe(canvasRef.value.parentElement)
  }
})

onUnmounted(() => resizeObserver?.disconnect())
</script>

<style scoped>
.indoor-layout {
  display: grid;
  grid-template-columns: 360px minmax(0, 1fr);
  gap: 22px;
  height: calc(100vh - 132px);
  padding: 0 24px 24px;
}

.indoor-panel,
.indoor-stage {
  border: 1px solid var(--color-rule);
  background: var(--color-surface);
}

.indoor-panel { overflow-y: auto; }
.indoor-head { padding: 22px 24px; border-bottom: 1px solid var(--color-rule); }
.indoor-head h2 { margin: 6px 0; font-size: 30px; }
.indoor-head p { color: var(--color-text-secondary); font-size: 13px; }
.indoor-control { display: grid; gap: 7px; padding: 16px 24px 0; }
.indoor-control label { color: var(--color-text-muted); font-size: 12px; font-weight: 800; }
.indoor-control select { min-height: 42px; padding: 0 10px; border: 1px solid var(--color-rule); background: var(--color-surface-muted); }
.indoor-segmented { display: grid; grid-template-columns: 1fr 1fr; border: 1px solid var(--color-rule); }
.indoor-segmented button { min-height: 40px; border: 0; background: var(--color-surface); font-weight: 700; cursor: pointer; }
.indoor-segmented button.active { color: white; background: var(--color-primary); }
.indoor-actions { display: grid; grid-template-columns: 1fr auto; gap: 8px; padding: 20px 24px; }
.indoor-actions button { min-height: 42px; padding: 0 15px; border: 1px solid var(--color-rule); background: var(--color-surface); cursor: pointer; }
.indoor-actions .indoor-primary { color: white; background: var(--color-ink); border-color: var(--color-ink); }
.indoor-actions button:disabled { opacity: .45; cursor: not-allowed; }
.indoor-result { padding: 18px 24px 24px; border-top: 1px solid var(--color-rule); background: var(--color-surface-muted); }
.indoor-metrics { display: grid; grid-template-columns: repeat(3, 1fr); gap: 1px; background: var(--color-rule); }
.indoor-metrics div { display: grid; gap: 3px; padding: 10px; background: var(--color-surface); }
.indoor-metrics span { color: var(--color-text-muted); font-size: 11px; }
.indoor-metrics strong { font-size: 18px; }
.indoor-result ol { display: grid; gap: 7px; margin: 15px 0 0; padding-left: 20px; color: var(--color-text-secondary); font-size: 13px; }
.indoor-stage { position: relative; overflow: hidden; }
.floor-tabs { position: absolute; z-index: 2; top: 18px; left: 18px; display: flex; gap: 5px; }
.floor-tabs button { min-width: 44px; height: 36px; border: 1px solid var(--color-rule); background: rgba(255,255,255,.9); font-weight: 800; cursor: pointer; }
.floor-tabs button.routed { border-bottom-color: var(--color-primary); }
.floor-tabs button.active { color: white; background: var(--color-primary); }
.indoor-canvas { display: block; width: 100%; height: 100%; }
.indoor-empty { position: absolute; inset: 0; display: grid; place-content: center; gap: 8px; text-align: center; color: var(--color-text-muted); }
.indoor-empty strong { color: var(--color-ink); }

@media (max-width: 980px) {
  .indoor-layout { grid-template-columns: 1fr; height: auto; padding: 0 16px 16px; }
  .indoor-stage { min-height: 560px; }
}
</style>
