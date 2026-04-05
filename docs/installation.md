---
layout: default
title: Installation — Elements
---

[← Back to Elements](index)

# Installation

## Requirements

**macOS**
- macOS 12 (Monterey) or later
- Apple Silicon or Intel Mac (Universal Binary)
- A VST3 compatible host (Ableton Live, Bitwig, Reaper, FL Studio, etc.)

**Windows**
- Windows 10/11 x64
- GPU with OpenGL 3.2+ support
- [Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) (required, one-time install)
- A VST3 compatible host

---

## Download

Download the latest release from the [Elements releases page](https://github.com/elements-synth/elements/releases).

- `Elements-VST3-macOS-Universal.zip` — macOS (Apple Silicon + Intel)
- `Elements-VST3-Windows-x64.zip` — Windows 64-bit

---

## Installation (macOS)

Unzip the downloaded file and copy the plugin to its standard macOS location:

**VST3**
```
/Library/Audio/Plug-Ins/VST3/Elements.vst3
```

You can navigate to this folder in Finder by opening a new Finder window, pressing `Cmd + Shift + G`, and pasting the path.

### Gatekeeper — Important

Because Elements is not notarized with an Apple Developer certificate, macOS will block it on first launch. This is expected and safe to bypass.

**To allow Elements:**

1. Open **System Settings → Privacy & Security**
2. Scroll down to the Security section
3. You will see a message saying *"Elements was blocked because it is not from an identified developer"*
4. Click **Allow Anyway**
5. Reopen your DAW and rescan plugins if needed

You only need to do this once.

---

## Installation (Windows)

1. Install the [Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) if you don't have it already
2. Unzip the downloaded file and copy the `Elements.vst3` folder to:

```
C:\Program Files\Common Files\VST3\
```

3. Rescan plugins in your DAW

> **Note:** Elements is developed primarily for macOS. The Windows build may contain bugs or visual differences not present in the Mac version. If you encounter any issues, please [open an issue](https://github.com/elements-synth/elements/issues).

---

## Verifying the installation

Open your DAW and scan for new plugins. Elements should appear as **Elements** in your plugin list.

If your DAW does not find the plugin after scanning, verify that the files are in the correct paths listed above. On macOS, make sure Gatekeeper has been cleared.

---

## Hosts tested in Beta v0.9.3

| Host | macOS VST3 | Windows VST3 |
|------|------------|--------------|
| Ableton Live 11+ | ✓ | ✓ |
| Bitwig Studio | ✓ | — |
| Reaper | — | ✓ |

*If you test Elements in a host not listed here, please [open an issue](https://github.com/elements-synth/elements/issues) and let us know.*

---

[← Back to Elements](index)
