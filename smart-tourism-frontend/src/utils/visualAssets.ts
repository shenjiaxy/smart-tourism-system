import type { Food, Spot } from '@/types'

type VisualTheme = {
  name: string
  background: string
  accent: string
}

const spotThemes: VisualTheme[] = [
  { name: 'imperial', background: 'linear-gradient(135deg, #2d3326 0%, #9f7c43 100%)', accent: '#c7322e' },
  { name: 'great-wall', background: 'linear-gradient(135deg, #4b5039 0%, #c7b27a 100%)', accent: '#8c5934' },
  { name: 'garden', background: 'linear-gradient(135deg, #42513a 0%, #c6c3a0 100%)', accent: '#5e6347' },
  { name: 'temple', background: 'linear-gradient(135deg, #243832 0%, #b78945 100%)', accent: '#c7322e' },
  { name: 'mountain', background: 'linear-gradient(135deg, #24333d 0%, #b7a06a 100%)', accent: '#445c6b' },
  { name: 'city', background: 'linear-gradient(135deg, #1f2c32 0%, #8a9aa0 100%)', accent: '#c8c2b0' },
  { name: 'campus', background: 'linear-gradient(135deg, #303825 0%, #d7c9a7 100%)', accent: '#5e6347' },
  { name: 'waterfront', background: 'linear-gradient(135deg, #233840 0%, #b6c2ba 100%)', accent: '#6a7b82' },
  { name: 'market', background: 'linear-gradient(135deg, #402a1e 0%, #d2a35d 100%)', accent: '#b64236' },
  { name: 'museum', background: 'linear-gradient(135deg, #292722 0%, #c8c2b0 100%)', accent: '#1a1a1a' },
]

export function getSpotVisual(spot: Pick<Spot, 'id' | 'category' | 'type' | 'name'>, index = 0): VisualTheme {
  const key = `${spot.category || ''}${spot.type || ''}${spot.name || ''}`
  if (key.includes('历史') || key.includes('故宫')) return spotThemes[0]
  if (key.includes('长城')) return spotThemes[1]
  if (key.includes('园')) return spotThemes[2]
  if (key.includes('天坛')) return spotThemes[3]
  if (key.includes('山') || key.includes('自然')) return spotThemes[4]
  if (key.includes('现代') || key.includes('鸟巢')) return spotThemes[5]
  if (key.includes('campus') || key.includes('大学') || key.includes('校园')) return spotThemes[6]
  return spotThemes[Math.abs((spot.id || index) + index) % spotThemes.length]
}

export function getFoodVisual(food: Pick<Food, 'id' | 'cuisine' | 'name'>, index = 0): VisualTheme {
  const base = spotThemes[(Math.abs((food.id || index) + index) + 8) % spotThemes.length]
  return {
    ...base,
    background: 'linear-gradient(135deg, #372419 0%, #cf9f56 58%, #f2efe7 100%)',
    accent: '#b64236',
  }
}

export function formatHeat(value: number | undefined): string {
  if (!value) return '0'
  if (value >= 10000) return `${Math.round(value / 1000) / 10}万`
  return String(value)
}
