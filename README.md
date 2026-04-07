# PGN Spy

PGN Spy is a desktop tool for analyzing chess game collections in PGN format with a UCI-compatible engine. It helps investigate engine correlation, move quality, and related statistics across one game or a large database.

This repository contains a modernized Windows build of the original project. The current version adds Windows 11 support, a bilingual English/Russian interface, annotated PGN export, and a more reliable analysis workflow.

Russian version: [README.ru.md](README.ru.md)

## What It Does

PGN Spy reads one or more PGN games, converts them into the format expected by the analyser, sends each analyzed position to a UCI engine such as Stockfish, and collects engine evaluations for reporting.

The program is useful for:

- comparing a player's moves against engine choices
- building baselines from strong players or tournament samples
- reviewing difficult positions with engine-based annotations
- exporting analysis in both machine-readable XML and human-readable PGN

PGN Spy is an investigative aid, not proof of cheating by itself. Results should be interpreted together with proper benchmarks, sample size, time controls, and other evidence.

## How It Works

The workflow is:

1. Select a PGN file or a PGN collection.
2. Choose the player to investigate, or leave the field blank to analyze all moves.
3. Set the engine, book depth, threads, hash, and search limits.
4. Start analysis.
5. PGN Spy prepares temporary per-game files, launches `uci-analyser`, and communicates with the selected UCI engine.
6. The analyser returns engine evaluations and summary data.
7. PGN Spy saves the results as XML and, when available, as an annotated PGN with engine comments.

The annotated PGN uses standard comments such as `[%eval ...]` after analyzed moves.

## Requirements

To run the GUI, keep these files together in the application folder:

- `PGN Spy.exe`
- `uci-analyser.exe`
- `pgn-extract.exe`

Any UCI-compatible engine can be used. Stockfish is recommended and tested.

The engine executable can live anywhere on disk because PGN Spy stores the full path in its settings.

## Quick Start

1. Launch `PGN Spy.exe`.
2. Choose the input PGN file.
3. Enter the player name if you want to analyze only one side.
4. Pick a UCI engine and adjust search settings if needed.
5. Start the analysis.
6. Watch the progress window for phase updates, pause/resume/stop controls, and per-game status.
7. When the run completes, open the results window to review the XML summary or export the annotated PGN again.

## Output Files

PGN Spy can produce the following outputs:

- XML results for machine-readable post-processing
- annotated PGN for manual review in chess GUI tools
- temporary PGN files in `%TEMP%` during analysis, which are removed automatically

By default, the final XML and annotated PGN are saved next to the source PGN with timestamped names so previous runs are not overwritten.

## Language

The application supports English and Russian. Use the `Language` selector in the main window, then restart the app to apply the change.

## History

Compared with the original release, this version adds:

- support for modern UCI handshake behavior and Stockfish 18
- Windows 11-ready manifest and DPI-aware UI scaling
- a cleaner modern MFC layout with updated fonts and spacing
- pause, resume, and stop controls during analysis
- automatic saving of XML and annotated PGN results
- a readable results window with saved file paths
- a bilingual English/Russian interface with persisted language selection

The original legacy notes are still available in `ReadMe.txt` for compatibility with existing setup files and project references.

## License

This repository keeps the original MIT-style licensing model. See `LICENSE` for details.

## Credits

Original PGN Spy project by Michael J. Gleason.
The `uci-analyser` component is based on work by David J. Barnes.
