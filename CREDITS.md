# Credits

TamaPoke is a **non-commercial, personal-use** project. It does not sell or
commercially redistribute any copyrighted material. Pokémon and all related
names, designs and characters are trademarks and © of **Nintendo / Game Freak /
The Pokémon Company**.

This project is not affiliated with or endorsed by any of those companies.

## Sprites and data

| Resource | Source | Use in the project |
|---|---|---|
| **All sprites** (idle, walk, sleep, eat, hurt, attack…) | [PMD Sprite Collaboration (PMDCollab/SpriteCollab)](https://github.com/PMDCollab/SpriteCollab) | Mystery-Dungeon-style animated sprites used everywhere: main screen, stat card, minigame, and the Pokédex grid + detail view |
| **Gen 1 base stats** | [PokéAPI](https://pokeapi.co) | Real ATK/DEF/SPD/HP for each species |
| **Ribbons and Z-Crystals** | [PokéSprite (msikma/pokesprite)](https://github.com/msikma/pokesprite) | Achievement ribbons (`tools/baixa_fitas.py`) and the Alola trial Z-Crystals shown as badges |
| **Badges, Hoenn to Paldea** | [WikiDex](https://www.wikidex.net/wiki/Medalla) | Gym badge images (`tools/baixa_insignias.py`) |

The **SpriteCollab** sprites are the work of its community of artists under their
own terms (Creative Commons Attribution-NonCommercial 4.0). Per-species/per-author
credit is in the original repository's
[tracker.json](https://github.com/PMDCollab/SpriteCollab/blob/master/tracker.json).
Huge thanks to that whole community for an enormous amount of work.

> **Important if you reuse this repo:** the packaged sprite files
> (`tools/sdcard/mons/*.bin`) are derived from the sources above. Don't
> redistribute them commercially. If you publish the project, the clean approach
> is to distribute **only the code and scripts**, and have each user download and
> package the sprites from the original sources with `tools/pack_*.py` (or the web
> installer).

## Pokémon Añil

This version's battle mechanics, move animations, weather and many battle
resources come from the fan game **Pokémon Añil (Definitive Edition)**, which
the TamaPoke data was checked against (`tools/contraste.py`). Credit and thanks
to its team, and to the people its own credits list for those resources:

| What | Credited in Añil to |
|---|---|
| Engine base ("La Base de Sky", on Essentials v21.1) | Skyflyer, DPertierra (+ DarmanInigo, Nieves1236, deNombreTuri, Pokepachito, Ebaru) |
| Move animations project | led by StCooler; DarryBD99, WolfPP, ardicoozer, riddlemeree, Kirik, Marin, Maruno, AiurJordan, Appletun, ThatWelshOne_; resources from the Reborn team |
| Attack sound effects | BellBlitzKing (Pokemon Sound Effects Pack, Gen 1-7) |
| Pokémon Essentials | Maruno and contributors |

These resources belong to their authors; they are used here in the same
non-commercial, personal-use spirit as the rest of this project.

## Software / hardware

| Component | Author / source |
|---|---|
| GFX Library for Arduino | [moononournation](https://github.com/moononournation/Arduino_GFX) |
| SensorLib (CST9217 touch, PCF85063 RTC) | [Lewis He / lewisxhe](https://github.com/lewisxhe/SensorLib) |
| XPowersLib (AXP2101 PMU) | [Lewis He / lewisxhe](https://github.com/lewisxhe/XPowersLib) |
| U8g2 (CJK font data only) | [olikraus](https://github.com/olikraus/u8g2) |
| Board and pinout | [Waveshare ESP32-S3-Touch-AMOLED-1.75](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75) |
| Web installer | [ESP Web Tools](https://esphome.github.io/esp-web-tools/) (Nabu Casa) |

TamaPoke's own code (firmware and tools) is original work.
