import { get, post } from '@/utils/http'
import type { AuthUser, LoginResult } from '@/types'

export function login(username: string, password: string) {
  return post<LoginResult>('/api/auth/login', { username, password })
}

export function getCurrentUser() {
  return get<AuthUser>('/api/auth/me')
}

export function logout() {
  return post<Record<string, never>>('/api/auth/logout')
}
