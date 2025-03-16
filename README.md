# CHIP-8 Emulator

A complete CHIP-8 emulator written in C with SDL2, supporting original CHIP-8, SuperChip, and XO-CHIP extensions.

## Overview

This CHIP-8 emulator provides an accurate implementation of the CHIP-8 virtual machine, a simple interpreted programming language developed in the 1970s. The emulator features customizable display options, audio settings, and support for various CHIP-8 extensions.

## Why an Emulator?

I chose to build a CHIP-8 emulator as my project for several key reasons:

- **Learning C and Low-Level Programming**: As I intend to pursue a career in the hardware industry, developing strong skills in C and understanding low-level programming concepts was essential. This project provided hands-on experience with memory management, bitwise operations, and direct hardware interaction.

- **Understanding Computer Architecture**: Building an emulator requires thoroughly understanding CPU operation, instruction execution cycles, memory management, and I/O handling - all fundamental concepts in computer architecture.

- **Real-Time Systems**: The emulator operates as a real-time system with precise timing requirements for instruction execution, display updates, and sound generation - skills directly applicable to embedded systems development.

- **Hardware/Software Interface**: This project stands at the intersection of hardware and software, providing practical experience with concepts often encountered in hardware development.

- **Practical Learning Path**: CHIP-8's simplicity made it an ideal starting point for emulation, offering the perfect balance of challenge and achievability while teaching core concepts applicable to more complex systems.

## Features

### Core Features

- Full implementation of all standard CHIP-8 instructions
- Support for CHIP-8, SuperChip, and XO-CHIP extensions
- 64x32 pixel display with configurable scaling
- 16-key hexadecimal keypad input
- Sound generation with configurable wave type
- Color interpolation for smoother display changes
- Configurable emulation speed

### Display Options

- Adjustable scaling factor
- Optional pixel outlines
- Color interpolation with adjustable rate

### Audio Options

- Sine wave or square wave sound generation
- Adjustable volume
- Standard 440Hz frequency

### Input Controls

- Standard CHIP-8 keypad mapped to keyboard:
  - Arrow key alternatives for movement (UP, DOWN, LEFT, RIGHT)

### Command Line Arguments

#### Options

- `--scale-factor <number>`: Set display scaling (default: 15)
- `--sine-wave`: Use sine wave for sound (default)
- `--square-wave`: Use square wave for sound
- `--pixel-outline`: Enable pixel outlines (default)
- `--no-pixel-outline`: Disable pixel outlines
- `--chip8`: Use original CHIP-8 behavior (default)
- `--superchip`: Use SuperChip extensions
- `--xochip`: Use XO-CHIP extensions

### Keyboard Controls

#### Emulator Controls

- `ESC`: Quit the emulator
- `SPACE`: Pause/resume emulation
- `=` (equals): Reset emulator
- `J/K`: Decrease/increase color lerp rate
- `O/P`: Decrease/increase volume
- `T`: Toggle between sine and square wave sound
- `Y`: Toggle pixel outlines

#### CHIP-8 Keypad

Arrow keys are mapped as follows:

- `UP`: 5
- `DOWN`: 8
- `LEFT`: 4
- `RIGHT`: 6

### Extension Support

The emulator supports different CHIP-8 variants with their specific behaviors:

#### CHIP-8 (Original)

- Original CHIP-8 behavior from the 1970s
- Operations like 8XY1, 8XY2, and 8XY3 reset VF to 0
- Shift operations 8XY6 and 8XYE first copy VY to VX, then shift
- FX55 and FX65 increment I
- Display wait behavior for drawing sprites

#### SuperChip (SCHIP)

- SuperChip extensions
- Logical operations do not affect VF
- Shift operations work directly on VX
- FX55 and FX65 do not increment I

#### XO-CHIP

- Modern XO-CHIP extensions
- Similar behavior to SuperChip for most operations

## Building from Source

### Prerequisites

- C compiler (gcc or clang)
- SDL2 library
- Make (optional but recommended)

### Compilation

```bash
make