# CHIP-8 Emulator

![C](https://img.shields.io/badge/C-Language-00599C?style=for-the-badge&logo=c&logoColor=white)
![SDL2](https://img.shields.io/badge/SDL2-Graphics%2FAudio-FF6600?style=for-the-badge&logo=libsdl&logoColor=white)
![Emulator](https://img.shields.io/badge/Emulator-CHIP--8-blueviolet?style=for-the-badge)

## Overview

A fully functional **CHIP-8 emulator written in C**, supporting **SuperChip** and **XO-CHIP** extensions.  
Uses **SDL2** for display and audio, with customizable graphics, sound, and control options.

---

## Motivation

Building this emulator served as a deep dive into systems-level programming, architecture, and real-time design:

- 📦 **C Mastery**: Memory handling, bitwise ops, and pointer logic in a low-level environment
- 🧠 **CPU Design Insights**: Opcode decoding, memory mapping, and fetch-decode-execute loop
- ⏱️ **Real-Time Systems**: Timers, audio sync, and frame-accurate refresh
- 🔌 **HW-SW Interface**: Bridging emulator behavior with real hardware expectations
- 🚀 **Scalable Complexity**: CHIP-8 → SuperChip → XO-CHIP extensions

---

## Features

### ✅ Core Functionality

- Full CHIP-8 instruction set
- SuperChip + XO-CHIP extensions
- 64×32 pixel display (scalable)
- 16-key hex keypad
- Sound synthesis with sine/square wave options
- Adjustable emulation speed

### 🎨 Display

- Scalable window size (`--scale-factor`)
- Toggleable pixel outlines
- Smooth color interpolation

### 🔊 Audio

- Sine or square wave generation (`--sine-wave`, `--square-wave`)
- Volume control via keyboard
- Default frequency: 440 Hz

### ⌨️ Input

- CHIP-8 keypad mapped to keyboard
- Arrow keys act as 4-directional input
- Real-time emulator control (pause/reset/tune)

---

## 🧪 Command-Line Options

| Option               | Description                                |
|----------------------|--------------------------------------------|
| `--scale-factor <n>` | Set display scaling (default: 15)          |
| `--sine-wave`        | Use sine wave for audio output             |
| `--square-wave`      | Use square wave for audio output           |
| `--pixel-outline`    | Enable pixel borders                       |
| `--no-pixel-outline` | Disable pixel borders                      |
| `--chip8`            | Use original CHIP-8 behavior (default)     |
| `--superchip`        | Enable SuperChip features                  |
| `--xochip`           | Enable XO-CHIP extensions                  |

---

## 🎮 Controls

### Emulator

- `ESC` — Quit
- `SPACE` — Pause / Resume
- `=` — Reset
- `J/K` — Adjust color interpolation
- `O/P` — Adjust volume
- `T` — Toggle sine/square wave
- `Y` — Toggle pixel outlines

### Keypad Mapping (CHIP-8)

- `UP` → 5
- `DOWN` → 8
- `LEFT` → 4
- `RIGHT` → 6

---

## 🧩 Extension Support

### ✅ CHIP-8
- VF reset on logic ops
- VY → VX before shifts
- FX55/FX65 increment `I`

### ✅ SuperChip
- Extended resolution
- Logic ops don’t touch VF
- FX55/FX65 don’t increment `I`

### ✅ XO-CHIP
- SuperChip-compatible
- Adds extended behaviors (multi-plane, audio, etc.)

---

## 🛠️ Building from Source

### Requirements

- GCC or Clang
- SDL2 development libraries
- `make` utility

### Compilation

```bash
make
```

---

## 🚧 TODO / Future Plans

- Debug overlay (registers, stack)
- Save/load emulator state
- Config file support
- Gamepad input support

---

## License

MIT License — free for academic and commercial use.
