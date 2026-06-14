<template>
  <div>
    <div class="section-heading">
      <div><p>系统运行数据</p><h2>全局数据总览</h2></div>
      <button @click="loadData"><RefreshCw :size="16" /> 刷新数据</button>
    </div>

    <div v-if="loading" class="loading-card">正在读取后台数据...</div>
    <div v-else class="stats-grid">
      <article v-for="item in cards" :key="item.key">
        <div class="stat-icon"><component :is="item.icon" :size="21" /></div>
        <span>{{ item.label }}</span>
        <strong>{{ overview?.[item.key] ?? 0 }}</strong>
        <small>{{ item.note }}</small>
      </article>
    </div>

    <div class="admin-summary">
      <section>
        <p class="summary-kicker">IDENTITY CONTROL</p>
        <h3>用户与权限</h3>
        <p>当前系统共有 {{ overview?.users || 0 }} 个账号，其中 {{ overview?.admins || 0 }} 个管理员账号。</p>
        <router-link to="/admin/users">进入用户管理 <ArrowRight :size="16" /></router-link>
      </section>
      <section>
        <p class="summary-kicker">CONTENT CONTROL</p>
        <h3>旅游内容</h3>
        <p>系统收录 {{ overview?.diaries || 0 }} 篇旅游日记和 {{ overview?.foods || 0 }} 条美食数据。</p>
        <router-link to="/admin/diaries">进入日记管理 <ArrowRight :size="16" /></router-link>
      </section>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import {
  ArrowRight,
  BookOpenText,
  MapPinned,
  Network,
  RefreshCw,
  Star,
  Store,
  Users,
  Waypoints,
} from 'lucide-vue-next'
import { getAdminOverview } from '@/api/admin'
import type { AdminOverview } from '@/types'

const loading = ref(false)
const overview = ref<AdminOverview | null>(null)
const definitions = [
  { key: 'users', label: '用户总数', note: '已注册账号', icon: Users },
  { key: 'admins', label: '管理员', note: '后台权限账号', icon: Star },
  { key: 'scenic_spots', label: '景区校园', note: '可选目的地', icon: MapPinned },
  { key: 'nodes', label: '地图节点', note: '路网空间节点', icon: Waypoints },
  { key: 'roads', label: '道路边数', note: '路径规划基础', icon: Network },
  { key: 'diaries', label: '旅游日记', note: '用户内容', icon: BookOpenText },
  { key: 'foods', label: '美食数据', note: '餐饮推荐', icon: Store },
  { key: 'ratings', label: '用户评分', note: '内容评价记录', icon: Star },
] as const
const cards = computed(() => definitions)

async function loadData() {
  try {
    loading.value = true
    overview.value = (await getAdminOverview()).data
  } finally {
    loading.value = false
  }
}

onMounted(loadData)
</script>

<style scoped>
.section-heading { display: flex; align-items: end; justify-content: space-between; margin-bottom: 24px; }
.section-heading p { color: #8b6a3f; font-size: 11px; font-weight: 900; letter-spacing: .13em; }
.section-heading h2 { font-family: Georgia, "Noto Serif SC", serif; font-size: 32px; }
.section-heading button { display: flex; align-items: center; gap: 7px; padding: 10px 13px; border: 1px solid #bdb6a7; background: #fffdf7; cursor: pointer; }
.loading-card { padding: 36px; border: 1px solid #cbc5b7; background: #fffdf7; }
.stats-grid { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 14px; }
.stats-grid article { position: relative; min-height: 170px; display: flex; flex-direction: column; padding: 20px; border: 1px solid #cbc5b7; background: #fffdf7; }
.stat-icon { position: absolute; right: 18px; top: 18px; color: #6c7054; }
.stats-grid span { color: #746f62; font-size: 12px; }
.stats-grid strong { margin: 26px 0 4px; font-family: Georgia, serif; font-size: 38px; line-height: 1; }
.stats-grid small { margin-top: auto; color: #948d7e; }
.admin-summary { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; margin-top: 20px; }
.admin-summary section { padding: 28px; color: white; background: #363b31; }
.admin-summary section:last-child { background: #7a5033; }
.summary-kicker { color: #d8c28e; font-size: 10px; font-weight: 900; letter-spacing: .15em; }
.admin-summary h3 { margin: 9px 0; font-family: Georgia, "Noto Serif SC", serif; font-size: 25px; }
.admin-summary section > p:not(.summary-kicker) { color: rgba(255,255,255,.68); }
.admin-summary a { display: inline-flex; align-items: center; gap: 6px; margin-top: 24px; color: white; font-weight: 800; text-decoration: none; }
@media (max-width: 1100px) { .stats-grid { grid-template-columns: repeat(2, 1fr); } }
@media (max-width: 700px) { .stats-grid, .admin-summary { grid-template-columns: 1fr; } }
</style>
