# LearnR

LearnR is a Geometry Dash practice mod for building consistent runs with guided start position routing, smart state syncing, and repeatable section training.

## Downloads

| Mod Version | Geode Version | GD Version | Downloads |
| --- | --- | --- | --- |
| [v1.0.3](https://github.com/Dashhhhhhhh/LearnR/releases/tag/v1.0.3) | 5.7.1 | 2.2081 | [Latest release](https://github.com/Dashhhhhhhh/LearnR/releases/latest) |

## Installation

LearnR is a [Geode](https://geode-sdk.org/) mod, so Geometry Dash needs Geode installed first. After that, install LearnR from the in-game Geode mods page when available, or install a packaged `.geode` release manually.

Once installed, open a level and pause to access LearnR.

## Features

- Switch between start positions with keybinds or on-screen arrow controls.
- Use Guided Mode to train individual sections before expanding into longer connected runs.
- Create Smart Startpos attempts that copy nearby portal state, including speed, mode, mini, and dual settings.
- Keep guided mode, smart mode, display options, cutoff, and attempt limits saved per level.
- Preserve music sync so startpos attempts line up with the correct song timing.
- Track current phase, route, stage progress, and guided run counts from the pause menu.
- Toggle optional practice overlays for startpos controls and guided zero-run chance.

## Building

LearnR builds like a standard Geode C++ mod. Install the Geode CLI, set `GEODE_SDK` to your local Geode SDK path, then run:

```sh
geode build
```

The project targets Geometry Dash 2.2081 through Geode 5.7.1 and uses C++23.

## Issues

Bug reports and feature requests can be opened on [GitHub Issues](https://github.com/Dashhhhhhhh/LearnR/issues).
