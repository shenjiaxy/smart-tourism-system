export interface Spot {
  id: number
  name: string
  type: string
  category: string
  description: string
  popularity: number
  rating: number
  rating_count: number
  city: string
  address: string
  open_time: string
  ticket_price: number | string
  canvas_width?: number
  canvas_height?: number
  image?: string
  image_url?: string
}

export interface MapNode {
  id: number
  area_id?: number
  name: string
  type: string
  sub_type: string
  pos_x: number
  pos_y: number
  description: string
  floor?: number
}

export interface Road {
  id: number
  area_id?: number
  from_node: number
  to_node: number
  distance: number
  congestion: number
  ideal_speed: number
  transport: number
  is_bidirectional: number | boolean
}

export interface GraphData {
  nodes: MapNode[]
  roads?: Road[]
  edges?: Road[]
}

export interface SingleRouteResult {
  path: number[]
  found?: boolean
  distance: number
  time: number
  total_distance?: number
  total_time?: number
  path_length?: number
  strategy?: string
  node_names?: string[]
  path_names?: string[]
  transport_modes?: number[]
  transfers?: Array<{
    node_id: number
    node_name: string
    from_transport: number
    to_transport: number
  }>
  transfer_count?: number
}

export interface MultiRouteResult {
  path: number[]
  found?: boolean
  segments?: SingleRouteResult[]
  distance?: number
  time?: number
  total_distance?: number
  total_time?: number
  path_length?: number
  node_names?: string[]
  path_names?: string[]
}

export interface RouteRequest {
  area_id: number
  from_node: number
  to_node: number
  waypoints?: number[]
  strategy: 'distance' | 'time' | 'mixed'
  transport?: number
}

export interface IndoorBuilding {
  id: number
  area_id: number
  name: string
  type: string
  total_floors: number
  has_elevator: number
  description: string
  node_count: number
}

export interface IndoorNode {
  id: number
  building_id: number
  floor: number
  name: string
  type: 'entrance' | 'elevator' | 'stairs' | 'room' | 'facility'
  pos_x: number
  pos_y: number
}

export interface IndoorRoad {
  id: number
  building_id: number
  from_node: number
  to_node: number
  distance: number
}

export interface IndoorGraphData {
  building_id: number
  nodes: IndoorNode[]
  roads: IndoorRoad[]
  node_count: number
  road_count: number
}

export interface IndoorRouteResult {
  found: boolean
  building_id: number
  path: number[]
  path_names: string[]
  floors: number[]
  steps: string[]
  floor_changes: number
  distance: number
  time: number
  strategy: 'distance' | 'time'
  message?: string
}

export interface FacilityItem {
  id?: number
  node_id: number
  name: string
  type: string
  sub_type: string
  distance: number
  path_distance?: number
  direct_distance: number
  straight_distance?: number
  description: string
  pos_x: number
  pos_y: number
}

export interface Diary {
  id: number
  user_id: number
  username?: string
  title: string
  content: string
  destination: string
  destination_id: number
  tags: string[]
  images: string[]
  videos: string[]
  popularity: number
  avg_rating: number
  rating_count: number
  created_at: string
  is_compressed: boolean | number
}

export interface CreateDiaryRequest {
  user_id: number
  title: string
  content: string
  destination?: string
  destination_id?: number
  tags?: string
  images?: string[]
  videos?: string[]
}

export interface DiarySearchResult extends Diary {
  diary_id: number
  score: number
  relevance_score?: number
  snippet?: string
}

export interface Food {
  id: number
  area_id: number
  name: string
  cuisine: string
  restaurant: string
  rating: number
  rating_count: number
  popularity: number
  node_id: number
  price: number
  description: string
  image?: string
  image_url?: string
  similarity?: number
}

export interface FuzzyMatchResult extends Partial<Food> {
  name: string
  similarity?: number
  food?: Food
}

export interface UserInterest {
  category: string
  weight: number
}

export interface AuthUser {
  id: number
  username: string
  nickname: string
  avatar: string
  role: 'user' | 'admin'
}

export interface LoginResult {
  token: string
  user: AuthUser
}

export interface AdminOverview {
  users: number
  admins: number
  scenic_spots: number
  nodes: number
  roads: number
  diaries: number
  foods: number
  ratings: number
}

export interface AdminUser extends AuthUser {
  created_at: string
}

export interface AdminDiary {
  id: number
  title: string
  destination: string
  popularity: number
  avg_rating: number
  rating_count: number
  created_at: string
  user_id: number
  username: string
  nickname: string
}

export interface PageResult<T = any> {
  items: T[]
  total: number
  page: number
  page_size: number
  total_pages: number
}
