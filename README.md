# uhohbs

uhohbs is an OBS plugin that provides a "dump button" for delayed live streams, to save yourself from "uh-oh" moments that happen.

It is designed to mimic broadcast profanity-delay workflows:

- `Cut`: Immediately discard currently buffered delayed content before it reaches viewers.
- `Fill`: Temporarily switch output to safe content long enough to rebuild the delay buffer.

## What It Does

uhohbs adds a dockable control panel in OBS and a hotkey action to trigger dump operations.

For stream-delay pipelines:

- `Cut` force-stops and restarts streaming to drop queued delayed frames.
- `Fill` switches to a configured safe scene/source/color for the configured delay duration, then returns to the original scene.

For replay buffer pipelines (current behavior):

- `Cut` and `Fill` both restart replay buffer recording to clear buffered content.

## Dock Controls

- `Delay to Dump (seconds)`: Target delay window used by fill.
- `Dump Mode`: `Cut` or `Fill`.
- `Fill Type`: `Solid Color`, `Source`, or `Scene`.
- `Fill Source/Scene Name`: Name of an existing OBS source/scene for non-color fill.
- `Fill Color (hex)`: Hex color for solid-color fill (example: `#ff0000`).
- `Pipeline Target`: `Stream Delay` or `Replay Buffer`.

## How To Use In OBS

1. Open the dock: `View` → `Docks` → `uhohbs Control`.
2. If that option is not in the dock, check Tools for `Show uhohbs Control Dock`
2. Select `Pipeline Target`:
	- `Stream Delay`: operates on the live stream delay buffer.
	- `Replay Buffer`: restarts the replay buffer to clear it.
3. Select `Dump Mode`:
	- `Cut`: drops buffered delayed content by force-restarting stream output.
	- `Fill`: shows safe content for the configured delay duration, then returns.
4. If using `Fill`, configure:
	- `Fill Type` (`Solid Color`, `Source`, or `Scene`).
	- `Fill Source/Scene Name` (exact OBS name for source/scene fill).
	- `Fill Color (hex)` in `#RRGGBB` format for solid color fill.
5. Set `Delay to Dump (seconds)` to the delay window you want to rebuild.
6. Trigger a dump by clicking `Dump` in the dock or assigning/using the hotkey
	`uhohbs: Trigger Dump` in `Settings` → `Hotkeys`.

### Required OBS Setting

- For `Stream Delay`, you must enable OBS stream delay.

## Safety Notes

- This plugin depends on OBS stream delay being enabled when using `Stream Delay` pipeline mode.
- `Cut` for live stream delay briefly interrupts and restarts the stream output.
- `Fill` for live stream delay intentionally replaces content for the selected duration.
- Test your setup privately before using it in production.

## Build (Linux)

This project uses the OBS plugin template CMake layout.

1. Configure:

```bash
cmake --preset ubuntu-x86_64
```

2. Build:

```bash
cmake --build --preset ubuntu-x86_64
```

## Build (Windows)

This project uses the OBS plugin template CMake layout.

1. Configure:

```powershell
cmake --preset windows-x64
```

2. Build:

```powershell
cmake --build --preset windows-x64
```

## Build (macOS)

This project uses the OBS plugin template CMake layout.

1. Configure:

```bash
cmake --preset macos
```

2. Build:

```bash
cmake --build --preset macos
```

If your environment uses a different preset, choose the matching value from `CMakePresets.json`.

## Install

Copy the built plugin artifacts into your OBS plugin directory for your platform, then restart OBS.

## Development Status

- Core dock UI, hotkey integration, and dump coordinator are implemented.
- Stream delay and replay buffer paths are both present.
- Project metadata in `buildspec.json` still contains template placeholder values and should be updated before release.

## License

GPL v2 or later. See [LICENSE](LICENSE).
