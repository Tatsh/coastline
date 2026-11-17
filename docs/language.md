# Language

`Language` in the `[Game]` section selects the language of the text, `en-us`, the default,
`en-gb`, or `ja`. `--language` selects one for a single run. The game reads the strings of the
language from `strings/LANGUAGE.json` in the assets, and takes the string of `strings/en-us.json`
for each key the language lacks. The text draws with the font of the cartridge of the language,
the European release for `en-gb`, the Japanese release for `ja`, and the USA release for `en-us`,
or the base fonts when the cartridge of the language was not imported. Region selects splash
screens, menu models, and textures independently of Language.

```ini
[Game]
Language = ja
```

## Strings of the disc

The disc import writes the strings of BRString.dll to `strings/en-us.json`, each under the key the
table of `src/sdl/consolestrings.cpp` gives its number, and the race of the game reads a string by
its number through the same table. A message the PC and the Nintendo 64 releases draw in the same
place with the same text has one `race.` key, written by the disc and by the cartridges, and a
string of the PC release alone has a `pc.` key.

## Strings of the cartridges

The import of a cartridge writes the text of the messages of its menus and its race to the file of
its language: the USA release to `strings/en-us.json`, the European release to `strings/en-gb.json`,
and the Japanese release to `strings/ja.json`. A message has the same key in every file, such as
`n64.carSelect.rumblePakNote`, and the table of `src/sdl/consolestrings.cpp` lists the address of
each line of the message in each release. A release draws the lines of a message one under the
other, and the import joins them with a newline. The lines of each release differ, and a message
of one release alone is only in the file of the release. The game saves to a folder, and the
import of the USA cartridge writes the messages that refer to the save folder in place of the
Controller Pak to `strings/en-us.json` under `sdl.` keys, and every language reads them there. The
import of the Japanese cartridge also writes the kana and kanji of its font to
`fonts/jpn/japanese.png`. Every file of strings is UTF-8.

The Japanese release has Japanese text only in the messages of the Controller Pak, the Rumble Pak,
the paint shop, and the second controller. Its menus, its races, and its titles are in English,
and its speed reads `Mile/h` and `Km/h`. With `ja`, the races draw the English strings and the
speed units of the Japanese cartridge, and the car selection shows the Japanese note of the Rumble
Pak. The European
release has the English text of the USA release, with the messages of its save to the Game Pak
and a few additional lines.
