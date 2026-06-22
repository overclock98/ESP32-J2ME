# Game jar goes here

This project runs the **Gameloft *Diamond Rush*** J2ME game, but the game is **not**
redistributed here — it is Gameloft's intellectual property.

Place your own legally-obtained copy in this folder:

```
game/DiamondRush_s40v5.jar
```

The build expects exactly that filename. To use a different path/name:

```bash
GAME_JAR=/full/path/to/your.jar ./build.sh
```

The build reads the jar's `META-INF/MANIFEST.MF` (the entry MIDlet is `GloftDIRU`), compiles
its classes into `files/FlintApp.jar`, and extracts its non-class resources (PNG/MIDI/etc.)
into `files/res/`, which become the on-device `/res` filesystem.

> Other MIDP-2.0 / CLDC-1.0 builds may work too, but the launcher
> (`apps/GameRun/src/Main.java`) instantiates `GloftDIRU` specifically; a different MIDlet
> class name would need that line changed.
