import assert from 'node:assert/strict'
import { readFileSync } from 'node:fs'

const routerSource = readFileSync(new URL('../src/router/index.ts', import.meta.url), 'utf8')
const headerSource = readFileSync(new URL('../src/components/layout/AppHeader.vue', import.meta.url), 'utf8')
const diarySource = readFileSync(new URL('../src/views/DiaryView.vue', import.meta.url), 'utf8')
const homeSource = readFileSync(new URL('../src/views/HomeView.vue', import.meta.url), 'utf8')

const overviewRoute = routerSource.match(/path:\s*'\/overview'[\s\S]*?\n\s*},/)?.[0] || ''
assert.match(overviewRoute, /adminOnly:\s*true/, 'data overview must be restricted to administrators')
assert.doesNotMatch(headerSource, /path:\s*'\/overview'/, 'normal user navigation must not expose data overview')
assert.doesNotMatch(homeSource, /to="\/overview"/, 'home page must not expose data overview')
assert.doesNotMatch(homeSource, /getOverviewStats/, 'home page must not request administrator-only statistics')

const deleteButton = diarySource.match(/<button[\s\S]*?@click="handleDelete\(currentDiary\.id\)"[\s\S]*?<\/button>/)?.[0] || ''
assert.match(deleteButton, /v-if="canManageCurrentDiary"/, 'diary delete action must only be visible to its owner or an administrator')

console.log('permission UI policy test passed')
