import { createRouter, createWebHashHistory, type RouteRecordRaw } from 'vue-router'
import { AUTH_TOKEN_KEY, readStoredUser } from '@/utils/authStorage'
import { resolveRouteAccess } from '@/utils/authRoute'

const userMeta = { requiresAuth: true, layout: 'user' }

const routes: RouteRecordRaw[] = [
  {
    path: '/login',
    name: 'Login',
    component: () => import('@/views/LoginView.vue'),
    meta: { title: '登录', guestOnly: true, layout: 'auth' },
  },
  {
    path: '/',
    name: 'Home',
    component: () => import('@/views/HomeView.vue'),
    meta: { ...userMeta, title: '首页' },
  },
  { path: '/diaries', redirect: '/diary' },
  { path: '/foods', redirect: '/food' },
  { path: '/facilities', redirect: '/facility' },
  {
    path: '/overview',
    name: 'Overview',
    component: () => import('@/views/OverviewView.vue'),
    meta: { ...userMeta, title: '数据概览', adminOnly: true },
  },
  {
    path: '/recommend',
    name: 'Recommend',
    component: () => import('@/views/RecommendView.vue'),
    meta: { ...userMeta, title: '旅游推荐' },
  },
  {
    path: '/map',
    name: 'Map',
    component: () => import('@/views/MapView.vue'),
    meta: { ...userMeta, title: '路线规划' },
  },
  {
    path: '/facility',
    name: 'Facility',
    component: () => import('@/views/FacilityView.vue'),
    meta: { ...userMeta, title: '场所查询' },
  },
  {
    path: '/diary',
    name: 'Diary',
    component: () => import('@/views/DiaryView.vue'),
    meta: { ...userMeta, title: '旅游日记' },
  },
  {
    path: '/food',
    name: 'Food',
    component: () => import('@/views/FoodView.vue'),
    meta: { ...userMeta, title: '美食推荐' },
  },
  {
    path: '/admin',
    component: () => import('@/components/layout/AdminLayout.vue'),
    meta: { title: '管理后台', requiresAuth: true, adminOnly: true, layout: 'admin' },
    children: [
      {
        path: '',
        name: 'AdminDashboard',
        component: () => import('@/views/admin/AdminDashboardView.vue'),
        meta: { title: '后台总览', requiresAuth: true, adminOnly: true, layout: 'admin' },
      },
      {
        path: 'users',
        name: 'AdminUsers',
        component: () => import('@/views/admin/AdminUsersView.vue'),
        meta: { title: '用户管理', requiresAuth: true, adminOnly: true, layout: 'admin' },
      },
      {
        path: 'diaries',
        name: 'AdminDiaries',
        component: () => import('@/views/admin/AdminDiariesView.vue'),
        meta: { title: '日记管理', requiresAuth: true, adminOnly: true, layout: 'admin' },
      },
    ],
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
  const user = readStoredUser()
  const redirect = resolveRouteAccess(
    {
      authenticated: Boolean(localStorage.getItem(AUTH_TOKEN_KEY) && user),
      role: user?.role || null,
    },
    {
      requiresAuth: Boolean(to.meta.requiresAuth),
      guestOnly: Boolean(to.meta.guestOnly),
      adminOnly: Boolean(to.meta.adminOnly),
    },
  )

  const title = (to.meta.title as string) || '智能旅游系统'
  document.title = `${title} - 智能旅游系统`
  return redirect || true
})

export default router
