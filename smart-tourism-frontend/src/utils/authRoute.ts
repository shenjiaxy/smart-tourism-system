export type AuthRole = 'user' | 'admin'

export interface AuthRouteState {
  authenticated: boolean
  role: AuthRole | null
}

export interface AuthRouteMeta {
  requiresAuth?: boolean
  guestOnly?: boolean
  adminOnly?: boolean
}

export function resolveRouteAccess(
  auth: AuthRouteState,
  meta: AuthRouteMeta,
): string | null {
  if (meta.requiresAuth && !auth.authenticated) return '/login'
  if (meta.adminOnly && auth.role !== 'admin') return auth.authenticated ? '/' : '/login'
  if (meta.guestOnly && auth.authenticated) return auth.role === 'admin' ? '/admin' : '/'
  return null
}
