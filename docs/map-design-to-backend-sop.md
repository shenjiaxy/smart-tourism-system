# 景区/校园地图母版到后端落库 SOP

## 目标

先在前端完成一张高质量示范地图，确认视觉、缩放、拖拽、路线动画和节点选择体验，再把同一套地图数据写入后端数据库。不要先堆随机后端点位，否则前端只能画出流程图，无法形成真实地图感。

示范对象：`北湖书院示范校园`。

## 参考风格

本项目地图 demo 采用“Google Maps 浅色底图”方向：低饱和背景、灰白道路、水体浅蓝、绿地浅绿、建筑浅米灰、POI 小圆点、路线使用高饱和蓝色。

设计依据：Google Maps 样式系统把地图拆成 road、water、park、poi、label 等 feature type；近期 Google Maps 的视觉更新也更偏灰白道路、浅蓝水体、浅绿自然区域。实现时不要照搬截图，而是复刻这种图层逻辑和视觉层级。

## 地图母版必须包含

1. `base area`
   - 使用固定设计坐标，例如 `1000 x 900` 或 `1100 x 900`。
   - 所有建筑、道路、水体、步道、节点都使用同一个坐标系。

2. `water / green / district polygons`
   - 水体、草地、教学区、住宿区、服务区先用不规则 polygon 画出来。
   - 不要全部矩形化；边界需要自然弯曲或折角。

3. `building footprints`
   - 每栋建筑必须有俯视轮廓 polygon，不能再用矩形、3D 方块或单一图标代替。
   - 建筑轮廓建议使用不规则多边形、折角、内院线、屋顶分区线、入口短线。
   - 每栋建筑至少包含 `name`, `kind`, `points`, `label`。
   - 建筑名称必须直接画在地图上，用户不用悬浮也能知道它是什么。

4. `road layers`
   - 主干路：白色道路主体 + 浅灰 casing，线宽最大。
   - 支路：白色道路主体 + 更细浅灰 casing。
   - 步道/景观路：低饱和灰绿虚线或细线。
   - 不要把道路画成黑色流程线；不要用大面积黄线，除非表现导航或交通状态。
   - 路网密度要符合景区/校园，核心区道路和步道要比外围更密。

5. `labels / POI`
   - 大区域标签：低透明度蓝灰或绿灰，例如湖泊、教学核心区、住宿区。
   - 建筑标签：深灰主标题 + 浅灰副标题。
   - POI 点：小圆点，选中态用蓝色，普通态用灰色或绿色。

6. `route graph`
   - 每个可导航点必须对应一个 `node`。
   - 每条可通行道路必须对应一条 `road edge`。
   - `road.distance` 必须来自坐标距离或真实路段长度，不能随便填。

## 前端母版数据结构

```ts
type Point = [number, number]

type Building = {
  name: string
  kind: string
  points: Point[]
  label: Point
}

type Node = {
  id: number
  area_id: number
  name: string
  type: 'building' | 'facility' | 'entrance' | 'junction'
  sub_type: string
  pos_x: number
  pos_y: number
  description: string
}

type Road = {
  id: number
  area_id: number
  from_node: number
  to_node: number
  distance: number
  congestion: number
  ideal_speed: number
  transport: number
  is_bidirectional: boolean
}
```

## 距离计算规则

基础距离：

```ts
distance = Math.round(Math.hypot(x1 - x2, y1 - y2))
```

如果道路是折线或曲线，按路径分段累加：

```ts
distance = sum(segmentLength(point[i], point[i + 1]))
```

后端数据库里的 `roads.distance` 必须使用同一规则。前端显示、后端 Dijkstra、路线动画三者才能一致。

## 后端落库步骤

1. 新增或更新 `scenic_spots`
   - `id`: 固定示例 ID，例如 `900001`
   - `name`: `北湖书院示范校园`
   - `type`: `campus`
   - `category`: `大学校园`

2. 写入 `nodes`
   - 每个建筑 footprint 生成一个建筑节点，坐标取 polygon 中心点。
   - 入口、湖岸、广场、服务点可以单独建节点。
   - `pos_x/pos_y` 必须使用前端母版坐标。

3. 写入 `roads`
   - 主干路、支路、步道中真正可通行的连接关系写入 `roads`。
   - `from_node/to_node` 连接节点 ID。
   - `distance` 按上面的距离规则计算。
   - `transport`: 可用 `1` 表示步行，`2` 表示主路。

4. 扩展建筑轮廓数据
   - 当前后端如果没有 footprint 表，建议新增表：

```sql
CREATE TABLE IF NOT EXISTS map_footprints (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  area_id INTEGER NOT NULL,
  node_id INTEGER,
  name TEXT NOT NULL,
  kind TEXT NOT NULL,
  points_json TEXT NOT NULL,
  label_x REAL NOT NULL,
  label_y REAL NOT NULL
);
```

5. 前端读取
   - `/api/map/graph/:area_id` 返回 `nodes` 和 `roads`。
   - 新增 `/api/map/footprints/:area_id` 返回建筑轮廓、地块、水体和标签。
   - 前端不再写死示范数据，只负责按图层渲染。

## 渲染顺序

1. 背景底色和极淡网格
2. 水体、绿地、分区 polygon
3. 主干路 casing + 主干路主体
4. 支路 casing + 支路主体
5. 步道、虚线边界、景观路径
6. 建筑 footprint
7. 建筑屋顶细节、内院线、入口符号
8. 区域标签和建筑标签
9. POI 节点
10. 路线动画
11. 图例和缩放控件

## Google 风格颜色建议

```css
--map-bg: #f5f3ee;
--map-road-casing: #cfd4d5;
--map-road-fill: #ffffff;
--map-path: #8d9a9a;
--map-water: #cfe8f7;
--map-green: #dcebd4;
--map-building: #eee8dc;
--map-label: #3c4043;
--map-muted-label: #5f6368;
--map-route: #1a73e8;
```

## 交互验收标准

- 默认视野像地图应用，不像一张被缩小的静态海报。
- 支持滚轮缩放、拖拽平移、点击选点。
- 路线生成后使用蓝色路径动画，从起点逐段绘制到终点。
- 缩放后标签、道路、POI 不应明显糊成一团。
- 右下角图例只保留必要信息，不要把地图做成花花绿绿的色块说明书。

## 视觉验收标准

- 第一眼能看出是校园/景区地图，不像随机流程图。
- 至少 12 个建筑 footprint，并且每个有名称。
- 建筑不能全是矩形，至少 70% 建筑要有不规则轮廓或内部细节。
- 至少 3 类道路层级：主路、支路、步道。
- 颜色必须低饱和，黑色只用于少量高优先级文字，不用于大面积道路。
- 水体、绿地、道路、建筑、POI、路线各自有稳定语义，不要临时乱加颜色。
- 路线生成使用 `roads.distance`，不是屏幕直线假动画。
- 后端重启后同一区域还能复现同一张地图。