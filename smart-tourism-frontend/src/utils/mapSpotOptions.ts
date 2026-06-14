export function mergeMapSpotOptions<T extends { id: number }>(
  demoOption: T,
  backendOptions: T[],
): T[] {
  return [
    demoOption,
    ...backendOptions.filter(option => option.id !== demoOption.id),
  ]
}
