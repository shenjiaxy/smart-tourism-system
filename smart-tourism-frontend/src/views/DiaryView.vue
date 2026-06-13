<template>
  <div class="page-content fade-in">
    <!-- 头部 -->
    <div class="flex items-center justify-between mb-6">
      <div>
        <h1 class="text-2xl font-bold mb-1" style="color: var(--color-primary)">旅游日记</h1>
        <p style="color: var(--color-text-muted)">记录旅途中的精彩瞬间，支持全文检索和 Huffman 压缩</p>
      </div>
      <button
        class="flex items-center gap-2 px-5 py-2.5 rounded-xl text-white text-sm font-medium cursor-pointer transition-all hover:shadow-md"
        style="background: linear-gradient(135deg, #2D6A4F, #40916C)"
        @click="openCreateDialog"
      >
        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24"
             fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <path d="M12 5v14"/><path d="M5 12h14"/>
        </svg>
        写日记
      </button>
    </div>

    <!-- 搜索和排序栏 -->
    <div class="bg-white rounded-2xl shadow-sm p-4 mb-6">
      <div class="flex items-center gap-4 flex-wrap">
        <div class="flex-1 min-w-[200px] flex items-center bg-gray-50 rounded-xl px-4 py-2">
          <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24"
               fill="none" stroke="var(--color-text-muted)" stroke-width="2"
               stroke-linecap="round" stroke-linejoin="round" class="shrink-0">
            <circle cx="11" cy="11" r="8"/><path d="m21 21-4.3-4.3"/>
          </svg>
          <input
            v-model="searchKeyword"
            type="text"
            placeholder="搜索日记标题或内容..."
            class="flex-1 bg-transparent text-sm outline-none ml-2"
            @keyup.enter="handleSearch"
          />
        </div>

        <div class="flex items-center gap-1">
          <button
            v-for="s in sortOptions"
            :key="s.value"
            class="px-3 py-1.5 rounded-lg text-xs font-medium transition-all cursor-pointer"
            :class="sortBy === s.value ? 'sort-active' : 'sort-default'"
            @click="sortBy = s.value; loadDiaries()"
          >
            {{ s.label }}
          </button>
        </div>

        <div class="flex items-center gap-2">
          <label class="text-xs" style="color: var(--color-text-muted)">搜索模式:</label>
          <el-radio-group v-model="searchMode" size="small">
            <el-radio-button value="fulltext">全文检索</el-radio-button>
            <el-radio-button value="exact">精确查找</el-radio-button>
          </el-radio-group>
        </div>

        <button
          class="px-4 py-2 rounded-xl text-sm font-medium cursor-pointer transition-all"
          style="background: var(--color-primary); color: white"
          @click="handleSearch"
        >
          搜索
        </button>
      </div>
    </div>

    <!-- 搜索结果提示 -->
    <div v-if="searchMode && searchKeyword && searchResults.length > 0" class="mb-4">
      <div class="text-sm mb-2" style="color: var(--color-text-secondary)">
        找到 {{ searchResults.length }} 条匹配结果：
      </div>
      <div class="flex flex-wrap gap-2">
        <div
          v-for="r in searchResults"
          :key="r.diary_id"
          class="bg-white rounded-xl px-4 py-2 shadow-sm cursor-pointer hover:shadow-md transition-shadow flex items-center gap-2"
          @click="showDiaryDetail(r.diary_id)"
        >
          <span class="text-sm font-medium" style="color: var(--color-text-primary)">{{ r.title }}</span>
          <span class="text-xs px-2 py-0.5 rounded-full"
                style="background: var(--color-primary-bg); color: var(--color-primary)">
            相关度: {{ (r.score * 100).toFixed(0) }}%
          </span>
          <span v-if="r.snippet" class="text-xs truncate max-w-[200px]" style="color: var(--color-text-muted)">
            ...{{ r.snippet }}...
          </span>
        </div>
      </div>
    </div>

    <!-- 加载中 -->
    <div v-if="loading" class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-5">
      <div v-for="i in 6" :key="i" class="card overflow-hidden">
        <div class="skeleton h-36" style="background: linear-gradient(135deg, #e8f5e9, #f1f8e9)"></div>
        <div class="p-4">
          <div class="skeleton h-5 w-3/4 mb-2 rounded"></div>
          <div class="skeleton h-3 w-full mb-1 rounded"></div>
          <div class="skeleton h-3 w-2/3 rounded"></div>
        </div>
      </div>
    </div>

    <!-- 日记卡片 -->
    <div v-else-if="diaries.length > 0" class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-5">
      <div
        v-for="(d, idx) in diaries"
        :key="d.id"
        class="card overflow-hidden cursor-pointer group"
        @click="showDiaryDetail(d.id)"
      >
        <div class="h-36 relative overflow-hidden">
          <img
            v-if="d.images?.length"
            :src="d.images[0]"
            :alt="d.title"
            class="w-full h-full object-cover transition-transform duration-500 group-hover:scale-110"
          />
          <div v-else class="w-full h-full flex items-center justify-center"
               style="background: linear-gradient(135deg, #e8f5e9, #f1f8e9)">
            <span class="text-4xl opacity-40">📝</span>
          </div>
          <div v-if="d.is_compressed" class="absolute top-2 right-2 bg-purple-500/90 text-white text-xs px-2 py-0.5 rounded-lg">
            已压缩
          </div>
          <div class="absolute bottom-2 right-2 bg-black/50 text-white text-xs px-2 py-0.5 rounded-lg backdrop-blur-sm">
            {{ d.destination || '未标注' }}
          </div>
        </div>
        <div class="p-4">
          <div class="flex items-center gap-2 mb-2">
            <div class="w-6 h-6 rounded-full flex items-center justify-center text-white text-xs"
                 style="background: var(--color-primary-lighter)">
              {{ (d.username || '旅')[0] }}
            </div>
            <span class="text-xs" style="color: var(--color-text-muted)">{{ d.username || '游客' }}</span>
            <span class="text-xs ml-auto" style="color: var(--color-text-muted)">{{ formatDate(d.created_at) }}</span>
          </div>
          <h3 class="font-semibold text-sm mb-1.5 truncate" style="color: var(--color-text-primary)">{{ d.title }}</h3>
          <p class="text-xs mb-3" style="color: var(--color-text-muted); display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; overflow: hidden;">
            {{ d.content?.slice(0, 100) || '暂无内容' }}
          </p>
          <div class="flex items-center justify-between">
            <div class="flex items-center gap-2">
              <span class="flex items-center gap-0.5 text-xs" style="color: #FFC107">
                ★ {{ d.avg_rating?.toFixed(1) || '-' }}
              </span>
              <span class="text-xs" style="color: var(--color-text-muted)">({{ d.rating_count || 0 }})</span>
            </div>
            <div class="flex items-center gap-1 text-xs" style="color: var(--color-text-muted)">
              <svg xmlns="http://www.w3.org/2000/svg" width="12" height="12" viewBox="0 0 24 24"
                   fill="none" stroke="currentColor" stroke-width="2">
                <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/>
                <circle cx="12" cy="12" r="3"/>
              </svg>
              {{ d.popularity || 0 }}
            </div>
          </div>
          <!-- 标签 -->
          <div v-if="d.tags?.length" class="flex gap-1 mt-2 flex-wrap">
            <span v-for="tag in d.tags.slice(0, 3)" :key="tag"
                  class="text-xs px-2 py-0.5 rounded-full"
                  style="background: var(--color-primary-bg); color: var(--color-primary)">
              {{ tag }}
            </span>
          </div>
        </div>
      </div>
    </div>

    <!-- 空状态 -->
    <div v-else-if="!loading" class="empty-state">
      <svg xmlns="http://www.w3.org/2000/svg" width="48" height="48" viewBox="0 0 24 24"
           fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
        <path d="M14.5 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7.5L14.5 2z"/>
        <polyline points="14 2 14 8 20 8"/>
      </svg>
      <p class="text-sm mt-3">还没有日记，快去写一篇吧</p>
    </div>

    <!-- 分页 -->
    <div v-if="total > pageSize" class="flex justify-center mt-8">
      <el-pagination
        v-model:current-page="currentPage"
        :page-size="pageSize"
        :total="total"
        layout="prev, pager, next"
        background
        @current-change="loadDiaries"
      />
    </div>

    <!-- 创建/编辑日记弹窗 -->
    <el-dialog
      v-model="createDialogVisible"
      :title="editingDiary ? '编辑日记' : '写日记'"
      width="680px"
      destroy-on-close
    >
      <div class="space-y-4">
        <div>
          <label class="block text-xs font-medium mb-1" style="color: var(--color-text-secondary)">标题</label>
          <input v-model="diaryForm.title" type="text" placeholder="给你的旅途起个标题..."
                 class="w-full px-3 py-2 rounded-lg text-sm border outline-none focus:border-green-500"
                 style="border-color: var(--color-primary-lightest)" />
        </div>
        <div>
          <label class="block text-xs font-medium mb-1" style="color: var(--color-text-secondary)">目的地</label>
          <input v-model="diaryForm.destination" type="text" placeholder="去过哪里？"
                 class="w-full px-3 py-2 rounded-lg text-sm border outline-none focus:border-green-500"
                 style="border-color: var(--color-primary-lightest)" />
        </div>
        <div>
          <label class="block text-xs font-medium mb-1" style="color: var(--color-text-secondary)">内容</label>
          <textarea v-model="diaryForm.content" rows="8" placeholder="记录旅途中的故事..."
                    class="w-full px-3 py-2 rounded-lg text-sm border outline-none resize-none focus:border-green-500"
                    style="border-color: var(--color-primary-lightest)"></textarea>
          <div class="text-xs mt-1 text-right" style="color: var(--color-text-muted)">
            {{ diaryForm.content.length }} 字
          </div>
        </div>
        <div>
          <label class="block text-xs font-medium mb-1" style="color: var(--color-text-secondary)">标签（逗号分隔）</label>
          <input v-model="diaryForm.tags" type="text" placeholder="美食, 自然, 文化..."
                 class="w-full px-3 py-2 rounded-lg text-sm border outline-none focus:border-green-500"
                 style="border-color: var(--color-primary-lightest)" />
        </div>
        <div>
          <label class="block text-xs font-medium mb-1" style="color: var(--color-text-secondary)">图片链接</label>
          <div class="media-input-row">
            <input v-model="diaryForm.imageUrl" type="url" placeholder="https://..."
                   @keyup.enter="addImageUrl" />
            <button title="添加图片" @click="addImageUrl"><Plus :size="17" /></button>
            <button title="选择本地图片" @click="imageFileInput?.click()"><Upload :size="17" /></button>
            <input ref="imageFileInput" class="media-file-input" type="file" accept="image/*" multiple @change="handleImageFiles" />
          </div>
          <div v-if="diaryForm.images.length" class="media-preview-grid">
            <div v-for="(image, index) in diaryForm.images" :key="image" class="media-preview-item">
              <img :src="image" alt="日记图片预览" />
              <button title="移除图片" @click="diaryForm.images.splice(index, 1)"><X :size="14" /></button>
            </div>
          </div>
        </div>
        <div>
          <label class="block text-xs font-medium mb-1" style="color: var(--color-text-secondary)">视频链接</label>
          <div class="media-input-row">
            <input v-model="diaryForm.videoUrl" type="url" placeholder="https://...mp4"
                   @keyup.enter="addVideoUrl" />
            <button title="添加视频" @click="addVideoUrl"><Plus :size="17" /></button>
            <button title="选择本地视频" @click="videoFileInput?.click()"><Upload :size="17" /></button>
            <input ref="videoFileInput" class="media-file-input" type="file" accept="video/*" @change="handleVideoFile" />
          </div>
          <div v-if="diaryForm.videos.length" class="video-link-list">
            <span v-for="(video, index) in diaryForm.videos" :key="video">
              <Video :size="14" />{{ mediaDisplayName(video, index) }}
              <button title="移除视频" @click="diaryForm.videos.splice(index, 1)"><X :size="13" /></button>
            </span>
          </div>
        </div>
      </div>
      <template #footer>
        <div class="flex gap-3">
          <button class="flex-1 py-2 rounded-xl text-sm border cursor-pointer transition-colors"
                  style="border-color: var(--color-primary-lightest); color: var(--color-text-secondary)"
                  @click="createDialogVisible = false">
            取消
          </button>
          <button class="flex-1 py-2 rounded-xl text-sm text-white font-medium cursor-pointer transition-colors"
                  style="background: var(--color-primary)"
                  :disabled="savingDiary"
                  :class="{ 'opacity-60 cursor-not-allowed': savingDiary }"
                  @click="handleSaveDiary">
            {{ savingDiary ? '发布中...' : (editingDiary ? '保存修改' : '发布日记') }}
          </button>
        </div>
      </template>
    </el-dialog>

    <!-- 日记详情弹窗 -->
    <el-dialog
      v-model="detailDialogVisible"
      :title="currentDiary?.title"
      width="680px"
      destroy-on-close
    >
      <div v-if="currentDiary">
        <div class="flex items-center gap-3 mb-4">
          <div class="w-8 h-8 rounded-full flex items-center justify-center text-white text-sm"
               style="background: var(--color-primary-lighter)">
            {{ (currentDiary.username || '旅')[0] }}
          </div>
          <div>
            <div class="text-sm font-medium">{{ currentDiary.username || '游客' }}</div>
            <div class="text-xs" style="color: var(--color-text-muted)">{{ formatDate(currentDiary.created_at) }}</div>
          </div>
          <div class="ml-auto flex items-center gap-2">
            <!-- 评分 -->
            <el-rate
              v-model="userRating"
              :texts="['很差', '较差', '一般', '不错', '很棒']"
              show-text
              size="small"
              @change="handleRate"
            />
          </div>
        </div>

        <!-- 内容 -->
        <div class="text-sm leading-relaxed mb-4 whitespace-pre-wrap" style="color: var(--color-text-secondary)">
          {{ currentDiary.content || '暂无内容' }}
        </div>

        <!-- 图片 -->
        <div v-if="currentDiary.images?.length" class="flex gap-2 overflow-x-auto mb-4">
          <img v-for="(img, i) in currentDiary.images" :key="i" :src="img"
               class="w-32 h-24 rounded-lg object-cover shrink-0" />
        </div>

        <div v-if="currentDiary.videos?.length" class="diary-video-list">
          <video v-for="video in currentDiary.videos" :key="video" :src="video" controls preload="metadata" />
        </div>

        <!-- 标签 -->
        <div v-if="currentDiary.tags?.length" class="flex gap-1 mb-4 flex-wrap">
          <span v-for="tag in currentDiary.tags" :key="tag"
                class="text-xs px-2 py-0.5 rounded-full"
                style="background: var(--color-primary-bg); color: var(--color-primary)">
            {{ tag }}
          </span>
        </div>

        <!-- 操作按钮 -->
        <div class="flex gap-2 pt-4 border-t" style="border-color: var(--color-primary-lightest)">
          <button
            class="flex items-center gap-1.5 px-4 py-2 rounded-xl text-xs font-medium cursor-pointer transition-all"
            style="background: #E8F1FF; color: #185ABC"
            @click="startTravelAnimation"
          >
            <Sparkles :size="15" />
            生成旅游动画
          </button>
          <button
            class="flex items-center gap-1.5 px-4 py-2 rounded-xl text-xs font-medium cursor-pointer transition-all"
            style="background: #F3E5F5; color: #7B1FA2"
            @click="handleCompress"
          >
            <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24"
                 fill="none" stroke="currentColor" stroke-width="2">
              <path d="m18 16 4-4-4-4"/><path d="m6 8-4 4 4 4"/>
              <path d="m14.5 4-5 16"/>
            </svg>
            Huffman 压缩
          </button>
          <button
            v-if="currentDiary?.is_compressed"
            class="flex items-center gap-1.5 px-4 py-2 rounded-xl text-xs font-medium cursor-pointer transition-all"
            style="background: #E8F5E9; color: #2E7D32"
            @click="handleDecompress"
          >
            <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24"
                 fill="none" stroke="currentColor" stroke-width="2">
              <path d="m18 16 4-4-4-4"/><path d="m6 8-4 4 4 4"/>
              <path d="m14.5 4-5 16"/>
            </svg>
            Huffman 解压
          </button>
          <button
            class="flex items-center gap-1.5 px-4 py-2 rounded-xl text-xs font-medium cursor-pointer transition-all"
            style="background: var(--color-primary-bg); color: var(--color-primary)"
            @click="openEditDialog(currentDiary)"
          >
            编辑
          </button>
          <button
            class="flex items-center gap-1.5 px-4 py-2 rounded-xl text-xs font-medium cursor-pointer transition-all"
            style="background: #FFEBEE; color: #C62828"
            @click="handleDelete(currentDiary.id)"
          >
            删除
          </button>
          <div v-if="compressResult" class="ml-auto flex items-center gap-3 text-xs">
            <span style="color: var(--color-text-muted)">压缩前: {{ compressResult.original_size }}B</span>
            <span style="color: var(--color-primary)">压缩后: {{ compressResult.compressed_size }}B</span>
            <span class="font-bold" style="color: #7B1FA2">
              压缩率: {{ compressResult.compression_ratio }}%
            </span>
          </div>
        </div>
      </div>
    </el-dialog>

    <div v-if="animationVisible && currentDiary" class="travel-animation-overlay">
      <button class="animation-close" title="关闭动画" @click="closeTravelAnimation"><X :size="22" /></button>
      <div class="animation-badge"><Sparkles :size="14" /> 本地智能生成演示</div>
      <div class="travel-animation-frame">
        <div class="animation-fallback">
          <div class="fallback-landmark"><ImageIcon :size="72" /></div>
        </div>
        <div
          v-for="(image, index) in animationImages"
          :key="`${image}-${index}`"
          class="animation-slide"
          :class="{ active: animationSlide === index }"
          :style="{ backgroundImage: `url(${image})` }"
        />
        <div class="animation-shade" />
        <div class="animation-route-line">
          <i v-for="index in 5" :key="index" :style="{ animationDelay: `${index * 0.18}s` }" />
        </div>
        <div class="animation-copy">
          <span>{{ currentDiary.destination || '旅途影像' }}</span>
          <h2>{{ currentDiary.title }}</h2>
          <p>{{ currentDiary.content.slice(0, 72) }}</p>
        </div>
        <div class="animation-progress"><i :style="{ width: `${animationProgress * 100}%` }" /></div>
      </div>
      <button class="animation-replay" @click="startTravelAnimation"><RotateCcw :size="17" />重新生成</button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, reactive, onMounted, onUnmounted } from 'vue'
import { ElMessageBox, ElMessage } from 'element-plus'
import { Image as ImageIcon, Plus, RotateCcw, Sparkles, Upload, Video, X } from 'lucide-vue-next'
import {
  getDiaryList, getDiaryDetail, createDiary, updateDiary, deleteDiary,
  searchDiaries, compressDiary, decompressDiary, rateDiary,
} from '@/api/diary'
import type { Diary, DiarySearchResult } from '@/types'

const diaries = ref<Diary[]>([])
const loading = ref(false)
const searchKeyword = ref('')
const sortBy = ref('created_at')
const searchMode = ref<'fulltext' | 'exact'>('fulltext')
const currentPage = ref(1)
const pageSize = ref(9)
const total = ref(0)

// 搜索结果
const searchResults = ref<DiarySearchResult[]>([])

// 弹窗
const createDialogVisible = ref(false)
const detailDialogVisible = ref(false)
const editingDiary = ref<Diary | null>(null)
const currentDiary = ref<Diary | null>(null)
const userRating = ref(0)
const savingDiary = ref(false)
const imageFileInput = ref<HTMLInputElement | null>(null)
const videoFileInput = ref<HTMLInputElement | null>(null)
const animationVisible = ref(false)
const animationProgress = ref(0)
const animationSlide = ref(0)
let animationFrameId = 0
let animationStartedAt = 0
const animationDuration = 8000

// 压缩
const compressResult = ref<{ original_size: number; compressed_size: number; compression_ratio: number } | null>(null)

const sortOptions = [
  { label: '最新', value: 'created_at' },
  { label: '热门', value: 'popularity' },
  { label: '好评', value: 'rating' },
]

const diaryForm = reactive({
  title: '',
  content: '',
  destination: '',
  tags: '',
  imageUrl: '',
  videoUrl: '',
  images: [] as string[],
  videos: [] as string[],
})

const animationImages = computed(() => currentDiary.value?.images?.filter(Boolean).slice(0, 6) || [])

function parseTags(value: unknown): string[] {
  if (Array.isArray(value)) return value
  if (typeof value !== 'string' || !value.trim()) return []
  try {
    const parsed = JSON.parse(value)
    if (Array.isArray(parsed)) return parsed.map(String)
  } catch {
    // Fall back to comma-separated tags.
  }
  return value.split(/[,，]/).map(t => t.trim()).filter(Boolean)
}

function parseMediaList(value: unknown): string[] {
  if (Array.isArray(value)) return value.map(String).filter(Boolean)
  if (typeof value !== 'string' || !value.trim()) return []
  try {
    const parsed = JSON.parse(value)
    return Array.isArray(parsed) ? parsed.map(String).filter(Boolean) : []
  } catch {
    return value.split(/[,\n]/).map(item => item.trim()).filter(Boolean)
  }
}

function normalizeDiary(item: any): Diary {
  return {
    ...item,
    tags: parseTags(item.tags),
    images: parseMediaList(item.images),
    videos: parseMediaList(item.videos),
    is_compressed: Boolean(item.is_compressed),
  }
}

function formatDate(dateStr: string): string {
  if (!dateStr) return ''
  const d = new Date(dateStr)
  return d.toLocaleDateString('zh-CN', { month: 'short', day: 'numeric' })
}

async function loadDiaries() {
  loading.value = true
  try {
    const res = await getDiaryList({
      page: currentPage.value,
      page_size: pageSize.value,
      sort_by: sortBy.value,
      order: 'desc',
    })
    const data = res.data as any
    const items = data?.items || data || []
    diaries.value = items.map(normalizeDiary)
    total.value = data?.total || diaries.value.length
  } catch (e) {
    console.error('加载日记失败:', e)
  } finally {
    loading.value = false
  }
}

async function handleSearch() {
  if (!searchKeyword.value.trim()) {
    searchResults.value = []
    loadDiaries()
    return
  }
  try {
    const res = await searchDiaries({
      keyword: searchKeyword.value,
      mode: searchMode.value,
      limit: 20,
    })
    const data = res.data as any
    const items = Array.isArray(data) ? data : (data?.data || [])
    searchResults.value = items.map((item: any) => ({
      ...item,
      diary_id: item.diary_id ?? item.id,
      score: item.score ?? item.relevance_score ?? 1,
      tags: Array.isArray(item.tags) ? item.tags : parseTags(item.tags),
      images: parseMediaList(item.images),
      videos: parseMediaList(item.videos),
    }))
  } catch (e) {
    console.error('搜索日记失败:', e)
  }
}

function openCreateDialog() {
  editingDiary.value = null
  diaryForm.title = ''
  diaryForm.content = ''
  diaryForm.destination = ''
  diaryForm.tags = ''
  diaryForm.imageUrl = ''
  diaryForm.videoUrl = ''
  diaryForm.images = []
  diaryForm.videos = []
  createDialogVisible.value = true
}

function openEditDialog(diary: Diary) {
  detailDialogVisible.value = false
  editingDiary.value = diary
  diaryForm.title = diary.title
  diaryForm.content = diary.content
  diaryForm.destination = diary.destination
  diaryForm.tags = Array.isArray(diary.tags) ? diary.tags.join(', ') : ''
  diaryForm.imageUrl = ''
  diaryForm.videoUrl = ''
  diaryForm.images = [...(diary.images || [])]
  diaryForm.videos = [...(diary.videos || [])]
  createDialogVisible.value = true
}

function isMediaUrl(value: string) {
  return /^(https?:\/\/|data:image\/|\/)/i.test(value.trim())
}

function addImageUrl() {
  const value = diaryForm.imageUrl.trim()
  if (!value) return
  if (!isMediaUrl(value)) {
    ElMessage.warning('请输入有效的图片链接')
    return
  }
  if (!diaryForm.images.includes(value)) diaryForm.images.push(value)
  diaryForm.imageUrl = ''
}

function addVideoUrl() {
  const value = diaryForm.videoUrl.trim()
  if (!value) return
  if (!isMediaUrl(value)) {
    ElMessage.warning('请输入有效的视频链接')
    return
  }
  if (!diaryForm.videos.includes(value)) diaryForm.videos.push(value)
  diaryForm.videoUrl = ''
}

function readAsDataUrl(file: File) {
  return new Promise<string>((resolve, reject) => {
    const reader = new FileReader()
    reader.onload = () => resolve(String(reader.result || ''))
    reader.onerror = () => reject(reader.error)
    reader.readAsDataURL(file)
  })
}

async function handleImageFiles(event: Event) {
  const input = event.target as HTMLInputElement
  const files = Array.from(input.files || [])
  for (const file of files) {
    if (file.size > 3 * 1024 * 1024) {
      ElMessage.warning(`${file.name} 超过 3MB，已跳过`)
      continue
    }
    diaryForm.images.push(await readAsDataUrl(file))
  }
  input.value = ''
}

async function handleVideoFile(event: Event) {
  const input = event.target as HTMLInputElement
  const file = input.files?.[0]
  if (!file) return
  if (file.size > 10 * 1024 * 1024) {
    ElMessage.warning('视频超过 10MB，请选择较短的演示片段')
    input.value = ''
    return
  }
  diaryForm.videos.push(await readAsDataUrl(file))
  input.value = ''
}

function mediaDisplayName(value: string, index: number) {
  return value.startsWith('data:') ? `本地视频 ${index + 1}` : value
}

async function handleSaveDiary() {
  if (savingDiary.value) return
  if (!diaryForm.title.trim()) {
    ElMessage.warning('请输入日记标题')
    return
  }
  if (!diaryForm.content.trim()) {
    ElMessage.warning('请输入日记内容')
    return
  }
  try {
    savingDiary.value = true
    const tags = diaryForm.tags.split(/[,，]/).map(t => t.trim()).filter(Boolean)
    if (editingDiary.value) {
      await updateDiary(editingDiary.value.id, {
        title: diaryForm.title,
        content: diaryForm.content,
        destination: diaryForm.destination,
        tags,
        images: diaryForm.images,
        videos: diaryForm.videos,
      })
      ElMessage.success('日记已更新')
    } else {
      await createDiary({
        user_id: 1, // 默认游客用户
        title: diaryForm.title,
        content: diaryForm.content,
        destination: diaryForm.destination,
        tags: JSON.stringify(tags),
        images: diaryForm.images,
        videos: diaryForm.videos,
      })
      ElMessage.success('日记已发布')
    }
    createDialogVisible.value = false
    loadDiaries()
  } catch (e) {
    console.error('保存日记失败:', e)
  } finally {
    savingDiary.value = false
  }
}

async function showDiaryDetail(id: number) {
  try {
    const res = await getDiaryDetail(id)
    currentDiary.value = normalizeDiary(res.data)
    userRating.value = currentDiary.value?.avg_rating || 0
    compressResult.value = null
    detailDialogVisible.value = true
  } catch (e) {
    console.error('加载日记详情失败:', e)
  }
}

async function handleDelete(id: number) {
  try {
    await ElMessageBox.confirm('确定要删除这篇日记吗？', '删除确认', {
      confirmButtonText: '删除',
      cancelButtonText: '取消',
      type: 'warning',
    })
    await deleteDiary(id)
    ElMessage.success('日记已删除')
    detailDialogVisible.value = false
    loadDiaries()
  } catch (e) {
    // 用户取消
  }
}

async function handleRate() {
  if (!currentDiary.value || !userRating.value) return
  try {
    await rateDiary({
      user_id: 1,
      diary_id: currentDiary.value.id,
      score: userRating.value,
    })
    ElMessage.success('评分成功')
    loadDiaries()
  } catch (e) {
    console.error('评分失败:', e)
  }
}

async function handleCompress() {
  if (!currentDiary.value) return
  try {
    const res = await compressDiary({ diary_id: currentDiary.value.id })
    compressResult.value = res.data
    ElMessage.success(`压缩完成！压缩率: ${res.data.compression_ratio}%`)
  } catch (e) {
    console.error('压缩失败:', e)
  }
}

async function handleDecompress() {
  if (!currentDiary.value) return
  try {
    const res = await decompressDiary({ diary_id: currentDiary.value.id })
    if (res.data.content && currentDiary.value) {
      currentDiary.value.content = res.data.content
    }
    ElMessage.success(`解压完成！原始大小: ${res.data.original_size}B`)
  } catch (e) {
    console.error('解压失败:', e)
  }
}

function startTravelAnimation() {
  if (!currentDiary.value) return
  animationVisible.value = true
  animationProgress.value = 0
  animationSlide.value = 0
  animationStartedAt = performance.now()
  if (animationFrameId) cancelAnimationFrame(animationFrameId)
  animationFrameId = requestAnimationFrame(updateTravelAnimation)
}

function updateTravelAnimation(now: number) {
  const elapsed = now - animationStartedAt
  animationProgress.value = Math.min(1, elapsed / animationDuration)
  const count = Math.max(1, animationImages.value.length)
  animationSlide.value = Math.min(count - 1, Math.floor(animationProgress.value * count))
  if (animationProgress.value < 1 && animationVisible.value) {
    animationFrameId = requestAnimationFrame(updateTravelAnimation)
  } else {
    animationFrameId = 0
  }
}

function closeTravelAnimation() {
  animationVisible.value = false
  if (animationFrameId) cancelAnimationFrame(animationFrameId)
  animationFrameId = 0
}

onMounted(() => {
  loadDiaries()
})

onUnmounted(() => {
  if (animationFrameId) cancelAnimationFrame(animationFrameId)
})
</script>

<style scoped>
.sort-active {
  background: var(--color-primary-bg);
  color: var(--color-primary);
}
.sort-default {
  color: var(--color-text-muted);
}
.sort-default:hover {
  color: var(--color-primary);
}

.media-input-row {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 42px 42px;
  gap: 8px;
}

.media-file-input {
  display: none;
}

.media-input-row input {
  min-height: 40px;
  padding: 0 11px;
  border: 1px solid var(--color-primary-lightest);
  border-radius: 6px;
  outline: none;
}

.media-input-row button,
.media-preview-item button,
.video-link-list button {
  display: inline-grid;
  place-items: center;
  border: 1px solid var(--color-rule);
  background: var(--color-surface);
  cursor: pointer;
}

.media-preview-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 8px;
  margin-top: 9px;
}

.media-preview-item {
  position: relative;
  aspect-ratio: 4 / 3;
  overflow: hidden;
  border: 1px solid var(--color-rule);
  border-radius: 6px;
}

.media-preview-item img { width: 100%; height: 100%; object-fit: cover; }
.media-preview-item button { position: absolute; top: 4px; right: 4px; width: 25px; height: 25px; border-radius: 50%; }

.video-link-list { display: grid; gap: 6px; margin-top: 9px; }
.video-link-list span { display: grid; grid-template-columns: auto minmax(0, 1fr) auto; align-items: center; gap: 7px; min-width: 0; padding: 7px 9px; border: 1px solid var(--color-rule); font-size: 11px; overflow: hidden; }
.video-link-list button { width: 24px; height: 24px; }

.diary-video-list { display: grid; gap: 10px; margin-bottom: 16px; }
.diary-video-list video { width: 100%; max-height: 320px; background: #111; border-radius: 6px; }

.travel-animation-overlay {
  position: fixed;
  z-index: 4000;
  inset: 0;
  display: grid;
  place-content: center;
  gap: 16px;
  padding: 28px;
  background: rgba(10, 18, 14, .94);
}

.travel-animation-frame {
  position: relative;
  width: min(1040px, calc(100vw - 56px));
  aspect-ratio: 16 / 9;
  overflow: hidden;
  border: 1px solid rgba(255,255,255,.22);
  background: #274537;
  box-shadow: 0 24px 70px rgba(0,0,0,.42);
}

.animation-slide,
.animation-fallback,
.animation-shade {
  position: absolute;
  inset: 0;
}

.animation-slide {
  opacity: 0;
  background-position: center;
  background-size: cover;
  transform: scale(1.12) translate3d(2%, 0, 0);
  transition: opacity .8s ease;
}

.animation-slide.active {
  opacity: 1;
  animation: travel-pan 3s ease-out both;
}

.animation-fallback {
  display: grid;
  place-content: center;
  color: rgba(255,255,255,.68);
  background-color: #416b56;
  background-image: linear-gradient(rgba(255,255,255,.08) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,.08) 1px, transparent 1px);
  background-size: 48px 48px;
}

.fallback-landmark { animation: fallback-float 2.4s ease-in-out infinite alternate; }
.animation-shade { background: linear-gradient(0deg, rgba(7,16,11,.9), rgba(7,16,11,.08) 72%); }

.animation-copy {
  position: absolute;
  left: 6%;
  right: 8%;
  bottom: 12%;
  color: white;
}

.animation-copy span { font-size: 13px; font-weight: 800; letter-spacing: .08em; }
.animation-copy h2 { max-width: 780px; margin: 8px 0; font-size: clamp(30px, 5vw, 64px); line-height: 1.05; }
.animation-copy p { max-width: 650px; color: rgba(255,255,255,.82); font-size: 15px; }

.animation-route-line {
  position: absolute;
  top: 15%;
  right: 8%;
  display: flex;
  align-items: center;
  gap: 28px;
}

.animation-route-line::before { content: ''; position: absolute; left: 6px; right: 6px; height: 2px; background: rgba(255,255,255,.55); }
.animation-route-line i { position: relative; width: 11px; height: 11px; border: 2px solid white; border-radius: 50%; background: #d85b3f; animation: route-pulse 1s ease-out both; }

.animation-progress { position: absolute; left: 0; right: 0; bottom: 0; height: 5px; background: rgba(255,255,255,.22); }
.animation-progress i { display: block; height: 100%; background: #e8b04c; transition: width .08s linear; }
.animation-badge { justify-self: start; display: flex; align-items: center; gap: 7px; color: rgba(255,255,255,.82); font-size: 12px; font-weight: 800; }
.animation-close { position: fixed; top: 24px; right: 28px; display: grid; place-items: center; width: 42px; height: 42px; border: 1px solid rgba(255,255,255,.3); border-radius: 50%; color: white; background: rgba(0,0,0,.25); cursor: pointer; }
.animation-replay { justify-self: end; display: flex; align-items: center; gap: 7px; min-height: 40px; padding: 0 16px; border: 1px solid rgba(255,255,255,.32); color: white; background: transparent; font-weight: 800; cursor: pointer; }

@keyframes travel-pan { from { transform: scale(1.12) translate3d(2%, 0, 0); } to { transform: scale(1.02) translate3d(-1%, -1%, 0); } }
@keyframes route-pulse { from { opacity: 0; transform: scale(.2); } to { opacity: 1; transform: scale(1); } }
@keyframes fallback-float { from { transform: translateY(6px); } to { transform: translateY(-6px); } }

@media (max-width: 720px) {
  .media-preview-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
  .travel-animation-overlay { padding: 16px; }
  .travel-animation-frame { width: calc(100vw - 32px); }
  .animation-copy p { display: none; }
}
</style>
