# Windows 11 Build Notes

PGN Spy now targets `PlatformToolset=v143` and `WindowsTargetPlatformVersion=10.0.22621.0` by default.

## Required Visual Studio components

- MSVC v143 build tools
- Windows 11 SDK `10.0.22621.0`
- MFC libraries for the selected architecture

Without MFC, `PGN Spy.vcxproj` fails with `MSB8041`.

## Build commands

Use the Visual Studio 2022 developer environment or call `vcvars32.bat` first.

Build `uci-analyser`:

```powershell
msbuild 'uci-analyser\uci-analyser.vcxproj' /t:Build /p:Configuration=Release;Platform=Win32
```

Build the GUI:

```powershell
msbuild 'PGN Spy\PGN Spy.vcxproj' /t:Build /p:Configuration=Release;Platform=Win32
```

Build the full solution:

```powershell
msbuild 'PGN Spy.sln' /t:Build /p:Configuration=Release;Platform=Win32
```

## Runtime files

To run the GUI successfully, keep these files together in the application folder:

- `PGN Spy.exe`
- `uci-analyser.exe`
- `pgn-extract.exe`

`stockfish.exe` can live anywhere, because the GUI stores the absolute engine path in its settings.

## Verified in this repository

- `uci-analyser` builds successfully with VS 2022 Build Tools after retargeting.
- `uci-analyser` runs correctly against Stockfish 18.
- The GUI project still requires local MFC installation before it can be built.
