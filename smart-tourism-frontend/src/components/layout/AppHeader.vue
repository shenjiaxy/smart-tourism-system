<template>
  <header class="site-header" :class="{ 'site-header--scrolled': isScrolled }">
    <router-link to="/" class="brand-link" aria-label="智能旅游首页">
      <span class="brand-mark">
        <MapPinned :size="20" />
      </span>
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
        <component :is="item.icon" :size="17" />
        <span>{{ item.label }}</span>
      </router-link>
    </nav>

    <div class="header-side">
      <router-link to="/recommend" class="header-cta">探索目的地</router-link>
      <div class="user-chip" title="游客模式">
        <span>游</span>
        <strong>游客</strong>
      </div>
    </div>
  </header>
</template>

<script setup lang="ts">
import { onMounted, onUnmounted, ref } from 'vue'
import { useRoute } from 'vue-router'
import { BarChart3, Coffee, Compass, Edit, Home, Map, MapPinned, Search } from 'lucide-vue-next'

const route = useRoute()
const isScrolled = ref(false)

const navItems = [
  { path: '/', label: '首页', icon: Home },
  { path: '/overview', label: '数据概览', icon: BarChart3 },
  { path: '/recommend', label: '旅游推荐', icon: Compass },
  { path: '/map', label: '路线规划', icon: Map },
  { path: '/facility', label: '场所查询', icon: Search },
  { path: '/diary', label: '旅游日记', icon: Edit },
  { path: '/food', label: '美食推荐', icon: Coffee },
]

function isActive(path: string): boolean {
  if (path === '/') return route.path === '/'
  return route.path === path
}

function handleScroll() {
  isScrolled.value = window.scrollY > 8
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
  gap: 16px;
  padding: 0 24px;
  border-bottom: 1px solid color-mix(in srgb, var(--color-rule) 78%, transparent);
  background: rgba(242, 239, 231, 0.9);
  backdrop-filter: blur(16px);
  transition: box-shadow 0.2s ease, background 0.2s ease;
}

.site-header--scrolled {
  background: rgba(255, 253, 247, 0.94);
  box-shadow: 0 10px 32px rgba(26, 26, 26, 0.08);
}

.brand-link {
  display: flex;
  align-items: center;
  gap: 12px;
  flex: 0 0 auto;
  min-width: 214px;
  color: var(--color-ink);
  text-decoration: none;
}

.brand-mark {
  width: 42px;
  height: 42px;
  display: grid;
  place-items: center;
  color: var(--color-surface);
  background: var(--color-ink);
  border: 1px solid var(--color-ink);
}

.brand-copy {
  display: grid;
  gap: 1px;
}

.brand-copy strong {
  font-size: 18px;
  line-height: 1.1;
}

.brand-copy small {
  color: var(--color-editorial-red);
  font-size: 10px;
  font-weight: 800;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.desktop-nav {
  display: flex;
  align-items: center;
  gap: 2px;
  flex: 1;
  min-width: 0;
  justify-content: center;
}

.nav-item {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 9px 8px;
  color: var(--color-text-secondary);
  border: 1px solid transparent;
  border-radius: 2px;
  font-size: 13px;
  font-weight: 700;
  text-decoration: none;
  white-space: nowrap;
  transition: color 0.2s ease, background 0.2s ease, border-color 0.2s ease;
}

.nav-item:hover,
.nav-item--active {
  color: var(--color-ink);
  background: var(--color-surface);
  border-color: var(--color-rule);
}

.header-side {
  display: flex;
  align-items: center;
  gap: 10px;
  margin-left: auto;
}

.header-cta {
  padding: 9px 10px;
  color: var(--color-surface);
  background: var(--color-editorial-red);
  font-size: 12px;
  font-weight: 800;
  text-decoration: none;
}

.user-chip {
  display: flex;
  align-items: center;
  gap: 8px;
  color: var(--color-text-secondary);
  font-size: 13px;
}

.user-chip span {
  width: 34px;
  height: 34px;
  display: grid;
  place-items: center;
  color: var(--color-surface);
  background: var(--color-primary);
  border-radius: 50%;
  font-weight: 900;
}

@media (max-width: 1080px) {
  .desktop-nav {
    overflow-x: auto;
    padding-bottom: 4px;
  }

  .header-cta {
    display: none;
  }
}

@media (max-width: 1240px) {
  .brand-link {
    min-width: auto;
  }

  .brand-copy small,
  .header-cta,
  .user-chip strong {
    display: none;
  }

  .nav-item {
    gap: 5px;
    padding-inline: 7px;
  }
}

@media (max-width: 760px) {
  .site-header {
    height: 76px;
    align-items: flex-start;
    flex-wrap: wrap;
    gap: 8px 14px;
    padding: 10px 16px;
  }

  .brand-copy small,
  .user-chip strong {
    display: none;
  }

  .desktop-nav {
    order: 3;
    width: 100%;
  }

  .nav-item {
    font-size: 13px;
    padding: 6px 8px;
  }
}
</style>
