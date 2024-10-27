## D2RTextExtractor

Extracts specific text and json files from the D2R CASC file to be used by other programs. The files that are extracted are:
- `data\global\animdata.d2`
- `data\global\dataversionbuild.txt`
- `data\hd\global\excel\desecratedzones.json`
- `data\global\excel\*.txt`
- `data\local\lng\strings\*?.json` from file names `item-gems`, `item-modifiers`, `item-nameaffixes`, `item-names`, `item-runes`, `levels`, `mercenaries`, `monsters`, `npcs`, `objects`, `quests`, `shrines`, `skills`, `ui`, `vo`

Additionally, `animdata.d2` is converted from the binary format to an excel text format `animdata.txt` using the **animdata_edit** source which can be found [here](https://d2mods.info/forum/viewtopic.php?t=63878).

The files are extracted using [CascLib](http://www.zezula.net/en/casc/casclib.html).

The program takes two command line arguments. The first is the path to the D2R folder and the second is the directory to extract the files to. The files will be extracted in the same hierarchy they are found in in the CASC format.
