# UhohBS

UhohBS is an OBS Studio plugin that provides a "dump button" for delayed live streams.

It is designed to mimic broadcast profanity-delay workflows:

- `Cut`: Immediately discard currently buffered delayed content before it reaches viewers.
- `Fill`: Temporarily switch output to safe content long enough to rebuild the delay buffer.

## What It Does

UhohBS adds a dockable control panel in OBS and a hotkey action to trigger dump operations.

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

## Safety Notes

- This plugin depends on OBS stream delay being enabled when using `Stream Delay` pipeline mode.
- `Cut` for live stream delay briefly interrupts and restarts the stream output.
- `Fill` for live stream delay intentionally replaces content for the selected duration.
- Test your setup privately before using it in production.

## Build (Linux)

This project uses the OBS plugin template CMake layout.

1. Configure:

```bash
cmake --preset linux-x86_64
```

2. Build:

```bash
cmake --build --preset linux-x86_64
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
