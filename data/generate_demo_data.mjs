import { writeFileSync, mkdirSync } from 'node:fs'
import { dirname, join } from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = dirname(fileURLToPath(import.meta.url))
const outFile = join(__dirname, 'generated_demo_seed.sql')
const backendOutFile = join(__dirname, '..', 'smart-tourism-backend', 'data', 'generated_demo_seed.sql')
const buildDataDir = join(__dirname, '..', 'smart-tourism-backend', 'build', 'bin', 'Release', 'data')

// ==================== Constants ====================
const AREA_START_ID = 1
const AREA_COUNT = 210
const CANVAS_W = 1000
const CANVAS_H = 700
const METERS_PER_UNIT = 1.2
const NODES_MIN = 35
const NODES_MAX = 50
const ROADS_MIN = 45

// ID counters
let nextRoadId = 100000
let nextFoodId = 200000
let nextDiaryId = 1000
let nextRatingId = 1000
let nextViewHistId = 1000
let nextIndoorNodeId = 1
let nextIndoorRoadId = 1

// ==================== Utilities ====================
function esc(v) { return String(v).replaceAll("'", "''") }
function q(v) { return `'${esc(v)}'` }
function nid(areaId, idx) { return areaId * 1000 + idx }

function seededRandom(seed) {
  let x = Math.sin(seed * 127.1 + 311.7) * 43758.5453
  return x - Math.floor(x)
}
function jitter(seed, amount) { return (seededRandom(seed) - 0.5) * amount * 2 }
function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)) }
function dist(a, b) { return Math.hypot(a.x - b.x, a.y - b.y) }
function roadDist(a, b) {
  const raw = dist(a, b) * METERS_PER_UNIT
  return Math.max(5, Math.round(raw / 5) * 5)
}
function randInt(seed, lo, hi) { return lo + Math.floor(seededRandom(seed) * (hi - lo + 1)) }

// BFS connectivity check
function checkConnectivity(nodes, roads) {
  if (nodes.length === 0) return true
  const adj = new Map()
  for (const n of nodes) adj.set(n.idx, [])
  for (const r of roads) {
    const fromLocal = r.fromNode % 1000
    const toLocal = r.toNode % 1000
    if (adj.has(fromLocal) && adj.has(toLocal)) {
      adj.get(fromLocal).push(toLocal)
      adj.get(toLocal).push(fromLocal)
    }
  }
  const visited = new Set()
  const queue = [nodes[0].idx]
  visited.add(nodes[0].idx)
  while (queue.length > 0) {
    const cur = queue.shift()
    for (const neighbor of adj.get(cur) || []) {
      if (!visited.has(neighbor)) { visited.add(neighbor); queue.push(neighbor) }
    }
  }
  return visited.size === nodes.length
}

function findDisconnectedComponents(nodes, roads) {
  if (nodes.length === 0) return []
  const adj = new Map()
  for (const n of nodes) adj.set(n.idx, [])
  for (const r of roads) {
    const fromLocal = r.fromNode % 1000
    const toLocal = r.toNode % 1000
    if (adj.has(fromLocal) && adj.has(toLocal)) {
      adj.get(fromLocal).push(toLocal)
      adj.get(toLocal).push(fromLocal)
    }
  }
  const visited = new Set()
  const components = []
  for (const n of nodes) {
    if (visited.has(n.idx)) continue
    const comp = []
    const queue = [n.idx]
    visited.add(n.idx)
    while (queue.length > 0) {
      const cur = queue.shift()
      comp.push(cur)
      for (const neighbor of adj.get(cur) || []) {
        if (!visited.has(neighbor)) { visited.add(neighbor); queue.push(neighbor) }
      }
    }
    components.push(comp)
  }
  return components
}

// ==================== Name Pools ====================
const cities = [
  '北京', '上海', '广州', '深圳', '杭州', '成都',
  '西安', '南京', '武汉', '重庆', '苏州', '青岛',
]
const scenicThemes = [
  '湖畔公园', '古城文化区', '山水风景区', '城市绿心', '艺术街区',
  '湿地公园', '历史街巷', '科技展馆', '森林步道', '滨江观光带',
]
const campusThemes = [
  '信息科技大学', '交通大学', '师范学院', '理工大学', '医科大学',
  '财经大学', '外国语大学', '艺术学院', '邮电大学', '农业大学',
]
const categories = ['自然', '历史', '文化', '现代', '综合']

const spotImages = {
  campus: '/images/demo/spot-campus.webp',
  palace: '/images/demo/spot-palace.webp',
  mountain: '/images/demo/spot-mountain.webp',
  garden: '/images/demo/spot-garden.webp',
  museum: '/images/demo/spot-museum.webp',
}

const foodImages = {
  spicy: '/images/demo/food-spicy.webp',
  dimsum: '/images/demo/food-dimsum.webp',
  noodles: '/images/demo/food-noodles.webp',
  hotpot: '/images/demo/food-hotpot.webp',
  roastDuck: '/images/demo/food-roast-duck.webp',
}

function getSpotImage(profile) {
  if (profile.type === 'campus') return spotImages.campus
  if (profile.category === '自然') return profile.areaId % 2 === 0 ? spotImages.mountain : spotImages.garden
  if (profile.category === '现代') return spotImages.museum
  if (profile.category === '综合') return profile.areaId % 2 === 0 ? spotImages.garden : spotImages.museum
  return profile.areaId % 3 === 0 ? spotImages.garden : spotImages.palace
}

function getFoodImage(name, cuisine, index) {
  const key = `${name}${cuisine}`
  if (key.includes('火锅') || key.includes('香锅')) return foodImages.hotpot
  if (key.includes('烤鸭') || cuisine === '京菜') return foodImages.roastDuck
  if (key.includes('面') || cuisine === '面食') return foodImages.noodles
  if (key.includes('粤') || key.includes('虾饺') || key.includes('蒸')) return foodImages.dimsum
  if (key.includes('川') || key.includes('湘') || key.includes('辣') || key.includes('酸菜')) return foodImages.spicy
  return Object.values(foodImages)[index % Object.keys(foodImages).length]
}

function getDiaryImages(profile, index) {
  if (profile.type === 'campus') {
    return ['/images/demo/diary-campus.webp', spotImages.campus]
  }
  if (profile.category === '自然' || index % 3 === 0) {
    return ['/images/demo/diary-park.webp', spotImages.mountain]
  }
  return ['/images/demo/diary-garden.webp', getSpotImage(profile)]
}

// Real scenic spots for areas 1-15
const realSpots = [
  { id: 1, name: '北京邮电大学', type: 'campus', category: '现代', city: '北京', address: '北京市海淀区西土城路10号', desc: '教育部直属全国重点大学，国家双一流建设高校。', pop: 9850, rating: '4.7', ticket: '免费', open: '全天开放' },
  { id: 2, name: '北京大学', type: 'campus', category: '历史', city: '北京', address: '北京市海淀区颐和园路5号', desc: '中国最著名的高等学府之一，博雅塔、未名湖等著名景点。', pop: 15600, rating: '4.9', ticket: '免费(需预约)', open: '08:00-18:00' },
  { id: 3, name: '清华大学', type: 'campus', category: '历史', city: '北京', address: '北京市海淀区清华园1号', desc: '中国著名高等学府，水木清华。', pop: 14200, rating: '4.8', ticket: '免费(需预约)', open: '08:00-18:00' },
  { id: 4, name: '颐和园', type: 'scenic', category: '历史', city: '北京', address: '北京市海淀区新建宫门路19号', desc: '中国现存规模最大、保存最完整的皇家园林。', pop: 25800, rating: '4.8', ticket: '30元/旺季60元', open: '06:00-20:00' },
  { id: 5, name: '故宫博物院', type: 'scenic', category: '历史', city: '北京', address: '北京市东城区景山前街4号', desc: '世界上现存规模最大、保存最为完整的木质结构古建筑群。', pop: 32000, rating: '4.9', ticket: '60元', open: '08:30-17:00' },
  { id: 6, name: '香山公园', type: 'scenic', category: '自然', city: '北京', address: '北京市海淀区买卖街甲17号', desc: '北京著名的赏秋胜地，香山红叶闻名全国。', pop: 12500, rating: '4.6', ticket: '10元/旺季15元', open: '06:00-19:00' },
  { id: 7, name: '北京师范大学', type: 'campus', category: '人文', city: '北京', address: '北京市海淀区新街口外大街19号', desc: '中国师范类最高学府。', pop: 15000, rating: '4.4', ticket: '免费', open: '全天' },
  { id: 8, name: '北京理工大学', type: 'campus', category: '现代', city: '北京', address: '北京市海淀区中关村南大街5号', desc: '以工科见长的985高校。', pop: 13000, rating: '4.3', ticket: '免费', open: '全天' },
  { id: 9, name: '天坛公园', type: 'scenic', category: '历史', city: '北京', address: '北京市东城区天坛东路甲1号', desc: '明清两代帝王祭天祈谷的场所。', pop: 65000, rating: '4.7', ticket: '15元', open: '06:00-22:00' },
  { id: 10, name: '圆明园遗址公园', type: 'scenic', category: '历史', city: '北京', address: '北京市海淀区清华西路28号', desc: '清代大型皇家园林遗址。', pop: 52000, rating: '4.6', ticket: '10元', open: '07:00-19:00' },
  { id: 11, name: '北海公园', type: 'scenic', category: '人文', city: '北京', address: '北京市西城区文津街1号', desc: '中国现存最古老、最完整的皇家园林之一。', pop: 45000, rating: '4.5', ticket: '10元', open: '06:30-20:00' },
  { id: 12, name: '鸟巢（国家体育场）', type: 'scenic', category: '现代', city: '北京', address: '北京市朝阳区国家体育场南路1号', desc: '2008年北京奥运会主场馆。', pop: 55000, rating: '4.4', ticket: '50元', open: '09:00-21:00' },
  { id: 13, name: '水立方', type: 'scenic', category: '现代', city: '北京', address: '北京市朝阳区天辰东路11号', desc: '2008年北京奥运会游泳场馆。', pop: 48000, rating: '4.3', ticket: '30元', open: '09:00-21:00' },
  { id: 14, name: '北京动物园', type: 'scenic', category: '综合', city: '北京', address: '北京市西城区西直门外大街137号', desc: '中国开放最早、饲养动物种类最多的动物园。', pop: 58000, rating: '4.5', ticket: '15元', open: '07:30-18:00' },
  { id: 15, name: '奥林匹克森林公园', type: 'scenic', category: '自然', city: '北京', address: '北京市朝阳区科荟路33号', desc: '北京最大的城市公园，跑步健身热门地点。', pop: 42000, rating: '4.6', ticket: '免费', open: '06:00-20:00' },
]

const scenicBuildingNames = [
  '游客服务中心', '文化展览馆', '茶艺馆', '观景阁', '艺术工坊',
  '历史陈列馆', '生态科普馆', '文创商店', '书吧', '摄影基地',
  '民俗体验馆', '陶艺坊', '国学讲堂', '禅意轩', '古戏台',
  '碑林', '藏经阁', '望江楼',
]
const campusBuildingNames = [
  '一号教学楼', '二号教学楼', '图书馆', '实验楼', '行政楼',
  '信息中心', '科学馆', '工程训练中心', '报告厅', '学生事务中心',
  '综合教学楼', '创新中心', '国际交流中心', '档案馆',
  '科研大楼', '计算中心', '阶梯教室楼',
]

const facilityNamePool = {
  toilet: ['公共卫生间A', '公共卫生间B', '东区卫生间', '西区卫生间', '无障碍卫生间'],
  restaurant: ['美食广场', '风味餐厅', '休闲餐厅', '快餐中心', '特色小吃街'],
  shop: ['纪念品商店', '文创小店', '便利店', '特产超市', '旅游商品店'],
  cafe: ['湖畔咖啡', '茶语小栈', '书咖', '观景茶室', '休闲茶座'],
  medical: ['医务室', '急救站', '医疗救助点', '健康服务站'],
  parking: ['南停车场', '北停车场', '东停车场', '地下停车场', '生态停车场'],
  service_center: ['游客中心', '服务中心', '咨询台', '导览服务站', '票务中心'],
  viewpoint: ['观景台', '最佳拍摄点', '瞭望台', '全景平台'],
  atm: ['自助银行', 'ATM服务点'],
}
const campusFacilityNames = {
  toilet: ['教学楼卫生间', '图书馆卫生间', '体育馆卫生间', '食堂卫生间', '实验楼卫生间'],
  restaurant: ['第一食堂', '第二食堂', '美食城', '教工餐厅', '清真食堂'],
  shop: ['校园超市', '教材中心', '文具店', '打印店', '水果店'],
  cafe: ['校园咖啡厅', '图书馆咖啡角', '学生茶吧', '创业咖啡'],
  medical: ['校医院', '心理咨询中心', '健康服务中心'],
  parking: ['地下车库', '地面停车场', '自行车棚'],
  service_center: ['学生事务大厅', '教务处', '信息中心', '一卡通中心'],
  viewpoint: ['校训石', '名人雕塑', '文化广场', '银杏大道'],
  atm: ['自助银行服务区', 'ATM取款机'],
}

const foodNamePool = [
  // === 热菜（中餐八大菜系） ===
  ['招牌红烧肉', '本帮菜', '食府楼'], ['酸菜鱼', '川菜', '川味轩'],
  ['虾饺皇', '粤菜', '粤鲜楼'], ['糖醋里脊', '鲁菜', '鲁味坊'],
  ['小炒黄牛肉', '湘菜', '湘辣居'], ['牛肉拉面', '面食', '面道馆'],
  ['扬州炒饭', '淮扬菜', '淮扬小厨'], ['麻辣香锅', '川菜', '辣尚瘾'],
  ['清蒸鲈鱼', '粤菜', '鲜鱼坊'], ['宫保鸡丁', '川菜', '蜀香园'],
  ['葱油拌面', '面食', '老面馆'], ['铁板牛排', '西餐', '西式简餐'],
  ['三杯鸡', '台菜', '台味小馆'], ['担担面', '川菜', '成都小吃'],
  ['蜜汁叉烧', '粤菜', '烧腊店'], ['煎饺', '小吃', '饺子馆'],
  ['奶茶', '饮品', '茶饮铺'], ['冰粉', '甜品', '甜品站'],
  ['烤串', '烧烤', '烤串吧'], ['生煎包', '小吃', '点心铺'],
  // === 新增热菜 ===
  ['回锅肉', '川菜', '巴蜀人家'], ['水煮牛肉', '川菜', '麻辣诱惑'],
  ['夫妻肺片', '川菜', '锦里小馆'], ['麻婆豆腐', '川菜', '陈麻婆'],
  ['白切鸡', '粤菜', '岭南风味'], ['豉汁蒸排骨', '粤菜', '蒸品轩'],
  ['干炒牛河', '粤菜', '潮味坊'], ['煲仔饭', '粤菜', '陶陶居'],
  ['佛跳墙', '闽菜', '聚春园'], ['荔枝肉', '闽菜', '榕城小厨'],
  ['红烧狮子头', '淮扬菜', '扬州饭店'], ['大煮干丝', '淮扬菜', '文思豆腐坊'],
  ['松鼠桂鱼', '苏帮菜', '松鹤楼'], ['叫花鸡', '浙菜', '楼外楼'],
  ['东坡肉', '浙菜', '杭州酒家'], ['西湖醋鱼', '浙菜', '湖畔居'],
  ['剁椒鱼头', '湘菜', '湘味馆'], ['毛氏红烧肉', '湘菜', '韶山冲'],
  ['臭鳜鱼', '徽菜', '徽煌府'], ['符离集烧鸡', '徽菜', '古徽州'],
  // === 面食/小吃 ===
  ['炸酱面', '面食', '京味面馆'], ['刀削面', '面食', '山西面馆'],
  ['热干面', '面食', '楚汉小吃'], ['臊子面', '面食', '关中味道'],
  ['重庆小面', '面食', '山城面庄'], ['螺蛳粉', '粉面', '柳州味道'],
  ['过桥米线', '粉面', '云南人家'], ['肉夹馍', '小吃', '长安小吃'],
  ['煎饼果子', '小吃', '津味早点'], ['灌汤包', '小吃', '开封菜馆'],
  ['肠粉', '小吃', '穗城早点'], ['烧麦', '小吃', '老字号烧麦'],
  ['锅贴', '小吃', '金饺园'], ['春卷', '小吃', '福来春'],
  // === 烧烤 ===
  ['烤羊排', '烧烤', '牧羊人烧烤'], ['烤鱼', '烧烤', '江边城外'],
  ['炭烤生蚝', '烧烤', '海味鲜烤'], ['烤羊腿', '烧烤', '草原情'],
  ['烤鸡翅', '烧烤', '翅吧'], ['烤茄子', '烧烤', '素味烤坊'],
  // === 火锅/干锅 ===
  ['毛肚火锅', '火锅', '蜀九香'], ['鸳鸯火锅', '火锅', '海底捞风味'],
  ['菌菇火锅', '火锅', '云南菌汤'], ['潮汕牛肉火锅', '火锅', '八合里'],
  ['干锅牛蛙', '干锅', '蛙来哒'], ['干锅虾', '干锅', '虾吃虾涮'],
  // === 海鲜 ===
  ['清蒸大闸蟹', '海鲜', '蟹王府'], ['蒜蓉粉丝扇贝', '海鲜', '海鲜码头'],
  ['椒盐皮皮虾', '海鲜', '渔人码头'], ['葱姜炒蟹', '海鲜', '东海渔港'],
  ['生鱼片', '日料', '樱花亭'], ['天妇罗', '日料', '和风料理'],
  ['鳗鱼饭', '日料', '鳗享屋'], ['寿司拼盘', '日料', '鲜寿司'],
  // === 韩式/东南亚 ===
  ['石锅拌饭', '韩料', '首尔味道'], ['韩式烤肉', '韩料', '韩宫烤肉'],
  ['冬阴功汤', '泰料', '曼谷风情'], ['菠萝炒饭', '泰料', '泰香阁'],
  ['越南河粉', '越料', '西贡味道'], ['海南鸡饭', '东南亚', '南洋风味'],
  // === 西餐/快餐 ===
  ['意大利面', '西餐', '罗马假日'], ['披萨', '西餐', '必胜小屋'],
  ['汉堡套餐', '西餐', '美式汉堡'], ['凯撒沙拉', '西餐', '轻食坊'],
  ['法式焗蜗牛', '法餐', '左岸餐厅'], ['提拉米苏', '甜品', '意式甜点'],
  // === 饮品/甜点 ===
  ['芝士奶盖茶', '饮品', '喜茶铺'], ['杨枝甘露', '甜品', '港式甜品'],
  ['手工冰淇淋', '甜品', '冰爽站'], ['芒果糯米饭', '甜品', '暹罗甜品'],
  ['双皮奶', '甜品', '顺德糖水'], ['豆花', '小吃', '老街豆花'],
  ['酸梅汤', '饮品', '老北京饮品'], ['水果捞', '甜品', '鲜果时光'],
  ['椰子冻', '甜品', '海岛甜品'], ['龟苓膏', '甜品', '养生糖水'],
  // === 更多地方特色 ===
  ['羊肉泡馍', '西北菜', '回民街'], ['大盘鸡', '新疆菜', '西域风情'],
  ['手抓饭', '新疆菜', '天山味道'], ['烤包子', '新疆菜', '馕坑烤包'],
  ['牦牛肉火锅', '藏餐', '雪域风情'], ['青稞饼', '藏餐', '高原味道'],
  ['汽锅鸡', '滇菜', '建水陶锅'], ['酸汤鱼', '黔菜', '苗寨风情'],
  ['花溪牛肉粉', '黔菜', '贵阳味道'], ['文昌鸡', '琼菜', '海南味道'],
  // === 特色店铺（含地方老字号） ===
  ['全聚德烤鸭', '京菜', '全聚德'], ['东来顺涮羊肉', '京菜', '东来顺'],
  ['狗不理包子', '津菜', '狗不理'], ['十八街麻花', '津菜', '桂发祥'],
  ['龙抄手', '川菜', '龙抄手'], ['赖汤圆', '川菜', '赖汤圆'],
  ['周黑鸭', '卤味', '周黑鸭'], ['绝味鸭脖', '卤味', '绝味'],
]

const diaryTemplates = [
  { title: '周末游记', content: '周末天气不错，约了几个朋友一起来逛逛。整体环境很好，绿树成荫，空气清新。走走停停，拍了不少照片。推荐大家周末来放松一下。' },
  { title: '半日闲游', content: '趁着假期来这里转了一圈，景色比想象中好。沿着小路慢慢走，看到了很多有意思的景观。休息区的长椅很舒服，坐下来喝杯茶，非常惬意。' },
  { title: '初来打卡', content: '第一次来这里，印象很不错。入口处就有清晰的导览图，沿着推荐路线走了一遍，每个景点都很有特色。下次还想再来深度体验。' },
  { title: '亲子出游', content: '带小朋友来玩了一天，孩子特别开心。沿途有休息点和小卖部，补给很方便。工作人员也很热情，推荐了适合小朋友的路线。' },
  { title: '秋日漫步', content: '秋天来这别有一番风味，满眼的金黄色非常漂亮。沿着步道慢慢走，微风拂面，心情特别舒畅。适合拍照的地方很多。' },
  { title: '雨后探访', content: '雨后来的，空气格外清新，草木带着露珠特别好看。人也不多，可以安静地欣赏风景。唯一的小遗憾是部分步道有点滑。' },
  { title: '深度体验', content: '这次特意花了一整天时间深度游览。早上从南门进，先逛了主景区，中午在美食区吃了午饭，下午去了文化展区。不虚此行。' },
  { title: '清晨晨练', content: '早上六点多就到了，晨练的人不少。沿着环形步道跑了一圈，沿途的风景让人心旷神怡。跑完步在茶室坐了一会儿。' },
]

// ==================== Area Profile ====================
function makeAreaProfile(i) {
  const areaId = AREA_START_ID + i

  // Areas 1-15: use real spot data
  if (i < 15) {
    const real = realSpots[i]
    return {
      areaId: real.id,
      name: real.name,
      type: real.type,
      theme: real.type === 'campus' ? campusThemes[i % 10] : scenicThemes[i % 10],
      category: real.category,
      city: real.city,
      nodeCount: randInt(real.id * 100 + 1, NODES_MIN, NODES_MAX),
      popularity: real.pop,
      rating: real.rating,
      address: real.address,
      description: real.desc,
      ticket: real.ticket,
      openTime: real.open,
    }
  }

  // Areas 1001+: generated data
  const genIdx = i - 15
  const genId = 1001 + genIdx
  const isCampus = genIdx % 5 === 0
  const type = isCampus ? 'campus' : 'scenic'
  const themeIdx = genIdx % 10
  const theme = isCampus ? campusThemes[themeIdx] : scenicThemes[themeIdx]
  const category = categories[genIdx % categories.length]
  const city = cities[genIdx % cities.length]
  const name = `${city}${theme}${String(genIdx + 1).padStart(3, '0')}`
  const nodeCount = randInt(genId * 100 + 1, NODES_MIN, NODES_MAX)
  const popularity = 3000 + ((genIdx * 7919) % 47000)
  const rating = (3.6 + ((genIdx * 37) % 14) / 10).toFixed(1)

  return {
    areaId: genId, name, type, theme, category, city, nodeCount, popularity, rating,
    address: `${city}市示范路${genIdx + 1}号`,
    description: `${name}，${city}知名${type === 'campus' ? '高校' : '景区'}，环境优美，设施完善。`,
    ticket: type === 'campus' ? '免费' : `${10 + (genIdx % 9) * 5}元`,
    openTime: '08:00-18:00',
  }
}

// ==================== Node Layout ====================
const ESSENTIAL_FACILITIES = ['toilet', 'restaurant', 'shop', 'parking', 'service_center']

function generateNodeLayout(profile) {
  const { areaId, type, nodeCount } = profile
  const nodes = []
  const seed = areaId
  const MIN_DIST = 15

  const numEntrances = randInt(seed + 1, 2, 4)
  const numBuildings = randInt(seed + 2, 8, 16)
  // At least 7 facilities: 5 essential + 2 optional
  const numFacilities = Math.max(7, randInt(seed + 3, 5, 10))
  const numJunctions = nodeCount - numEntrances - numBuildings - numFacilities

  let idx = 1

  function tryPlace(x, y, nodeType, subType, maxAttempts) {
    for (let attempt = 0; attempt < maxAttempts; attempt++) {
      const px = Math.round(clamp(x + jitter(seed + idx * 37 + attempt, 15), 50, 950))
      const py = Math.round(clamp(y + jitter(seed + idx * 41 + attempt, 15), 60, 640))
      let tooClose = false
      for (const n of nodes) {
        if (Math.hypot(n.x - px, n.y - py) < MIN_DIST) { tooClose = true; break }
      }
      if (!tooClose) {
        nodes.push({ idx: idx++, x: px, y: py, type: nodeType, subType })
        return
      }
    }
    nodes.push({ idx: idx++, x: Math.round(clamp(x, 50, 950)), y: Math.round(clamp(y, 60, 640)), type: nodeType, subType })
  }

  // Entrances on perimeter
  for (let e = 0; e < numEntrances; e++) {
    const angle = (e / numEntrances) * Math.PI * 2 + seededRandom(seed + e * 7) * 0.6
    tryPlace(500 + Math.cos(angle) * 440, 350 + Math.sin(angle) * 300, 'entrance', 'gate', 20)
  }

  // Buildings in center zone
  for (let b = 0; b < numBuildings; b++) {
    tryPlace(200 + seededRandom(seed + 100 + b * 3) * 600, 120 + seededRandom(seed + 100 + b * 5) * 460, 'building', '', 20)
  }

  // Facilities: first 5 are essential, rest are varied
  const facilitySubTypes = [...ESSENTIAL_FACILITIES]
  const optionalTypes = ['cafe', 'viewpoint', 'medical', 'atm']
  for (let f = 5; f < numFacilities; f++) {
    facilitySubTypes.push(optionalTypes[(f - 5) % optionalTypes.length])
  }

  for (let f = 0; f < numFacilities; f++) {
    tryPlace(100 + seededRandom(seed + 200 + f * 3) * 800, 100 + seededRandom(seed + 200 + f * 5) * 500, 'facility', facilitySubTypes[f], 20)
  }

  // Junctions
  for (let j = 0; j < numJunctions; j++) {
    tryPlace(80 + seededRandom(seed + 300 + j * 3) * 840, 80 + seededRandom(seed + 300 + j * 5) * 540, 'junction', '', 20)
  }

  assignNodeNames(nodes, profile)
  return nodes
}

function assignNodeNames(nodes, profile) {
  const { type: areaType, areaId } = profile
  let bIdx = 0, fIdx = 0, eIdx = 0
  const entranceNames = ['南门', '北门', '东门', '西门', '主入口', '次入口']

  const scenicSubTypes = ['landmark', 'landmark', 'museum', 'landmark', 'landmark',
                          'museum', 'landmark', 'landmark', 'landmark', 'landmark',
                          'museum', 'landmark', 'landmark', 'landmark', 'landmark', 'landmark']
  const campusSubTypes = ['teaching', 'teaching', 'library', 'research', 'admin',
                          'teaching', 'research', 'teaching', 'landmark', 'admin',
                          'teaching', 'research', 'admin', 'library', 'research', 'research']

  for (const node of nodes) {
    if (node.type === 'entrance') {
      node.name = entranceNames[eIdx % entranceNames.length]; eIdx++
    } else if (node.type === 'building') {
      if (areaType === 'campus') {
        node.name = campusBuildingNames[bIdx % campusBuildingNames.length]
        node.subType = campusSubTypes[bIdx % campusSubTypes.length]
      } else {
        node.name = scenicBuildingNames[bIdx % scenicBuildingNames.length]
        node.subType = scenicSubTypes[bIdx % scenicSubTypes.length]
      }
      bIdx++
    } else if (node.type === 'facility') {
      const pool = areaType === 'campus' ? campusFacilityNames : facilityNamePool
      const names = pool[node.subType] || ['服务设施']
      node.name = names[fIdx % names.length]
      fIdx++
    } else {
      node.name = '路口'
    }
  }
}

// ==================== Road Generation with Connectivity ====================
function generateRoads(nodes, profile) {
  const { areaId } = profile
  const roads = []
  const edgeSet = new Set()

  function addEdge(fromIdx, toIdx) {
    const key = Math.min(fromIdx, toIdx) + '-' + Math.max(fromIdx, toIdx)
    if (edgeSet.has(key)) return false
    edgeSet.add(key)
    const from = nodes.find(n => n.idx === fromIdx)
    const to = nodes.find(n => n.idx === toIdx)
    if (!from || !to) return false
    const d = roadDist(from, to)
    if (d < 5) return false

    const seed = areaId * 1000 + fromIdx + toIdx
    const congestion = (0.2 + seededRandom(seed) * 0.7).toFixed(2)
    const r = seededRandom(seed + 99)
    const transport = r < 0.55 ? 0 : r < 0.85 ? 1 : 2
    const speed = transport === 0 ? 1.4 : transport === 1 ? 4.0 : 7.0

    roads.push({
      id: nextRoadId++,
      areaId,
      fromNode: nid(areaId, fromIdx),
      toNode: nid(areaId, toIdx),
      distance: d,
      congestion, speed, transport,
      isBidirectional: 1,
    })
    return true
  }

  // Connect each node to its 2 nearest neighbors
  for (const node of nodes) {
    const others = nodes
      .filter(n => n.idx !== node.idx)
      .map(n => ({ node: n, d: dist(node, n) }))
      .sort((a, b) => a.d - b.d)
    for (let k = 0; k < Math.min(2, others.length); k++) {
      if (others[k].d < 260) addEdge(node.idx, others[k].node.idx)
    }
  }

  // Connect entrances to nearest node
  const entrances = nodes.filter(n => n.type === 'entrance')
  for (const ent of entrances) {
    const near = nodes
      .filter(n => n.idx !== ent.idx)
      .map(n => ({ node: n, d: dist(ent, n) }))
      .sort((a, b) => a.d - b.d)
    if (near.length > 0 && near[0].d < 300) addEdge(ent.idx, near[0].node.idx)
  }

  // Ensure minimum road count by adding more cross-links
  let attempts = 0
  while (roads.length < ROADS_MIN && attempts < 200) {
    const a = nodes[Math.floor(seededRandom(areaId * 999 + attempts) * nodes.length)]
    const b = nodes[Math.floor(seededRandom(areaId * 999 + attempts + 100) * nodes.length)]
    if (a && b && a.idx !== b.idx && dist(a, b) < 200) addEdge(a.idx, b.idx)
    attempts++
  }

  // BFS connectivity check - add bridge edges if disconnected
  let bridgeAttempts = 0
  while (!checkConnectivity(nodes, roads) && bridgeAttempts < 100) {
    const components = findDisconnectedComponents(nodes, roads)
    if (components.length < 2) break

    // Find closest pair between component 0 and each other component
    for (let c = 1; c < components.length; c++) {
      let bestDist = Infinity
      let bestA = -1, bestB = -1
      for (const aIdx of components[0]) {
        for (const bIdx of components[c]) {
          const a = nodes.find(n => n.idx === aIdx)
          const b = nodes.find(n => n.idx === bIdx)
          if (a && b) {
            const d = dist(a, b)
            if (d < bestDist) { bestDist = d; bestA = aIdx; bestB = bIdx }
          }
        }
      }
      if (bestA >= 0 && bestB >= 0) {
        addEdge(bestA, bestB)
        // Merge component c into component 0
        components[0].push(...components[c])
      }
    }
    bridgeAttempts++
  }

  return roads
}

// ==================== Building Records ====================
function generateBuildings(nodes, profile) {
  const { areaId, type: areaType } = profile
  const buildings = []
  const buildingNodes = nodes.filter(n => n.type === 'building')
  const count = Math.max(4, Math.floor(buildingNodes.length * 0.7))

  const scenicBTypes = ['museum', 'museum', 'museum', 'gym', 'library', 'museum', 'museum', 'museum', 'museum', 'museum', 'museum', 'museum', 'museum', 'museum', 'museum', 'museum']
  const campusBTypes = ['teaching', 'teaching', 'library', 'office', 'office', 'teaching', 'office', 'teaching', 'gym', 'office', 'teaching', 'office', 'office', 'library', 'office', 'office']

  for (let i = 0; i < count && i < buildingNodes.length; i++) {
    const node = buildingNodes[i]
    let floors, hasElevator
    if (areaType === 'campus') {
      const bType = campusBTypes[i % campusBTypes.length]
      if (bType === 'dorm') { floors = randInt(areaId + i + 200, 6, 18); hasElevator = 1 }
      else if (bType === 'teaching') { floors = randInt(areaId + i + 200, 3, 8); hasElevator = floors >= 4 ? 1 : 0 }
      else if (bType === 'library') { floors = randInt(areaId + i + 200, 4, 12); hasElevator = 1 }
      else { floors = randInt(areaId + i + 200, 2, 6); hasElevator = floors >= 4 ? 1 : 0 }
    } else {
      const bType = scenicBTypes[i % scenicBTypes.length]
      floors = randInt(areaId + i + 200, 1, 4)
      hasElevator = floors >= 4 ? 1 : 0
    }
    buildings.push({
      id: node.id || nid(areaId, node.idx),
      areaId,
      name: node.name,
      type: (areaType === 'campus' ? campusBTypes : scenicBTypes)[i % (areaType === 'campus' ? campusBTypes : scenicBTypes).length],
      totalFloors: floors,
      hasElevator,
      description: `${node.name}，${profile.name}主要建筑`,
    })
  }
  return buildings
}

// ==================== Food Generation ====================
function generateFoods(nodes, profile) {
  const { areaId, name: areaName } = profile
  const foods = []
  const foodNodes = nodes.filter(n =>
    n.type === 'facility' && ['restaurant', 'cafe', 'shop'].includes(n.subType)
  )
  const buildingFoodNodes = nodes.filter(n => n.type === 'building').slice(0, 2)
  const allFoodNodes = [...foodNodes, ...buildingFoodNodes]

  const count = Math.max(10, randInt(areaId + 500, 10, 20))
  const baseNodes = allFoodNodes.length > 0 ? allFoodNodes : nodes.slice(0, 3)

  for (let f = 0; f < count; f++) {
    const node = baseNodes[f % baseNodes.length]
    const foodTemplate = foodNamePool[(areaId + f) % foodNamePool.length]
    const price = 15 + Math.floor(seededRandom(areaId + f * 31) * 85)
    const foodRating = (3.5 + seededRandom(areaId + f * 17) * 1.5).toFixed(1)
    const ratingCount = 50 + Math.floor(seededRandom(areaId + f * 23) * 500)
    const popularity = 500 + Math.floor(seededRandom(areaId + f * 29) * 8000)

    foods.push({
      id: nextFoodId++, areaId,
      name: foodTemplate[0], cuisine: foodTemplate[1], restaurant: foodTemplate[2],
      rating: foodRating, ratingCount, popularity,
      nodeId: node.id || nid(areaId, node.idx),
      price,
      description: `${foodTemplate[1]}风味，${areaName}人气美食`,
      image: getFoodImage(foodTemplate[0], foodTemplate[1], f),
    })
  }
  return foods
}

// ==================== Diary/Rating/ViewHistory ====================
function generateDiaries(profile) {
  const { areaId, name: areaName } = profile
  const diaries = []
  const count = randInt(areaId + 600, 1, 3)
  for (let d = 0; d < count; d++) {
    const template = diaryTemplates[(areaId + d) % diaryTemplates.length]
    const userId = 1 + ((areaId + d) % 10)
    diaries.push({
      id: nextDiaryId++, userId,
      title: `${areaName}${template.title}`,
      content: template.content,
      destination: areaName, destinationId: areaId,
      tags: JSON.stringify(['旅游', '休闲', areaName]),
      images: JSON.stringify(getDiaryImages(profile, d)), videos: JSON.stringify([]),
      popularity: 10 + Math.floor(seededRandom(areaId + d * 41) * 200),
      avgRating: (3.5 + seededRandom(areaId + d * 43) * 1.5).toFixed(1),
      ratingCount: Math.floor(seededRandom(areaId + d * 47) * 10),
    })
  }
  return diaries
}

function generateRatings(diaries) {
  const ratings = []
  for (const diary of diaries) {
    const count = Math.min(diary.ratingCount || 1, 5)
    for (let r = 0; r < count; r++) {
      ratings.push({
        id: nextRatingId++,
        userId: 1 + ((diary.id + r) % 10),
        diaryId: diary.id,
        score: 3 + Math.floor(seededRandom(diary.id * 100 + r) * 3),
      })
    }
  }
  return ratings
}

function generateViewHistory(profile) {
  const entries = []
  const count = randInt(profile.areaId + 700, 2, 6)
  for (let v = 0; v < count; v++) {
    entries.push({
      id: nextViewHistId++,
      userId: 1 + ((profile.areaId + v) % 10),
      spotId: profile.areaId,
    })
  }
  return entries
}

// ==================== Indoor Navigation (ALL buildings with 3+ floors) ====================
function generateIndoor(buildings, nodes, profile) {
  const { areaId } = profile
  const indoorNodes = []
  const indoorRoads = []

  const eligible = buildings.filter(b => b.totalFloors >= 3)

  for (const building of eligible) {
    const maxFloor = Math.min(building.totalFloors, 3)

    for (let floor = 1; floor <= maxFloor; floor++) {
      const floorNodes = []
      const baseX = 100
      const baseY = 80 + (floor - 1) * 150
      const roomCount = randInt(building.id + floor, 1, 3)
      const floorNodeDefs = [
        { name: floor === 1 ? '大门' : '走廊入口', type: 'entrance', x: baseX, y: baseY + 60 },
        { name: '电梯', type: 'elevator', x: baseX + 300, y: baseY + 60 },
        { name: '楼梯', type: 'stairs', x: baseX + 500, y: baseY + 60 },
        { name: '卫生间', type: 'facility', x: baseX + 700, y: baseY + 60 },
      ]
      for (let r = 0; r < roomCount; r++) {
        floorNodeDefs.push({ name: `${floor}0${r + 1}室`, type: 'room', x: baseX + 100 + r * 150, y: baseY + 120 })
      }

      for (const def of floorNodeDefs) {
        const id = nextIndoorNodeId++
        indoorNodes.push({
          id, buildingId: building.id, floor,
          name: def.name, type: def.type,
          pos_x: def.x, pos_y: def.y,
          x: def.x, y: def.y,
        })
        floorNodes.push(id)
      }

      // Chain floor nodes
      for (let i = 0; i < floorNodes.length - 1; i++) {
        const from = indoorNodes.find(n => n.id === floorNodes[i])
        const to = indoorNodes.find(n => n.id === floorNodes[i + 1])
        if (from && to) {
          indoorRoads.push({
            id: nextIndoorRoadId++, buildingId: building.id,
            fromNode: from.id, toNode: to.id,
            distance: Math.max(5, Math.round(dist(from, to) * 0.8)),
          })
        }
      }

      // Connect rooms to entrance
      const entrance = indoorNodes.find(n => n.buildingId === building.id && n.floor === floor && n.type === 'entrance')
      const rooms = indoorNodes.filter(n => n.buildingId === building.id && n.floor === floor && n.type === 'room')
      for (const room of rooms) {
        if (entrance) {
          indoorRoads.push({
            id: nextIndoorRoadId++, buildingId: building.id,
            fromNode: entrance.id, toNode: room.id,
            distance: Math.max(5, Math.round(dist(entrance, room) * 0.8)),
          })
        }
      }
    }

    // Connect floors via elevator/stairs
    for (let floor = 1; floor < maxFloor; floor++) {
      const thisElev = indoorNodes.find(n => n.buildingId === building.id && n.floor === floor && n.type === 'elevator')
      const nextElev = indoorNodes.find(n => n.buildingId === building.id && n.floor === floor + 1 && n.type === 'elevator')
      const thisStairs = indoorNodes.find(n => n.buildingId === building.id && n.floor === floor && n.type === 'stairs')
      const nextStairs = indoorNodes.find(n => n.buildingId === building.id && n.floor === floor + 1 && n.type === 'stairs')
      if (thisElev && nextElev) indoorRoads.push({ id: nextIndoorRoadId++, buildingId: building.id, fromNode: thisElev.id, toNode: nextElev.id, distance: 15 })
      if (thisStairs && nextStairs) indoorRoads.push({ id: nextIndoorRoadId++, buildingId: building.id, fromNode: thisStairs.id, toNode: nextStairs.id, distance: 25 })
    }
  }
  return { indoorNodes, indoorRoads }
}

// ==================== SQL Generation ====================
function generateSQL() {
  const sql = []
  sql.push('-- ============================================================')
  sql.push('-- Smart Tourism System - Generated Demo Data')
  sql.push(`-- Generated: ${new Date().toISOString()}`)
  sql.push(`-- Areas: ${AREA_COUNT} (IDs ${AREA_START_ID}..${AREA_START_ID + AREA_COUNT - 1})`)
  sql.push('-- ============================================================')
  sql.push('BEGIN TRANSACTION;')

  // DELETE existing data for ALL areas we'll regenerate
  const minId = AREA_START_ID
  const maxId = AREA_START_ID + AREA_COUNT - 1
  // For areas > 100, delete by range; for 1-15, also delete to replace seed data
  sql.push(`DELETE FROM indoor_roads WHERE building_id IN (SELECT id FROM buildings WHERE area_id BETWEEN ${minId} AND 15);`)
  sql.push(`DELETE FROM indoor_nodes WHERE building_id IN (SELECT id FROM buildings WHERE area_id BETWEEN ${minId} AND 15);`)
  sql.push(`DELETE FROM view_history WHERE spot_id BETWEEN ${minId} AND 15;`)
  sql.push(`DELETE FROM ratings WHERE diary_id IN (SELECT id FROM diaries WHERE destination_id BETWEEN ${minId} AND 15);`)
  sql.push(`DELETE FROM diaries WHERE destination_id BETWEEN ${minId} AND 15;`)
  sql.push(`DELETE FROM foods WHERE area_id BETWEEN ${minId} AND 15;`)
  sql.push(`DELETE FROM buildings WHERE area_id BETWEEN ${minId} AND 15;`)
  sql.push(`DELETE FROM roads WHERE area_id BETWEEN ${minId} AND 15;`)
  sql.push(`DELETE FROM nodes WHERE area_id BETWEEN ${minId} AND 15;`)
  sql.push(`DELETE FROM scenic_spots WHERE id BETWEEN ${minId} AND 15;`)
  sql.push(`DELETE FROM indoor_roads WHERE building_id IN (SELECT id FROM buildings WHERE area_id BETWEEN 1001 AND 1210);`)
  sql.push(`DELETE FROM indoor_nodes WHERE building_id IN (SELECT id FROM buildings WHERE area_id BETWEEN 1001 AND 1210);`)
  sql.push(`DELETE FROM view_history WHERE spot_id BETWEEN 1001 AND 1210;`)
  sql.push(`DELETE FROM ratings WHERE diary_id IN (SELECT id FROM diaries WHERE destination_id BETWEEN 1001 AND 1210);`)
  sql.push(`DELETE FROM diaries WHERE destination_id BETWEEN 1001 AND 1210;`)
  sql.push(`DELETE FROM foods WHERE area_id BETWEEN 1001 AND 1210;`)
  sql.push(`DELETE FROM buildings WHERE area_id BETWEEN 1001 AND 1210;`)
  sql.push(`DELETE FROM roads WHERE area_id BETWEEN 1001 AND 1210;`)
  sql.push(`DELETE FROM nodes WHERE area_id BETWEEN 1001 AND 1210;`)
  sql.push(`DELETE FROM scenic_spots WHERE id BETWEEN 1001 AND 1210;`)

  // Demo users
  const usernames = ['zhangsan','lisi','wangwu','zhaoliu','sunqi','zhouba','wujiu','zhengshi','admin','testuser']
  const nicknames = ['张三','李四','王五','赵六','孙七','周八','吴九','郑十','管理员','测试用户']
  for (let i = 1; i <= 25; i++) {
    const username = i <= 10 ? usernames[i - 1] : `demo_user_${i}`
    const nickname = i <= 10 ? nicknames[i - 1] : `演示用户${i}`
    const role = i === 9 ? 'admin' : 'user'
    sql.push(`INSERT OR REPLACE INTO users (id, username, password, nickname, role) VALUES (${i}, ${q(username)}, 'pass123', ${q(nickname)}, '${role}');`)
  }

  const allStats = { nodes: 0, roads: 0, buildings: 0, foods: 0, diaries: 0, ratings: 0, viewHistory: 0, indoorNodes: 0, indoorRoads: 0 }

  for (let i = 0; i < AREA_COUNT; i++) {
    const profile = makeAreaProfile(i)

    sql.push(
      `INSERT INTO scenic_spots (id, name, type, category, description, popularity, rating, rating_count, image, city, address, open_time, ticket_price, canvas_width, canvas_height) VALUES ` +
      `(${profile.areaId}, ${q(profile.name)}, ${q(profile.type)}, ${q(profile.category)}, ${q(profile.description)}, ${profile.popularity}, ${profile.rating}, ${120 + i}, ${q(getSpotImage(profile))}, ${q(profile.city)}, ${q(profile.address)}, ${q(profile.openTime)}, ${q(profile.ticket)}, ${CANVAS_W}, ${CANVAS_H});`
    )

    const nodes = generateNodeLayout(profile)
    for (const node of nodes) {
      node.id = nid(profile.areaId, node.idx)
      sql.push(`INSERT INTO nodes (id, area_id, name, type, sub_type, pos_x, pos_y, description, floor) VALUES (${node.id}, ${profile.areaId}, ${q(node.name)}, ${q(node.type)}, ${q(node.subType)}, ${node.x}, ${node.y}, ${q(`${node.name}，${profile.name}`)}, 1);`)
    }
    allStats.nodes += nodes.length

    const roads = generateRoads(nodes, profile)
    for (const road of roads) {
      sql.push(`INSERT OR IGNORE INTO roads (id, area_id, from_node, to_node, distance, congestion, ideal_speed, transport, is_bidirectional) VALUES (${road.id}, ${road.areaId}, ${road.fromNode}, ${road.toNode}, ${road.distance}, ${road.congestion}, ${road.speed}, ${road.transport}, ${road.isBidirectional});`)
    }
    allStats.roads += roads.length

    const buildings = generateBuildings(nodes, profile)
    for (const b of buildings) {
      sql.push(`INSERT INTO buildings (id, area_id, name, type, total_floors, has_elevator, description) VALUES (${b.id}, ${b.areaId}, ${q(b.name)}, ${q(b.type)}, ${b.totalFloors}, ${b.hasElevator}, ${q(b.description)});`)
    }
    allStats.buildings += buildings.length

    const foods = generateFoods(nodes, profile)
    for (const food of foods) {
      sql.push(`INSERT INTO foods (id, area_id, name, cuisine, restaurant, rating, rating_count, popularity, node_id, price, description, image) VALUES (${food.id}, ${food.areaId}, ${q(food.name)}, ${q(food.cuisine)}, ${q(food.restaurant)}, ${food.rating}, ${food.ratingCount}, ${food.popularity}, ${food.nodeId}, ${food.price}, ${q(food.description)}, ${q(food.image)});`)
    }
    allStats.foods += foods.length

    const diaries = generateDiaries(profile)
    for (const d of diaries) {
      sql.push(`INSERT INTO diaries (id, user_id, title, content, destination, destination_id, tags, images, videos, popularity, avg_rating, rating_count, created_at) VALUES (${d.id}, ${d.userId}, ${q(d.title)}, ${q(d.content)}, ${q(d.destination)}, ${d.destinationId}, ${q(d.tags)}, ${q(d.images)}, ${q(d.videos)}, ${d.popularity}, ${d.avgRating}, ${d.ratingCount}, datetime('now'));`)
    }
    allStats.diaries += diaries.length

    const ratings = generateRatings(diaries)
    for (const r of ratings) {
      sql.push(`INSERT OR IGNORE INTO ratings (id, user_id, diary_id, score, created_at) VALUES (${r.id}, ${r.userId}, ${r.diaryId}, ${r.score}, datetime('now'));`)
    }
    allStats.ratings += ratings.length

    const viewHistory = generateViewHistory(profile)
    for (const v of viewHistory) {
      sql.push(`INSERT INTO view_history (id, user_id, spot_id, view_time) VALUES (${v.id}, ${v.userId}, ${v.spotId}, datetime('now', '-${Math.floor(seededRandom(v.id) * 30)} days'));`)
    }
    allStats.viewHistory += viewHistory.length

    const { indoorNodes, indoorRoads } = generateIndoor(buildings, nodes, profile)
    for (const inode of indoorNodes) {
      sql.push(`INSERT INTO indoor_nodes (id, building_id, floor, name, type, pos_x, pos_y) VALUES (${inode.id}, ${inode.buildingId}, ${inode.floor}, ${q(inode.name)}, ${q(inode.type)}, ${inode.pos_x}, ${inode.pos_y});`)
    }
    allStats.indoorNodes += indoorNodes.length
    for (const iroad of indoorRoads) {
      sql.push(`INSERT INTO indoor_roads (id, building_id, from_node, to_node, distance) VALUES (${iroad.id}, ${iroad.buildingId}, ${iroad.fromNode}, ${iroad.toNode}, ${iroad.distance});`)
    }
    allStats.indoorRoads += indoorRoads.length
  }

  // User interests
  sql.push(`DELETE FROM user_interests WHERE user_id BETWEEN 1 AND 25;`)
  const interestCategories = ['自然', '历史', '文化', '现代', '综合', '美食', '户外', '人文']
  for (let userId = 1; userId <= 15; userId++) {
    const numInterests = randInt(userId * 7, 2, 4)
    for (let j = 0; j < numInterests; j++) {
      const cat = interestCategories[(userId + j) % interestCategories.length]
      const weight = (0.3 + seededRandom(userId * 100 + j) * 0.7).toFixed(1)
      sql.push(`INSERT OR IGNORE INTO user_interests (user_id, category, weight) VALUES (${userId}, ${q(cat)}, ${weight});`)
    }
  }

  sql.push('COMMIT;')
  sql.push('')

  return { sql: sql.join('\n'), stats: allStats }
}

// ==================== Main ====================
const { sql: content, stats } = generateSQL()
writeFileSync(outFile, content, 'utf8')

const backendDataDir = join(__dirname, '..', 'smart-tourism-backend', 'data')
try { mkdirSync(backendDataDir, { recursive: true }) } catch {}
writeFileSync(backendOutFile, content, 'utf8')

// Also copy to build directory
try { mkdirSync(buildDataDir, { recursive: true }) } catch {}
writeFileSync(join(buildDataDir, 'generated_demo_seed.sql'), content, 'utf8')

console.log(`Generated: ${outFile}`)
console.log(`Generated: ${backendOutFile}`)
console.log(`Generated: ${join(buildDataDir, 'generated_demo_seed.sql')}`)
console.log(`Stats:`)
for (const [key, val] of Object.entries(stats)) {
  console.log(`  ${key}: ${val}`)
}
