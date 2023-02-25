# Like a Dragon: Ishin! Fix
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)</br>
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/IshinFix/total.svg)](https://github.com/Lyall/IshinFix/releases)

This is a fix to remove pillarboxing/letterboxing with ultrawide/wider displays in Like a Dragon: Ishin!

## Features
- Removes pillarboxing/letterboxing in gameplay and cutscenes.
- Disable cutscene FPS cap. This removes the 30fps cap in cutscenes.
- Controller icon override. This lets you force PS4/PS5/XBOX controller icons.

## Installation
- **Note:** ***Please make sure any ultrawide-related hex edits or pak mods are removed/reverted first***.
- Grab the latest release of IshinFix from [here.](https://github.com/Lyall/IshinFix/releases)
- Extract the contents of the release zip in to the the Win64 folder.<br />(e.g. "**steamapps\common\LikeADragonIshin\LikeaDragonIshin\Binaries\Win64**" for Steam).

### Linux/Steam Deck
- Make sure you set the Steam launch options to `WINEDLLOVERRIDES="winmm.dll=n,b" %command%`

## Configuration
- See **IshinFix.ini** to adjust settings for the fix.

## Known Issues
Please report any issues you see.

- Brothel "Sensual Healing" minigame has visual bugs. [(#4)](https://github.com/Lyall/IshinFix/issues/4)<br>
This is a known bug in the game at wider than 16:9 resolutions and not related to IshinFix.

## Screenshots

| ![ezgif-3-ffd6bc6ca3](https://user-images.githubusercontent.com/695941/220556346-b40c2d23-7c33-4545-abc5-32b8186507fb.gif) |
|:--:|
| Disabled pillarboxing/letterboxing in gameplay. |

| ![ezgif-3-cd50579b2d](https://user-images.githubusercontent.com/695941/220556453-ff9f70a9-e762-4351-9a29-b2c1a792aad9.gif) |
|:--:|
| Disabled pillarboxing/letterboxing in cutscenes. |

## Credits
[Flawless Widescreen](https://www.flawlesswidescreen.org/) for the LOD fix.<br />
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[Loguru](https://github.com/emilk/loguru) for logging. <br />
[length-disassembler](https://github.com/Nomade040/length-disassembler) for length disassembly.
