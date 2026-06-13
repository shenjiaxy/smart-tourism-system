<template>
  <div class="route-workspace fade-in">
    <div class="route-mode-switch" aria-label="导航模式">
      <button :class="{ active: navigationMode === 'outdoor' }" @click="navigationMode = 'outdoor'">室外导航</button>
      <button :class="{ active: navigationMode === 'indoor' }" @click="navigationMode = 'indoor'">室内导航</button>
    </div>

    <div v-if="navigationMode === 'outdoor'" class="map-page">
    <aside class="planner-panel">
      <div class="panel-head">
        <p class="editorial-kicker">Route Atelier</p>
        <h1 class="editorial-heading">路线规划</h1>
        <p>先用前端母版确定校园/景区地图，再把建筑、道路和距离数据反写到后端。</p>
      </div>

      <div class="control-group">
        <label>选择景区 / 校园</label>
        <select v-model="selectedAreaId" @change="loadGraphData">
          <option value="">请选择区域...</option>
          <option v-for="s in spotOptions" :key="s.id" :value="s.id">{{ s.name }}</option>
        </select>
      </div>

      <div class="control-group">
        <label>规划策略</label>
        <div class="segmented">
          <button
            v-for="s in strategies"
            :key="s.value"
            :class="{ active: strategy === s.value }"
            @click="strategy = s.value"
          >
            {{ s.label }}
          </button>
        </div>
      </div>

      <div class="control-group">
        <label>出行方式</label>
        <div class="transport-options">
          <button
            v-for="option in transportOptions"
            :key="option.value"
            :class="{ active: transportChoice === option.value }"
            :disabled="strategy === 'mixed' && option.value !== 'mixed'"
            @click="transportChoice = option.value"
          >
            {{ option.label }}
          </button>
        </div>
      </div>

      <div class="control-group" v-if="graphNodes.length">
        <label>节点选择</label>
        <select v-model="dropdownNodeId" @change="onDropdownSelect">
          <option value="">选择节点作为下一步...</option>
          <option v-for="n in selectableNodes" :key="n.id" :value="n.id">
            {{ n.name }} - {{ nodeTypeLabel(n.type, n.sub_type) }}
          </option>
        </select>
      </div>

      <div class="steps">
        <div class="step" :class="{ done: startNode }">
          <span>01</span>
          <div>
            <strong>起点</strong>
            <p>{{ startNode?.name || '点击地图或下拉选择起点' }}</p>
          </div>
        </div>
        <div class="step" :class="{ done: endNode }">
          <span>02</span>
          <div>
            <strong>终点</strong>
            <p>{{ endNode?.name || '继续选择终点' }}</p>
          </div>
        </div>
        <div class="step" :class="{ done: waypoints.length }">
          <span>03</span>
          <div>
            <strong>途经点</strong>
            <p>{{ waypoints.length ? waypoints.map(n => n.name).join(' / ') : '可选，终点后继续点击添加' }}</p>
          </div>
        </div>
      </div>

      <div class="panel-actions">
        <button class="primary-btn" :disabled="!canPlan || loading" @click="planRoute">
          {{ loading ? '生成中...' : (waypoints.length ? '生成多点路线' : '生成最短路线') }}
        </button>
        <div class="action-row">
          <button @click="clearAll">重置</button>
          <button :disabled="waypoints.length === 0" @click="undoLastWaypoint">撤销途经点</button>
        </div>
      </div>

      <div v-if="routeResult" class="route-summary">
        <p class="editorial-kicker">Route result</p>
        <div class="summary-grid">
          <div><span>距离</span><strong>{{ routeDistanceLabel }}</strong></div>
          <div><span>时间</span><strong>{{ routeTime }}min</strong></div>
        </div>
        <div v-if="routeTransfers.length" class="transfer-list">
          <span v-for="transfer in routeTransfers" :key="`${transfer.node_id}-${transfer.to_transport}`">
            {{ transfer.node_name }}：{{ transportLabel(transfer.from_transport) }}转{{ transportLabel(transfer.to_transport) }}
          </span>
        </div>
        <p class="route-names">{{ routeNodeNames.join(' -> ') }}</p>
      </div>
    </aside>

    <section class="map-stage">
      <div class="map-toolbar">
        <div>
          <span>{{ selectedAreaName || '未选择区域' }}</span>
          <strong>{{ hintText }}</strong>
        </div>
        <div class="zoom-controls">
          <button @click="zoomBy(1.18)">+</button>
          <button @click="zoomBy(0.84)">-</button>
          <button @click="resetView">复位</button>
        </div>
      </div>

      <canvas
        ref="canvasRef"
        class="map-canvas"
        @click="handleCanvasClick"
        @mousemove="handleCanvasMouseMove"
        @mousedown="startPan"
        @mouseup="endPan"
        @mouseleave="endPan"
        @wheel.prevent="handleWheel"
        @dblclick.prevent="focusHovered"
      />

      <div v-if="false" class="map-empty">
        <MapIcon :size="56" />
        <strong>选择一个景区或校园开始</strong>
        <p>地图将展示建筑轮廓、道路层级、步道、标注和路径动画。</p>
      </div>
    </section>
    </div>

    <IndoorNavigationPanel
      v-else
      :areas="spotOptions"
      :initial-area-id="selectedAreaId"
    />
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, onMounted, onUnmounted, ref, watch } from 'vue'
import { Map as MapIcon } from 'lucide-vue-next'
import { getGraphData, getSpotOptions } from '@/api/spot'
import { planMultiRoute, planSingleRoute } from '@/api/route'
import IndoorNavigationPanel from '@/components/map/IndoorNavigationPanel.vue'
import type { MapNode, MultiRouteResult, Road, SingleRouteResult, Spot } from '@/types'

const canvasRef = ref<HTMLCanvasElement | null>(null)
let ctx: CanvasRenderingContext2D | null = null
let animationFrame = 0
let routeAnimationStart = 0

const spotOptions = ref<Spot[]>([])
const selectedAreaId = ref<number | string>('')
const graphNodes = ref<MapNode[]>([])
const graphRoads = ref<Road[]>([])
const startNode = ref<MapNode | null>(null)
const endNode = ref<MapNode | null>(null)
const waypoints = ref<MapNode[]>([])
const hoveredNode = ref<MapNode | null>(null)
const dropdownNodeId = ref<number | string>('')
const strategy = ref<'distance' | 'time' | 'mixed'>('distance')
const routeResult = ref<SingleRouteResult | MultiRouteResult | null>(null)
const loading = ref(false)
const navigationMode = ref<'outdoor' | 'indoor'>('outdoor')
const transportChoice = ref<'walk' | 'bike' | 'shuttle' | 'mixed'>('walk')

const view = {
  zoom: 1,
  targetZoom: 1,
  panX: 0,
  panY: 0,
  targetPanX: 0,
  targetPanY: 0,
  baseScale: 1,
  baseX: 0,
  baseY: 0,
}

const pointer = {
  dragging: false,
  lastX: 0,
  lastY: 0,
  moved: false,
}

const strategies = [
  { label: '最短距离', value: 'distance' as const },
  { label: '最短时间', value: 'time' as const },
  { label: '混合策略', value: 'mixed' as const },
]

const selectedAreaName = computed(() =>
  spotOptions.value.find(s => String(s.id) === String(selectedAreaId.value))?.name || ''
)

const selectedArea = computed(() =>
  spotOptions.value.find(s => String(s.id) === String(selectedAreaId.value))
)

const transportOptions = computed(() => {
  const vehicle = selectedArea.value?.type === 'campus'
    ? { label: '自行车', value: 'bike' as const }
    : { label: '电瓶车', value: 'shuttle' as const }
  return [
    { label: '步行', value: 'walk' as const },
    vehicle,
    { label: '混合', value: 'mixed' as const },
  ]
})

const transportMask = computed(() => {
  if (transportChoice.value === 'walk') return 0x1
  if (transportChoice.value === 'bike') return 0x2
  if (transportChoice.value === 'shuttle') return 0x4
  return selectedArea.value?.type === 'campus' ? 0x3 : 0x5
})

watch(strategy, value => {
  if (value === 'mixed') transportChoice.value = 'mixed'
})

watch(selectedAreaId, () => {
  if (transportChoice.value === 'bike' && selectedArea.value?.type !== 'campus') transportChoice.value = 'walk'
  if (transportChoice.value === 'shuttle' && selectedArea.value?.type === 'campus') transportChoice.value = 'walk'
})

type DemoPoint = [number, number]
type DemoBuilding = {
  name: string
  kind: string
  points: DemoPoint[]
  label: DemoPoint
}
const DEMO_AREA_ID = 900001

const demoWaterShapes: DemoPoint[][] = [
  [[-80, 620], [48, 584], [148, 618], [212, 698], [198, 806], [96, 884], [-80, 918]],
  [[382, 294], [494, 236], [612, 286], [658, 414], [604, 528], [474, 548], [372, 478], [342, 366]],
]

const demoLandShapes: DemoPoint[][] = [
  [[62, 68], [330, 30], [380, 210], [282, 352], [94, 310]],
  [[650, 74], [936, 42], [1030, 198], [952, 338], [712, 302]],
  [[668, 456], [954, 430], [1040, 588], [958, 766], [724, 710]],
  [[162, 474], [330, 456], [366, 600], [244, 708], [126, 630]],
]

const demoMajorRoads: DemoPoint[][] = [
  [[82, 780], [186, 656], [266, 512], [328, 378], [424, 260], [560, 214], [728, 226], [906, 286], [1060, 348]],
  [[-40, 410], [128, 386], [300, 390], [452, 428], [616, 476], [810, 490], [1060, 458]],
  [[256, -44], [314, 112], [338, 270], [332, 430], [294, 594], [308, 762], [372, 940]],
  [[594, -42], [634, 112], [654, 274], [660, 436], [694, 608], [762, 786], [894, 940]],
]

const demoMinorRoads: DemoPoint[][] = [
  [[132, 174], [242, 150], [336, 176], [420, 176], [522, 150]],
  [[142, 250], [258, 238], [360, 266], [486, 248], [596, 284]],
  [[118, 332], [246, 318], [360, 346], [492, 336], [606, 362]],
  [[408, 598], [518, 586], [632, 620], [752, 600], [878, 618]],
  [[394, 688], [516, 700], [642, 720], [768, 700], [926, 716]],
  [[706, 120], [820, 112], [920, 156], [1000, 142]],
  [[720, 206], [838, 214], [944, 252], [1028, 238]],
  [[710, 304], [812, 340], [934, 330], [1034, 364]],
  [[746, 520], [842, 546], [962, 536], [1042, 574]],
  [[738, 648], [854, 640], [972, 672], [1046, 654]],
  [[208, 522], [284, 566], [346, 638], [448, 662]],
  [[170, 700], [246, 742], [350, 760], [464, 800]],
]

const demoFootpaths: DemoPoint[][] = [
  [[360, 356], [418, 308], [500, 298], [574, 342], [604, 420], [558, 494], [474, 512], [394, 470], [360, 390]],
  [[430, 252], [470, 186], [550, 164], [628, 204], [660, 286]],
  [[342, 454], [258, 438], [178, 472], [118, 548], [96, 650]],
  [[606, 456], [700, 408], [804, 390], [920, 410], [1008, 474]],
  [[482, 548], [470, 638], [522, 704], [622, 732], [720, 700]],
  [[166, 120], [204, 206], [194, 300], [238, 382], [238, 494]],
  [[798, 92], [832, 156], [820, 244], [868, 336], [862, 452]],
]

const demoDistrictRoads: DemoPoint[][] = [
  [[728, 84], [782, 56], [840, 86], [800, 134], [858, 172], [934, 148]],
  [[762, 250], [834, 236], [902, 270], [870, 324], [958, 344]],
  [[748, 514], [820, 486], [900, 520], [982, 502], [1038, 548]],
  [[790, 604], [886, 622], [968, 604], [1048, 648]],
]

const demoBuildings: DemoBuilding[] = [
  { name: '北门游客中心', kind: 'visitor', points: [[124, 82], [196, 64], [226, 88], [218, 132], [162, 148], [134, 126]], label: [132, 62] },
  { name: '行政楼', kind: 'admin', points: [[282, 88], [374, 78], [416, 100], [422, 154], [354, 158], [352, 128], [294, 134]], label: [308, 72] },
  { name: '明德教学楼', kind: 'teaching', points: [[126, 206], [236, 184], [270, 214], [262, 278], [176, 290], [134, 266]], label: [142, 188] },
  { name: '格物教学楼', kind: 'teaching', points: [[310, 204], [430, 204], [462, 232], [450, 292], [342, 286], [306, 258]], label: [326, 194] },
  { name: '校史馆', kind: 'museum', points: [[520, 94], [588, 102], [620, 132], [602, 178], [540, 166], [510, 136]], label: [524, 82] },
  { name: '中心图书馆', kind: 'library', points: [[386, 558], [502, 548], [560, 584], [566, 654], [462, 668], [398, 636]], label: [414, 540] },
  { name: '书院广场', kind: 'plaza', points: [[518, 390], [604, 382], [656, 446], [622, 510], [532, 500], [496, 438]], label: [536, 378] },
  { name: '学生活动中心', kind: 'activity', points: [[642, 132], [714, 108], [752, 150], [744, 202], [670, 214], [638, 176]], label: [650, 100] },
  { name: '东区教学组团', kind: 'teaching', points: [[786, 148], [900, 154], [952, 192], [936, 252], [820, 242], [778, 208]], label: [804, 132] },
  { name: '实验楼群', kind: 'research', points: [[716, 300], [820, 322], [856, 360], [834, 424], [730, 394], [704, 350]], label: [730, 286] },
  { name: '艺术中心', kind: 'arts', points: [[892, 314], [980, 328], [1002, 368], [984, 424], [904, 404], [870, 358]], label: [902, 300] },
  { name: '体育馆', kind: 'gym', points: [[716, 532], [806, 510], [856, 548], [868, 624], [780, 652], [738, 620]], label: [742, 506] },
  { name: '第一食堂', kind: 'canteen', points: [[882, 540], [960, 548], [998, 590], [976, 638], [888, 626], [858, 584]], label: [900, 526] },
  { name: '南区宿舍 A', kind: 'dorm', points: [[744, 674], [828, 666], [858, 704], [846, 754], [764, 764], [734, 724]], label: [748, 660] },
  { name: '南区宿舍 B', kind: 'dorm', points: [[878, 690], [972, 702], [1002, 742], [988, 794], [890, 780], [860, 734]], label: [890, 674] },
  { name: '西门服务站', kind: 'facility', points: [[114, 506], [174, 488], [202, 522], [188, 558], [126, 568], [104, 538]], label: [112, 486] },
]

const demoNodes: MapNode[] = demoBuildings.map((building, index) => {
  const [x, y] = polygonCenter(building.points)
  const nodeType = building.kind === 'facility' ? 'facility' : 'building'
  return {
    id: DEMO_AREA_ID + index + 1,
    area_id: DEMO_AREA_ID,
    name: building.name,
    type: nodeType,
    sub_type: building.kind,
    pos_x: x,
    pos_y: y,
    description: `北湖书院示范地图节点：${building.name}`,
  }
})

const demoJunctionNodes: MapNode[] = [
  { id: 900101, area_id: DEMO_AREA_ID, name: '北门环路口', type: 'junction', sub_type: 'road', pos_x: 260, pos_y: 180, description: '北湖书院道路路口' },
  { id: 900102, area_id: DEMO_AREA_ID, name: '校史馆路口', type: 'junction', sub_type: 'road', pos_x: 470, pos_y: 205, description: '北湖书院道路路口' },
  { id: 900103, area_id: DEMO_AREA_ID, name: '东区北路口', type: 'junction', sub_type: 'road', pos_x: 760, pos_y: 215, description: '北湖书院道路路口' },
  { id: 900104, area_id: DEMO_AREA_ID, name: '东区中路口', type: 'junction', sub_type: 'road', pos_x: 875, pos_y: 365, description: '北湖书院道路路口' },
  { id: 900105, area_id: DEMO_AREA_ID, name: '体育馆东路口', type: 'junction', sub_type: 'road', pos_x: 845, pos_y: 560, description: '北湖书院道路路口' },
  { id: 900106, area_id: DEMO_AREA_ID, name: '南区宿舍路口', type: 'junction', sub_type: 'road', pos_x: 700, pos_y: 650, description: '北湖书院道路路口' },
  { id: 900107, area_id: DEMO_AREA_ID, name: '图书馆南路口', type: 'junction', sub_type: 'road', pos_x: 555, pos_y: 610, description: '北湖书院道路路口' },
  { id: 900108, area_id: DEMO_AREA_ID, name: '教学核心路口', type: 'junction', sub_type: 'road', pos_x: 340, pos_y: 390, description: '北湖书院道路路口' },
  { id: 900109, area_id: DEMO_AREA_ID, name: '北湖东路口', type: 'junction', sub_type: 'road', pos_x: 605, pos_y: 420, description: '北湖书院道路路口' },
  { id: 900110, area_id: DEMO_AREA_ID, name: '西门环路口', type: 'junction', sub_type: 'road', pos_x: 190, pos_y: 360, description: '北湖书院道路路口' },
]

const demoGraphNodes = [...demoNodes, ...demoJunctionNodes]

const demoRoadConnections = [
  [900002, 900101],
  [900003, 900101], [900003, 900102],
  [900004, 900110], [900004, 900108],
  [900005, 900108], [900005, 900102],
  [900006, 900102],
  [900007, 900107],
  [900008, 900109], [900008, 900107],
  [900009, 900103],
  [900010, 900103], [900010, 900104],
  [900011, 900104], [900011, 900109],
  [900012, 900104],
  [900013, 900105], [900013, 900106],
  [900014, 900105],
  [900015, 900106],
  [900016, 900105], [900016, 900106],
  [900017, 900110], [900017, 900108],
  [900101, 900102], [900102, 900103], [900103, 900104],
  [900104, 900105], [900105, 900106], [900106, 900107],
  [900107, 900108], [900108, 900110], [900102, 900109],
  [900108, 900109], [900109, 900104], [900109, 900107],
]

const demoRoads: Road[] = demoRoadConnections.map(([fromId, toId], index) => {
  const from = demoGraphNodes.find(node => node.id === fromId)!
  const to = demoGraphNodes.find(node => node.id === toId)!
  return {
    id: DEMO_AREA_ID + index + 1,
    area_id: DEMO_AREA_ID,
    from_node: from.id,
    to_node: to.id,
    distance: Math.round(Math.hypot(from.pos_x - to.pos_x, from.pos_y - to.pos_y)),
    congestion: 1,
    ideal_speed: 1.25,
    transport: index % 3 === 0 ? 2 : 1,
    is_bidirectional: true,
  }
})

const selectableNodes = computed(() =>
  graphNodes.value.filter(n => n.type !== 'junction')
)

const canPlan = computed(() =>
  Boolean(selectedAreaId.value && startNode.value && endNode.value)
)

const hintText = computed(() => {
  if (!selectedAreaId.value) return '等待选择区域'
  if (!startNode.value) return '点击建筑或入口选择起点'
  if (!endNode.value) return '继续选择终点'
  if (routeResult.value) return '路线已生成，可以缩放拖拽查看细节'
  return '可继续添加途经点，或直接生成路线'
})

const routeNodeNames = computed(() => {
  if (!routeResult.value) return []
  const r = routeResult.value
  if (r.path_names?.length) return r.path_names
  if (r.node_names?.length) return r.node_names
  return r.path.map((id: number) => graphNodes.value.find(node => node.id === id)?.name || `#${id}`)
})

const routeDistance = computed(() => {
  if (!routeResult.value) return 0
  const r = routeResult.value
  return Math.round(r.total_distance ?? r.distance ?? 0)
})

const routeDistanceLabel = computed(() => formatDistance(routeDistance.value))

const routeTime = computed(() => {
  if (!routeResult.value) return 0
  const r = routeResult.value
  return Math.round((r.total_time ?? r.time ?? 0) / 60)
})

const routePathIds = computed(() => routeResult.value?.path || [])
const routeTransportModes = computed(() =>
  (routeResult.value as SingleRouteResult | null)?.transport_modes || []
)
const routeTransfers = computed(() =>
  (routeResult.value as SingleRouteResult | null)?.transfers || []
)

function transportLabel(mode: number) {
  if (mode === 1) return '自行车'
  if (mode === 2) return '电瓶车'
  return '步行'
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
  ctx = canvas.getContext('2d')
  ctx?.setTransform(dpr, 0, 0, dpr, 0, 0)
  calcBaseTransform()
}

function calcBaseTransform() {
  if (isDemoArea() && canvasRef.value) {
    const rect = canvasRef.value.getBoundingClientRect()
    const bounds = { minX: 68, minY: 42, maxX: 1042, maxY: 842 }
    const mapWidth = bounds.maxX - bounds.minX
    const mapHeight = bounds.maxY - bounds.minY
    const fitWidth = (rect.width - 26) / mapWidth
    const fitHeight = (rect.height - 88) / mapHeight
    view.baseScale = Math.max(fitWidth, fitHeight)
    view.baseX = (rect.width - mapWidth * view.baseScale) / 2 - bounds.minX * view.baseScale
    view.baseY = (rect.height - mapHeight * view.baseScale) / 2 - bounds.minY * view.baseScale + 8
    return
  }
  if (!graphNodes.value.length || !canvasRef.value) return
  const rect = canvasRef.value.getBoundingClientRect()
  const xs = graphNodes.value.map(n => n.pos_x)
  const ys = graphNodes.value.map(n => n.pos_y)
  const minX = Math.min(...xs)
  const maxX = Math.max(...xs)
  const minY = Math.min(...ys)
  const maxY = Math.max(...ys)
  const rangeX = maxX - minX || 1
  const rangeY = maxY - minY || 1
  view.baseScale = Math.min((rect.width - 120) / rangeX, (rect.height - 120) / rangeY)
  view.baseX = (rect.width - rangeX * view.baseScale) / 2 - minX * view.baseScale
  view.baseY = (rect.height - rangeY * view.baseScale) / 2 - minY * view.baseScale
}

function toScreen(x: number, y: number): [number, number] {
  return [
    x * view.baseScale * view.zoom + view.baseX * view.zoom + view.panX,
    y * view.baseScale * view.zoom + view.baseY * view.zoom + view.panY,
  ]
}

function isDemoArea() {
  return Number(selectedAreaId.value) === DEMO_AREA_ID
}

function draw() {
  const canvas = canvasRef.value
  if (!ctx || !canvas) return
  const rect = canvas.getBoundingClientRect()
  ctx.clearRect(0, 0, rect.width, rect.height)
  drawTerrain(rect.width, rect.height)
  drawRoads()
  drawRoute()
  drawNodes()
  drawLegend(rect.width, rect.height)
}

function drawTerrain(w: number, h: number) {
  if (!ctx) return
  ctx.fillStyle = '#f5f3ee'
  ctx.fillRect(0, 0, w, h)

  ctx.strokeStyle = 'rgba(90, 104, 111, 0.014)'
  ctx.lineWidth = 1
  for (let x = -80 + (view.panX % 120); x < w + 80; x += 120) {
    ctx.beginPath()
    ctx.moveTo(x, 0)
    ctx.lineTo(x, h)
    ctx.stroke()
  }
  for (let y = -80 + (view.panY % 120); y < h + 80; y += 120) {
    ctx.beginPath()
    ctx.moveTo(0, y)
    ctx.lineTo(w, y)
    ctx.stroke()
  }

  ctx.save()
  ctx.translate(view.baseX * view.zoom + view.panX, view.baseY * view.zoom + view.panY)
  ctx.scale(view.baseScale * view.zoom, view.baseScale * view.zoom)
  const lineAdjust = 1 / Math.max(view.baseScale, 0.1)

  drawMapPolygons(demoWaterShapes, '#cfe8f7', '#b7d9eb', 1.2)
  drawMapPolygons(demoLandShapes, '#dcebd4', '#c8ddbe', 0.8)
  drawMapLineSet(demoFootpaths, '#8d9a9a', 1.4 * lineAdjust, [5, 7])
  drawRoadLayer(demoMajorRoads, 14 * lineAdjust, '#cfd4d5', 10 * lineAdjust, '#ffffff')
  drawRoadLayer(demoMinorRoads, 5.5 * lineAdjust, '#c9ced1', 3.6 * lineAdjust, '#ffffff')
  drawRoadLayer(demoDistrictRoads, 6.5 * lineAdjust, '#c2c8cb', 4.4 * lineAdjust, '#ffffff')
  drawDemoBuildings()
  drawMapLabels()
  ctx.restore()
}

function drawRoads() {
  if (!ctx) return
  if (isDemoArea()) return
  const faintRoads = graphRoads.value.slice(0, 40)
  const nodeMap = new Map(graphNodes.value.map(n => [n.id, n]))
  ctx.save()
  ctx.lineCap = 'round'
  ctx.lineJoin = 'round'
  ctx.strokeStyle = 'rgba(20,20,20,0.18)'
  ctx.lineWidth = 1
  for (const road of faintRoads) {
    const from = nodeMap.get(road.from_node)
    const to = nodeMap.get(road.to_node)
    if (!from || !to) continue
    const [x1, y1] = toScreen(from.pos_x, from.pos_y)
    const [x2, y2] = toScreen(to.pos_x, to.pos_y)
    drawCurvedSegment(x1, y1, x2, y2, (road.id % 5 - 2) * 12)
    ctx.stroke()
  }
  ctx.restore()
}

function drawRoute() {
  if (!ctx || routePathIds.value.length < 2) return
  const points = routePathIds.value
    .map(id => graphNodes.value.find(n => n.id === id))
    .filter(Boolean)
    .map(n => toScreen(n!.pos_x, n!.pos_y))
  if (points.length < 2) return

  const progress = Math.min(1, routeAnimationStart ? (performance.now() - routeAnimationStart) / 1200 : 1)
  const totalSegments = points.length - 1
  const visible = progress * totalSegments

  ctx.save()
  ctx.lineCap = 'round'
  ctx.lineJoin = 'round'
  ctx.lineWidth = 7
  ctx.shadowColor = 'rgba(26, 115, 232, 0.22)'
  ctx.shadowBlur = 8
  const modeColors = ['#1a73e8', '#18864b', '#d97706']
  for (let i = 0; i < totalSegments; i++) {
    const [x1, y1] = points[i]
    const [x2, y2] = points[i + 1]
    const mode = routeTransportModes.value[i] ?? 0
    if (x1 === x2 && y1 === y2) {
      if (i <= visible) {
        ctx.strokeStyle = modeColors[routeTransportModes.value[i + 1] ?? mode] || modeColors[0]
        ctx.lineWidth = 3
        ctx.beginPath()
        ctx.arc(x1, y1, 12, 0, Math.PI * 2)
        ctx.stroke()
        ctx.lineWidth = 7
      }
      continue
    }
    if (i >= visible) break
    const t = Math.min(1, visible - i)
    ctx.strokeStyle = modeColors[mode] || modeColors[0]
    ctx.beginPath()
    ctx.moveTo(x1, y1)
    ctx.lineTo(x1 + (x2 - x1) * t, y1 + (y2 - y1) * t)
    ctx.stroke()
  }
  ctx.restore()

  if (progress < 1) requestDraw()
}

function drawNodes() {
  if (!ctx) return
  const sorted = selectableNodes.value.slice(0, 26)
  for (const node of sorted) {
    const [x, y] = toScreen(node.pos_x, node.pos_y)
    const isSelected = node.id === startNode.value?.id || node.id === endNode.value?.id || waypoints.value.some(w => w.id === node.id)
    const isHovered = hoveredNode.value?.id === node.id
    drawMapPoi(x, y, node, isSelected, isHovered)
  }
}

function drawMapPolygons(polygons: DemoPoint[][], fill: string, stroke: string, lineWidth: number) {
  if (!ctx) return
  const c = ctx
  c.fillStyle = fill
  c.strokeStyle = stroke
  c.lineWidth = lineWidth
  for (const points of polygons) {
    c.beginPath()
    points.forEach(([x, y], index) => {
      if (index === 0) c.moveTo(x, y)
      else {
        const [px, py] = points[index - 1]
        const mx = (px + x) / 2
        const my = (py + y) / 2
        c.quadraticCurveTo(px, py, mx, my)
      }
    })
    c.closePath()
    c.fill()
    c.stroke()
  }
}

function drawDemoBuildings() {
  if (!ctx) return
  const c = ctx
  for (const building of demoBuildings) {
    c.save()
    const center = polygonCenter(building.points)
    c.shadowColor = 'rgba(60, 64, 67, 0.08)'
    c.shadowBlur = building.kind === 'plaza' ? 0 : 3
    c.shadowOffsetX = 0
    c.shadowOffsetY = 1
    c.fillStyle = buildingFill(building.kind)
    c.strokeStyle = building.kind === 'plaza' ? '#b8c7b0' : '#d2cec4'
    c.lineWidth = building.kind === 'plaza' ? 1 : 0.9
    c.setLineDash(building.kind === 'plaza' ? [7, 5] : [])
    drawStraightPolygon(building.points)
    c.fill()
    c.stroke()
    c.shadowColor = 'transparent'
    c.setLineDash([])
    drawRoofDetails(building, center)
    drawEntranceMark(building.points, center)
    c.font = '600 13px "Noto Sans SC", sans-serif'
    c.fillStyle = '#4f5b62'
    drawTextHalo(building.name, building.label[0], building.label[1])
    c.font = '500 9px "Noto Sans SC", sans-serif'
    c.fillStyle = 'rgba(95, 99, 104, 0.72)'
    drawTextHalo(buildingKindLabel(building.kind), building.label[0], building.label[1] + 14)
    c.restore()
  }
}

function drawSoftPolygon(points: DemoPoint[]) {
  if (!ctx || points.length === 0) return
  if (points.length < 3) return
  const midpoint = (a: DemoPoint, b: DemoPoint): DemoPoint => [(a[0] + b[0]) / 2, (a[1] + b[1]) / 2]
  const first = midpoint(points[points.length - 1], points[0])
  ctx.beginPath()
  ctx.moveTo(first[0], first[1])
  for (let i = 0; i < points.length; i++) {
    const current = points[i]
    const next = points[(i + 1) % points.length]
    const mid = midpoint(current, next)
    ctx.quadraticCurveTo(current[0], current[1], mid[0], mid[1])
  }
  ctx.closePath()
}

function drawStraightPolygon(points: DemoPoint[]) {
  if (!ctx || points.length === 0) return
  ctx.beginPath()
  ctx.moveTo(points[0][0], points[0][1])
  for (const [x, y] of points.slice(1)) ctx.lineTo(x, y)
  ctx.closePath()
}

function drawRoofDetails(building: DemoBuilding, center: DemoPoint) {
  if (!ctx || building.kind === 'plaza') return
  const c = ctx
  const inner = building.points.map(([x, y]) => [center[0] + (x - center[0]) * 0.72, center[1] + (y - center[1]) * 0.72] as DemoPoint)
  c.save()
  c.strokeStyle = 'rgba(95, 99, 104, 0.20)'
  c.lineWidth = 0.8
  c.setLineDash([])
  drawSoftPolygon(inner)
  c.stroke()

  const [minX, minY, maxX, maxY] = polygonBounds(building.points)
  if (building.kind === 'library' || building.kind === 'museum' || building.kind === 'activity') {
    c.strokeStyle = 'rgba(95, 99, 104, 0.26)'
    c.strokeRect(minX + (maxX - minX) * 0.34, minY + (maxY - minY) * 0.3, (maxX - minX) * 0.28, (maxY - minY) * 0.34)
  } else if (building.kind === 'gym') {
    c.beginPath()
    c.ellipse(center[0], center[1], (maxX - minX) * 0.24, (maxY - minY) * 0.22, -0.1, 0, Math.PI * 2)
    c.stroke()
  } else {
    c.beginPath()
    c.moveTo(minX + 12, center[1] - 6)
    c.lineTo(maxX - 12, center[1] + 4)
    c.moveTo(center[0] - 16, minY + 10)
    c.lineTo(center[0] + 10, maxY - 10)
    c.stroke()
  }
  c.restore()
}

function drawEntranceMark(points: DemoPoint[], center: DemoPoint) {
  if (!ctx) return
  let bestA = points[0]
  let bestB = points[1]
  let bestY = -Infinity
  for (let i = 0; i < points.length; i++) {
    const a = points[i]
    const b = points[(i + 1) % points.length]
    const y = (a[1] + b[1]) / 2
    if (y > bestY) {
      bestY = y
      bestA = a
      bestB = b
    }
  }
  const mx = (bestA[0] + bestB[0]) / 2
  const my = (bestA[1] + bestB[1]) / 2
  const dx = bestB[0] - bestA[0]
  const dy = bestB[1] - bestA[1]
  const len = Math.hypot(dx, dy) || 1
  const nx = (center[0] - mx) / Math.hypot(center[0] - mx, center[1] - my)
  const ny = (center[1] - my) / Math.hypot(center[0] - mx, center[1] - my)
  ctx.save()
  ctx.strokeStyle = '#879097'
  ctx.lineWidth = 1.7
  ctx.beginPath()
  ctx.moveTo(mx - (dx / len) * 8, my - (dy / len) * 8)
  ctx.lineTo(mx + (dx / len) * 8, my + (dy / len) * 8)
  ctx.stroke()
  ctx.strokeStyle = 'rgba(95,99,104,0.24)'
  ctx.lineWidth = 1
  ctx.beginPath()
  ctx.moveTo(mx, my)
  ctx.lineTo(mx - nx * 18, my - ny * 18)
  ctx.stroke()
  ctx.restore()
}

function buildingFill(kind: string) {
  const fills: Record<string, string> = {
    visitor: '#f2eee3',
    admin: '#eee9df',
    teaching: '#f0ede5',
    museum: '#efe6d8',
    library: '#f4ead6',
    plaza: 'rgba(255,255,255,0.62)',
    activity: '#ece8dd',
    research: '#eeeae2',
    arts: '#f1e7d8',
    gym: '#e9ece4',
    canteen: '#f1e6d6',
    dorm: '#e9e5dc',
    facility: '#f0ece3',
  }
  return fills[kind] || '#ece8df'
}

function polygonBounds(points: DemoPoint[]) {
  const xs = points.map(([x]) => x)
  const ys = points.map(([, y]) => y)
  return [Math.min(...xs), Math.min(...ys), Math.max(...xs), Math.max(...ys)] as const
}

function drawTextHalo(text: string, x: number, y: number) {
  if (!ctx) return
  ctx.save()
  ctx.lineWidth = 4
  ctx.strokeStyle = 'rgba(255,255,255,0.92)'
  ctx.strokeText(text, x, y)
  ctx.fillText(text, x, y)
  ctx.restore()
}

function polygonCenter(points: DemoPoint[]): DemoPoint {
  const total = points.reduce((acc, [x, y]) => [acc[0] + x, acc[1] + y] as DemoPoint, [0, 0])
  return [Math.round(total[0] / points.length), Math.round(total[1] / points.length)]
}

function buildingKindLabel(kind: string) {
  const labels: Record<string, string> = {
    visitor: '游客服务',
    admin: '行政办公',
    teaching: '教学建筑',
    museum: '文化展陈',
    library: '学习中心',
    plaza: '开放广场',
    activity: '学生活动',
    research: '实验科研',
    arts: '艺术场馆',
    gym: '体育设施',
    canteen: '餐饮服务',
    dorm: '住宿区',
    facility: '服务设施',
  }
  return labels[kind] || '建筑'
}

function seedDemoGraph() {
  selectedAreaId.value = DEMO_AREA_ID
  if (!spotOptions.value.some(s => s.id === DEMO_AREA_ID)) {
    spotOptions.value = [
      {
        id: DEMO_AREA_ID,
        name: '北湖书院示范校园',
        type: 'campus',
        category: '校园景区',
        description: '用于地图视觉与数据落库的前端示范母版',
        popularity: 96000,
        rating: 4.9,
        rating_count: 0,
        city: '示范城市',
        address: '北湖书院',
        open_time: '全天开放',
        ticket_price: 0,
      },
      ...spotOptions.value,
    ]
  }
  graphNodes.value = demoGraphNodes
  graphRoads.value = demoRoads
}

function drawMapLineSet(lines: DemoPoint[][], color: string, width: number, dash: number[]) {
  if (!ctx) return
  ctx.save()
  ctx.lineCap = 'round'
  ctx.lineJoin = 'round'
  ctx.strokeStyle = color
  ctx.lineWidth = width
  ctx.setLineDash(dash)
  for (const points of lines) {
    drawSmoothPath(points)
    ctx.stroke()
  }
  ctx.restore()
}

function drawRoadLayer(
  lines: DemoPoint[][],
  casingWidth: number,
  casingColor: string,
  fillWidth: number,
  fillColor: string,
) {
  if (!ctx) return
  drawMapLineSet(lines, casingColor, casingWidth, [])
  drawMapLineSet(lines, fillColor, fillWidth, [])
}

function drawSmoothPath(points: DemoPoint[]) {
  if (!ctx || points.length < 2) return
  ctx.beginPath()
  ctx.moveTo(points[0][0], points[0][1])
  for (let i = 1; i < points.length - 1; i++) {
    const [x, y] = points[i]
    const [nx, ny] = points[i + 1]
    ctx.quadraticCurveTo(x, y, (x + nx) / 2, (y + ny) / 2)
  }
  ctx.lineTo(points[points.length - 1][0], points[points.length - 1][1])
}

function drawMapLabels() {
  if (!ctx) return
  ctx.save()
  ctx.font = '700 22px "Noto Sans SC", sans-serif'
  ctx.fillStyle = 'rgba(66, 133, 244, 0.55)'
  ctx.fillText('北湖', 472, 398)
  ctx.font = '500 13px "Noto Sans SC", sans-serif'
  ctx.fillStyle = 'rgba(95, 99, 104, 0.62)'
  ctx.fillText('北门入口', 74, 392)
  ctx.fillText('教学核心区', 144, 356)
  ctx.fillText('东区教学组团', 732, 374)
  ctx.fillText('南区宿舍区', 756, 816)
  ctx.fillText('体育生活区', 720, 474)
  ctx.fillText('西湖岸步道', 86, 742)
  ctx.restore()
}

function drawCurvedSegment(x1: number, y1: number, x2: number, y2: number, bend: number) {
  if (!ctx) return
  const mx = (x1 + x2) / 2
  const my = (y1 + y2) / 2
  const dx = x2 - x1
  const dy = y2 - y1
  const len = Math.hypot(dx, dy) || 1
  const cx = mx + (-dy / len) * bend
  const cy = my + (dx / len) * bend
  ctx.beginPath()
  ctx.moveTo(x1, y1)
  ctx.quadraticCurveTo(cx, cy, x2, y2)
}

function drawMapPoi(x: number, y: number, node: MapNode, selected: boolean, hovered: boolean) {
  if (!ctx) return
  const isFacility = node.type === 'facility'
  const r = selected ? 6.5 : hovered ? 5.5 : 4.5
  ctx.save()
  ctx.beginPath()
  ctx.arc(x, y, r, 0, Math.PI * 2)
  ctx.fillStyle = selected ? '#1a73e8' : isFacility ? '#34a853' : '#5f6368'
  ctx.fill()
  ctx.strokeStyle = '#ffffff'
  ctx.lineWidth = 2
  ctx.stroke()
  if (selected || hovered || view.zoom > 0.9) {
    drawNodeLabel(node.name, x + 8, y - 8, selected || hovered)
  }
  ctx.restore()
}

function drawBuilding(x: number, y: number, node: MapNode, selected: boolean, hovered: boolean) {
  if (!ctx) return
  const width = 26 * Math.sqrt(view.zoom)
  const depth = 18 * Math.sqrt(view.zoom)
  const height = buildingHeight(node.sub_type) * Math.sqrt(view.zoom)
  const color = selected ? '#c7322e' : buildingColor(node.sub_type)
  const top = y - height
  ctx.fillStyle = 'rgba(26, 26, 26, 0.14)'
  isoPoly(x + 5, y + 6, width, depth, 0)
  ctx.fill()
  ctx.fillStyle = shade(color, -18)
  isoSideRight(x, y, width, depth, height)
  ctx.fill()
  ctx.fillStyle = shade(color, -8)
  isoSideLeft(x, y, width, depth, height)
  ctx.fill()
  ctx.fillStyle = hovered ? shade(color, 18) : color
  isoPoly(x, top, width, depth, 0)
  ctx.fill()
  ctx.strokeStyle = selected ? '#fffdf7' : 'rgba(26,26,26,0.16)'
  ctx.lineWidth = selected ? 2 : 1
  ctx.stroke()
  drawNodeLabel(node.name, x, y + depth + 12, selected || hovered)
}

function drawFacility(x: number, y: number, node: MapNode, selected: boolean, hovered: boolean) {
  if (!ctx) return
  const r = selected ? 8 : 6
  ctx.beginPath()
  ctx.arc(x, y, hovered ? r + 2 : r, 0, Math.PI * 2)
  ctx.fillStyle = selected ? '#c7322e' : '#445c6b'
  ctx.fill()
  ctx.strokeStyle = '#fffdf7'
  ctx.lineWidth = 2
  ctx.stroke()
  drawNodeLabel(node.name, x, y + 13, selected || hovered)
}

function drawJunction(x: number, y: number) {
  if (!ctx) return
  ctx.beginPath()
  ctx.arc(x, y, 2.5, 0, Math.PI * 2)
  ctx.fillStyle = 'rgba(94, 99, 71, 0.5)'
  ctx.fill()
}

function isoPoly(x: number, y: number, w: number, d: number, _h: number) {
  if (!ctx) return
  ctx.beginPath()
  ctx.moveTo(x, y - d / 2)
  ctx.lineTo(x + w / 2, y)
  ctx.lineTo(x, y + d / 2)
  ctx.lineTo(x - w / 2, y)
  ctx.closePath()
}

function isoSideRight(x: number, y: number, w: number, d: number, h: number) {
  if (!ctx) return
  ctx.beginPath()
  ctx.moveTo(x + w / 2, y - h)
  ctx.lineTo(x, y + d / 2 - h)
  ctx.lineTo(x, y + d / 2)
  ctx.lineTo(x + w / 2, y)
  ctx.closePath()
}

function isoSideLeft(x: number, y: number, w: number, d: number, h: number) {
  if (!ctx) return
  ctx.beginPath()
  ctx.moveTo(x - w / 2, y - h)
  ctx.lineTo(x, y + d / 2 - h)
  ctx.lineTo(x, y + d / 2)
  ctx.lineTo(x - w / 2, y)
  ctx.closePath()
}

function drawNodeLabel(text: string, x: number, y: number, strong: boolean) {
  if (!ctx || view.zoom < 0.72) return
  ctx.font = `${strong ? '700' : '500'} ${strong ? 12 : 10}px "Noto Sans SC", sans-serif`
  ctx.textAlign = 'left'
  ctx.fillStyle = strong ? '#3c4043' : 'rgba(95, 99, 104, 0.82)'
  ctx.fillText(text, x, y)
}

function drawLegend(w: number, h: number) {
  drawMapLegend(w, h)
}

function drawMapLegend(w: number, h: number) {
  drawGoogleStyleLegend(w, h)
}

function drawGoogleStyleLegend(w: number, h: number) {
  if (!ctx) return
  const c = ctx!
  const x = w - 166
  const y = h - 132
  c.fillStyle = 'rgba(255, 255, 255, 0.92)'
  c.strokeStyle = 'rgba(218, 220, 224, 0.95)'
  roundRect(x, y, 140, 104, 6)
  c.fill()
  c.stroke()
  c.fillStyle = '#3c4043'
  c.font = '700 11px "Noto Sans SC", sans-serif'
  c.fillText('图例', x + 16, y + 20)
  const items = [
    { label: '主干路', casing: '#cfd4d5', fill: '#ffffff', width: 5, dash: [] as number[] },
    { label: '支路', casing: '#c9ced1', fill: '#ffffff', width: 3, dash: [] as number[] },
    { label: '步道', casing: '#8d9a9a', fill: '#8d9a9a', width: 2, dash: [4, 4] as number[] },
    { label: '规划路径', casing: '#1a73e8', fill: '#1a73e8', width: 3, dash: [] as number[] },
  ]
  c.font = '10px "Noto Sans SC", sans-serif'
  items.forEach((item, i) => {
    const iy = y + 38 + i * 15
    c.strokeStyle = item.casing
    c.lineWidth = item.width + (i < 2 ? 2 : 0)
    c.setLineDash(item.dash)
    c.beginPath()
    c.moveTo(x + 14, iy)
    c.lineTo(x + 30, iy)
    c.stroke()
    c.strokeStyle = item.fill
    c.lineWidth = item.width
    c.beginPath()
    c.moveTo(x + 14, iy)
    c.lineTo(x + 30, iy)
    c.stroke()
    c.setLineDash([])
    c.fillStyle = '#5f6368'
    c.fillText(item.label, x + 36, iy + 3)
  })
}

function roundRect(x: number, y: number, w: number, h: number, r: number) {
  if (!ctx) return
  ctx.beginPath()
  ctx.moveTo(x + r, y)
  ctx.lineTo(x + w - r, y)
  ctx.quadraticCurveTo(x + w, y, x + w, y + r)
  ctx.lineTo(x + w, y + h - r)
  ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h)
  ctx.lineTo(x + r, y + h)
  ctx.quadraticCurveTo(x, y + h, x, y + h - r)
  ctx.lineTo(x, y + r)
  ctx.quadraticCurveTo(x, y, x + r, y)
  ctx.closePath()
}

function buildingHeight(subType: string | undefined) {
  if (subType === 'landmark' || subType === 'research') return 34
  if (subType === 'teaching' || subType === 'museum') return 28
  if (subType === 'admin' || subType === 'library') return 24
  return 20
}

function buildingColor(subType: string | undefined) {
  const colors: Record<string, string> = {
    teaching: '#687251',
    library: '#8a7048',
    research: '#5f7478',
    admin: '#826451',
    museum: '#776f56',
    landmark: '#9b7a42',
    canteen: '#9a6a3f',
  }
  return colors[subType || ''] || '#6f735c'
}

function shade(hex: string, amount: number) {
  const n = Number.parseInt(hex.slice(1), 16)
  const r = Math.max(0, Math.min(255, (n >> 16) + amount))
  const g = Math.max(0, Math.min(255, ((n >> 8) & 255) + amount))
  const b = Math.max(0, Math.min(255, (n & 255) + amount))
  return `rgb(${r}, ${g}, ${b})`
}

function formatDistance(distance: number): string {
  if (!Number.isFinite(distance) || distance <= 0) return '0m'
  if (distance >= 1000) return `${(distance / 1000).toFixed(1)}km`
  return `${Math.round(distance)}m`
}

function nodeTypeLabel(type: string, subType?: string): string {
  const labels: Record<string, string> = {
    entrance: '入口',
    building: '建筑',
    facility: '设施',
    junction: '路口',
  }
  return subType ? `${labels[type] || type}/${buildingKindLabel(subType)}` : labels[type] || type
}

function findNodeAt(clientX: number, clientY: number): MapNode | null {
  const rect = canvasRef.value?.getBoundingClientRect()
  if (!rect) return null
  const mx = clientX - rect.left
  const my = clientY - rect.top
  let best: MapNode | null = null
  let bestDist = 22
  for (const node of graphNodes.value) {
    if (node.type === 'junction') continue
    const [x, y] = toScreen(node.pos_x, node.pos_y)
    const dist = Math.hypot(mx - x, my - y)
    if (dist < bestDist) {
      bestDist = dist
      best = node
    }
  }
  return best
}

function selectNode(node: MapNode) {
  if (!startNode.value) {
    startNode.value = node
  } else if (!endNode.value && node.id !== startNode.value.id) {
    endNode.value = node
  } else if (
    endNode.value &&
    node.id !== startNode.value.id &&
    node.id !== endNode.value.id &&
    !waypoints.value.some(w => w.id === node.id)
  ) {
    waypoints.value.push(node)
  }
  routeResult.value = null
  requestDraw()
}

function handleCanvasClick(event: MouseEvent) {
  if (pointer.moved) {
    pointer.moved = false
    return
  }
  const node = findNodeAt(event.clientX, event.clientY)
  if (node) selectNode(node)
}

function handleCanvasMouseMove(event: MouseEvent) {
  if (pointer.dragging) {
    const dx = event.clientX - pointer.lastX
    const dy = event.clientY - pointer.lastY
    if (Math.abs(dx) + Math.abs(dy) > 2) pointer.moved = true
    view.targetPanX += dx
    view.targetPanY += dy
    pointer.lastX = event.clientX
    pointer.lastY = event.clientY
    requestDraw()
    return
  }
  const node = findNodeAt(event.clientX, event.clientY)
  if (hoveredNode.value?.id !== node?.id) {
    hoveredNode.value = node
    if (canvasRef.value) canvasRef.value.style.cursor = node ? 'pointer' : 'grab'
    requestDraw()
  }
}

function startPan(event: MouseEvent) {
  pointer.dragging = true
  pointer.moved = false
  pointer.lastX = event.clientX
  pointer.lastY = event.clientY
  if (canvasRef.value) canvasRef.value.style.cursor = 'grabbing'
}

function endPan() {
  pointer.dragging = false
  if (canvasRef.value) canvasRef.value.style.cursor = hoveredNode.value ? 'pointer' : 'grab'
}

function handleWheel(event: WheelEvent) {
  const factor = event.deltaY < 0 ? 1.12 : 0.9
  zoomBy(factor)
}

function zoomBy(factor: number) {
  view.targetZoom = Math.max(0.55, Math.min(2.8, view.targetZoom * factor))
  requestDraw()
}

function resetView() {
  view.targetZoom = 1
  view.targetPanX = 0
  view.targetPanY = 0
  requestDraw()
}

function focusHovered() {
  const node = hoveredNode.value
  const canvas = canvasRef.value
  if (!node || !canvas) return
  const rect = canvas.getBoundingClientRect()
  const [x, y] = toScreen(node.pos_x, node.pos_y)
  view.targetPanX += rect.width / 2 - x
  view.targetPanY += rect.height / 2 - y
  view.targetZoom = Math.min(2.2, Math.max(view.targetZoom, 1.4))
  requestDraw()
}

function requestDraw() {
  if (animationFrame) return
  animationFrame = requestAnimationFrame(tick)
}

function tick() {
  animationFrame = 0
  view.zoom += (view.targetZoom - view.zoom) * 0.18
  view.panX += (view.targetPanX - view.panX) * 0.18
  view.panY += (view.targetPanY - view.panY) * 0.18
  draw()
  if (
    Math.abs(view.targetZoom - view.zoom) > 0.002 ||
    Math.abs(view.targetPanX - view.panX) > 0.5 ||
    Math.abs(view.targetPanY - view.panY) > 0.5 ||
    (routeAnimationStart && performance.now() - routeAnimationStart < 1300)
  ) {
    requestDraw()
  }
}

function onDropdownSelect() {
  if (!dropdownNodeId.value) return
  const node = graphNodes.value.find(n => n.id === Number(dropdownNodeId.value))
  if (node) selectNode(node)
  dropdownNodeId.value = ''
}

async function planRoute() {
  if (!canPlan.value || !selectedAreaId.value) return
  loading.value = true
  try {
    const areaId = Number(selectedAreaId.value)
    if (waypoints.value.length > 0) {
      const res = await planMultiRoute({
        area_id: areaId,
        from_node: startNode.value!.id,
        to_node: endNode.value!.id,
        waypoints: waypoints.value.map(w => w.id),
        strategy: strategy.value,
        transport: transportMask.value,
      })
      routeResult.value = res.data
    } else {
      const res = await planSingleRoute({
        area_id: areaId,
        from_node: startNode.value!.id,
        to_node: endNode.value!.id,
        strategy: strategy.value,
        transport: transportMask.value,
      })
      routeResult.value = res.data
    }
    routeAnimationStart = performance.now()
    requestDraw()
  } catch (error) {
    console.error('路线规划失败:', error)
  } finally {
    loading.value = false
  }
}

function clearAll() {
  startNode.value = null
  endNode.value = null
  waypoints.value = []
  routeResult.value = null
  routeAnimationStart = 0
  requestDraw()
}

function undoLastWaypoint() {
  waypoints.value.pop()
  routeResult.value = null
  requestDraw()
}

async function loadGraphData() {
  clearAll()
  if (isDemoArea()) {
    seedDemoGraph()
    await nextTick()
    resizeCanvas()
    resetView()
    return
  }
  if (!selectedAreaId.value) {
    graphNodes.value = []
    graphRoads.value = []
    requestDraw()
    return
  }
  try {
    const res = await getGraphData(Number(selectedAreaId.value))
    graphNodes.value = res.data?.nodes || []
    graphRoads.value = res.data?.roads || res.data?.edges || []
    await nextTick()
    resizeCanvas()
    resetView()
  } catch (error) {
    console.error('加载路网失败:', error)
  }
}

async function loadDefaultGraphArea() {
  const candidates = [...spotOptions.value].sort((a, b) => {
    const aScore = (a.type === 'campus' ? 0 : 1) + (a.id >= 1001 ? 0 : 2)
    const bScore = (b.type === 'campus' ? 0 : 1) + (b.id >= 1001 ? 0 : 2)
    return aScore - bScore || a.id - b.id
  })

  for (const spot of candidates.slice(0, 40)) {
    try {
      const res = await getGraphData(Number(spot.id))
      const nodes = res.data?.nodes || []
      if (nodes.length === 0) continue

      selectedAreaId.value = spot.id
      graphNodes.value = nodes
      graphRoads.value = res.data?.roads || res.data?.edges || []
      await nextTick()
      resizeCanvas()
      resetView()
      return
    } catch (error) {
      console.error('默认地图区域探测失败:', error)
    }
  }

  if (candidates[0]) {
    selectedAreaId.value = candidates[0].id
    await loadGraphData()
  }
}

function handleResize() {
  resizeCanvas()
  requestDraw()
}

onMounted(async () => {
  try {
    const res = await getSpotOptions()
    spotOptions.value = res.data || []
    await loadDefaultGraphArea()
  } catch (error) {
    console.error('加载区域列表失败:', error)
  }
  await nextTick()
  resizeCanvas()
  requestDraw()
  window.addEventListener('resize', handleResize, { passive: true })
})

onUnmounted(() => {
  window.removeEventListener('resize', handleResize)
  if (animationFrame) cancelAnimationFrame(animationFrame)
})
</script>

<style scoped>
.route-workspace {
  min-height: calc(100vh - 72px);
}

.route-mode-switch {
  display: inline-grid;
  grid-template-columns: 1fr 1fr;
  margin: 16px 24px 12px;
  border: 1px solid var(--color-rule);
  background: var(--color-surface);
}

.route-mode-switch button {
  min-width: 128px;
  min-height: 40px;
  border: 0;
  background: transparent;
  color: var(--color-text-secondary);
  font-weight: 800;
  cursor: pointer;
}

.route-mode-switch button.active {
  color: white;
  background: var(--color-primary);
}

.map-page {
  display: grid;
  grid-template-columns: 360px minmax(0, 1fr);
  gap: 22px;
  height: calc(100vh - 132px);
  padding: 0 24px 24px;
}

.planner-panel,
.map-stage {
  border: 1px solid var(--color-rule);
  background: var(--color-surface);
}

.planner-panel {
  display: flex;
  flex-direction: column;
  min-height: 0;
  overflow-y: auto;
}

.panel-head {
  padding: 24px;
  border-bottom: 1px solid var(--color-rule);
}

.panel-head h1 {
  margin: 8px 0;
  font-size: 38px;
  line-height: 1;
}

.panel-head p {
  color: var(--color-text-secondary);
}

.control-group {
  display: grid;
  gap: 8px;
  padding: 18px 24px 0;
}

.control-group label {
  color: var(--color-text-muted);
  font-size: 12px;
  font-weight: 900;
  letter-spacing: 0.08em;
}

.control-group select {
  width: 100%;
  min-height: 42px;
  padding: 0 12px;
  border: 1px solid var(--color-rule);
  background: var(--color-surface-muted);
  color: var(--color-ink);
  outline: none;
}

.segmented {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 1px;
  background: var(--color-rule);
  border: 1px solid var(--color-rule);
}

.segmented button,
.panel-actions button,
.zoom-controls button {
  border: 0;
  background: var(--color-surface);
  color: var(--color-ink);
  cursor: pointer;
  font-weight: 800;
}

.segmented button {
  min-height: 40px;
  font-size: 12px;
}

.segmented button.active {
  color: var(--color-surface);
  background: var(--color-primary);
}

.transport-options {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 6px;
}

.transport-options button {
  min-height: 38px;
  border: 1px solid var(--color-rule);
  background: var(--color-surface);
  color: var(--color-text-secondary);
  font-size: 12px;
  font-weight: 800;
  cursor: pointer;
}

.transport-options button.active {
  color: white;
  border-color: var(--color-primary);
  background: var(--color-primary);
}

.transport-options button:disabled:not(.active) {
  opacity: .35;
  cursor: not-allowed;
}

.steps {
  display: grid;
  gap: 10px;
  padding: 22px 24px;
}

.step {
  display: grid;
  grid-template-columns: 34px 1fr;
  gap: 12px;
  padding-bottom: 14px;
  border-bottom: 1px solid var(--color-rule);
}

.step span {
  color: var(--color-editorial-red);
  font-size: 12px;
  font-weight: 900;
}

.step strong {
  display: block;
  margin-bottom: 3px;
}

.step p {
  color: var(--color-text-muted);
  font-size: 13px;
}

.step.done p {
  color: var(--color-ink);
}

.panel-actions {
  display: grid;
  gap: 10px;
  padding: 0 24px 22px;
}

.primary-btn {
  min-height: 46px;
  color: var(--color-surface) !important;
  background: var(--color-ink) !important;
}

.primary-btn:disabled,
.action-row button:disabled {
  opacity: 0.42;
  cursor: not-allowed;
}

.action-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

.action-row button {
  min-height: 40px;
  border: 1px solid var(--color-rule);
}

.route-summary {
  margin-top: auto;
  padding: 20px 24px 24px;
  border-top: 1px solid var(--color-rule);
  background: var(--color-surface-muted);
}

.summary-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 1px;
  margin: 12px 0;
  background: var(--color-rule);
}

.summary-grid div {
  display: grid;
  gap: 4px;
  padding: 12px;
  background: var(--color-surface);
}

.summary-grid span {
  color: var(--color-text-muted);
  font-size: 12px;
}

.summary-grid strong {
  font-family: Georgia, "Times New Roman", serif;
  font-size: 24px;
}

.route-names {
  color: var(--color-text-secondary);
  font-size: 13px;
}

.transfer-list {
  display: grid;
  gap: 5px;
  margin-bottom: 10px;
  color: #9a5a00;
  font-size: 12px;
  font-weight: 700;
}

.map-stage {
  position: relative;
  overflow: hidden;
}

.map-toolbar {
  position: absolute;
  z-index: 2;
  top: 18px;
  left: 18px;
  right: 18px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  pointer-events: none;
}

.map-toolbar > div {
  pointer-events: auto;
}

.map-toolbar span {
  display: block;
  color: var(--color-editorial-red);
  font-size: 11px;
  font-weight: 900;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.map-toolbar strong {
  display: block;
  margin-top: 2px;
  font-size: 14px;
}

.zoom-controls {
  display: flex;
  gap: 6px;
}

.zoom-controls button {
  min-width: 38px;
  height: 36px;
  border: 1px solid var(--color-rule);
  background: rgba(255, 253, 247, 0.86);
  backdrop-filter: blur(10px);
}

.map-canvas {
  display: block;
  width: 100%;
  height: 100%;
  cursor: grab;
}

.map-empty {
  position: absolute;
  inset: 0;
  display: grid;
  place-content: center;
  gap: 8px;
  text-align: center;
  color: var(--color-text-muted);
  background: rgba(242, 239, 231, 0.72);
}

.map-empty strong {
  color: var(--color-ink);
  font-size: 18px;
}

@media (max-width: 980px) {
  .map-page {
    grid-template-columns: 1fr;
    height: auto;
    min-height: calc(100vh - 72px);
    padding: 0 16px 16px;
  }

  .route-mode-switch { margin: 12px 16px; }

  .planner-panel {
    max-height: none;
  }

  .map-stage {
    min-height: 620px;
  }
}
</style>
