import { del, get, put } from '@/utils/http'
import type { AdminDiary, AdminOverview, AdminUser } from '@/types'

export function getAdminOverview() {
  return get<AdminOverview>('/api/admin/overview')
}

export function getAdminUsers() {
  return get<AdminUser[]>('/api/admin/users')
}

export function updateAdminUserRole(id: number, role: 'user' | 'admin') {
  return put<{ id: number; role: 'user' | 'admin' }>(`/api/admin/users/${id}/role`, { role })
}

export function getAdminDiaries() {
  return get<AdminDiary[]>('/api/admin/diaries')
}

export function deleteAdminDiary(id: number) {
  return del<{ id: number }>(`/api/admin/diaries/${id}`)
}
