# CHIP-8 Emulator

[![C Language](https://img.shields.io/badge/C-Language-00599C?style=for-the-badge&logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![SDL2](https://img.shields.io/badge/SDL2-Graphics%2FAudio-FF6600?style=for-the-badge&logo=libsdl&logoColor=white)](https://www.libsdl.org/)
[![Emulator](https://img.shields.io/badge/Emulator-CHIP--8-blueviolet?style=for-the-badge)](https://en.wikipedia.org/wiki/CHIP-8)

## Table of Contents
1. [Overview](#overview)
2. [Technical Objectives](#technical-objectives)
3. [Features](#features)
   - [Core Functionality](#core-functionality)
   - [Display System](#display-system)
   - [Audio System](#audio-system)
   - [Input Handling](#input-handling)
4. [Command-Line Interface](#command-line-interface)
5. [Control Scheme](#control-scheme)
6. [Extension Support](#extension-support)
7. [Build Instructions](#build-instructions)
8. [Development Roadmap](#development-roadmap)

## Overview
A complete CHIP-8 emulator implementation written in C, featuring support for both SuperChip and XO-CHIP extensions. The emulator utilizes SDL2 for display rendering and audio output, with configurable graphics, sound, and input handling.

## Technical Objectives
This project was developed to explore several key areas of computer science and systems programming:

- Low-level C Programming: Implementation of memory management, bitwise operations, and pointer arithmetic
- CPU Architecture: Design of opcode decoding, memory mapping, and the fetch-decode-execute cycle
- Real-Time Systems: Integration of timing subsystems, audio synchronization, and frame-accurate display
- Hardware Emulation: Accurate representation of CHIP-8 hardware behavior in software
- Extensible Architecture: Progressive implementation from base CHIP-8 to SuperChip and XO-CHIP extensions

## Features

### Core Functionality
- Complete CHIP-8 instruction set implementation
- SuperChip and XO-CHIP extension support
- 64×32 pixel display with configurable scaling
- 16-key hexadecimal input system
- Audio synthesis with waveform selection
- Adjustable emulation speed

### Display System
- Configurable window scaling (via `--scale-factor` parameter)
- Optional pixel border rendering
- Color interpolation between pixel states
- Support for extended resolution modes (SuperChip)

### Audio System
- Selectable waveform generation (sine or square)
- Runtime volume adjustment
- Default output frequency of 440Hz
- Audio timer synchronization

### Input Handling
- Keyboard-to-keypad mapping system
- Dedicated directional input (arrow keys)
- Runtime emulator control (pause/reset/configuration)

## Command-Line Interface

| Option               | Description                                  | Default Value |
|----------------------|----------------------------------------------|---------------|
| `--scale-factor <n>` | Display scaling multiplier                   | 15            |
| `--sine-wave`        | Enable sine wave audio generation            | Off           |
| `--square-wave`      | Enable square wave audio generation          | On            |
| `--pixel-outline`    | Render pixel borders                         | Off           |
| `--no-pixel-outline` | Disable pixel borders                        | On            |
| `--chip8`            | Standard CHIP-8 mode                         | Default       |
| `--superchip`        | Enable SuperChip extensions                  | Off           |
| `--xochip`           | Enable XO-CHIP extensions                    | Off           |

## Control Scheme

### Emulator Controls

- `ESC`: Terminate emulator  
- `SPACE`: Toggle emulation pause state  
- `=`: Reset emulator state  
- `J/K`: Adjust color interpolation parameters  
- `O/P`: Modify audio output volume  
- `T`: Toggle between sine and square wave audio  
- `Y`: Toggle pixel border rendering  

### CHIP-8 Keypad Mapping

```
Keyboard      CHIP-8
------------------------
1 2 3 4       1 2 3 C
Q W E R       4 5 6 D
A S D F       7 8 9 E
Z X C V       A 0 B F

Arrow Keys:
UP    → 5
DOWN  → 8
LEFT  → 4
RIGHT → 6
```

## Extension Support

### CHIP-8 Compatibility

- VF flag reset during logical operations  
- Source register preservation during shift operations  
- Auto-increment of I register during load/store operations  

### SuperChip Extensions

- High-resolution display mode (128×64)  
- Logical operations preserve VF flag  
- Load/store operations leave I register unchanged  
- Additional drawing and scrolling instructions  

### XO-CHIP Extensions

- Full SuperChip compatibility  
- Additional memory planes  
- Enhanced audio capabilities  
- Extended instruction set  

## Build Instructions

### Prerequisites

- GCC or Clang compiler  
- SDL2 development libraries  
- GNU Make utility  

### Compilation

```bash
make clean && make
```

### Execution

```bash
./chip8 path/to/rom.ch8 [options]
```

## Development Roadmap

1. **Debugging Features**  
   - Register state visualization  
   - Stack trace display  
   - Execution history logging  

2. **State Management**  
   - Save state functionality  
   - Load state functionality  
   - State serialization  

3. **Configuration System**  
   - Persistent configuration files  
   - Per-game configuration profiles  

4. **Input Expansion**  
   - Gamepad/joystick support  
   - Custom key mapping  



