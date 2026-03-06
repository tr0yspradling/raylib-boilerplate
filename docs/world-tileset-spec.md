# `world_tileset.png` Object Spec

## Sheet Metadata
- Source: `assets/sprites/world_tileset.png`
- Dimensions: `256x256` px
- Grid: `16x16` tiles
- Tile size: `16x16` px
- Non-empty tiles: `127`
- Coordinate system: `(x,y)`, zero-based, origin at top-left
- Row-major tile id formula: `tileId = y * 16 + x`
- Naming note: object labels are visual descriptors (no embedded source metadata was found).

## Core Terrain Block Families (Rows `0-2`)
These are visually grouped block variants (top/fill/special).

| Family | Top | Fill | Special / Marker |
|---|---|---|---|
| Green dirt | `(0,0)` | `(0,1)` | `(0,2)` swirl glyph |
| Brown dirt | `(1,0)` | `(1,1)` | `(1,2)` exclamation glyph |
| Beige stone | `(2,0)` | `(2,1)` | `(2,2)` mixed cap block |
| Gray-beige stone | `(3,0)` | `(3,1)` | `(3,2)` question glyph |
| Yellow/amber | `(4,0)` | `(4,1)` | `(4,2)` question glyph |
| Rose/brick | `(5,0)` | `(5,1)` | `(5,2)` exclamation glyph |
| Blue/slate | `(6,0)` | `(6,1)` | `(6,2)` dotted variant |
| Cyan/water-like | `(7,0)` | `(7,1)` | `(7,2)` swirl glyph |
| Dark slate | `(8,0)` | `(8,1)` | `(8,2)` exclamation glyph |
| Pebble strips (dark/med/light) | `(9,0) (10,0) (11,0)` | `(9,1) (10,1) (11,1)` | `(9,2) (10,2) (11,2)` |

## World Props and Objects (Rows `3-8`)

| Object | Tiles | Notes |
|---|---|---|
| Bush/grass clumps | `(0,3) (1,3) (0,4) (1,4) (0,5) (1,5)` | Green foliage variants |
| Marker block (beige) | `(2,3)` | Vertical exclamation marker block |
| Lava fall / blob sequence | `(5,3) (5,4) (5,5) (5,6) (5,7) (5,8)` | Top-to-small progression |
| Green vine/trunk column | `(6,3) (6,4) (6,5)` | Vertical plant/trunk set |
| Crate/sign set (brown + rose) | `(7,3) (8,3) (7,4) (8,4)` | Wooden panel/sign-like tiles |
| Fence segments | `(9,3) (9,4)` | Horizontal then vertical |
| Palm tree (multi-tile) canopy | `(2,5) (3,5) (4,5)` | 3-tile canopy |
| Palm tree trunk | `(3,6) (3,7) (3,8)` | 3-tile trunk |
| Palm helper pixels | `(2,4) (3,4) (4,4) (2,6) (4,6)` | Sparse connector/anti-alias tiles |
| Mushroom variants | `(7,5) (8,5) (7,6) (8,6) (7,7) (8,7) (7,8) (8,8)` | Warm, purple, green caps |
| Small foliage/rock variants | `(6,6) (6,7) (6,8)` | Rounded green mound set |
| Flower patch | `(1,6)` | White flower cluster |
| Potion/flask set | `(0,7) (0,8) (1,8)` | Green, blue, purple bottles |
| Flame/torch icon | `(1,7)` | Orange flame tile |
| Brown patch fragment | `(2,8)` | Decorative chunk |
| Ring/pumpkin icon | `(4,8)` | Circular orange emblem |
| Orange blob chunk | `(5,8)` | Rounded orange piece |

## Color-Palette / Autotile Region (Rows `9-15`)
This region is a palette + transition set (fills, motifs, and top-edge overlays).

### Column Families
- `x=0` blue/navy family: `(0,9) (0,10) (0,11) (0,12) (0,13) (0,14) (0,15)`
- `x=1` peach/yellow/orange family: `(1,9) (1,10) (1,11) (1,12) (1,13) (1,14) (1,15)`
- `x=2` red/magenta/purple family: `(2,9) (2,10) (2,11) (2,12) (2,13) (2,14) (2,15)`
- `x=3` gray/slate family: `(3,9) (3,10) (3,11) (3,12) (3,13) (3,14) (3,15)`

### Edge/Overlay Tiles
- Cyan top-edge + fill: `(4,9)`, fill `(4,10)`
- Cyan transparent overlay: `(5,9)`
- Periwinkle top-edge + fill: `(6,9)`, fill `(6,10)`
- Periwinkle transparent overlay: `(7,9)`
- Magenta fill + edge overlay: fill `(4,11)`, overlay `(5,11)`
- Orange/lava set: patterned fill `(4,12)`, top-edge `(4,13)`, fill `(4,14)`, overlay `(5,13)`

## Empty Region
- Most tiles in columns `10-15` and lower-right rows are transparent/unused.
