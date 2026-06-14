<template>
  <div class="admin-shell">
    <aside class="admin-sidebar">
      <router-link to="/admin" class="admin-brand">
        <span><ShieldCheck :size="23" /></span>
        <div><strong>智能旅游</strong><small>ADMIN CONSOLE</small></div>
      </router-link>

      <p class="nav-label">管理中心</p>
      <nav>
        <router-link to="/admin" exact-active-class="active">
          <LayoutDashboard :size="18" /> 后台总览
        </router-link>
        <router-link to="/admin/users" active-class="active">
          <Users :size="18" /> 用户管理
        </router-link>
        <router-link to="/admin/diaries" active-class="active">
          <NotebookTabs :size="18" /> 日记管理
        </router-link>
      </nav>

      <div class="sidebar-footer">
        <router-link to="/" class="return-link"><ExternalLink :size="16" /> 返回用户端</router-link>
        <button @click="handleLogout"><LogOut :size="16" /> 退出登录</button>
      </div>
    </aside>

    <main class="admin-main">
      <header class="admin-topbar">
        <div>
          <p>SMART TOURISM CONTROL</p>
          <h1>{{ route.meta.title }}</h1>
        </div>
        <div class="admin-user">
          <span>{{ initial }}</span>
          <div><strong>{{ auth.user?.nickname }}</strong><small>{{ auth.user?.username }}</small></div>
        </div>
      </header>
      <section class="admin-content">
        <router-view />
      </section>
    </main>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import {
  ExternalLink,
  LayoutDashboard,
  LogOut,
  NotebookTabs,
  ShieldCheck,
  Users,
} from 'lucide-vue-next'
import { useAuthStore } from '@/stores/auth'

const route = useRoute()
const router = useRouter()
const auth = useAuthStore()
const initial = computed(() => (auth.user?.nickname || auth.user?.username || '管').slice(0, 1))

async function handleLogout() {
  await auth.logout()
  router.replace('/login')
}
</script>

<style scoped>
.admin-shell { min-height: 100vh; display: grid; grid-template-columns: 250px 1fr; background: #ece9e0; }
.admin-sidebar { position: sticky; top: 0; height: 100vh; display: flex; flex-direction: column; padding: 24px 18px; color: #f8f5ed; background: #20231e; }
.admin-brand { display: flex; align-items: center; gap: 12px; padding: 6px; color: white; text-decoration: none; }
.admin-brand > span { width: 42px; height: 42px; display: grid; place-items: center; color: #20231e; background: #d8c28e; }
.admin-brand div { display: grid; }
.admin-brand strong { font-size: 17px; }
.admin-brand small { color: #d8c28e; font-size: 9px; letter-spacing: .15em; }
.nav-label { margin: 48px 10px 12px; color: rgba(255,255,255,.38); font-size: 10px; font-weight: 900; letter-spacing: .14em; }
nav { display: grid; gap: 5px; }
nav a { display: flex; align-items: center; gap: 11px; padding: 12px 13px; color: rgba(255,255,255,.68); border: 1px solid transparent; text-decoration: none; font-weight: 700; }
nav a:hover, nav a.active { color: white; border-color: rgba(255,255,255,.12); background: rgba(255,255,255,.08); }
.sidebar-footer { display: grid; gap: 8px; margin-top: auto; padding-top: 22px; border-top: 1px solid rgba(255,255,255,.12); }
.sidebar-footer a, .sidebar-footer button { display: flex; align-items: center; gap: 8px; padding: 10px; color: rgba(255,255,255,.68); border: 0; background: transparent; text-decoration: none; cursor: pointer; }
.admin-main { min-width: 0; }
.admin-topbar { height: 94px; display: flex; align-items: center; justify-content: space-between; padding: 0 34px; border-bottom: 1px solid #cbc5b7; background: rgba(255,253,247,.84); backdrop-filter: blur(14px); }
.admin-topbar p { color: #a06c37; font-size: 9px; font-weight: 900; letter-spacing: .17em; }
.admin-topbar h1 { margin-top: 2px; font-family: Georgia, "Noto Serif SC", serif; font-size: 26px; }
.admin-user { display: flex; align-items: center; gap: 10px; }
.admin-user > span { width: 38px; height: 38px; display: grid; place-items: center; color: white; background: #5e6347; border-radius: 50%; font-weight: 900; }
.admin-user div { display: grid; line-height: 1.25; }
.admin-user small { color: #746f62; }
.admin-content { padding: 30px 34px 44px; }
@media (max-width: 760px) {
  .admin-shell { grid-template-columns: 72px 1fr; }
  .admin-sidebar { padding-inline: 10px; }
  .admin-brand div, nav a { font-size: 0; }
  .admin-brand { justify-content: center; }
  nav a { justify-content: center; }
  .nav-label, .sidebar-footer { display: none; }
  .admin-topbar, .admin-content { padding-left: 20px; padding-right: 20px; }
}
</style>
