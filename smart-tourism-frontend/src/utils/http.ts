import axios, {
  type AxiosInstance,
  type AxiosRequestConfig,
  type AxiosResponse,
  type InternalAxiosRequestConfig,
} from 'axios'
import { ElMessage } from 'element-plus'
import { AUTH_TOKEN_KEY, clearStoredAuth } from '@/utils/authStorage'

/** 后端API基础地址 */
const BASE_URL = import.meta.env.VITE_API_BASE_URL || ''

/** 创建 Axios 实例 */
const http: AxiosInstance = axios.create({
  baseURL: BASE_URL,
  timeout: 15000,
  headers: {
    'Content-Type': 'application/json',
  },
})

// ==================== 请求拦截 ====================
http.interceptors.request.use(
  (config: InternalAxiosRequestConfig) => {
    const token = localStorage.getItem(AUTH_TOKEN_KEY)
    if (token) config.headers.Authorization = `Bearer ${token}`
    // 可在此添加 token 等认证信息
    // const token = localStorage.getItem('token')
    // if (token) config.headers.Authorization = `Bearer ${token}`
    return config
  },
  (error) => {
    return Promise.reject(error)
  }
)

// ==================== 响应拦截 ====================
http.interceptors.response.use(
  (response: AxiosResponse) => {
    const res = response.data

    // 后端统一格式: { code: 200, message: "success", data: ... }
    if (res.code !== undefined && res.code !== 200) {
      ElMessage.error(res.message || '请求失败')
      return Promise.reject(new Error(res.message || '请求失败'))
    }

    return res
  },
  (error) => {
    const msg =
      error.response?.data?.message ||
      error.message ||
      '网络连接失败，请检查后端服务是否启动'

    if (error.response) {
      switch (error.response.status) {
        case 401:
          clearStoredAuth()
          if (!window.location.hash.startsWith('#/login')) {
            window.location.hash = '#/login'
          }
          ElMessage.error(msg)
          break
        case 403:
          ElMessage.error(`没有操作权限: ${msg}`)
          break
        case 400:
          ElMessage.error(`请求参数错误: ${msg}`)
          break
        case 404:
          ElMessage.error(`资源不存在: ${msg}`)
          break
        case 500:
          ElMessage.error(`服务器错误: ${msg}`)
          break
        default:
          ElMessage.error(msg)
      }
    } else {
      ElMessage.error(msg)
    }

    return Promise.reject(error)
  }
)

// ==================== 类型安全的请求方法 ====================

/** 通用请求方法 */
export async function request<T = any>(
  config: AxiosRequestConfig
): Promise<ApiResponse<T>> {
  return http.request<any, ApiResponse<T>>(config)
}

/** GET 请求 */
export async function get<T = any>(
  url: string,
  params?: Record<string, any>
): Promise<ApiResponse<T>> {
  return http.get<any, ApiResponse<T>>(url, { params })
}

/** POST 请求 */
export async function post<T = any>(
  url: string,
  data?: Record<string, any>
): Promise<ApiResponse<T>> {
  return http.post<any, ApiResponse<T>>(url, data)
}

/** PUT 请求 */
export async function put<T = any>(
  url: string,
  data?: Record<string, any>
): Promise<ApiResponse<T>> {
  return http.put<any, ApiResponse<T>>(url, data)
}

/** DELETE 请求 */
export async function del<T = any>(
  url: string,
  params?: Record<string, any>
): Promise<ApiResponse<T>> {
  return http.delete<any, ApiResponse<T>>(url, { params })
}

export default http

// ==================== 通用响应类型 ====================

/** 后端统一响应格式 */
export interface ApiResponse<T = any> {
  code: number
  message: string
  data: T
}

/** 分页响应 */
export interface PageResult<T = any> {
  items: T[]
  total: number
  page: number
  page_size: number
  total_pages: number
}
