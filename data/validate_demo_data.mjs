import { readFileSync } from 'node:fs'
import { dirname, join } from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = dirname(fileURLToPath(import.meta.url))
const sqlPath = join(__dirname, 'generated_demo_seed.sql')
const sql = readFileSync(sqlPath, 'utf8')

const spotMatches = [...sql.matchAll(/INSERT INTO scenic_spots .*?VALUES \((\d+), '((?:''|[^'])*)'/g)]
const areaIds = new Set(spotMatches.map(match => Number(match[1])))
const spotNames = new Map(
  spotMatches.map(match => [Number(match[1]), match[2].replaceAll("''", "'")]),
)
const foodCounts = new Map()
const roadCounts = new Map()

for (const match of sql.matchAll(/INSERT INTO foods .*?VALUES \(\d+, (\d+),/g)) {
  const areaId = Number(match[1])
  foodCounts.set(areaId, (foodCounts.get(areaId) || 0) + 1)
}

for (const match of sql.matchAll(/INSERT OR IGNORE INTO roads .*?VALUES \(\d+, (\d+),/g)) {
  const areaId = Number(match[1])
  roadCounts.set(areaId, (roadCounts.get(areaId) || 0) + 1)
}

const sqlLines = sql.split(/\r?\n/)
const spotRows = sqlLines.filter(line => line.startsWith('INSERT INTO scenic_spots '))
const foodRows = sqlLines.filter(line => line.startsWith('INSERT INTO foods '))
const diaryRows = sqlLines.filter(line => line.startsWith('INSERT INTO diaries '))
const spotImageCount = spotRows.filter(line => line.includes('/images/demo/spot-')).length
const foodImageCount = foodRows.filter(line => line.includes('/images/demo/food-')).length
const diaryImageCount = diaryRows.filter(line => line.includes('/images/demo/diary-')).length
const diaryCount = diaryRows.length
const totalFoods = [...foodCounts.values()].reduce((sum, count) => sum + count, 0)
const artificialNameTokens = ['春晓', '云栖', '晴川', '星河', '青禾', '望舒']
const seenSpotNames = new Map()

const failures = []
if (areaIds.size < 210) failures.push(`Area count is ${areaIds.size}, expected at least 210`)
if (spotImageCount < areaIds.size) failures.push(`Spot images are incomplete: ${spotImageCount}/${areaIds.size}`)
if (foodImageCount < totalFoods) failures.push(`Food images are incomplete: ${foodImageCount}/${totalFoods}`)
if (diaryImageCount < diaryCount) failures.push(`Diary images are incomplete: ${diaryImageCount}/${diaryCount}`)

for (const areaId of areaIds) {
  const foodCount = foodCounts.get(areaId) || 0
  if (foodCount < 10 || foodCount > 20) {
    failures.push(`Area ${areaId} has ${foodCount} foods, expected 10-20`)
  }

  const roadCount = roadCounts.get(areaId) || 0
  if (roadCount < 200) {
    failures.push(`Area ${areaId} has ${roadCount} roads, expected at least 200`)
  }

  const spotName = spotNames.get(areaId) || ''
  if (/[0-9０-９]{2,}/.test(spotName)) {
    failures.push(`Area ${areaId} name contains a numeric code: ${spotName}`)
  }
  const artificialToken = artificialNameTokens.find(token => spotName.includes(token))
  if (artificialToken) {
    failures.push(`Area ${areaId} name contains artificial test suffix "${artificialToken}": ${spotName}`)
  }
  if (seenSpotNames.has(spotName)) {
    failures.push(`Duplicate area name "${spotName}" for ${seenSpotNames.get(spotName)} and ${areaId}`)
  } else {
    seenSpotNames.set(spotName, areaId)
  }
}

if (failures.length > 0) {
  console.error(failures.slice(0, 20).join('\n'))
  if (failures.length > 20) console.error(`${failures.length - 20} more validation failures hidden`)
  process.exit(1)
}

const foodCountValues = [...foodCounts.values()]
const roadCountValues = [...roadCounts.values()]
console.log(`Validation passed: ${areaIds.size} areas, ${totalFoods} foods`)
console.log(`Foods per area: min ${Math.min(...foodCountValues)}, max ${Math.max(...foodCountValues)}`)
console.log(`Roads per area: min ${Math.min(...roadCountValues)}, max ${Math.max(...roadCountValues)}`)
