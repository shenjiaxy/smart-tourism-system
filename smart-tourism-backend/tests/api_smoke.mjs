import assert from 'node:assert/strict'

const baseUrl = process.env.SMART_TOURISM_BASE_URL || 'http://127.0.0.1:8080'

async function request(path, { token, method = 'GET', body } = {}) {
  const response = await fetch(`${baseUrl}${path}`, {
    method,
    headers: {
      ...(token ? { Authorization: `Bearer ${token}` } : {}),
      ...(body ? { 'Content-Type': 'application/json' } : {}),
    },
    body: body ? JSON.stringify(body) : undefined,
  })
  const payload = await response.json()
  return { status: response.status, payload }
}

async function login(username) {
  const result = await request('/api/auth/login', {
    method: 'POST',
    body: { username, password: 'pass123' },
  })
  assert.equal(result.status, 200)
  return result.payload.data
}

const user = await login('zhangsan')
const secondUser = await login('lisi')
const admin = await login('admin')

try {
  assert.equal((await request('/api/stats/overview', { token: user.token })).status, 403)
  assert.equal((await request('/api/admin/overview', { token: admin.token })).status, 200)

  const diaries = await request('/api/diaries?page=1&page_size=100', { token: user.token })
  const otherDiary = diaries.payload.data.items.find(item => item.user_id !== user.user.id)
  assert.ok(otherDiary)
  assert.equal((await request(`/api/diaries/${otherDiary.id}`, {
    token: user.token,
    method: 'DELETE',
  })).status, 403)
  assert.equal((await request('/api/diaries/compress', {
    token: user.token,
    method: 'POST',
    body: { diary_id: otherDiary.id },
  })).status, 403)

  const diaryBody = {
    title: 'permission smoke test',
    content: 'temporary',
    destination: 'test',
    tags: '[]',
  }
  const ownDiary = await request('/api/diaries', {
    token: user.token,
    method: 'POST',
    body: diaryBody,
  })
  assert.equal((await request(`/api/diaries/${ownDiary.payload.data.id}`, {
    token: user.token,
    method: 'DELETE',
  })).status, 200)

  const foreignDiary = await request('/api/diaries', {
    token: secondUser.token,
    method: 'POST',
    body: diaryBody,
  })
  assert.equal((await request(`/api/admin/diaries/${foreignDiary.payload.data.id}`, {
    token: admin.token,
    method: 'DELETE',
  })).status, 200)

  await request(`/api/admin/users/${secondUser.user.id}/role`, {
    token: admin.token,
    method: 'PUT',
    body: { role: 'admin' },
  })
  assert.equal((await request('/api/admin/overview', { token: secondUser.token })).status, 200)
  await request(`/api/admin/users/${secondUser.user.id}/role`, {
    token: admin.token,
    method: 'PUT',
    body: { role: 'user' },
  })
  assert.equal((await request('/api/admin/overview', { token: secondUser.token })).status, 403)

  const options = await request('/api/spots/options', { token: user.token })
  assert.ok(options.payload.data.length >= 200)
  const area = options.payload.data.find(item => item.id >= 1001) || options.payload.data[0]
  const graph = await request(`/api/map/graph/${area.id}`, { token: user.token })
  const road = graph.payload.data.roads[0]
  const route = await request('/api/route/single', {
    token: user.token,
    method: 'POST',
    body: {
      area_id: area.id,
      from_node: road.from_node,
      to_node: road.to_node,
      strategy: 'distance',
    },
  })
  assert.equal(route.status, 200)
  assert.equal(route.payload.data.found, true)
  assert.ok(route.payload.data.path.length >= 2)

  console.log('API smoke test passed: permissions, roles, diaries, and route planning')
} finally {
  await request(`/api/admin/users/${secondUser.user.id}/role`, {
    token: admin.token,
    method: 'PUT',
    body: { role: 'user' },
  }).catch(() => {})
}
