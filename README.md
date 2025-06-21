# CHIP-8 Emulator

## Overview

This project is a complete CHIP-8 emulator written in C, utilizing SDL2 for graphics and audio. It supports the original CHIP-8 instruction set along with SuperChip and XO-CHIP extensions. The emulator provides customizable display settings, audio configurations, and real-time execution control.

## Motivation

Building a CHIP-8 emulator served as a practical project to deepen my understanding of low-level programming, computer architecture, and real-time systems. Key learning outcomes include:

- **Proficiency in C and Low-Level Programming**: Strengthening concepts in memory management, bitwise operations, and direct hardware interaction.
- **Understanding of Computer Architecture**: Implementing an emulator required a deep dive into CPU operation, instruction cycles, memory management, and I/O handling.
- **Real-Time System Design**: Precise timing control for instruction execution, display refresh, and sound generation aligns with embedded system development.
- **Hardware-Software Interface**: Bridging the gap between software and hardware design, gaining experience with how processors interpret and execute instructions.
- **Scalable Learning Path**: The CHIP-8 system is an ideal entry point into emulator development, offering a balance between complexity and accessibility.

## Features

### Core Functionality

- Complete implementation of all standard CHIP-8 instructions
- Support for CHIP-8, SuperChip, and XO-CHIP extensions
- 64x32 pixel display with configurable scaling
- 16-key hexadecimal keypad input
- Sound generation with configurable waveform options
- Color interpolation for smoother display rendering
- Adjustable emulation speed

### Display Settings

- Scalable rendering
- Optional pixel outlines
- Adjustable color interpolation rate

### Audio Configuration

- Selectable sine wave or square wave sound generation
- Adjustable volume control
- Default frequency set to 440Hz

### Input Handling

- Standard CHIP-8 keypad mapped to a keyboard
- Alternative arrow key mappings for movement (UP, DOWN, LEFT, RIGHT)

## Command-Line Arguments

The emulator supports various command-line options for customization:

| Option               | Description                                      |
|----------------------|--------------------------------------------------|
| `--scale-factor <n>` | Set display scaling (default: 15)               |
| `--sine-wave`        | Use sine wave for sound (default)               |
| `--square-wave`      | Use square wave for sound                       |
| `--pixel-outline`    | Enable pixel outlines (default)                 |
| `--no-pixel-outline` | Disable pixel outlines                          |
| `--chip8`            | Use original CHIP-8 behavior (default)          |
| `--superchip`        | Enable SuperChip extensions                     |
| `--xochip`           | Enable XO-CHIP extensions                       |

## Controls

### Emulator Controls

- `ESC` - Quit emulator
- `SPACE` - Pause/resume emulation
- `=` - Reset emulator
- `J/K` - Decrease/increase color interpolation rate
- `O/P` - Decrease/increase volume
- `T` - Toggle sine/square wave sound
- `Y` - Toggle pixel outlines

### CHIP-8 Keypad Mapping

- `UP` → 5
- `DOWN` → 8
- `LEFT` → 4
- `RIGHT` → 6

## Extension Support

### CHIP-8 (Original)

- Implements the original 1970s CHIP-8 behavior
- Logical operations (8XY1, 8XY2, 8XY3) reset VF to 0
- Shift operations (8XY6, 8XYE) copy VY to VX before shifting
- FX55 and FX65 increment I register
- Display behavior requires waiting for draw operations

### SuperChip (SCHIP)

- Enhanced functionality with additional instructions
- Logical operations do not modify VF
- Shift operations directly modify VX
- FX55 and FX65 do not increment I register

### XO-CHIP

- Introduces modern CHIP-8 extensions
- Mostly follows SuperChip behavior with added capabilities

## Building from Source

### Prerequisites

Ensure the following dependencies are installed:

- C compiler (GCC or Clang)
- SDL2 library
- `make` (optional but recommended)

### Compilation

To compile the emulator, run:

```bash
make

./chip8 path/to/rom.ch8 [CLI options]
