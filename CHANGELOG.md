# Changelog

All notable changes to the firmware. Versions match the number shown at the
bottom of the clock/settings screen (swipe down) and `web/manifest.json`.

Updating from the [web installer](https://socquique.github.io/TamaPoke/web/)
**without** ticking "Erase device" keeps your Pokémon.

## [Unreleased]

### Added

- **Sound, beyond beeps.** The 151 cries, six battle themes and seven
  fanfares, taken from the Anil and converted to raw 16 kHz mono PCM by
  `tools/pack_audio.py` (about 11 MB on the card). A Pokémon cries when it
  hatches, when it evolves, when either side comes out in battle, and when
  you open its Pokédex page. The music says who you are fighting before you
  read it — wild, trainer, gym, Elite Four and champion each have their own —
  and winning, catching, evolving, hatching and earning a badge each get
  their jingle. The audio task now **mixes**: the old square-wave beeps sit on
  top of the music instead of replacing it, and they still work with no card
  in the device, because they are synthesised rather than sampled.
- **Switching mid-battle**, and not only when your Pokémon goes down. A team
  button next to the bag opens the list; picking someone costs you the turn,
  so the opponent hits while they come in. When the one out there faints and
  you still have someone, the battle no longer ends — the list opens and you
  have to send the next one, and that switch is free because the faint
  already paid for it. Whoever fought keeps the HP and PP they came back
  with, substitute or not.
- **Terrains** (Electric, Grassy, Misty, Psychic), the four moves that set
  them, and the rule that makes them different from weather: a terrain only
  reaches whoever is standing on the ground, so a Flying type or a Levitate
  gets neither the boost nor the protection. Grassy heals, Misty blocks
  status, Electric blocks sleep, and each raises its own type by 30%.
- A field indicator in battle showing the weather and terrain with the turns
  they have left. There wasn't one even for weather.

### Changed

- Status pills use the Anil's own colours and the shape with the darker lip,
  instead of flat rectangles.
- Evolution happens on the full-screen glow from the games, not over the
  house with the bars still showing.
- **Several landscapes per place, not one.** Each biome now has three or four
  scenes and the battle picks one at random; the home screen rotates every 20
  minutes, so the screen you leave switched on doesn't show the same picture
  all day. 42 scenes, about 17 MB on the card.
- Professor Oak's intro, the boy/girl question and the starter choice happen
  in a real lab. The clock/settings and name screens use the Pokédex chrome.
  The sandbag session borrows whichever landscape home is already showing —
  you train where you live, and it costs no extra loading.
- **The shell, not just the sprites.** The home screen now stands on a real
  scene from the SD (the species' biome by day, a night scene after dark)
  instead of a painted gradient, and the bottom strip is the HGSS frame
  rather than a flat beige rectangle. Names over the scene are outlined so
  they stay readable.
- **The Pokédex looks like a Pokédex**: the dark grid, ghost Poké Ball
  watermark and teal-framed panels of the Anil's own dex screen, drawn (four
  flat colours, some lines and a circle) rather than packed.
- The eight card pages use that same dex chrome, with their ink inverted.
- Feeding shows the real berries (Cheri, Oran, Lum) and a Rare Candy in a
  dialogue frame; they were four hand-drawn 16×16 blobs.
- The ball thrown at a wild Pokémon is the game's own ball sprite, and the
  one you actually threw — Poké, Great, Ultra or Master.
- Fixed: the "rare egg" label was drawn under the bottom strip, so it had
  never been visible.

### Added

- Battle backgrounds. 20 scenes packed to the SD card (`/bg/NN.bin`, about
  8.5 MB) by `tools/pack_bg.py`. The scene is chosen when the battle starts:
  gym leaders and the Elite Four each get their own room, trainers fight in
  town, and a wild battle uses the species' biome, so a Geodude shows up on a
  rocky slope and a Tentacool on the beach. **Without the files nothing
  breaks** — the painted sky is still there as the fallback.
- Type chips and the move list now show the real type symbol, in the colour
  the symbol was drawn for.
- Move effects in battle, in three tiers. **125 moves have their own
  animation**, taken from the Anil's own sheet for that move. The rest fall
  back to one of five by **what the move does** — raising stats, lowering
  them, healing, guarding, anything else — and then to one of 18 by **type**.
  Tail Whip was showing the slash from Vice Grip, so lowering a defence
  looked like a scratch. Weather, terrain and status moves keep their type
  animation on purpose: a Thunder Wave should look electric. Nothing is
  drawn at all when the move misses or is blocked. All of it lives in flash
  (457 KB), so there is nothing to load when a move goes off and it works
  with no card in the device.
- A volume control in the settings: the sound pill now steps through off,
  low, medium and high instead of just on and off. It moves the DAC's own
  volume register rather than scaling the samples, which would throw away
  bits at low volume.
- Home-screen music, several tracks rotating on the same 20-minute turn as
  the landscape, plus a separate one for the night.
- Shiny Pokémon sparkle when they come out, like in the games.
- Shiny is marked with a star sprite instead of an asterisk: main screen, card
  header, Pokédex detail, and the HP plate in battle — where the wild-encounter
  message used to carry it, because the plate stays for the whole fight. The
  team and bank lists show it too; until now a shiny in the box looked exactly
  like a normal one.
- HGSS-style dialogue frame (dark edge, gold rim, white paper) on Professor
  Oak's intro, the battle message box and the move card.
- The four main-screen buttons and the title screen's Poké Ball are real
  sprites instead of hand-drawn pixel maps.

### Changed

- `tools/send_sd.py` takes `--dir` (so it can push `/bg` as well as `/mons`)
  and finds the board on Windows. Copying the card on a PC is much faster for
  the backgrounds.
- `FIGHT <dex> <lv> S` forces a shiny opponent, like `WILD` already did.

## [1.17] - 2026-09-18

### Added

- **Korean, the eighth language.** Full UI (85 strings), the three medal name
  lengths and the 151 gen-1 species names, on the same `STRINGS[LANG][StrId]`
  structure as every other language. The text measurement centralised in 1.15
  carries almost all of it; the four byte-based spots a UTF-8 row turned up are
  listed under Fixed.
- `u8g2_font_unifont_t_korean2` for Hangul, selected per language in
  `applyLangFont()` rather than through a single `CJK_FONT`. The Korean added
  uses 296 distinct syllables, all of them in KS X 1001, the 2350-syllable set
  that matches the size of `korean2`; `korean1` holds 478. Coverage was checked
  as membership in KS X 1001 rather than by walking the font table, and the
  ASCII in the strings is assumed present. Not yet checked on the board:
  whether `CJK_SIZE_DIV` suits `korean2` as it does the Japanese subset.

### Fixed

- **The release dialog could cut a Korean name mid-character.**
  `renderRelease()` built into `char q[28]`. `"%s 놓아줄까요?"` is 17 bytes
  before the name, so names of four syllables or more (34 of the 151) did not
  fit, and `snprintf` truncated inside a 3-byte sequence, leaving a tail the
  font cannot decode. Now 48; the longest case needs 33 bytes including the
  terminator.
- **Three places sized and positioned text with `strlen()`**, which counts
  bytes: the stat-card header, the species name under a nickname, and the
  gallery detail header. On a UTF-8 row the centring offset and the auto-shrink
  threshold worked off a byte count rather than a width. All three now go
  through `textW()` / `centerX()`. With the classic font `textW()` returns
  `strlen()*6*size`, the same expression as before, so the six Latin languages
  land on the same coordinate at the same size, checked at every length from 1
  to 25.
- **Three Japanese texts were cut short**, all already in 1.15 and 1.16.
  Their buffers were sized when every string was one byte per character, and
  `snprintf` truncates without a trace. The minigame record read `きろく 2`
  with a best of 219, since `char rec[12]` left room for one digit. The streak
  milestone banner never fit at all: `"%u にちれんぞく！"` passes 20 bytes with a
  single digit, and the cut landed mid-character. The streak line on the stat
  card lost digits from 100 days on. The release dialog fixed above was the
  fourth.
- **Every formatted text is now checked against its buffer, in all eight
  languages.** A new test in `test/test_tools.py` finds each
  `snprintf(buf, sizeof(buf), T(...))` in the sketch, reads the size of `buf`,
  and computes the worst case per language from the argument types (5 digits
  for `%u`/`%d`, 10 for `%lu`, and a declared maximum for each `%s`). It is what
  caught the three above, and it would have caught all four. To keep it passing
  by type rather than by typical value, four more buffers get headroom they had
  not yet needed: the medal count, the next-level line, the evolution countdown
  and the profile line.
- **`tools/test_i18n_formats.py` reads only the first `len(LANGS)` language
  blocks**, and `LANGS` listed six, so later rows were skipped without a
  warning. It now lists all eight, and the docstring notes that it needs to
  stay complete.
- **`test/test_i18n.cpp`'s `LANG_NAME[LANG_COUNT]` had six entries**, fewer
  than `LANG_COUNT`, so every entry past the sixth was null. Four of the tests
  build a label from `LANG_NAME[lang]` on every iteration, not only when a
  check fails, so each run passed a null pointer to `%s`. Now lists all eight.

## [1.16] - 2026-09-09

### Fixed

- **Japanese was silently dropping every `！` and `？`.** 21 exclamation marks
  and 4 question marks — a quarter of the Japanese strings, including the mood
  line on the main screen (`ごきげん！` rendered as `ごきげん`). The cause is that
  `u8g2_font_unifont_t_japanese1` carries kana and kanji but no CJK punctuation
  at all — no `！？。、「」` — and `Arduino_GFX` draws nothing for a glyph it can't
  find, without even advancing the cursor, so the loss is invisible in the
  source and on screen. Switched to `japanese3`, the only U8g2 Japanese font
  that has them. Same unifont family: the metrics header is byte-identical and
  all 226 codepoints the firmware uses have byte-identical bitmaps, so nothing
  moves. Costs 102 KB of flash (705 KB → 810 KB, 22% → 25% of the app
  partition); RAM is unchanged.
- **Nidoran♀ and Nidoran♂ both showed as ニドラン in Japanese**, leaving #29
  and #32 indistinguishable in the Pokédex. No U8g2 unifont subset carries
  ♀ (U+2640) or ♂ (U+2642) — not `japanese*`, `korean*` or `chinese*` — and
  `Arduino_GFX` draws nothing for a glyph it can't find, without even
  advancing the cursor. `gen_names.py` already substituted F and M on the
  Latin path; it now does the same on the UTF-8 one, so the names read
  ニドランF and ニドランM. Convention chosen by @usakomint, who preferred it
  over ニドラン(女)/(男) as closer to a species name and shorter on a small
  round screen.

Both confirmed on hardware by @usakomint, who also — without knowing it —
supplied the evidence for the first one: the missing `！` was visible in a
photo he posted of the v1.15 main screen.

## [1.15] - 2026-09-08

### Added

- **Japanese.** UI, medals and all 151 species names in katakana (フシギダネ,
  ピカチュウ). Selectable from the settings screen like any other language.

### Changed

- Text measurement and cursor positioning are now centralized (`textW()`,
  `centerX()`, `setCur()`, `setSize()`, `printT()`), replacing ~180 sites that
  assumed one byte per character and a fixed 6 px width. Latin languages render
  identically — the equivalence is arithmetic, not approximate. This is what
  makes CJK possible without a per-language branch at every draw site, and
  Chinese and Korean now only need their strings.
- Bar positions on the stat card and the care row are derived from the width of
  the widest translated label instead of fixed coordinates, so they stay aligned
  in any language. With Latin labels the numbers come out identical to before.
- New dependency: **U8g2**, for its CJK font data only.

Japanese font handling, readability and layout were all tuned against real
hardware by @usakomint, who found and diagnosed every issue in the process.

## [1.11] - 2026-09-07

### Fixed

- **The whole loop froze for exactly 1 second when touching the screen.**
  `touch.getPoint()` hung on a sleeping CST9217 for the ESP32 I2C driver's
  default 1000 ms timeout, which `Wire.setTimeOut(50)` doesn't bound because
  SensorLib doesn't route through it. Most visible in the ball minigame, where
  taps come fast and two or three stalls ran together — reported as "freezes for
  2–3 seconds on ball tap". An address-only Wire transaction before the read
  fixes it. Measured on hardware over equivalent play (~470 touch reads): 5
  stalls in 60 s before, 0 after. Reported by @gingsiro.

## [1.10] - 2026-09-07

### Fixed

- TPK2 sprites with zero-length frame durations spun `pmdFrameAt()` forever,
  hanging the render loop. These files arrive over USB, so they aren't trusted
  input: durations are clamped at load time and the loop is bounded regardless.
- `thumbs.bin` was mapped into PSRAM without bounding its size or checking that
  the offsets it contains point inside what was actually read. A truncated file
  (plausible — the transfer takes minutes) could read past the allocation.
- The serial `PUT` handler ignored `f.write()`'s return value, so a full SD card
  still reported `DONE` while leaving a truncated file behind. It also took the
  destination path straight from the serial line, so a crafted `PUT` could write
  anywhere on the card; writes are now confined to `/mons/`.
- Touches just outside the Pokédex grid and the on-screen keyboard were treated
  as cell 0. The guard divided first and then checked for a negative index, but
  integer division truncates toward zero, so it never saw one. On a round screen
  that strip is exactly where a finger lands near the edge.

All four from @ajtudela.

## [1.9] - 2026-09-07

### Fixed

- All ~13 game timers compared `millis()` against an absolute deadline, which
  gives the wrong answer once `millis()` wraps at ~49.7 days of uptime: a timer
  could stay active forever, or a ceremony cut off instantly.
- The sprite transfer restarted from scratch on every attempt. It now records
  each confirmed file, so a reload after a dropped connection doesn't redo the
  full ~40 MB / ~10 min.
- Minigame physics advanced per frame rather than per unit of time, so the ball
  fell measurably slower with a large sprite on screen (Charizard) than a small
  one (Diglett) — high scores weren't comparable between species.

### Security

- `esp-web-tools` was loaded from a floating `@10` range with no integrity
  check. Pinned to 10.4.0 with a verified sha384 SRI hash.

All from @ajtudela.

## [1.8] - 2026-09-07

### Fixed

- **The level counter wrapped to 0 after ~10.6 days of play.** `level()`
  returned `uint8_t` and, at 60 minutes per level, the level is the hour count —
  so it rolled over at 256. Reachable in ordinary play, since "stay together"
  postpones the farewell indefinitely. Now `uint16_t`, capped at 999.
- Two related truncations found while fixing it: `calcStat()` took the level as
  `uint8_t`, so stats would have overflowed anyway; and `canEvolveNow()` compared
  against `(uint8_t)(evolveLevel + careMistakes)`, which wrapped with many care
  mistakes and let evolution happen early. The stat card already computed that
  threshold as `int`, so the screen and the actual check could disagree.

Diagnosed by @ajtudela.

## [1.7] - 2026-09-07

### Fixed

- **The UI had no accents, and the assumption behind that was wrong.** The code
  stated in two places that the font had none. In fact `glcdfont.h` is a full
  256-glyph CP437 table and `write()` doesn't filter bytes ≥ 0x80. Spanish now
  has its tildes and ñ, German its umlauts, French its accents. Characters are
  written as single bytes in octal, never UTF-8, because the UI centres with
  `strlen(t) * 6`. Only strings whose length was unchanged were touched.
  Uppercase accented vowels other than É aren't in CP437, so `EVOLUCION` and
  `PROGRES` stay unaccented, as do Portuguese ã/õ.
- German grammar, from the same contributor: "Wähle deinen Starter" (accusative),
  "Leb wohl" as two words, "Entwicklung" rather than "Entwickelt", and more.

Discovered by @danielberndt.

## [1.6] - 2026-09-07

### Fixed

- **Short sound effects were inaudible.** The amplifier was switched on 8 ms
  before each effect, far less than the NS4150B needs to settle, so a 35 ms tap
  finished before the speaker woke up. The boot jingle survived because it's
  ~440 ms — which is why sound seemed to work at startup and nowhere else. The
  amplifier now stays powered while sound is on and the pet is awake, matching
  Waveshare's reference. Notes and amplitude are unchanged: this doesn't alter
  the sound design, it just makes it audible.

Diagnosed by @djyf1.

## [1.5] - 2026-08-19

### Added

- Localized Pokémon names in French and German (Bulbizarre, Bisasam…). Only
  those two differ in gen 1; Spanish, Italian and Portuguese officially use the
  English names. Names come from PokéAPI via `tools/gen_names.py`.

## [1.4] - 2026-08-07

### Fixed

- **Bond was mathematically stuck.** It could gain at most 12 points a day, but
  a single care mistake cost 3 and could repeat every 30 minutes — up to 144 a
  day. With poops draining hygiene, mistakes chained and bond never recovered.
  Two players independently reported it pinned at exactly 10, which is the
  first-day ceiling of 13 minus one mistake. Daily cap 8 → 20, penalty 3 → 1,
  cooldown 30 → 60 min.

## [1.3] - 2026-08-07

### Fixed

- **The egg hatched by itself during starter selection.** Game time kept running
  while the starter screen was open, so taking more than 3 minutes to choose
  meant the egg hatched into the species already rolled — no egg animation, and
  the player's choice ignored. Three community reports matched (Pikachu twice,
  Eevee once). Time now freezes until a starter is chosen.

## [1.2] - 2026-06

### Added

- Firmware version shown at the bottom of the clock/settings screen.
