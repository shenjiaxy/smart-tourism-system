<template>
  <div class="login-page">
    <section class="login-story">
      <div class="story-mark"><MapPinned :size="30" /></div>
      <p class="eyebrow">SMART TOURISM PLATFORM</p>
      <h1>一次登录，进入你的智慧旅程。</h1>
      <p class="story-copy">
        普通用户使用旅游推荐、路线规划、设施查询、日记和美食功能；
        管理员进入独立后台管理用户与内容。
      </p>
      <div class="story-stats">
        <div><strong>200+</strong><span>景区与校园</span></div>
        <div><strong>4000+</strong><span>路网节点</span></div>
        <div><strong>2</strong><span>角色空间</span></div>
      </div>
    </section>

    <section class="login-panel">
      <div class="panel-inner">
        <p class="eyebrow">ACCOUNT ACCESS</p>
        <h2>登录智能旅游系统</h2>
        <p class="panel-note">系统会根据账号角色自动进入用户端或管理员端。</p>

        <form class="login-form" @submit.prevent="handleSubmit">
          <label>
            <span>用户名</span>
            <input v-model.trim="form.username" autocomplete="username" placeholder="请输入用户名" />
          </label>
          <label>
            <span>密码</span>
            <input
              v-model="form.password"
              type="password"
              autocomplete="current-password"
              placeholder="请输入密码"
            />
          </label>
          <button type="submit" :disabled="loading">
            {{ loading ? '正在验证...' : '登录系统' }}
            <ArrowRight :size="18" />
          </button>
        </form>

        <div class="demo-accounts">
          <p>演示账号</p>
          <button type="button" @click="fillAccount('zhangsan')">
            <UserRound :size="16" />
            <span><strong>普通用户</strong><small>zhangsan / pass123</small></span>
          </button>
          <button type="button" @click="fillAccount('admin')">
            <ShieldCheck :size="16" />
            <span><strong>管理员</strong><small>admin / pass123</small></span>
          </button>
        </div>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { reactive, ref } from 'vue'
import { useRouter } from 'vue-router'
import { ArrowRight, MapPinned, ShieldCheck, UserRound } from 'lucide-vue-next'
import { ElMessage } from 'element-plus'
import { useAuthStore } from '@/stores/auth'

const router = useRouter()
const auth = useAuthStore()
const loading = ref(false)
const form = reactive({ username: '', password: '' })

function fillAccount(username: 'zhangsan' | 'admin') {
  form.username = username
  form.password = 'pass123'
}

async function handleSubmit() {
  if (!form.username || !form.password) {
    ElMessage.warning('请输入用户名和密码')
    return
  }
  try {
    loading.value = true
    const user = await auth.login(form.username, form.password)
    ElMessage.success(`欢迎，${user.nickname || user.username}`)
    await router.replace(user.role === 'admin' ? '/admin' : '/')
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.login-page { min-height: 100vh; display: grid; grid-template-columns: minmax(0, 1.1fr) minmax(420px, .9fr); background: var(--color-paper); }
.login-story { position: relative; display: flex; flex-direction: column; justify-content: center; padding: 8vw; overflow: hidden; color: white; background: #242820; }
.login-story::after { content: ""; position: absolute; width: 520px; height: 520px; right: -150px; bottom: -190px; border: 1px solid rgba(255,255,255,.18); border-radius: 50%; box-shadow: 0 0 0 70px rgba(255,255,255,.035), 0 0 0 140px rgba(255,255,255,.025); }
.story-mark { width: 58px; height: 58px; display: grid; place-items: center; margin-bottom: 44px; color: #242820; background: #f2efe7; }
.eyebrow { color: #d2b37c; font-size: 11px; font-weight: 900; letter-spacing: .18em; }
h1 { max-width: 720px; margin: 16px 0 24px; font-family: Georgia, "Noto Serif SC", serif; font-size: clamp(44px, 6vw, 86px); font-weight: 500; line-height: 1.02; }
.story-copy { max-width: 620px; color: rgba(255,255,255,.72); font-size: 17px; }
.story-stats { display: flex; gap: 50px; margin-top: 64px; }
.story-stats div { display: grid; }
.story-stats strong { font-family: Georgia, serif; font-size: 30px; }
.story-stats span { color: rgba(255,255,255,.55); font-size: 12px; }
.login-panel { display: grid; place-items: center; padding: 52px; background: var(--color-surface); }
.panel-inner { width: min(440px, 100%); }
h2 { margin: 10px 0 8px; font-family: Georgia, "Noto Serif SC", serif; font-size: 36px; }
.panel-note { color: var(--color-muted); }
.login-form { display: grid; gap: 20px; margin-top: 38px; }
.login-form label { display: grid; gap: 8px; font-weight: 800; }
.login-form input { width: 100%; height: 50px; padding: 0 15px; color: var(--color-ink); border: 1px solid var(--color-rule); background: #fff; outline: none; }
.login-form input:focus { border-color: var(--color-primary); box-shadow: 0 0 0 3px rgba(94,99,71,.12); }
.login-form > button { height: 52px; display: flex; align-items: center; justify-content: center; gap: 8px; margin-top: 4px; color: white; border: 0; background: var(--color-ink); font-weight: 900; cursor: pointer; }
.login-form > button:disabled { opacity: .6; cursor: wait; }
.demo-accounts { display: grid; gap: 9px; margin-top: 34px; padding-top: 24px; border-top: 1px solid var(--color-rule); }
.demo-accounts > p { color: var(--color-muted); font-size: 12px; font-weight: 800; }
.demo-accounts button { display: flex; align-items: center; gap: 12px; padding: 12px; text-align: left; border: 1px solid var(--color-rule); background: var(--color-surface-muted); cursor: pointer; }
.demo-accounts button:hover { border-color: var(--color-primary); }
.demo-accounts button span { display: grid; }
.demo-accounts small { color: var(--color-muted); }
@media (max-width: 900px) {
  .login-page { grid-template-columns: 1fr; }
  .login-story { min-height: 42vh; padding: 50px 28px; }
  .story-mark, .story-stats { display: none; }
  .login-panel { padding: 42px 24px; }
}
</style>
