import { get, post } from '@/utils/http'
import type {
  RouteRequest,
  SingleRouteResult,
  MultiRouteResult,
  IndoorBuilding,
  IndoorGraphData,
  IndoorRouteResult,
} from '@/types'

/** 单目标最短路径规划 */
export function planSingleRoute(data: RouteRequest) {
  return post<SingleRouteResult>('/api/route/single', data)
}

/** 多目标TSP路径规划 */
export function planMultiRoute(data: RouteRequest) {
  return post<MultiRouteResult>('/api/route/multi', data)
}

export function getIndoorBuildings(areaId: number) {
  return get<IndoorBuilding[]>('/api/route/indoor/buildings', { area_id: areaId })
}

export function getIndoorGraph(buildingId: number) {
  return get<IndoorGraphData>(`/api/route/indoor/graph/${buildingId}`)
}

export function planIndoorRoute(data: {
  building_id: number
  from_node: number
  to_node: number
  strategy: 'distance' | 'time'
}) {
  return post<IndoorRouteResult>('/api/route/indoor', data)
}
