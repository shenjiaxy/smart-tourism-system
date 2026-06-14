<template>
  <div class="home-page fade-in">
    <section class="hero">
      <div class="hero-media">
        <div class="hero-photo"></div>
        <div class="hero-overlay"></div>
      </div>
      <div class="hero-content">
        <p class="editorial-kicker">Destination Intelligence</p>
        <h1 class="editorial-heading">把景区、校园与城市路线，变成一张可探索的旅行地图。</h1>
        <p class="hero-dek">
          智能旅游系统整合景点推荐、路线规划、设施查询、日记与美食数据，为课程设计展示一个完整、可信、可操作的旅游产品。
        </p>
        <div class="hero-actions">
          <router-link to="/recommend" class="primary-action">开始探索</router-link>
          <router-link to="/map" class="secondary-action">查看路线规划</router-link>
        </div>
      </div>
      <aside class="hero-card">
        <span>Route modes</span>
        <strong>4</strong>
        <small>路线规划策略</small>
        <div class="mini-rule"></div>
        <p>最短距离、最短时间、混合交通与室内导航。</p>
      </aside>
    </section>

    <section class="feature-strip">
      <router-link to="/facility" class="strip-item">
        <span>01</span>
        <strong>附近场所</strong>
        <small>按实际路径距离查找周边服务设施</small>
      </router-link>
      <router-link to="/recommend" class="strip-item">
        <span>02</span>
        <strong>目的地推荐</strong>
        <small>景点、校园、城市专题发现</small>
      </router-link>
      <router-link to="/map" class="strip-item">
        <span>03</span>
        <strong>路线规划</strong>
        <small>Dijkstra / TSP 路径演示</small>
      </router-link>
      <router-link to="/food" class="strip-item">
        <span>04</span>
        <strong>美食与日记</strong>
        <small>模糊搜索、评分、全文检索</small>
      </router-link>
    </section>

    <section class="home-section">
      <div class="section-copy">
        <p class="editorial-kicker">Featured Destinations</p>
        <h2 class="editorial-heading">热门目的地不是列表，而是可以被阅读的专题。</h2>
      </div>
      <div class="destination-grid">
        <article
          v-for="(spot, index) in featuredSpots"
          :key="spot.id"
          class="destination-card"
          :class="{ 'destination-card--large': index === 0 }"
          :style="{ '--visual-bg': getSpotVisual(spot, index).background }"
        >
          <div class="destination-image">
            <img
              v-if="spot.image_url || spot.image"
              :src="spot.image_url || spot.image"
              :alt="spot.name"
              @error="hideBrokenImage"
            />
          </div>
          <div class="destination-content">
            <span>{{ spot.city }} / {{ spot.category || spot.type }}</span>
            <h3>{{ spot.name }}</h3>
            <p>{{ spot.description }}</p>
          </div>
        </article>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { getSpotList } from '@/api/spot'
import type { Spot } from '@/types'
import { getSpotVisual } from '@/utils/visualAssets'

const spots = ref<Spot[]>([])

const featuredSpots = computed(() => spots.value.slice(0, 4))

function hideBrokenImage(event: Event) {
  ;(event.currentTarget as HTMLImageElement).style.display = 'none'
}

onMounted(async () => {
  try {
    const spotsRes = await getSpotList({ page: 1, page_size: 4, sort_by: 'popularity', order: 'desc' })
    const data = spotsRes.data as any
    spots.value = data?.items || data || []
  } catch (error) {
    console.error('加载首页数据失败:', error)
  }
})
</script>

<style scoped>
.home-page {
  min-height: 100vh;
}

.hero {
  position: relative;
  min-height: calc(100vh - 72px);
  display: grid;
  grid-template-columns: minmax(0, 1fr) 300px;
  align-items: end;
  gap: 32px;
  padding: 72px 42px 42px;
  color: var(--color-surface);
  overflow: hidden;
}

.hero-media,
.hero-photo,
.hero-overlay {
  position: absolute;
  inset: 0;
}

.hero-photo {
  background:
    radial-gradient(circle at 74% 18%, rgba(199, 50, 46, 0.55), transparent 16%),
    linear-gradient(120deg, rgba(10, 24, 20, 0.92), rgba(10, 24, 20, 0.42)),
    linear-gradient(135deg, #24352a 0%, #75653c 42%, #c8b37a 100%);
  transform: scale(1.04);
  animation: slowPan 18s ease-in-out infinite alternate;
}

.hero-overlay {
  background:
    linear-gradient(0deg, rgba(11, 18, 15, 0.72), transparent 50%),
    linear-gradient(90deg, rgba(11, 18, 15, 0.82), transparent 62%);
}

.hero-content {
  position: relative;
  z-index: 1;
  max-width: 840px;
}

.hero h1 {
  max-width: 820px;
  margin: 16px 0 22px;
  font-size: clamp(44px, 6vw, 88px);
  line-height: 0.98;
}

.hero-dek {
  max-width: 640px;
  color: rgba(255, 253, 247, 0.82);
  font-size: 18px;
  line-height: 1.8;
}

.hero-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  margin-top: 30px;
}

.primary-action,
.secondary-action {
  display: inline-flex;
  align-items: center;
  min-height: 46px;
  padding: 0 18px;
  font-weight: 900;
  text-decoration: none;
}

.primary-action {
  color: var(--color-surface);
  background: var(--color-editorial-red);
}

.secondary-action {
  color: var(--color-surface);
  border: 1px solid rgba(255, 253, 247, 0.5);
}

.hero-card {
  position: relative;
  z-index: 1;
  padding: 22px;
  border: 1px solid rgba(255, 253, 247, 0.28);
  background: rgba(255, 253, 247, 0.12);
  backdrop-filter: blur(14px);
}

.hero-card span,
.hero-card small {
  color: rgba(255, 253, 247, 0.72);
  font-size: 12px;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.hero-card strong {
  display: block;
  margin: 6px 0;
  font-family: Georgia, "Times New Roman", serif;
  font-size: 58px;
  line-height: 1;
}

.hero-card p {
  color: rgba(255, 253, 247, 0.78);
  font-size: 13px;
}

.mini-rule {
  height: 1px;
  margin: 18px 0;
  background: rgba(255, 253, 247, 0.28);
}

.feature-strip {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  border-top: 1px solid var(--color-rule);
  border-bottom: 1px solid var(--color-rule);
  background: var(--color-surface);
}

.strip-item {
  display: grid;
  gap: 6px;
  padding: 24px 28px;
  color: var(--color-ink);
  border-right: 1px solid var(--color-rule);
  text-decoration: none;
}

.strip-item span {
  color: var(--color-editorial-red);
  font-size: 12px;
  font-weight: 900;
}

.strip-item strong {
  font-size: 18px;
}

.strip-item small {
  color: var(--color-muted);
}

.home-section {
  max-width: 1280px;
  margin: 0 auto;
  padding: 76px 32px;
}

.section-copy {
  max-width: 680px;
  margin-bottom: 30px;
}

.section-copy h2 {
  margin-top: 10px;
  font-size: clamp(34px, 4vw, 56px);
  line-height: 1.08;
}

.destination-grid {
  display: grid;
  grid-template-columns: 1.15fr 0.85fr 0.85fr;
  gap: 18px;
}

.destination-card {
  min-height: 320px;
  position: relative;
  display: flex;
  align-items: end;
  overflow: hidden;
  border: 1px solid var(--color-rule);
  background: var(--visual-bg);
  color: var(--color-surface);
}

.destination-card--large {
  grid-row: span 2;
  min-height: 660px;
}

.destination-image {
  position: absolute;
  inset: 0;
}

.destination-image img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.destination-image::after {
  position: absolute;
  inset: 0;
  content: "";
  background:
    linear-gradient(0deg, rgba(0, 0, 0, 0.72), transparent 62%);
}

.destination-content {
  position: relative;
  z-index: 1;
  padding: 24px;
}

.destination-content span {
  color: rgba(255, 253, 247, 0.72);
  font-size: 11px;
  font-weight: 900;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.destination-content h3 {
  margin: 8px 0;
  font-family: Georgia, "Times New Roman", "Noto Serif SC", serif;
  font-size: 30px;
  line-height: 1.08;
}

.destination-card--large h3 {
  font-size: 46px;
}

.destination-content p {
  color: rgba(255, 253, 247, 0.76);
  max-width: 520px;
}

@keyframes slowPan {
  from { transform: scale(1.04) translate3d(0, 0, 0); }
  to { transform: scale(1.1) translate3d(-18px, -10px, 0); }
}

@media (max-width: 980px) {
  .hero {
    grid-template-columns: 1fr;
    min-height: 760px;
    padding: 64px 22px 28px;
  }

  .hero-card {
    max-width: 320px;
  }

  .feature-strip,
  .destination-grid {
    grid-template-columns: 1fr;
  }

  .destination-card,
  .destination-card--large {
    min-height: 360px;
  }
}
</style>
