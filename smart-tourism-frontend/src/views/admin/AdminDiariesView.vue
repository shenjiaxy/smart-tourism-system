<template>
  <div>
    <div class="section-heading">
      <div><p>CONTENT MODERATION</p><h2>旅游日记管理</h2><span>共 {{ diaries.length }} 篇日记</span></div>
      <button @click="loadDiaries"><RefreshCw :size="16" /> 刷新</button>
    </div>

    <div class="table-card">
      <table>
        <thead><tr><th>日记</th><th>作者</th><th>目的地</th><th>热度</th><th>评分</th><th>发布时间</th><th>操作</th></tr></thead>
        <tbody>
          <tr v-for="diary in diaries" :key="diary.id">
            <td><strong>{{ diary.title }}</strong><small>#{{ diary.id }}</small></td>
            <td>{{ diary.nickname || diary.username }}<small>@{{ diary.username }}</small></td>
            <td>{{ diary.destination || '-' }}</td>
            <td>{{ diary.popularity }}</td>
            <td>{{ diary.avg_rating.toFixed(1) }} <small>({{ diary.rating_count }})</small></td>
            <td>{{ formatDate(diary.created_at) }}</td>
            <td><button class="delete-button" @click="handleDelete(diary)"><Trash2 :size="15" /> 删除</button></td>
          </tr>
        </tbody>
      </table>
      <div v-if="loading" class="table-empty">正在加载日记...</div>
      <div v-else-if="!diaries.length" class="table-empty">暂无旅游日记</div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { RefreshCw, Trash2 } from 'lucide-vue-next'
import { ElMessage, ElMessageBox } from 'element-plus'
import { deleteAdminDiary, getAdminDiaries } from '@/api/admin'
import type { AdminDiary } from '@/types'

const diaries = ref<AdminDiary[]>([])
const loading = ref(false)

function formatDate(value: string) {
  return value ? value.replace('T', ' ').slice(0, 16) : '-'
}

async function loadDiaries() {
  try {
    loading.value = true
    diaries.value = (await getAdminDiaries()).data
  } finally {
    loading.value = false
  }
}

async function handleDelete(diary: AdminDiary) {
  await ElMessageBox.confirm(`确定删除《${diary.title}》吗？此操作不可撤销。`, '删除日记', {
    confirmButtonText: '确认删除',
    cancelButtonText: '取消',
    type: 'warning',
  })
  await deleteAdminDiary(diary.id)
  diaries.value = diaries.value.filter((item) => item.id !== diary.id)
  ElMessage.success('日记已删除')
}

onMounted(loadDiaries)
</script>

<style scoped>
.section-heading { display: flex; align-items: end; justify-content: space-between; margin-bottom: 24px; }
.section-heading p { color: #8b6a3f; font-size: 10px; font-weight: 900; letter-spacing: .15em; }
.section-heading h2 { font-family: Georgia, "Noto Serif SC", serif; font-size: 32px; }
.section-heading span { color: #746f62; }
.section-heading > button { display: flex; align-items: center; gap: 7px; padding: 10px 13px; border: 1px solid #bdb6a7; background: #fffdf7; cursor: pointer; }
.table-card { overflow-x: auto; border: 1px solid #cbc5b7; background: #fffdf7; }
table { width: 100%; min-width: 960px; border-collapse: collapse; }
th { padding: 13px 15px; color: #746f62; background: #f3efe5; font-size: 11px; text-align: left; }
td { padding: 14px 15px; border-top: 1px solid #ddd7ca; }
td > strong, td > small { display: block; }
td small { color: #8b8477; }
.delete-button { display: inline-flex; align-items: center; gap: 5px; padding: 7px 9px; color: #a6342e; border: 1px solid #d9aaa6; background: #fff7f6; cursor: pointer; }
.table-empty { padding: 28px; color: #746f62; text-align: center; }
</style>
