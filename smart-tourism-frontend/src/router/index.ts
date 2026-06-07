import { createRouter, createWebHashHistory, type RouteRecordRaw } from 'vue-router'

const routes: RouteRecordRaw[] = [
  {
    path: '/',
    name: 'Home',
    component: () => import('@/views/HomeView.vue'),
    meta: { title: '首页', icon: 'Home' },
  },
  {
    path: '/diaries',
    redirect: '/diary',
  },
  {
    path: '/foods',
    redirect: '/food',
  },
  {
    path: '/facilities',
    redirect: '/facility',
  },
  {
    path: '/overview',
    name: 'Overview',
    component: () => import('@/views/OverviewView.vue'),
    meta: { title: '数据概览', icon: 'BarChart' },
  },
  {
    path: '/recommend',
    name: 'Recommend',
    component: () => import('@/views/RecommendView.vue'),
    meta: { title: '旅游推荐', icon: 'Compass' },
  },
  {
    path: '/map',
    name: 'Map',
    component: () => import('@/views/MapView.vue'),
    meta: { title: '路线规划', icon: 'Map' },
  },
  {
    path: '/facility',
    name: 'Facility',
    component: () => import('@/views/FacilityView.vue'),
    meta: { title: '场所查询', icon: 'Search' },
  },
  {
    path: '/diary',
    name: 'Diary',
    component: () => import('@/views/DiaryView.vue'),
    meta: { title: '旅游日记', icon: 'Edit' },
  },
  {
    path: '/food',
    name: 'Food',
    component: () => import('@/views/FoodView.vue'),
    meta: { title: '美食推荐', icon: 'Coffee' },
  },
]

const router = createRouter({
  history: createWebHashHistory(),
  routes,
  scrollBehavior(_to, _from, savedPosition) {
    return savedPosition || { top: 0 }
  },
})

router.beforeEach((to) => {
  const title = (to.meta.title as string) || '智能旅游系统'
  document.title = `${title} - 智能旅游系统`
})

export default router
