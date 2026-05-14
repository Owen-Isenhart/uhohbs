# uhohbs

uhohbs is an OBS plugin that provides a "dump button" for delayed live streams. It saves you from those accidental "uh-oh" moments that inevitably happen while live.

It perfectly mimics professional broadcast profanity-delay workflows by immediately discarding your currently buffered delayed stream content before it ever reaches your viewers.

## See it in Action

Here is what it looks like for the streamer when an "uh-oh" moment occurs and they press the dump button:

<video src="videos/streamer_view.mp4" controls="controls" width="100%"></video>

And here is what that exact same moment looks like to the viewers on Twitch:

<video src="videos/twitch_view.mp4" controls="controls" width="100%"></video>

## What It Does

uhohbs adds a dockable control panel directly into OBS along with a customizable hotkey action to trigger a dump.

When triggered, it force-stops and instantly restarts your streaming output to seamlessly drop any queued frames sitting in your stream delay buffer.

## How To Use In OBS

1. Go to **Settings** → **Advanced** in OBS.
2. In the **Stream Delay** section, turn on **Enable** and set your desired **Duration** (e.g., 10s).
3. Open the plugin dock: **View** → **Docks** → **uhohbs Control**. (If it's missing, check **Tools** → **Show uhohbs Control Dock**).
4. Configure the hotkey (optional): Go to **Settings** → **Hotkeys** and assign a key to `uhohbs: Trigger Dump`.
5. When an "uh-oh" moment happens on stream, simply press the **Dump** button in the dock (or your assigned hotkey) to immediately cut the delayed frames.
6. **(Optional)** If you check the **Disable delay after dump (One-time use)** option in the dock, dumping will force the stream to restart instantly without rebuilding the delay buffer. This ensures a much faster recovery time for viewers, but restricts the dump button to a single use for the remainder of that stream. Your original delay settings will be automatically restored for your next broadcast.

## Safety Notes

- This plugin depends on OBS stream delay being enabled.
- `Cut` for live stream delay briefly interrupts and restarts the stream output.
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
- Stream delay path is present.
- Project metadata in `buildspec.json` still contains template placeholder values and should be updated before release.

## License

GPL v2 or later. See [LICENSE](LICENSE).
