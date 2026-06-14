import type { AuthUser } from '@/types'

export const AUTH_TOKEN_KEY = 'smart-tourism-token'
export const AUTH_USER_KEY = 'smart-tourism-user'

export function readStoredUser(): AuthUser | null {
  const raw = localStorage.getItem(AUTH_USER_KEY)
  if (!raw) return null
  try {
    return JSON.parse(raw) as AuthUser
  } catch {
    localStorage.removeItem(AUTH_USER_KEY)
    return null
  }
}

export function clearStoredAuth() {
  localStorage.removeItem(AUTH_TOKEN_KEY)
  localStorage.removeItem(AUTH_USER_KEY)
}
