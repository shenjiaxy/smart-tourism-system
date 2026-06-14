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

        <!-- AIGC 图生视频 -->
        <div class="aigc-section">
          <button
            class="aigc-toggle"
            @click="aigcPanelVisible = !aigcPanelVisible"
          >
            <Sparkles :size="16" />
            <span>AIGC 图生视频</span>
            <svg
              xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24"
              fill="none" stroke="currentColor" stroke-width="2"
              stroke-linecap="round" stroke-linejoin="round"
              :style="{ transform: aigcPanelVisible ? 'rotate(180deg)' : 'rotate(0deg)', transition: 'transform 0.2s' }"
            >
              <path d="m6 9 6 6 6-6"/>
            </svg>
          </button>

          <div v-if="aigcPanelVisible" class="aigc-panel">
            <div class="aigc-field">
              <label>选择图片</label>
              <select v-model="aigcSelectedImageIndex" class="aigc-select">
                <option :value="-1" disabled>-- 请选择已上传的图片 --</option>
                <option v-for="(img, idx) in diaryForm.images" :key="idx" :value="idx">
                  图片 {{ idx + 1 }}
                </option>
              </select>
              <span v-if="diaryForm.images.length === 0" class="aigc-hint">
                请先在"图片链接"区域上传图片
              </span>
            </div>

            <div class="aigc-config-group">
              <div class="aigc-config-header">
                <span>API 配置</span>
                <span class="aigc-config-saved" v-if="aigcConfigLoaded">已从本地加载</span>
              </div>
              <div class="aigc-field">
                <label>API 地址</label>
                <input
                  v-model="aigcConfig.api_base_url"
                  type="url"
                  placeholder="https://api.example.com"
                  class="aigc-input"
                  @change="saveAigcConfig"
                />
              </div>
              <div class="aigc-field">
                <label>API Key</label>
                <input
                  v-model="aigcConfig.api_key"
                  type="password"
                  placeholder="sk-..."
                  class="aigc-input"
                  @change="saveAigcConfig"
                />
              </div>
              <div class="aigc-field">
                <label>模型名称</label>
                <input
                  v-model="aigcConfig.model"
                  type="text"
                  placeholder="cogvideo-3"
                  class="aigc-input"
                  @change="saveAigcConfig"
                />
              </div>
            </div>

            <div class="aigc-field">
              <label>视频描述提示词</label>
              <textarea
                v-model="aigcPrompt"
                rows="3"
                placeholder="描述你想要的视频效果，例如：阳光明媚的风景区，镜头缓慢推进，4K画质..."
                class="aigc-textarea"
              ></textarea>
            </div>

            <button
              class="aigc-generate-btn"
              :disabled="aigcLoading || aigcSelectedImageIndex < 0"
              @click="handleGenerateVideo"
            >
              <svg v-if="aigcLoading" class="aigc-spinner" viewBox="0 0 24 24" width="16" height="16">
                <circle cx="12" cy="12" r="9" fill="none" stroke="currentColor" stroke-width="2" opacity="0.2" />
                <path d="M21 12a9 9 0 0 0-11.2-8.8" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round">
                  <animateTransform attributeName="transform" type="rotate" from="0 12 12" to="360 12 12" dur="1s" repeatCount="indefinite" />
                </path>
              </svg>
              <Sparkles v-else :size="16" />
              {{ aigcLoading ? '生成中，请耐心等待...' : '生成视频' }}
            </button>

            <div v-if="aigcError" class="aigc-error">{{ aigcError }}</div>

            <div v-if="aigcResult" class="aigc-result">
              <div class="aigc-result-label">生成结果</div>
              <video
                v-if="aigcResult.video_url"
                :src="aigcResult.video_url"
                controls
                class="aigc-video-preview"
              />
              <div v-else class="aigc-raw-response">
                <pre>{{ JSON.stringify(aigcResult, null, 2) }}</pre>
              </div>
              <button
                v-if="aigcResult.video_url"
                class="aigc-add-btn"
                @click="addAigcVideoToDiary"
              >
                <Plus :size="14" />
                添加到日记视频
              </button>
            </div>
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
            v-if="currentDiary.user_id === auth.user?.id || auth.isAdmin"
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
            v-if="currentDiary.user_id === auth.user?.id || auth.isAdmin"
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

  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { ElMessageBox, ElMessage } from 'element-plus'
import { Plus, Sparkles, Upload, Video, X } from 'lucide-vue-next'
import {
  getDiaryList, getDiaryDetail, createDiary, updateDiary, deleteDiary,
  searchDiaries, compressDiary, decompressDiary, rateDiary,
} from '@/api/diary'
import { generateVideo } from '@/api/aigc'
import type { Diary, DiarySearchResult } from '@/types'
import { useAuthStore } from '@/stores/auth'

const auth = useAuthStore()
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
// 压缩
const compressResult = ref<{ original_size: number; compressed_size: number; compression_ratio: number } | null>(null)

// AIGC 图生视频
const aigcPanelVisible = ref(false)
const aigcSelectedImageIndex = ref(-1)
const aigcPrompt = ref('')
const aigcLoading = ref(false)
const aigcError = ref('')
const aigcResult = ref<{ video_url?: string; raw_response?: any; raw_body?: string } | null>(null)
const aigcConfigLoaded = ref(false)
const aigcConfig = reactive({
  api_base_url: '',
  api_key: '',
  model: 'cogvideo-3',
})

function loadAigcConfig() {
  try {
    const saved = localStorage.getItem('aigc_config')
    if (saved) {
      const parsed = JSON.parse(saved)
      aigcConfig.api_base_url = parsed.api_base_url || ''
      aigcConfig.api_key = parsed.api_key || ''
      aigcConfig.model = parsed.model || 'cogvideo-3'
      aigcConfigLoaded.value = true
    }
  } catch { /* ignore */ }
}

function saveAigcConfig() {
  localStorage.setItem('aigc_config', JSON.stringify({
    api_base_url: aigcConfig.api_base_url,
    api_key: aigcConfig.api_key,
    model: aigcConfig.model,
  }))
}

async function handleGenerateVideo() {
  aigcError.value = ''
  aigcResult.value = null

  if (aigcSelectedImageIndex.value < 0 || !diaryForm.images[aigcSelectedImageIndex.value]) {
    aigcError.value = '请先选择一张图片'
    return
  }

  if (!aigcConfig.api_base_url || !aigcConfig.api_key) {
    aigcError.value = '请填写 API 地址和 API Key'
    return
  }

  if (!aigcPrompt.value.trim()) {
    aigcError.value = '请输入视频描述提示词'
    return
  }

  try {
    aigcLoading.value = true
    const image = diaryForm.images[aigcSelectedImageIndex.value]
    const res = await generateVideo({
      image,
      prompt: aigcPrompt.value,
      api_base_url: aigcConfig.api_base_url,
      api_key: aigcConfig.api_key,
      model: aigcConfig.model,
    })
    const data = res.data as any
    if (data?.success) {
      aigcResult.value = data
      ElMessage.success('视频生成成功！')
    } else {
      aigcError.value = data?.raw_response?.detail || data?.message || '生成失败，请检查配置'
    }
  } catch (e: any) {
    aigcError.value = e?.message || '请求失败，请检查网络和 API 配置'
  } finally {
    aigcLoading.value = false
  }
}

function addAigcVideoToDiary() {
  if (!aigcResult.value?.video_url) return
  const url = aigcResult.value.video_url
  if (!diaryForm.videos.includes(url)) {
    diaryForm.videos.push(url)
    ElMessage.success('视频已添加到日记')
  } else {
    ElMessage.warning('该视频已在列表中')
  }
}

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
  aigcPanelVisible.value = false
  aigcSelectedImageIndex.value = -1
  aigcPrompt.value = ''
  aigcError.value = ''
  aigcResult.value = null
  loadAigcConfig()
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
  aigcPanelVisible.value = false
  aigcSelectedImageIndex.value = -1
  aigcPrompt.value = ''
  aigcError.value = ''
  aigcResult.value = null
  loadAigcConfig()
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
        user_id: auth.user!.id,
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
      user_id: auth.user!.id,
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

onMounted(() => {
  loadDiaries()
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

@media (max-width: 720px) {
  .media-preview-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
}

/* ========== AIGC 图生视频 ========== */
.aigc-section {
  border: 1px solid var(--color-primary-lightest);
  border-radius: 12px;
  overflow: hidden;
}

.aigc-toggle {
  display: flex;
  align-items: center;
  gap: 8px;
  width: 100%;
  padding: 12px 16px;
  border: none;
  background: linear-gradient(135deg, #f0f7ff, #f5f0ff);
  color: #5b3cc4;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: background 0.2s;
}

.aigc-toggle:hover {
  background: linear-gradient(135deg, #e8efff, #ede4ff);
}

.aigc-panel {
  padding: 16px;
  display: flex;
  flex-direction: column;
  gap: 14px;
  border-top: 1px solid var(--color-primary-lightest);
}

.aigc-field {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.aigc-field label {
  font-size: 12px;
  font-weight: 500;
  color: var(--color-text-secondary);
}

.aigc-input,
.aigc-select,
.aigc-textarea {
  padding: 8px 12px;
  border: 1px solid var(--color-primary-lightest);
  border-radius: 8px;
  font-size: 13px;
  outline: none;
  font-family: inherit;
}

.aigc-input:focus,
.aigc-select:focus,
.aigc-textarea:focus {
  border-color: var(--color-primary);
}

.aigc-textarea {
  resize: none;
}

.aigc-hint {
  font-size: 11px;
  color: var(--color-text-muted);
  margin-top: 2px;
}

.aigc-config-group {
  background: #fafafa;
  border: 1px solid #eee;
  border-radius: 10px;
  padding: 12px;
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.aigc-config-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-size: 12px;
  font-weight: 600;
  color: var(--color-text-secondary);
}

.aigc-config-saved {
  font-size: 10px;
  color: #2e7d32;
  background: #e8f5e9;
  padding: 2px 8px;
  border-radius: 10px;
}

.aigc-generate-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 10px 20px;
  border: none;
  border-radius: 10px;
  background: linear-gradient(135deg, #7c3aed, #a855f7);
  color: white;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: opacity 0.2s, transform 0.1s;
}

.aigc-generate-btn:hover:not(:disabled) {
  opacity: 0.92;
  transform: translateY(-1px);
}

.aigc-generate-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.aigc-spinner {
  animation: aigc-spin 1s linear infinite;
}

@keyframes aigc-spin {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

.aigc-error {
  padding: 10px 14px;
  background: #fff2f0;
  border: 1px solid #ffccc7;
  border-radius: 8px;
  color: #cf1322;
  font-size: 13px;
}

.aigc-result {
  background: #f6ffed;
  border: 1px solid #b7eb8f;
  border-radius: 10px;
  padding: 14px;
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.aigc-result-label {
  font-size: 12px;
  font-weight: 600;
  color: #389e0d;
}

.aigc-video-preview {
  width: 100%;
  max-height: 280px;
  background: #111;
  border-radius: 8px;
}

.aigc-raw-response {
  max-height: 200px;
  overflow: auto;
  background: #fafafa;
  border-radius: 6px;
  padding: 8px;
}

.aigc-raw-response pre {
  margin: 0;
  font-size: 11px;
  white-space: pre-wrap;
  word-break: break-all;
}

.aigc-add-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 8px 16px;
  border: 1px solid #389e0d;
  border-radius: 8px;
  background: white;
  color: #389e0d;
  font-size: 13px;
  font-weight: 500;
  cursor: pointer;
  transition: background 0.2s;
}

.aigc-add-btn:hover {
  background: #f6ffed;
}
</style>
