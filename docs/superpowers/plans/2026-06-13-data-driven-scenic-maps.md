# Data-Driven Scenic Maps Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the shared decorative map with deterministic, backend-graph-driven scenes for every scenic area.

**Architecture:** Add a pure scene generator for bounds, palette, terrain, and complete graph projection. Keep interaction in `MapView.vue`, but make all non-demo drawing consume the generated scene and all backend nodes and roads.

**Tech Stack:** Vue 3, TypeScript, Canvas 2D, Node test runner

---

### Task 1: Scene Generation

**Files:**
- Create: `smart-tourism-frontend/src/utils/mapScene.ts`
- Create: `smart-tourism-frontend/tests/map-scene.test.mjs`

- [ ] Write tests that require deterministic scene signatures, distinct scenes
  for different area IDs, node-derived bounds, and preservation of all roads.
- [ ] Run `node tests/map-scene.test.mjs` and confirm it fails because
  `mapScene.ts` does not exist.
- [ ] Implement seeded random helpers, bounds calculation, palette selection,
  terrain polygon generation, and graph preservation.
- [ ] Run `node tests/map-scene.test.mjs` and confirm it passes.

### Task 2: Canvas Integration

**Files:**
- Modify: `smart-tourism-frontend/src/views/MapView.vue`
- Modify: `smart-tourism-frontend/tests/map-scene.test.mjs`

- [ ] Add source assertions proving non-demo rendering no longer directly draws
  the handcrafted demo terrain and no longer slices roads or selectable nodes.
- [ ] Run the test and confirm those assertions fail against the current view.
- [ ] Compute the current scene from selected spot and graph data.
- [ ] Use scene bounds for fitting, scene terrain for background, every backend
  road for roads, and every selectable node for POIs.
- [ ] Keep handcrafted rendering only inside the demo-area branch.
- [ ] Run the scene and existing map option tests.

### Task 3: Verification

**Files:**
- Verify: `smart-tourism-frontend/src/views/MapView.vue`
- Verify: `smart-tourism-frontend/src/utils/mapScene.ts`

- [ ] Run `npm.cmd run build` and confirm Vue TypeScript and Vite succeed.
- [ ] Open `/map`, select at least three backend areas, and record each area's
  scene signature, road count, and node count.
- [ ] Confirm signatures differ and browser console errors are empty.
