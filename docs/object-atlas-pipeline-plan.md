# Object Atlas Pipeline Plan

## Objective
Build a reusable pipeline that converts atlas art assets into generic code specifiers used by rendering and animation.

Key requirement:
- No animal-specific domain classes (`AnimalId`, `AnimalAssets`, etc.).
- Animals are one dataset under a broader object-specifier system (NPCs, props, tiles, interactables).

Reference input docs:
- `docs/animals.md`
- `docs/world-tileset-spec.md`

## Current Baseline
- Render path is primitive-based (`circle` players) in `src/client/systems/render_system.hpp`.
- No formal atlas/specifier pipeline.
- Specs currently exist as markdown docs, not machine-readable source contracts.

## Design Principles
- Data-driven over type-per-object.
- One canonical source contract per atlas dataset.
- Generated code for fast runtime lookup and type safety.
- Separation of concerns:
  - Authoring data (`assets/specs/*.json`)
  - Validation/codegen (`tools/asset_pipeline/*`)
  - Runtime lookup/render (`src/client/assets/*`)

## Target Data Model (Generic)

### Runtime IDs
- `ObjectSpecifierId` (stable, generic ID for any visual object)
- `AtlasId` (texture atlas identity)
- `ClipId` (animation clip identity, e.g. `idle`)

### Runtime Structs
- `ObjectSpecifier`
  - `ObjectSpecifierId id`
  - `std::string_view key` (ex: `npc.tiny_chick`, `tile.grass_top`)
  - `ObjectCategory` (npc, tile, prop, fx, ui, etc.)
  - `AtlasId atlasId`
  - `RectI sourceRect1x`
  - `Pivot2f pivot`
  - `Span<ClipSpecifierRef> clips`
  - optional metadata tags
- `ClipSpecifier`
  - `ClipId id`
  - `frameWidth`, `frameHeight`
  - `frameCount`
  - `fps`
  - `sheetPath` or atlas reference

Animals become `ObjectSpecifier` entries with category `npc`.

## Pipeline Contract

### 1) Authoring Manifest (Source of Truth)
Add machine-readable manifests:
- `assets/specs/objects/animals.json`
- later: `assets/specs/objects/world_tileset.json`, etc.

Each manifest defines:
- atlas variants (`1x`, `2x`, `4x`)
- object keys + source rects (`1x` canonical)
- clip definitions (`idle`, etc.)
- per-object clip binding

### 2) Validation Stage
`tools/asset_pipeline/validate.py`:
- unique keys/IDs
- rect bounds vs source image
- scale variant consistency (`2x`, `4x` exact upscale if required)
- clip frame bounds/count checks

### 3) Codegen Stage
`tools/asset_pipeline/generate_cpp.py` emits:
- `src/client/assets/generated/object_specifiers.generated.hpp`
- optional `object_specifiers.generated.cpp`

Generated artifacts include:
- constexpr tables for specifiers/clips
- key->ID lookup map
- atlas variant metadata

### 4) Runtime Load Stage
Client loads textures once and queries via generated tables:
- `ObjectSpecifierRegistry` (generic)
- `AtlasTextureSet` for scale variants
- `AnimationResolver` for clip/frame rect lookup

## Integration into Existing Architecture

### Client Assets Layer
New modules:
- `src/client/assets/object_specifier_registry.hpp`
- `src/client/assets/object_atlas_store.hpp`
- `src/client/assets/generated/object_specifiers.generated.hpp` (generated)

### Client Components
Extend render-facing state with generic visual fields:
- `ObjectSpecifierId objectSpecifier`
- optional `ClipId activeClip` (default `idle`)
- animation playback phase/facing flags

No animal-specific component type.

### Animation System
Generic clip-frame resolver:
- `src/client/systems/animation_system.hpp`
- resolves current frame index from clip spec + time/tick

### Render System
`src/client/systems/render_system.hpp`:
- draws texture from `objectSpecifier` + resolved clip frame
- keeps primitive fallback for missing specifier/asset

### Optional Network Extension (Phase B)
When needed, wire generic visual ID across network:
- add `visualSpecifierId` field to spawn/snapshot payloads
- keep field generic (not `animalId`)

## Phased Implementation

### Phase 1: Manifest + Schema Foundation
- Introduce `assets/specs/objects/animals.json`.
- Add schema and parser in pipeline scripts.

Acceptance:
- Manifest can represent all entries from `docs/animals.md`.

### Phase 2: Validator + Consistency Checks
- Implement atlas bounds, uniqueness, clip and scale checks.

Acceptance:
- Fails fast on inconsistent or missing assets.

### Phase 3: C++ Code Generation
- Generate `object_specifiers.generated.hpp/.cpp`.
- Hook generation into build (manual command first, then CMake target).

Acceptance:
- Client compiles with generated specifier tables.

### Phase 4: Runtime Registry + Render Wiring
- Add generic registry/store modules.
- Update `GameClient` + `RenderSystem` to use object specifiers.

Acceptance:
- Players render from object specifiers, not hardcoded primitives.

### Phase 5: Mapping Strategy
- Initial mapping in client config or deterministic fallback:
  - example key set: `npc.*` for players
- No protocol change required in first pass.

Acceptance:
- Multi-client sessions show consistent visuals using same mapping logic.

### Phase 6: Network-Authoritative Visual IDs (Optional)
- Add generic `visualSpecifierId` to protocol snapshot/spawn if server authority is needed.

Acceptance:
- Server-selected visual IDs replicate correctly.

### Phase 7: Expand Beyond Animals
- Add world tileset/props manifests using same pipeline.
- Reuse validator/codegen without architecture changes.

Acceptance:
- Second non-animal dataset integrated with no new special-case code path.

## File Impact Map (Planned)
- New docs/data:
  - `assets/specs/objects/animals.json`
- New pipeline tools:
  - `tools/asset_pipeline/schema.json`
  - `tools/asset_pipeline/validate.py`
  - `tools/asset_pipeline/generate_cpp.py`
- New client runtime modules:
  - `src/client/assets/object_specifier_registry.hpp`
  - `src/client/assets/object_atlas_store.hpp`
  - `src/client/systems/animation_system.hpp`
  - `src/client/assets/generated/object_specifiers.generated.hpp` (generated)
- Updated runtime modules:
  - `src/client/components/components.hpp`
  - `src/client/systems/render_system.hpp`
  - `src/client/game_client.hpp`
  - `src/client/game_client.cpp`
- Optional later updates:
  - `src/shared/net/protocol.hpp`
  - `src/shared/net/snapshot.hpp`
  - `src/server/game_server.hpp/.cpp`

## Risks and Mitigations
- Risk: markdown specs diverge from runtime data.
  - Mitigation: manifests become canonical; markdown becomes documentation only.
- Risk: generated files drift from assets.
  - Mitigation: add validation/codegen checks to CI and local build target.
- Risk: too-generic IDs become hard to debug.
  - Mitigation: keep stable string keys (`npc.tiny_chick`) alongside numeric IDs.

## Definition of Done
- Animals are integrated via generic object specifiers, not animal-specific classes.
- A repeatable pipeline exists: `manifest -> validate -> generate -> runtime`.
- Render path consumes generic specifiers and clip data.
- Pipeline is proven by integrating at least one additional non-animal dataset.
