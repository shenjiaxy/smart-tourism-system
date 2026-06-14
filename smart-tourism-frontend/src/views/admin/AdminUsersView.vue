<template>
  <div>
    <div class="section-heading">
      <div><p>ROLE MANAGEMENT</p><h2>用户与角色</h2><span>共 {{ users.length }} 个账号</span></div>
      <button @click="loadUsers"><RefreshCw :size="16" /> 刷新</button>
    </div>

    <div class="table-card">
      <table>
        <thead><tr><th>ID</th><th>用户</th><th>用户名</th><th>角色</th><th>注册时间</th><th>状态</th></tr></thead>
        <tbody>
          <tr v-for="user in users" :key="user.id">
            <td class="muted">#{{ user.id }}</td>
            <td><div class="person"><span>{{ (user.nickname || user.username).slice(0, 1) }}</span><strong>{{ user.nickname || '-' }}</strong></div></td>
            <td>{{ user.username }}</td>
            <td>
              <select
                :value="user.role"
                :disabled="user.id === auth.user?.id || updatingId === user.id"
                @change="handleRoleChange(user, $event)"
              >
                <option value="user">普通用户</option>
                <option value="admin">管理员</option>
              </select>
            </td>
            <td class="muted">{{ formatDate(user.created_at) }}</td>
            <td><span :class="['role-badge', user.role]">{{ user.role === 'admin' ? '管理员' : '用户' }}</span></td>
          </tr>
        </tbody>
      </table>
      <div v-if="loading" class="table-empty">正在加载用户...</div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { RefreshCw } from 'lucide-vue-next'
import { ElMessage } from 'element-plus'
import { getAdminUsers, updateAdminUserRole } from '@/api/admin'
import { useAuthStore } from '@/stores/auth'
import type { AdminUser } from '@/types'

const auth = useAuthStore()
const users = ref<AdminUser[]>([])
const loading = ref(false)
const updatingId = ref<number | null>(null)

function formatDate(value: string) {
  return value ? value.replace('T', ' ').slice(0, 16) : '-'
}

async function loadUsers() {
  try {
    loading.value = true
    users.value = (await getAdminUsers()).data
  } finally {
    loading.value = false
  }
}

async function handleRoleChange(user: AdminUser, event: Event) {
  const role = (event.target as HTMLSelectElement).value as 'user' | 'admin'
  try {
    updatingId.value = user.id
    await updateAdminUserRole(user.id, role)
    user.role = role
    ElMessage.success(`${user.nickname || user.username} 已设为${role === 'admin' ? '管理员' : '普通用户'}`)
  } catch {
    ;(event.target as HTMLSelectElement).value = user.role
  } finally {
    updatingId.value = null
  }
}

onMounted(loadUsers)
</script>

<style scoped>
.section-heading { display: flex; align-items: end; justify-content: space-between; margin-bottom: 24px; }
.section-heading p { color: #8b6a3f; font-size: 10px; font-weight: 900; letter-spacing: .15em; }
.section-heading h2 { font-family: Georgia, "Noto Serif SC", serif; font-size: 32px; }
.section-heading span { color: #746f62; }
.section-heading button { display: flex; align-items: center; gap: 7px; padding: 10px 13px; border: 1px solid #bdb6a7; background: #fffdf7; cursor: pointer; }
.table-card { overflow-x: auto; border: 1px solid #cbc5b7; background: #fffdf7; }
table { width: 100%; border-collapse: collapse; }
th { padding: 13px 16px; color: #746f62; background: #f3efe5; font-size: 11px; text-align: left; }
td { padding: 14px 16px; border-top: 1px solid #ddd7ca; }
.muted { color: #857e70; }
.person { display: flex; align-items: center; gap: 10px; }
.person span { width: 34px; height: 34px; display: grid; place-items: center; color: white; background: #64694e; border-radius: 50%; font-weight: 900; }
select { padding: 7px 28px 7px 9px; border: 1px solid #bdb6a7; background: white; }
select:disabled { opacity: .55; }
.role-badge { display: inline-block; padding: 4px 8px; font-size: 11px; font-weight: 900; }
.role-badge.user { color: #4d5a3f; background: #e5eadb; }
.role-badge.admin { color: #8f2d27; background: #f5dedb; }
.table-empty { padding: 28px; color: #746f62; text-align: center; }
</style>
