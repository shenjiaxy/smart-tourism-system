import { readFileSync } from 'node:fs'
import { dirname, join } from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = dirname(fileURLToPath(import.meta.url))
const sqlPath = join(__dirname, 'generated_demo_seed.sql')
const sql = readFileSync(sqlPath, 'utf8')

const areaIds = new Set(
  [...sql.matchAll(/INSERT INTO scenic_spots .*?VALUES \((\d+),/g)].map(match => Number(match[1])),
)
const foodCounts = new Map()

for (const match of sql.matchAll(/INSERT INTO foods .*?VALUES \(\d+, (\d+),/g)) {
  const areaId = Number(match[1])
  foodCounts.set(areaId, (foodCounts.get(areaId) || 0) + 1)
}

const sqlLines = sql.split(/\r?\n/)
const spotRows = sqlLines.filter(line => line.startsWith('INSERT INTO scenic_spots '))
const foodRows = sqlLines.filter(line => line.startsWith('INSERT INTO foods '))
const diaryRows = sqlLines.filter(line => line.startsWith('INSERT INTO diaries '))
const spotImageCount = spotRows.filter(line => line.includes('/images/demo/spot-')).length
const foodImageCount = foodRows.filter(line => line.includes('/images/demo/food-')).length
const diaryImageCount = diaryRows.filter(line => line.includes('/images/demo/diary-')).length
const diaryCount = diaryRows.length

const failures = []
if (areaIds.size < 210) failures.push(`地点数量不足：${areaIds.size}，要求至少 210`)
if (spotImageCount < areaIds.size) failures.push(`景点图片不足：${spotImageCount}/${areaIds.size}`)
if (foodImageCount < [...foodCounts.values()].reduce((sum, count) => sum + count, 0)) {
  failures.push(`美食图片不足：${foodImageCount}/${[...foodCounts.values()].reduce((sum, count) => sum + count, 0)}`)
}
if (diaryImageCount < diaryCount) failures.push(`日记图片不足：${diaryImageCount}/${diaryCount}`)

for (const areaId of areaIds) {
  const count = foodCounts.get(areaId) || 0
  if (count < 10 || count > 20) {
    failures.push(`地点 ${areaId} 的美食数量为 ${count}，要求 10–20`)
  }
}

if (failures.length > 0) {
  console.error(failures.slice(0, 20).join('\n'))
  if (failures.length > 20) console.error(`另有 ${failures.length - 20} 项未显示`)
  process.exit(1)
}

const counts = [...foodCounts.values()]
console.log(`校验通过：${areaIds.size} 个地点，${counts.reduce((sum, count) => sum + count, 0)} 条美食`)
console.log(`每处美食数量：最少 ${Math.min(...counts)}，最多 ${Math.max(...counts)}`)
