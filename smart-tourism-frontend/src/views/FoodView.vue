<template>
  <div class="page-content fade-in">
    <div class="mb-6">
      <h1 class="text-2xl font-bold mb-1" style="color: var(--color-primary)">美食推荐</h1>
      <p style="color: var(--color-text-muted)">发现景区周边的地道美食，支持编辑距离模糊搜索</p>
    </div>

    <!-- 筛选栏 -->
    <div class="bg-white rounded-2xl shadow-sm p-5 mb-6">
      <div class="flex flex-wrap items-end gap-4">
        <div class="flex-1 min-w-[180px]">
          <label class="block text-xs font-medium mb-1.5" style="color: var(--color-text-secondary)">
            所在区域
          </label>
          <select
            v-model="areaId"
            class="w-full px-3 py-2 rounded-lg text-sm border outline-none focus:border-green-500"
            style="border-color: var(--color-primary-lightest)"
            @change="loadFoods"
          >
            <option value="">选择景区/校园</option>
            <option v-for="s in spotOptions" :key="s.id" :value="s.id">{{ s.name }}</option>
          </select>
        </div>

        <div class="flex-1 min-w-[240px]">
          <label class="block text-xs font-medium mb-1.5" style="color: var(--color-text-secondary)">
            模糊搜索（编辑距离匹配）
          </label>
          <div class="flex items-center bg-gray-50 rounded-xl px-3 py-2">
            <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24"
                 fill="none" stroke="var(--color-text-muted)" stroke-width="2"
                 stroke-linecap="round" stroke-linejoin="round" class="shrink-0">
              <circle cx="11" cy="11" r="8"/><path d="m21 21-4.3-4.3"/>
            </svg>
            <input
              v-model="searchKeyword"
              type="text"
              placeholder="输入菜名或饭店名，支持模糊匹配..."
              class="flex-1 bg-transparent text-sm outline-none ml-2"
              @keyup.enter="handleSearch"
            />
          </div>
        </div>

        <button
          :disabled="!areaId"
          class="px-5 py-2 rounded-xl text-white text-sm font-medium transition-all cursor-pointer disabled:opacity-40 disabled:cursor-not-allowed"
          style="background: var(--color-primary)"
          @click="handleSearch"
        >
          搜索
        </button>

        <div class="flex items-center gap-2 ml-auto">
          <span class="text-xs" style="color: var(--color-text-muted)">排序:</span>
          <select
            v-model="sortBy"
            class="px-3 py-2 rounded-lg text-xs border outline-none focus:border-green-500"
            style="border-color: var(--color-primary-lightest)"
            @change="onSortChange"
          >
            <option value="rating">评价最高</option>
            <option value="popularity">热度最高</option>
            <option value="distance">距离最近</option>
            <option value="price_asc">价格最低</option>
          </select>
        </div>
      </div>
    </div>

    <!-- 菜系过滤 -->
    <div class="flex gap-2 mb-6 overflow-x-auto pb-1">
      <button
        v-for="cuisine in cuisines"
        :key="cuisine"
        class="whitespace-nowrap px-4 py-1.5 rounded-full text-xs font-medium transition-all cursor-pointer"
        :class="activeCuisine === cuisine ? 'cuisine-active' : 'cuisine-default'"
        @click="activeCuisine = cuisine; loadFoods()"
      >
        {{ cuisine || '全部菜系' }}
      </button>
    </div>

    <!-- 算法说明 -->
    <div v-if="searchResults.length > 0" class="bg-amber-50 rounded-xl p-4 mb-6 flex items-start gap-3">
      <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24"
           fill="none" stroke="#F4A261" stroke-width="2"
           stroke-linecap="round" stroke-linejoin="round" class="shrink-0 mt-0.5">
        <circle cx="12" cy="12" r="10"/><path d="M12 16v-4"/><path d="M12 8h.01"/>
      </svg>
      <div class="text-xs leading-relaxed" style="color: var(--color-text-secondary)">
        <strong style="color: #E65100">模糊搜索结果：</strong>
        使用 <strong>编辑距离（Levenshtein Distance）</strong> 算法进行模糊匹配，
        计算输入关键词与美食名称的最小编辑操作次数（插入、删除、替换），
        并转换为相似度百分比。结果按相似度降序排列。
      </div>
    </div>

    <!-- 加载中 -->
    <div v-if="loading" class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-5">
      <div v-for="i in 6" :key="i" class="bg-white rounded-xl p-4 shadow-sm flex gap-4">
        <div class="w-24 h-24 rounded-xl skeleton shrink-0"
             style="background: linear-gradient(135deg, #fff3e0, #ffe0b2)"></div>
        <div class="flex-1 min-w-0">
          <div class="skeleton h-5 w-3/4 mb-2 rounded"></div>
          <div class="skeleton h-4 w-1/2 mb-2 rounded"></div>
          <div class="skeleton h-3 w-full mb-1 rounded"></div>
          <div class="skeleton h-3 w-2/3 rounded"></div>
        </div>
      </div>
    </div>

    <!-- 模糊搜索结果 -->
    <div v-else-if="searchResults.length > 0">
      <h3 class="text-base font-medium mb-4 flex items-center gap-2" style="color: var(--color-text-primary)">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24"
             fill="none" stroke="var(--color-primary)" stroke-width="2"
             stroke-linecap="round" stroke-linejoin="round">
          <path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"/>
          <polyline points="3.27 6.96 12 12.01 20.73 6.96"/>
          <line x1="12" y1="22.08" x2="12" y2="12"/>
        </svg>
        模糊匹配结果（"{{ searchKeyword }}"）
      </h3>

      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-5">
        <div
          v-for="(result, idx) in searchResults"
          :key="result.name"
          class="bg-white rounded-xl p-4 shadow-sm hover:shadow-md transition-all slide-up cursor-pointer"
          :style="{ animationDelay: `${idx * 0.05}s` }"
          @click="selectSearchResult(result)"
        >
          <div class="flex gap-4">
            <div class="food-thumb w-20 h-20">
              <span>{{ getFoodEmoji(result.name) }}</span>
              <img
                v-if="result.food && foodImage(result.food)"
                :src="foodImage(result.food)"
                :alt="result.name"
                @error="hideBrokenImage"
              />
            </div>
            <div class="flex-1 min-w-0">
              <div class="flex items-center gap-2 mb-1">
                <h4 class="text-sm font-semibold truncate" style="color: var(--color-text-primary)">
                  {{ result.name }}
                </h4>
              </div>
              <!-- 相似度条 -->
              <div class="flex items-center gap-2 mb-2">
                <div class="flex-1 h-2 bg-gray-100 rounded-full overflow-hidden">
                  <div
                    class="h-full rounded-full transition-all duration-500"
                    :style="{
                      width: `${(result.similarity ?? 1) * 100}%`,
                      background: similarityColor(result.similarity ?? 1),
                    }"
                  ></div>
                </div>
                <span class="text-xs font-bold" :style="{ color: similarityColor(result.similarity ?? 1) }">
                  {{ ((result.similarity ?? 1) * 100).toFixed(0) }}%
                </span>
              </div>
              <div v-if="result.food" class="space-y-1">
                <div class="flex items-center gap-1 text-xs" style="color: var(--color-text-muted)">
                  <span class="tag tag-primary">{{ result.food.cuisine }}</span>
                  <span>{{ result.food.restaurant }}</span>
                </div>
                <div class="flex items-center justify-between text-xs">
                  <div class="flex items-center gap-2">
                    <span style="color: #FFC107">★ {{ result.food.rating?.toFixed(1) }}</span>
                    <span v-if="sortBy === 'distance' && (result.food as any).distance != null" class="font-medium" style="color: #E65100">
                      📍 {{ (((result.food as any).distance) * 1.2).toFixed(0) }}m
                    </span>
                  </div>
                  <span style="color: var(--color-text-secondary)">¥{{ result.food.price }}</span>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Top-K 推荐列表 -->
    <div v-else-if="foods.length > 0">
      <h3 class="text-base font-medium mb-4 flex items-center gap-2" style="color: var(--color-text-primary)">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24"
             fill="none" stroke="var(--color-primary)" stroke-width="2"
             stroke-linecap="round" stroke-linejoin="round">
          <polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/>
        </svg>
        推荐美食 Top 10
      </h3>

      <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-5">
        <div
          v-for="(food, idx) in foods"
          :key="food.id"
          class="card p-4 hover:shadow-md transition-all slide-up cursor-pointer"
          :style="{ animationDelay: `${idx * 0.05}s` }"
        >
          <div class="flex gap-4">
            <!-- 排名 -->
            <div class="w-8 h-8 rounded-lg flex items-center justify-center text-sm font-bold shrink-0"
                 :style="{
                   background: idx < 3 ? 'var(--color-primary)' : 'var(--color-primary-bg)',
                   color: idx < 3 ? 'white' : 'var(--color-primary)',
                 }">
              {{ idx + 1 }}
            </div>
            <div class="food-thumb w-20 h-20">
              <span>{{ getFoodEmoji(food.name) }}</span>
              <img
                v-if="foodImage(food)"
                :src="foodImage(food)"
                :alt="food.name"
                @error="hideBrokenImage"
              />
            </div>
            <div class="flex-1 min-w-0">
              <h4 class="text-sm font-semibold truncate mb-1" style="color: var(--color-text-primary)">
                {{ food.name }}
              </h4>
              <div class="flex items-center gap-2 mb-1">
                <span class="tag tag-primary text-xs">{{ food.cuisine }}</span>
                <span class="text-xs truncate" style="color: var(--color-text-muted)">{{ food.restaurant }}</span>
              </div>
              <p class="text-xs mb-2 truncate" style="color: var(--color-text-muted)">
                {{ food.description || '暂无描述' }}
              </p>
              <div class="flex items-center justify-between">
                <div class="flex items-center gap-2 text-xs">
                  <span style="color: #FFC107">★ {{ food.rating?.toFixed(1) }}</span>
                  <span style="color: var(--color-text-muted)">({{ food.rating_count }})</span>
                  <span v-if="sortBy === 'distance' && (food as any).distance != null" class="text-xs font-medium" style="color: #E65100">
                    📍 {{ ((food as any).distance * 1.2).toFixed(0) }}m
                  </span>
                </div>
                <span class="text-sm font-bold" style="color: var(--color-danger)">
                  ¥{{ food.price }}
                </span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- 未选择区域 -->
    <div v-else-if="!areaId" class="empty-state">
      <svg xmlns="http://www.w3.org/2000/svg" width="48" height="48" viewBox="0 0 24 24"
           fill="none" stroke="currentColor" stroke-width="1.5"
           stroke-linecap="round" stroke-linejoin="round">
        <path d="M18 8h1a4 4 0 0 1 0 8h-1"/>
        <path d="M2 8h16v9a4 4 0 0 1-4 4H6a4 4 0 0 1-4-4V8z"/>
        <line x1="6" y1="1" x2="6" y2="4"/>
        <line x1="10" y1="1" x2="10" y2="4"/>
        <line x1="14" y1="1" x2="14" y2="4"/>
      </svg>
      <p class="text-sm mt-3">请先选择所在景区或校园</p>
    </div>

    <!-- 无数据 -->
    <div v-else class="empty-state">
      <svg xmlns="http://www.w3.org/2000/svg" width="48" height="48" viewBox="0 0 24 24"
           fill="none" stroke="currentColor" stroke-width="1.5"
           stroke-linecap="round" stroke-linejoin="round">
        <path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"/>
      </svg>
      <p class="text-sm mt-3">该区域暂无美食数据</p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { getSpotOptions } from '@/api/spot'
import { getFoodRecommendations, searchFoods } from '@/api/food'
import type { Spot, Food, FuzzyMatchResult } from '@/types'

const spotOptions = ref<Spot[]>([])
const areaId = ref<number | string>('')
const searchKeyword = ref('')
const activeCuisine = ref('')
const sortBy = ref('rating')
const loading = ref(false)

const foods = ref<Food[]>([])
const searchResults = ref<FuzzyMatchResult[]>([])

const cuisines = [
  '', '川菜', '粤菜', '湘菜', '鲁菜', '苏菜', '浙菜', '闽菜',
  '京菜', '日料', '韩餐', '西餐', '甜品', '小吃', '烧烤', '火锅',
]

function getFoodEmoji(name: string): string {
  const emojis = ['🍜', '🍛', '🍣', '🍕', '🥗', '🍲', '🧋', '🥟', '🌮', '🥘', '🍛', '🍱']
  let hash = 0
  for (let i = 0; i < name.length; i++) hash = (hash * 31 + name.charCodeAt(i)) % emojis.length
  return emojis[Math.abs(hash)]
}

function foodImage(food: Partial<Food>): string {
  return food.image_url || food.image || ''
}

function hideBrokenImage(event: Event) {
  ;(event.currentTarget as HTMLImageElement).style.display = 'none'
}

function similarityColor(sim: number): string {
  if (sim >= 0.8) return '#2E7D32'
  if (sim >= 0.6) return '#40916C'
  if (sim >= 0.4) return '#F4A261'
  return '#E63946'
}

function onSortChange() {
  if (searchKeyword.value.trim() && searchResults.value.length > 0) {
    handleSearch()
  } else {
    loadFoods()
  }
}

function selectSearchResult(result: FuzzyMatchResult) {
  if (result.food) {
    // 选中后显示该食物的详细信息
    searchKeyword.value = result.food.name
    foods.value = [result.food]
    searchResults.value = []
  }
}

function normalizeSearchResult(item: any): FuzzyMatchResult {
  if (item.food) return item
  return {
    ...item,
    similarity: item.similarity ?? 1,
    food: {
      id: item.id ?? 0,
      area_id: item.area_id ?? Number(areaId.value),
      name: item.name,
      cuisine: item.cuisine ?? '',
      restaurant: item.restaurant ?? '',
      rating: item.rating ?? 0,
      rating_count: item.rating_count ?? 0,
      popularity: item.popularity ?? 0,
      node_id: item.node_id ?? 0,
      price: item.price ?? 0,
      description: item.description ?? '',
      image: item.image,
    },
  }
}

function normalizeFood(item: any): Food {
  return {
    id: item.id ?? 0,
    area_id: item.area_id ?? Number(areaId.value),
    name: item.name ?? '',
    cuisine: item.cuisine ?? '',
    restaurant: item.restaurant ?? '',
    rating: Number(item.rating ?? 0),
    rating_count: Number(item.rating_count ?? 0),
    popularity: Number(item.popularity ?? 0),
    node_id: item.node_id ?? 0,
    price: Number(item.price ?? 0),
    description: item.description ?? '',
    image: item.image,
    image_url: item.image_url,
    similarity: item.similarity,
  }
}

function unwrapItems(data: any): any[] {
  if (Array.isArray(data)) return data
  return data?.items || data?.data || []
}

function pickDemoReadySpot(options: Spot[]): Spot | undefined {
  return options.find(s => s.id >= 1001) || options[0]
}

async function loadFoods() {
  if (!areaId.value) {
    foods.value = []
    searchResults.value = []
    return
  }
  loading.value = true
  searchResults.value = []
  searchKeyword.value = ''
  try {
    const params: any = {
      area_id: Number(areaId.value),
      limit: 10,
      sort_by: sortBy.value,
      cuisine: activeCuisine.value || undefined,
    }
    if (sortBy.value === 'distance') {
      params.ref_x = 500
      params.ref_y = 350
    }
    const res = await getFoodRecommendations(params)
    foods.value = unwrapItems(res.data).map(normalizeFood)
  } catch (e) {
    console.error('加载美食失败:', e)
    foods.value = []
  } finally {
    loading.value = false
  }
}

async function handleSearch() {
  if (!areaId.value || !searchKeyword.value.trim()) return
  loading.value = true
  foods.value = []
  try {
    const params: any = {
      area_id: Number(areaId.value),
      keyword: searchKeyword.value,
      limit: 10,
      sort_by: sortBy.value,
    }
    if (sortBy.value === 'distance') {
      params.ref_x = 500
      params.ref_y = 350
    }
    const res = await searchFoods(params)
    const data = res.data as any
    const items = unwrapItems(data)
    searchResults.value = items.map(normalizeSearchResult)
  } catch (e) {
    console.error('搜索美食失败:', e)
    searchResults.value = []
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  getSpotOptions().then(res => {
    spotOptions.value = res.data || []
    const selected = pickDemoReadySpot(spotOptions.value)
    if (selected) {
      areaId.value = selected.id
      loadFoods()
    }
  })
})
</script>

<style scoped>
.cuisine-active {
  background: var(--color-primary);
  color: white;
}
.cuisine-default {
  background: white;
  color: var(--color-text-secondary);
  border: 1px solid #e5e7eb;
}
.cuisine-default:hover {
  border-color: var(--color-primary-light);
  color: var(--color-primary);
}

.food-thumb {
  position: relative;
  display: grid;
  place-items: center;
  flex: 0 0 auto;
  overflow: hidden;
  border-radius: 8px;
  background: #fff3e0;
  font-size: 28px;
}

.food-thumb img {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
  object-fit: cover;
}
</style>
