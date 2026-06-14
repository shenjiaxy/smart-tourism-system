<template>
  <header class="site-header" :class="{ 'site-header--scrolled': isScrolled }">
    <router-link to="/" class="brand-link" aria-label="智能旅游首页">
      <span class="brand-mark"><MapPinned :size="20" /></span>
      <span class="brand-copy">
        <strong>智能旅游</strong>
        <small>Destination Intelligence</small>
      </span>
    </router-link>

    <nav class="desktop-nav" aria-label="主导航">
      <router-link
        v-for="item in navItems"
        :key="item.path"
        :to="item.path"
        class="nav-item"
        :class="{ 'nav-item--active': isActive(item.path) }"
      >
        <component :is="item.icon" :size="16" />
        <span>{{ item.label }}</span>
      </router-link>
    </nav>

    <div class="header-side">
      <router-link v-if="auth.isAdmin" to="/admin" class="admin-entry">
        <ShieldCheck :size="15" /> 管理后台
      </router-link>
      <div class="user-chip">
        <span>{{ initial }}</span>
        <div>
          <strong>{{ auth.user?.nickname || auth.user?.username }}</strong>
          <small>{{ auth.isAdmin ? '管理员' : '普通用户' }}</small>
        </div>
      </div>
      <button class="logout-button" title="退出登录" @click="handleLogout">
        <LogOut :size="17" />
      </button>
    </div>
  </header>
</template>

<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import {
  Coffee,
  Compass,
  Edit,
  Home,
  LogOut,
  Map,
  MapPinned,
  Search,
  ShieldCheck,
} from 'lucide-vue-next'
import { useAuthStore } from '@/stores/auth'

const route = useRoute()
const router = useRouter()
const auth = useAuthStore()
const isScrolled = ref(false)

const initial = computed(() => {
  const name = auth.user?.nickname || auth.user?.username || '用户'
  return name.slice(0, 1)
})

const navItems = [
  { path: '/', label: '首页', icon: Home },
  { path: '/recommend', label: '旅游推荐', icon: Compass },
  { path: '/map', label: '路线规划', icon: Map },
  { path: '/facility', label: '场所查询', icon: Search },
  { path: '/diary', label: '旅游日记', icon: Edit },
  { path: '/food', label: '美食推荐', icon: Coffee },
]

function isActive(path: string) {
  return path === '/' ? route.path === '/' : route.path === path
}

function handleScroll() {
  isScrolled.value = window.scrollY > 8
}

async function handleLogout() {
  await auth.logout()
  router.replace('/login')
}

onMounted(() => window.addEventListener('scroll', handleScroll, { passive: true }))
onUnmounted(() => window.removeEventListener('scroll', handleScroll))
</script>

<style scoped>
.site-header {
  position: fixed;
  inset: 0 0 auto;
  z-index: 50;
  height: 72px;
  display: flex;
  align-items: center;
  gap: 14px;
  padding: 0 22px;
  border-bottom: 1px solid var(--color-rule);
  background: rgba(242, 239, 231, 0.92);
  backdrop-filter: blur(16px);
}
.site-header--scrolled { background: rgba(255, 253, 247, 0.96); box-shadow: var(--shadow-md); }
.brand-link { display: flex; align-items: center; gap: 10px; color: var(--color-ink); text-decoration: none; }
.brand-mark { width: 40px; height: 40px; display: grid; place-items: center; color: white; background: var(--color-ink); }
.brand-copy { display: grid; min-width: 132px; }
.brand-copy strong { font-size: 17px; line-height: 1.1; }
.brand-copy small { color: var(--color-editorial-red); font-size: 9px; font-weight: 800; letter-spacing: .1em; }
.desktop-nav { display: flex; align-items: center; justify-content: center; gap: 2px; flex: 1; min-width: 0; }
.nav-item { display: inline-flex; align-items: center; gap: 5px; padding: 8px 7px; color: var(--color-text-secondary); border: 1px solid transparent; font-size: 12px; font-weight: 700; text-decoration: none; white-space: nowrap; }
.nav-item:hover, .nav-item--active { color: var(--color-ink); background: var(--color-surface); border-color: var(--color-rule); }
.header-side { display: flex; align-items: center; gap: 9px; }
.admin-entry { display: inline-flex; align-items: center; gap: 5px; padding: 8px 9px; color: white; background: var(--color-editorial-red); font-size: 12px; font-weight: 800; text-decoration: none; }
.user-chip { display: flex; align-items: center; gap: 8px; min-width: 112px; }
.user-chip > span { width: 34px; height: 34px; display: grid; place-items: center; color: white; background: var(--color-primary); border-radius: 50%; font-weight: 900; }
.user-chip div { display: grid; line-height: 1.2; }
.user-chip strong { font-size: 12px; }
.user-chip small { color: var(--color-muted); font-size: 10px; }
.logout-button { width: 34px; height: 34px; display: grid; place-items: center; color: var(--color-text-secondary); border: 1px solid var(--color-rule); background: var(--color-surface); cursor: pointer; }
@media (max-width: 1180px) {
  .brand-copy small, .user-chip div, .admin-entry { display: none; }
  .desktop-nav { overflow-x: auto; justify-content: flex-start; }
}
</style>
