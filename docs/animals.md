# animals

This document is a dataset reference for the object-atlas pipeline (`docs/object-atlas-pipeline-plan.md`), not the runtime code contract.

## Sheet Metadata
- Base sheet: `assets/sprites/Basic Sprites 1x.png`
- Variants:
  - `assets/sprites/Basic Sprites 1x.png` (`scaleMultiplier = 1`, size `90x54`)
  - `assets/sprites/Basic Sprites 2x.png` (`scaleMultiplier = 2`, size `180x108`)
  - `assets/sprites/Basic Sprites 4x.png` (`scaleMultiplier = 4`, size `360x216`)
- Validation: `2x` and `4x` are exact nearest-neighbor upscales of `1x`.
- Packing: non-uniform tight packing (not a fixed tile grid).
- Coordinate system: `(x,y,w,h)` in pixels, origin at top-left.
- Scaling rule:
  - `x_scaled = x * scaleMultiplier`
  - `y_scaled = y * scaleMultiplier`
  - `w_scaled = w * scaleMultiplier`
  - `h_scaled = h * scaleMultiplier`

## Visual Order (Left-to-Right, Top-to-Bottom)
- Row 1: `TinyChick`, `CluckingChicken`, `HonkingGoose`, `DaintyPig`, `PasturingSheep`
- Row 2: `StinkySkunk`, `SpikeyPorcupine`, `MadBoar`, `SnowFox`, `TimberWolf`
- Row 3: `SlowTurtle`, `LeapingFrog`, `CroakingToad`, `CoralCrab`, `MeowingCat`

## Animal Atlas Map
| Animal | 1x bbox `(x,y,w,h)` | 2x bbox | 4x bbox | Idle sheet | Idle layout |
|---|---|---|---|---|---|
| `TinyChick` | `(3,6,11,11)` | `(6,12,22,22)` | `(12,24,44,44)` | `assets/animations/Tiny Chick/TinyChick.png` | `64x16`, 4 frames @ `16x16` |
| `CluckingChicken` | `(19,2,14,15)` | `(38,4,28,30)` | `(76,8,56,60)` | `assets/animations/Clucking Chicken/CluckingChicken.png` | `64x16`, 4 frames @ `16x16` |
| `HonkingGoose` | `(38,1,14,16)` | `(76,2,28,32)` | `(152,4,56,64)` | `assets/animations/Honking Goose/HonkingGoose.png` | `64x16`, 4 frames @ `16x16` |
| `DaintyPig` | `(55,5,16,12)` | `(110,10,32,24)` | `(220,20,64,48)` | `assets/animations/Dainty Pig/DaintyPig.png` | `64x16`, 4 frames @ `16x16` |
| `PasturingSheep` | `(73,3,16,14)` | `(146,6,32,28)` | `(292,12,64,56)` | `assets/animations/Pasturing Sheep/PasturingSheep.png` | `64x16`, 4 frames @ `16x16` |
| `StinkySkunk` | `(1,21,16,14)` | `(2,42,32,28)` | `(4,84,64,56)` | `assets/animations/Stinky Skunk/StinkySkunk.png` | `64x16`, 4 frames @ `16x16` |
| `SpikeyPorcupine` | `(19,22,16,13)` | `(38,44,32,26)` | `(76,88,64,52)` | `assets/animations/Spikey Porcupine/SpikeyPorcupine.png` | `64x16`, 4 frames @ `16x16` |
| `MadBoar` | `(37,23,16,12)` | `(74,46,32,24)` | `(148,92,64,48)` | `assets/animations/Mad Boar/MadBoar.png` | `64x16`, 4 frames @ `16x16` |
| `SnowFox` | `(55,22,16,13)` | `(110,44,32,26)` | `(220,88,64,52)` | `assets/animations/Snow Fox/SnowFox.png` | `64x16`, 4 frames @ `16x16` |
| `TimberWolf` | `(73,20,16,15)` | `(146,40,32,30)` | `(292,80,64,60)` | `assets/animations/Timber Wolf/TimberWolf.png` | `64x16`, 4 frames @ `16x16` |
| `SlowTurtle` | `(1,44,16,9)` | `(2,88,32,18)` | `(4,176,64,36)` | `assets/animations/Slow Turtle/SlowTurtle.png` | `64x16`, 4 frames @ `16x16` |
| `LeapingFrog` | `(20,42,15,11)` | `(40,84,30,22)` | `(80,168,60,44)` | `assets/animations/Leaping Frog/LeapingFrog.png` | `64x16`, 4 frames @ `16x16` |
| `CroakingToad` | `(38,42,15,11)` | `(76,84,30,22)` | `(152,168,60,44)` | `assets/animations/Croaking Toad/CroakingToad.png` | `64x16`, 4 frames @ `16x16` |
| `CoralCrab` | `(55,41,16,12)` | `(110,82,32,24)` | `(220,164,64,48)` | `assets/animations/Coral Crab/CoralCrab.png` | `64x16`, 4 frames @ `16x16` |
| `MeowingCat` | `(73,41,16,12)` | `(146,82,32,24)` | `(292,164,64,48)` | `assets/animations/Meowing Cat/MeowingCat.png` | `64x16`, 4 frames @ `16x16` |

## Idle Animation Notes
- Every idle sheet under `assets/animations/*/*.png` is `64x16`.
- Idle frame count is `4`.
- Frame rects are:
  - `frame0 = (0,0,16,16)`
  - `frame1 = (16,0,16,16)`
  - `frame2 = (32,0,16,16)`
  - `frame3 = (48,0,16,16)`
