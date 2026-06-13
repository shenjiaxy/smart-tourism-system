import { post } from '@/utils/http'

export interface GenerateVideoRequest {
  image: string
  prompt: string
  api_base_url: string
  api_key: string
  model: string
  endpoint?: string
}

export interface GenerateVideoResult {
  success: boolean
  video_url?: string
  raw_response?: any
  raw_body?: string
}

/** AIGC 图生视频 */
export function generateVideo(data: GenerateVideoRequest) {
  return post<GenerateVideoResult>('/api/aigc/generate-video', data as any)
}
