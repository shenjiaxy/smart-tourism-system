import { defineStore } from 'pinia'
import * as authApi from '@/api/auth'
import type { AuthUser } from '@/types'
import {
  AUTH_TOKEN_KEY,
  AUTH_USER_KEY,
  clearStoredAuth,
  readStoredUser,
} from '@/utils/authStorage'

export const useAuthStore = defineStore('auth', {
  state: () => ({
    token: localStorage.getItem(AUTH_TOKEN_KEY) || '',
    user: readStoredUser() as AuthUser | null,
  }),
  getters: {
    isAuthenticated: (state) => Boolean(state.token && state.user),
    isAdmin: (state) => state.user?.role === 'admin',
  },
  actions: {
    persist() {
      if (!this.token || !this.user) return
      localStorage.setItem(AUTH_TOKEN_KEY, this.token)
      localStorage.setItem(AUTH_USER_KEY, JSON.stringify(this.user))
    },
    clear() {
      this.token = ''
      this.user = null
      clearStoredAuth()
    },
    async login(username: string, password: string) {
      const response = await authApi.login(username, password)
      this.token = response.data.token
      this.user = response.data.user
      this.persist()
      return this.user
    },
    async restore() {
      if (!this.token) return
      try {
        const response = await authApi.getCurrentUser()
        this.user = response.data
        this.persist()
      } catch {
        this.clear()
      }
    },
    async logout() {
      try {
        if (this.token) await authApi.logout()
      } finally {
        this.clear()
      }
    },
  },
})
