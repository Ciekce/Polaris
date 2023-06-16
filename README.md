<div align="center">

# Polaris

[![License](https://img.shields.io/github/license/Ciekce/Polaris?style=for-the-badge)](https://github.com/Ciekce/Polaris/blob/main/LICENSE)  
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/Ciekce/Polaris?style=for-the-badge)](https://github.com/Ciekce/Polaris/releases/latest)
[![Commits since latest release](https://img.shields.io/github/commits-since/Ciekce/Polaris/latest?style=for-the-badge)](https://github.com/Ciekce/Polaris/commits/main)

</div>

a work-in-progress UCI chess and [chess960](https://en.wikipedia.org/wiki/Fischer_random_chess) engine  
this branch is an experiment for fun, using [Viridithas](https://github.com/cosmobobak/viridithas)' current network (net 60) and inference  
due to the radically different eval, WDL normalisation and output are removed :3

## Strength
| Version | [CCRL 40/15](https://www.computerchess.org.uk/ccrl/4040/) | [CCRL Blitz](https://www.computerchess.org.uk/ccrl/404/) | [CCRL 40/2 FRC](https://www.computerchess.org.uk/ccrl/404FRC/) | [MCERL](https://www.chessengeria.com/mcerl) |
|:-------:|:---------------------------------------------------------:|:--------------------------------------------------------:|:--------------------------------------------------------------:|:-------------------------------------------:|
|  1.7.0  |                             -                             |                            -                             |                              2839                              |                      -                      |
|  1.6.x  |                           2806                            |                           2884                           |                              2745                              |                      -                      |
|  1.5.0  |                           2679                            |                           2749                           |                              2508                              |                    2713                     |
|  1.4.x  |                           2640                            |                            -                             |                              2455                              |                      -                      |
|  1.3.0  |                           2450                            |                           2513                           |                              N/A                               |                      -                      |
|  1.2.0  |               ~2300 (very few games played)               |                           2329                           |                              N/A                               |                      -                      |

## Features
- standard PVS with quiescence search and iterative deepening
  - aspiration windows
  - check extensions
  - continuation history (1- and 2-ply)
  - countermoves
  - futility pruning
  - history
  - internal iterative reduction
  - killers (1 per ply)
  - late move reductions
  - mate distance pruning
  - nullmove pruning
  - reverse futility pruning
  - SEE move ordering and pruning
- Texel-tuned HCE (private tuner because that code hurts me to reread)
  - tuner based on Andrew Grant's [paper](https://github.com/AndyGrant/Ethereal/blob/master/Tuning.pdf)
  - tuned on a combination of the Zurichess and lichess-big3-resolved datasets
- BMI2 attacks in the `bmi2` build, otherwise fancy black magic
  - `pext`/`pdep` for rooks
  - `pext` for bishops
- lazy SMP

## To-do
- tune search constants
- contempt
- make it stronger uwu

## UCI options
| Name            |  Type   | Default value |  Valid values   | Description                                                                                                     |
|:----------------|:-------:|:-------------:|:---------------:|:----------------------------------------------------------------------------------------------------------------|
| Hash            | integer |      64       |   [1, 131072]   | Memory allocated to the transposition table (in MB). Rounded down internally to the next-lowest power of 2.     |
| Clear Hash      | button  |      N/A      |       N/A       | Clears the transposition table.                                                                                 |
| Threads         | integer |       1       |    [1, 2048]    | Number of threads used to search.                                                                               |
| UCI_Chess960    |  check  |    `false`    | `false`, `true` | Whether Polaris plays Chess960 instead of standard chess.                                                       |
| Underpromotions |  check  |    `false`    | `false`, `true` | Whether underpromotions to rooks and bishops are generated.                                                     |
| Move Overhead   | integer |      10       |   [0, 50000]    | Amount of time Polaris assumes to be lost to overhead when making a move (in ms).                               |

## Builds
`bmi2`: requires BMI2 and assumes fast `pext` and `pdep` (i.e. no Zen 1 and 2)  
`modern`: requires BMI (`blsi`, `blsr`, `tzcnt`) - primarily useful for pre-Zen 3 AMD CPUs back to Piledriver  
`popcnt`: just needs `popcnt`  
`compat`: should run on anything back to an original Core 2

Alternatively, build the CMake target `polaris-native` for a binary tuned for your specific CPU (see below)  
(note that this does *not* automatically disable `pext` and `pdep` for pre-Zen 3 AMD CPUs that implement them in microcode)

### Note:  
- If you have an AMD Zen 1 (Ryzen x 1xxx) or 2 (Ryzen x 2xxx) CPU, use the `modern` build even though your CPU supports BMI2. These CPUs implement the BMI2 instructions `pext` and `pdep` in microcode, which makes them unusably slow for Polaris' purposes. 
- Builds other than `bmi2` are untested and might crash on CPUs lacking newer instructions; I don't have older hardware to test them on.

## Building
**The makefile is not intended for building by users. It exists purely for OpenBench compliance.**  
Requires CMake and a competent C++20 compiler (tested with Clang 15 and 16 on Windows, GCC 11 and Clang 15 and 16 on Linux, and Apple Clang 14 on macOS on Apple Silicon)
```bash
> cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/
> cmake --build build/ --target polaris-<TARGET>
```
(replace `<TARGET>` with your preferred target - `native`/`bmi2`/`modern`/`popcnt`/`compat`)

If you have a pre-Zen 3 AMD Ryzen CPU (see the notes in Builds above) and want to build the `native` target, use these commands instead (the second is unchanged):
```bash
> cmake -DCMAKE_BUILD_TYPE=Release -DPS_FAST_PEXT=OFF -S . -B build/
> cmake --build build/ --target polaris-native
```
Disabling the CMake option `PS_FAST_PEXT` builds the non-BMI2 attack getters.

## Credit
Polaris uses [Fathom](https://github.com/jdart1/Fathom) for tablebase probing, licensed under the MIT license.
