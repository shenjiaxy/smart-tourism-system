import { get } from '@/utils/http'
import type { Food, FuzzyMatchResult, PageResult } from '@/types'

/** 获取美食推荐 */
export function getFoodRecommendations(params: {
  area_id: number
  limit?: number
  sort_by?: string
  cuisine?: string
  ref_x?: number
  ref_y?: number
}) {
  return get<Food[]>('/api/foods/recommend', params)
}

/** 模糊搜索美食 */
export function searchFoods(params: {
  area_id: number
  keyword: string
  limit?: number
  sort_by?: string
  ref_x?: number
  ref_y?: number
}) {
  return get<FuzzyMatchResult[]>('/api/foods/search', params)
}

/** 获取美食列表（分页） */
export function getFoodList(params: {
  area_id: number
  page?: number
  page_size?: number
  sort_by?: string
  cuisine?: string
}) {
  return get<PageResult<Food>>('/api/foods', params)
}
