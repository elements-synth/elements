---
layout: default
title: Installation — Elements
---

# Installation

## Requirements

- macOS 12 (Monterey) or later
- Apple Silicon or Intel Mac
- A VST3 or AU compatible host (Ableton Live, Logic Pro, Reaper, Bitwig, etc.)

---

## Download

Download the latest release from the [Elements releases page](https://github.com/elements-synth/elements/releases).

The download is a `.zip` file containing both plugin formats:

- `Elements.vst3` — for most DAWs
- `Elements.component` — AU format, for Logic Pro and GarageBand

---

## Installation

Unzip the downloaded file and copy each component to its standard macOS location:

**VST3**
```
/Library/Audio/Plug-Ins/VST3/Elements.vst3
```

**AU**
```
/Library/Audio/Plug-Ins/Components/Elements.component
```

You can navigate to these folders in Finder by opening a new Finder window, pressing `Cmd + Shift + G`, and pasting the path.

---

## Gatekeeper — Important

Because Elements is not notarized with an Apple Developer certificate, macOS will block it on first launch. This is expected and safe to bypass.

**To allow Elements:**

1. Open **System Settings → Privacy & Security**
2. Scroll down to the Security section
3. You will see a message saying *"Elements was blocked because it is not from an identified developer"*
4. Click **Allow Anyway**
5. Reopen your DAW and rescan plugins if needed

You only need to do this once per plugin format. If you installed both VST3 and AU, you may need to repeat the process for each.

---

## Verifying the installation

After allowing Elements through Gatekeeper, open your DAW and scan for new plugins. Elements should appear as **Elements** in your plugin list.

If your DAW does not find the plugin after scanning, verify that the files are in the correct paths listed above and that Gatekeeper has been cleared for both formats.

---

## Hosts tested in Beta 1.0

| Host | VST3 | AU |
|------|------|----|
| Ableton Live 11+ | ✓ | — |
| Bitwig Studio | ✓ | — |

*If you test Elements in a host not listed here, please [open an issue](https://github.com/elements-synth/elements/issues) and let us know.*

---

*Windows support is planned for v1.1.*
