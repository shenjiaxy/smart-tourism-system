<template>
  <div class="recommend-page fade-in">
    <section class="recommend-hero">
      <div class="hero-copy">
        <p class="editorial-kicker">Curated routes and places</p>
        <h1 class="editorial-heading">发现你的下一站旅行</h1>
        <p>用更像旅行杂志的方式浏览景点、校园和城市目的地，同时保留排序、搜索和算法推荐能力。</p>
      </div>
      <div class="search-panel">
        <div class="search-box">
          <Search :size="20" />
          <input
            v-model="searchKeyword"
            type="text"
            placeholder="搜索景点名称、城市、类别..."
            @keyup.enter="handleSearch"
          />
          <button @click="handleSearch">搜索</button>
        </div>
        <div class="quick-tags">
          <button
            v-for="tag in quickTags"
            :key="tag.value"
            :class="{ active: activeCategory === tag.value }"
            @click="selectTag(tag.value)"
          >
            {{ tag.label }}
          </button>
        </div>
      </div>
    </section>

    <section v-if="featuredSpot" class="lead-feature">
      <article
        class="lead-card"
        :style="{ '--visual-bg': getSpotVisual(featuredSpot, 0).background }"
        @click="showDetail(featuredSpot)"
      >
        <div class="lead-image"></div>
        <div class="lead-content">
          <span>{{ featuredSpot.city }} / {{ featuredSpot.category || featuredSpot.type }}</span>
          <h2 class="editorial-heading">{{ featuredSpot.name }}</h2>
          <p>{{ featuredSpot.description }}</p>
        </div>
      </article>
      <aside class="lead-aside">
        <p class="editorial-kicker">Why it leads</p>
        <h3>热度 {{ formatHeat(featuredSpot.popularity) }} · 评分 {{ featuredSpot.rating?.toFixed(1) }}</h3>
        <p>推荐结果按热度、评分和兴趣权重排列。你可以切换类别或排序，快速形成一组可展示的目的地专题。</p>
      </aside>
    </section>

    <section class="results-section">
      <div class="section-bar">
        <div>
          <p class="editorial-kicker">{{ isSearchMode ? 'Search result' : 'Popular picks' }}</p>
          <h2 class="editorial-heading">{{ isSearchMode ? '搜索结果' : '热门推荐' }}</h2>
        </div>
        <div class="sort-tabs">
          <button
            v-for="s in sortOptions"
            :key="s.value"
            :class="{ active: sortBy === s.value }"
            @click="sortBy = s.value; loadSpots()"
          >
            {{ s.label }}
          </button>
        </div>
      </div>

      <div v-if="loading" class="spot-grid">
        <div v-for="i in 9" :key="i" class="spot-card skeleton-card">
          <div class="skeleton visual"></div>
          <div class="skeleton line wide"></div>
          <div class="skeleton line"></div>
        </div>
      </div>

      <div v-else-if="spots.length > 0" class="spot-grid">
        <article
          v-for="(spot, idx) in displaySpots"
          :key="spot.id"
          class="spot-card"
          :style="{ '--visual-bg': getSpotVisual(spot, idx + 1).background }"
          @click="showDetail(spot)"
        >
          <div class="spot-visual">
            <img v-if="spot.image_url || spot.image" :src="spot.image_url || spot.image" :alt="spot.name" />
            <div v-else class="generated-visual">
              <span>{{ spot.city }}</span>
            </div>
          </div>
          <div class="spot-body">
            <div class="spot-meta">
              <span>{{ spot.category || spot.type }}</span>
              <span>{{ spot.city }}</span>
            </div>
            <h3>{{ spot.name }}</h3>
            <p>{{ spot.description || '暂无描述' }}</p>
            <div class="spot-stats">
              <span>★ {{ spot.rating?.toFixed(1) }} ({{ spot.rating_count }})</span>
              <strong>热度 {{ formatHeat(spot.popularity) }}</strong>
            </div>
          </div>
        </article>
      </div>

      <div v-else class="empty-state">
        <Search :size="56" />
        <p>未找到相关景点，换个关键词或类别试试</p>
      </div>
    </section>

    <div v-if="total > pageSize" class="pagination-row">
      <el-pagination
        v-model:current-page="currentPage"
        :page-size="pageSize"
        :total="total"
        layout="prev, pager, next"
        background
        @current-change="loadSpots"
      />
    </div>

    <el-dialog v-model="detailVisible" :title="selectedSpot?.name" width="680px" destroy-on-close>
      <div v-if="selectedSpot" class="detail-dialog">
        <div class="detail-visual" :style="{ '--visual-bg': getSpotVisual(selectedSpot, 0).background }">
          <img v-if="selectedSpot.image_url || selectedSpot.image" :src="selectedSpot.image_url || selectedSpot.image" :alt="selectedSpot.name" />
        </div>
        <div class="detail-grid">
          <div><span>评分</span><strong>{{ selectedSpot.rating?.toFixed(1) }}</strong></div>
          <div><span>热度</span><strong>{{ formatHeat(selectedSpot.popularity) }}</strong></div>
          <div><span>门票</span><strong>{{ selectedSpot.ticket_price === 0 ? '免费' : selectedSpot.ticket_price }}</strong></div>
          <div><span>开放</span><strong>{{ selectedSpot.open_time || '全天' }}</strong></div>
        </div>
        <p>{{ selectedSpot.description || '暂无详细描述' }}</p>
        <small>{{ selectedSpot.address }}，{{ selectedSpot.city }}</small>
      </div>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { Search } from 'lucide-vue-next'
import { getSpotList, searchSpots } from '@/api/spot'
import type { Spot } from '@/types'
import { formatHeat, getSpotVisual } from '@/utils/visualAssets'

const spots = ref<Spot[]>([])
const loading = ref(false)
const searchKeyword = ref('')
const activeCategory = ref('')
const sortBy = ref('popularity')
const currentPage = ref(1)
const pageSize = ref(12)
const total = ref(0)
const isSearchMode = ref(false)
const detailVisible = ref(false)
const selectedSpot = ref<Spot | null>(null)

const quickTags = [
  { label: '全部', value: '' },
  { label: '自然风光', value: '自然' },
  { label: '人文历史', value: '历史' },
  { label: '文化体验', value: '文化' },
  { label: '现代地标', value: '现代' },
  { label: '综合休闲', value: '综合' },
  { label: '大学校园', value: 'campus' },
]

const sortOptions = [
  { label: '最热', value: 'popularity' },
  { label: '好评', value: 'rating' },
  { label: '综合', value: 'interest' },
]

const featuredSpot = computed(() => spots.value[0])
const displaySpots = computed(() => spots.value.slice(featuredSpot.value ? 1 : 0))

async function loadSpots() {
  loading.value = true
  try {
    if (isSearchMode.value && searchKeyword.value.trim()) {
      const res = await searchSpots({ keyword: searchKeyword.value.trim(), limit: 50 })
      spots.value = res.data || []
      total.value = spots.value.length
    } else {
      const selectedType = activeCategory.value === 'campus' ? 'campus' : ''
      const selectedCategory = activeCategory.value === 'campus' ? '' : activeCategory.value
      const res = await getSpotList({
        page: currentPage.value,
        page_size: pageSize.value,
        sort_by: sortBy.value,
        order: 'desc',
        type: selectedType,
        category: selectedCategory,
      })
      const data = res.data as any
      spots.value = data?.items || data || []
      total.value = data?.total || (res as any).total || spots.value.length
    }
  } catch (error) {
    console.error('加载景点失败:', error)
    spots.value = []
  } finally {
    loading.value = false
  }
}

function handleSearch() {
  isSearchMode.value = Boolean(searchKeyword.value.trim())
  currentPage.value = 1
  loadSpots()
}

function selectTag(value: string) {
  activeCategory.value = value
  isSearchMode.value = false
  searchKeyword.value = ''
  currentPage.value = 1
  loadSpots()
}

function showDetail(spot: Spot) {
  selectedSpot.value = spot
  detailVisible.value = true
}

onMounted(loadSpots)
</script>

<style scoped>
.recommend-page {
  max-width: 1320px;
  margin: 0 auto;
  padding: 34px 28px 72px;
}

.recommend-hero {
  display: grid;
  grid-template-columns: minmax(0, 0.95fr) minmax(360px, 0.75fr);
  gap: 44px;
  align-items: end;
  min-height: 380px;
  padding: 48px 0 34px;
}

.hero-copy h1 {
  margin: 14px 0 16px;
  font-size: clamp(48px, 6vw, 82px);
  line-height: 0.96;
}

.hero-copy p:last-child {
  max-width: 620px;
  color: var(--color-text-secondary);
  font-size: 17px;
}

.search-panel {
  border: 1px solid var(--color-rule);
  background: var(--color-surface);
  padding: 18px;
}

.search-box {
  display: flex;
  align-items: center;
  gap: 12px;
  border-bottom: 1px solid var(--color-rule);
  padding-bottom: 16px;
}

.search-box input {
  flex: 1;
  min-width: 0;
  border: 0;
  outline: 0;
  background: transparent;
  color: var(--color-ink);
}

.search-box button,
.quick-tags button,
.sort-tabs button {
  border: 1px solid var(--color-rule);
  background: var(--color-surface);
  color: var(--color-ink);
  cursor: pointer;
  transition: background 0.2s ease, color 0.2s ease, border-color 0.2s ease;
}

.search-box button {
  padding: 10px 16px;
  color: var(--color-surface);
  background: var(--color-ink);
  border-color: var(--color-ink);
  font-weight: 900;
}

.quick-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-top: 16px;
}

.quick-tags button {
  padding: 7px 11px;
  font-size: 13px;
  font-weight: 700;
}

.quick-tags button.active,
.sort-tabs button.active {
  color: var(--color-surface);
  background: var(--color-primary);
  border-color: var(--color-primary);
}

.lead-feature {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 320px;
  gap: 18px;
  margin-bottom: 48px;
}

.lead-card {
  min-height: 420px;
  position: relative;
  display: flex;
  align-items: end;
  overflow: hidden;
  border: 1px solid var(--color-rule);
  background: var(--visual-bg);
  color: var(--color-surface);
  cursor: pointer;
}

.lead-image,
.generated-visual::before,
.detail-visual::after {
  position: absolute;
  inset: 0;
  content: "";
  background:
    radial-gradient(circle at 26% 22%, rgba(255, 253, 247, 0.28), transparent 13%),
    linear-gradient(0deg, rgba(0, 0, 0, 0.72), transparent 64%);
}

.lead-content {
  position: relative;
  z-index: 1;
  max-width: 720px;
  padding: 32px;
}

.lead-content span,
.spot-meta span {
  font-size: 11px;
  font-weight: 900;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.lead-content h2 {
  margin: 10px 0 12px;
  font-size: clamp(42px, 5vw, 68px);
  line-height: 0.98;
}

.lead-content p {
  color: rgba(255, 253, 247, 0.78);
  font-size: 16px;
}

.lead-aside {
  padding: 22px;
  border: 1px solid var(--color-rule);
  background: var(--color-surface);
}

.lead-aside h3 {
  margin: 12px 0;
  font-family: Georgia, "Times New Roman", serif;
  font-size: 28px;
  line-height: 1.1;
}

.lead-aside p {
  color: var(--color-text-secondary);
}

.section-bar {
  display: flex;
  justify-content: space-between;
  align-items: end;
  gap: 20px;
  margin-bottom: 20px;
  padding-top: 28px;
  border-top: 1px solid var(--color-rule);
}

.section-bar h2 {
  font-size: 42px;
  line-height: 1;
}

.sort-tabs {
  display: flex;
  gap: 6px;
}

.sort-tabs button {
  padding: 8px 12px;
  font-size: 13px;
  font-weight: 800;
}

.spot-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 18px;
}

.spot-card {
  overflow: hidden;
  border: 1px solid var(--color-rule);
  background: var(--color-surface);
  cursor: pointer;
  transition: transform 0.2s ease, box-shadow 0.2s ease;
}

.spot-card:hover {
  transform: translateY(-4px);
  box-shadow: var(--shadow-md);
}

.spot-visual {
  height: 230px;
  position: relative;
  overflow: hidden;
  background: var(--visual-bg);
}

.spot-visual img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.generated-visual {
  position: absolute;
  inset: 0;
  display: flex;
  align-items: end;
  padding: 18px;
  color: rgba(255, 253, 247, 0.78);
}

.generated-visual span {
  position: relative;
  z-index: 1;
  font-size: 12px;
  font-weight: 900;
  letter-spacing: 0.12em;
}

.spot-body {
  padding: 18px;
}

.spot-meta {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  color: var(--color-editorial-red);
}

.spot-body h3 {
  margin: 8px 0 6px;
  font-size: 22px;
  line-height: 1.18;
}

.spot-body p {
  min-height: 44px;
  color: var(--color-text-secondary);
  display: -webkit-box;
  overflow: hidden;
  -webkit-box-orient: vertical;
  -webkit-line-clamp: 2;
}

.spot-stats {
  display: flex;
  justify-content: space-between;
  gap: 10px;
  margin-top: 18px;
  padding-top: 12px;
  border-top: 1px solid var(--color-rule);
  color: var(--color-text-muted);
  font-size: 13px;
}

.spot-stats strong {
  color: var(--color-primary);
}

.pagination-row {
  display: flex;
  justify-content: center;
  margin-top: 30px;
}

.detail-visual {
  height: 280px;
  position: relative;
  overflow: hidden;
  background: var(--visual-bg);
}

.detail-visual img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.detail-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 1px;
  margin: 18px 0;
  background: var(--color-rule);
}

.detail-grid div {
  display: grid;
  gap: 4px;
  padding: 14px;
  background: var(--color-surface);
}

.detail-grid span,
.detail-dialog small {
  color: var(--color-text-muted);
  font-size: 12px;
}

.detail-grid strong {
  font-size: 18px;
}

.detail-dialog p {
  color: var(--color-text-secondary);
  line-height: 1.8;
}

.skeleton-card {
  padding: 14px;
}

.skeleton-card .visual {
  height: 220px;
  margin-bottom: 16px;
}

.skeleton-card .line {
  height: 14px;
  width: 65%;
  margin-top: 10px;
}

.skeleton-card .line.wide {
  width: 86%;
}

@media (max-width: 980px) {
  .recommend-page {
    padding: 24px 18px 56px;
  }

  .recommend-hero,
  .lead-feature,
  .spot-grid {
    grid-template-columns: 1fr;
  }

  .lead-card {
    min-height: 440px;
  }

  .section-bar {
    align-items: start;
    flex-direction: column;
  }

  .detail-grid {
    grid-template-columns: repeat(2, 1fr);
  }
}
</style>
