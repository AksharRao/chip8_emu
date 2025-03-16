#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "SDL.h"

// Emulator states
typedef enum {
    QUIT,
    RUNNING,
    PAUSED,
} emul_state_t;

// CHIP-8 extensions/quirks support
typedef enum {
    CHIP8,      // Original CHIP-8 behavior
    SUPERCHIP,  // Super CHIP-8 (SCHIP) extensions
    XOCHIP,     // XO-CHIP extensions
} extension_t;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_AudioSpec want, have;
    SDL_AudioDeviceID dev;
} sdl_t;

typedef struct {
    uint32_t window_width;  // SDL Window Dimensions
    uint32_t window_height;
    uint32_t fg_color;      // Foreground color (RGB and A)
    uint32_t bg_color;      // Background color (RGB and A)
    uint32_t scale_factor;  // Scaling factor for window size
    uint16_t PC;
    bool pixel_outlines;
    uint32_t instr_per_sec;         // Number of instructions to be executed per second
    uint32_t square_wave_freq;      // Frequency of the square wave
    uint32_t audio_sample_rate;     // Audio sample rate
    int16_t volume ;                // Volume of the sound
    float color_lerp_rate;          // Rate of interpolation between 0.0 and 1.0, inclusive
    bool use_sine_wave;             // Flag to choose between square and sine wave
    extension_t current_extension;  // Current quirks/extension support
} config_t;

//chip 8 instruction format
typedef struct {
    uint16_t opcode;    // every opcode is 2 bytes in length
    uint16_t nnn;       // 12 bit address/constant (lower 12 bits)
    uint8_t nn;         // 8 bit constant
    uint8_t n;          //4 bit nibble (lower bits)
    uint8_t x;
    uint8_t y;          //4 bit register identifiers
}instruction_t;

// Chip8 Machine object
typedef struct {
    emul_state_t state;
    uint8_t ram[4096]; 
    bool display[64*32];
    uint32_t pixel_color[64*32];    //color to lerp (?)
    uint16_t stack[16];
    uint16_t *stack_ptr;
    uint8_t V[16];                  //the register file, V0 --> VF
    uint16_t I;                     //The register to store memory address
    uint16_t PC;
    uint8_t delay_timer; 
    uint8_t sound_timer;
    bool keypad[16];
    const char *rom_name;
    instruction_t inst;             //currently executing instruction for debugging purposes
    bool draw;                      //flag to indicate if the screen needs to be redrawn
} chip8_t;

//Lerp function to interpolate between two colors, as in to smoothly transition between two colors
uint32_t color_lerp(const uint32_t start_color, const uint32_t end_color, const float t)
{
    const uint8_t start_r = (start_color >> 24) & 0xFF;
    const uint8_t start_g = (start_color >> 16) & 0xFF;
    const uint8_t start_b = (start_color >> 8) & 0xFF;
    const uint8_t start_a = (start_color >> 0) & 0xFF;

    const uint8_t end_r = (end_color >> 24) & 0xFF;
    const uint8_t end_g = (end_color >> 16) & 0xFF;
    const uint8_t end_b = (end_color >> 8) & 0xFF;
    const uint8_t end_a = (end_color >> 0) & 0xFF;

    const u_int8_t ret_r = ((1 - t)*start_r) + (t*end_r);
    const u_int8_t ret_g = ((1 - t)*start_g) + (t*end_g);
    const u_int8_t ret_b = ((1 - t)*start_b) + (t*end_b);
    const u_int8_t ret_a = ((1 - t)*start_a) + (t*end_a);

    return (ret_r << 24) | (ret_g << 16) | (ret_b << 8) | ret_a;
}

//SDL Audio callback function
void audio_callback(void *userdata, uint8_t *stream, int len) 
{
    config_t *config = (config_t *)userdata;
    int16_t *audio_data = (int16_t *)stream;
    // Phase accumulator for wave generation
    static double phase = 0.0; 

    // Calculate the phase increment per sample
    double phase_increment = (2.0 * M_PI * config->square_wave_freq) / config->audio_sample_rate;

    // Fill the audio buffer with appropriate wave samples
    for (int i = 0; i < len / 2; i++) {
        if (config->use_sine_wave) 
        {
    // Generate sine wave (your existing code)
            audio_data[i] = (int16_t)(config->volume * sin(phase));
        } 
        else 
        {
            // Generate square wave
            // Simply check if we're in the first or second half of the wave cycle
            audio_data[i] = (phase < M_PI) ? config->volume : -config->volume;
        }

        // Increment the phase
        phase += phase_increment;

        // Wrap the phase to avoid overflow
        if (phase >= 2.0 * M_PI) {
            phase -= 2.0 * M_PI;
        }
    }
}

// Initialize SDL
bool init_sdl(sdl_t *sdl, config_t *config) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        SDL_Log("Could not initialize SDL Subsystems! %s", SDL_GetError());
        return false;
    }

    sdl->window = SDL_CreateWindow(
        "Chip 8 Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config->window_width * config->scale_factor,
        config->window_height * config->scale_factor,
        0);
    if (!sdl->window) {
        SDL_Log("Could not create SDL Window: %s\n", SDL_GetError());
        return false;
    }

    sdl->renderer = SDL_CreateRenderer(sdl->window, -1, SDL_RENDERER_ACCELERATED);
    if (!sdl->renderer) {
        SDL_Log("Could not create SDL Renderer: %s\n", SDL_GetError());
        return false;
    }

    sdl->want = (SDL_AudioSpec){
        .freq = 44100,
        .format = AUDIO_S16LSB, // Signed 16-bit samples in little-endian byte order
        .channels = 1, // Mono
        .samples = 512, // Buffer size
        .callback = audio_callback, // Function to call when audio device needs data
        .userdata = config,
    };
     sdl -> dev = SDL_OpenAudioDevice(NULL, 0, &sdl->want, &sdl->have, 0);

    if(sdl->dev==0) {
        SDL_Log("Failed to open audio: %s\n", SDL_GetError());
        return false;
    }

    if((sdl->want.format != sdl->have.format) || (sdl->want.channels != sdl->have.channels))
    {
        SDL_Log("Failed to get the desired AudioSpec\n");
        return false;
    }

    return true;  // Success
}

// Set up initial emulator configurations from passed-in arguments
bool set_config_from_args(config_t *config, const int argc, char **argv) {
    // Set default configuration
    *config = (config_t){
        .window_height = 32,
        .window_width = 64,
        .fg_color = 0xFFFFFFFF,  //0x39FF14FF neon green , for yellow 0xf8d200ff
        .bg_color = 0X000000FF,  // Yellow with full opacity (ARGB format) and yellowish shir 0x9e6f05ff
        .scale_factor = 15,      // Default resolution 64*10 x 32*10
        .pixel_outlines = true,  // by default, the outlines are drawn
        .instr_per_sec = 700,
        .square_wave_freq = 440, // Frequency of the square wave
        .audio_sample_rate = 44100, // Audio sample rate
        .volume = 3000,           // Volume of the sound 
        .color_lerp_rate = 0.7,        // Rate of interpolation between 0.0 and 1.0, inclusive, default is 0.7
        .use_sine_wave = true,  //By default, use sine wave
        .current_extension = CHIP8, // Default extension is CHIP8
    };
    
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--scale-factor", strlen("--scale-factor")) == 0) {
            i++;
            config->scale_factor = (uint32_t)strtol(argv[i], NULL, 10);
        } else if (strncmp(argv[i], "--sine-wave", strlen("--sine-wave")) == 0) {
            config->use_sine_wave = true;
            printf("Using sine wave sound\n");
        } else if (strncmp(argv[i], "--square-wave", strlen("--square-wave")) == 0) {
            config->use_sine_wave = false;
            printf("Using square wave sound\n");
        }
        else if (strncmp(argv[i], "--pixel-outline", strlen("--pixel-outline")) == 0) {
            // Toggle pixel outlines
            config->pixel_outlines = true;
            printf("Using pixel outlines\n");
        }
        else if (strncmp(argv[i], "--no-pixel-outline", strlen("--no-pixel-outline")) == 0) {
            config->pixel_outlines = false;
            printf("Disabling pixel outlines\n");
        }
         // Add extension selection options
        else if (strncmp(argv[i], "--chip8", strlen("--chip8")) == 0) {
            config->current_extension = CHIP8;
            printf("Using original CHIP-8 behavior\n");
        } else if (strncmp(argv[i], "--superchip", strlen("--superchip")) == 0) {
            config->current_extension = SUPERCHIP;
            printf("Using Super CHIP-8 (SCHIP) extensions\n");
        } else if (strncmp(argv[i], "--xochip", strlen("--xochip")) == 0) {
            config->current_extension = XOCHIP;
            printf("Using XO-CHIP extensions\n");
        }
    }
    return true;
}

//initialise chip8
bool init_chip8(chip8_t *chip8, const config_t config, const char rom_name[]) {
    const uint32_t entry_point = 0x200; // CHIP8 Roms will be loaded to 0x200
    const unsigned char font[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0,   // 0   
        0x20, 0x60, 0x20, 0x20, 0x70,   // 1  
        0xF0, 0x10, 0xF0, 0x80, 0xF0,   // 2 
        0xF0, 0x10, 0xF0, 0x10, 0xF0,   // 3
        0x90, 0x90, 0xF0, 0x10, 0x10,   // 4    
        0xF0, 0x80, 0xF0, 0x10, 0xF0,   // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0,   // 6
        0xF0, 0x10, 0x20, 0x40, 0x40,   // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0,   // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0,   // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90,   // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0,   // B
        0xF0, 0x80, 0x80, 0x80, 0xF0,   // C
        0xE0, 0x90, 0x90, 0x90, 0xE0,   // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0,   // E
        0xF0, 0x80, 0xF0, 0x80, 0x80,   // F
    };
    //insiitalise entire chip8
    memset(chip8, 0, sizeof(chip8_t));

    //load font
    memcpy(&chip8->ram[0], font, sizeof(font));
    //SDL_Log("Size of font: %zu\n", sizeof(font));
    //SDL_Log("Size of font: %zx", sizeof(font));
    //load rom
    FILE * rom = fopen(rom_name, "rb");
    if(!rom)
    {
        SDL_Log("Rom file %s is invalid, or does not exist\n", rom_name);
        return false;
    }

    fseek(rom, 0, SEEK_END); //set the pointer to an offset specified in the file
    const size_t rom_size = ftell(rom); //to assess the size of the rom to be loaded
    const size_t max_size = sizeof chip8 ->ram - entry_point;
    rewind(rom); //will set the pointer back to the start of the file

    if (rom_size > max_size) {
        SDL_Log("Rom file %s is too big! Rom size: %llu, Max size allowed: %llu\n", 
                rom_name, (long long unsigned)rom_size, (long long unsigned)max_size);
        return false;
    }

    if (fread(&chip8->ram[entry_point], rom_size, 1, rom)!=1){
        SDL_Log("Rom file %s cannot be loaded onto CHIP8 memory!\n", rom_name);
        return false;
    }
    fclose(rom);

    // Default machine state on running
    chip8->state = RUNNING;  
    chip8->PC = entry_point;
    chip8->rom_name = rom_name;
    chip8->stack_ptr = &chip8->stack[0]; //SP points to the start of the stack
    memset(&chip8->pixel_color[0], config.bg_color, sizeof(chip8->pixel_color)); //initialising pixels to background color
    
    return true;
}

// Final cleanup
void final_cleanup(const sdl_t sdl) {
    SDL_DestroyRenderer(sdl.renderer);
    SDL_DestroyWindow(sdl.window);
    SDL_CloseAudioDevice(sdl.dev);
    SDL_Quit();  // Shuts down SDL subsystems
}

// Clear screen / SDL window to background color
void clear_screen(const sdl_t sdl, const config_t config) {
    // Extract color components (ARGB format)
    const uint8_t r = (config.bg_color >> 24) & 0xFF;  // Alpha component
    const uint8_t g = (config.bg_color >> 16) & 0xFF;  // Red component
    const uint8_t b = (config.bg_color >> 8) & 0xFF;   // Green component
    const uint8_t a = (config.bg_color >> 0) & 0xFF;   // Blue component

    // Debug print to verify color values
    //printf("Setting background color: R=%u, G=%u, B=%u, A=%u\n", r, g, b, a);

SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
    SDL_RenderClear(sdl.renderer);
}

// Update window with any changes
void update_screen(const sdl_t sdl, const config_t config, chip8_t *chip8) {
    SDL_Rect rect = {.x = 0, .y = 0, .w = config.scale_factor, .h = config.scale_factor};

    // Grab bg color values to draw outlines
    const uint8_t bg_r = (config.bg_color >> 24) & 0xFF;
    const uint8_t bg_g = (config.bg_color >> 16) & 0xFF;
    const uint8_t bg_b = (config.bg_color >>  8) & 0xFF;
    const uint8_t bg_a = (config.bg_color >>  0) & 0xFF;

    // Loop through display pixels, draw a rectangle per pixel to the SDL window
    for (uint32_t i = 0; i < sizeof chip8->display; i++) {
        // Translate 1D index i value to 2D X/Y coordinates
        // X = i % window width
        // Y = i / window width
        rect.x = (i % config.window_width) * config.scale_factor;
        rect.y = (i / config.window_width) * config.scale_factor;

        if (chip8->display[i]) {
            // Pixel is on, draw foreground color
            if (chip8->pixel_color[i] != config.fg_color) {
                // Lerp towards fg_color
                chip8->pixel_color[i] = color_lerp(chip8->pixel_color[i], 
                                               config.fg_color, 
                                               config.color_lerp_rate);
            }

            const uint8_t r = (chip8->pixel_color[i] >> 24) & 0xFF;
            const uint8_t g = (chip8->pixel_color[i] >> 16) & 0xFF;
            const uint8_t b = (chip8->pixel_color[i] >>  8) & 0xFF;
            const uint8_t a = (chip8->pixel_color[i] >>  0) & 0xFF;

            SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
            SDL_RenderFillRect(sdl.renderer, &rect);
        
            // Only draw outlines if pixel_outlines is enabled
            if (config.pixel_outlines) {
                // If user requested drawing pixel outlines, draw those here
                SDL_SetRenderDrawColor(sdl.renderer, bg_r, bg_g, bg_b, bg_a);
                SDL_RenderDrawRect(sdl.renderer, &rect);
            }
        } else {
            // Pixel is off, draw background color
            if (chip8->pixel_color[i] != config.bg_color) {
                // Lerp towards bg_color
                chip8->pixel_color[i] = color_lerp(chip8->pixel_color[i], 
                                               config.bg_color, 
                                               config.color_lerp_rate);
            }

            const uint8_t r = (chip8->pixel_color[i] >> 24) & 0xFF;
            const uint8_t g = (chip8->pixel_color[i] >> 16) & 0xFF;
            const uint8_t b = (chip8->pixel_color[i] >>  8) & 0xFF;
            const uint8_t a = (chip8->pixel_color[i] >>  0) & 0xFF;

            SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
            SDL_RenderFillRect(sdl.renderer, &rect);
        }
    }

    SDL_RenderPresent(sdl.renderer);
}

void handle_input(chip8_t *chip8, config_t *config) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                chip8->state = QUIT;
                break;
            // Exit window, end program

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: //pressing escape button quits it
                        printf("==== QUIT ====\n");
                        chip8->state = QUIT;
                        break;
                    case SDLK_SPACE: //pressing space to pause the emulator action
                        if (chip8->state == RUNNING) {
                            chip8->state = PAUSED;
                            puts("==== PAUSED ====");
                        } else {
                            chip8->state = RUNNING;
                        }
                        break;

                    case SDLK_EQUALS:
                        //press '=' to reset the emulator
                        init_chip8(chip8, *config, chip8->rom_name);
                        break;
                    
                    case SDLK_j:
                        //press 'j' tp decrease lerping rate
                        if (config->color_lerp_rate > 0.1) {
                            config->color_lerp_rate -= 0.1;
                        }
                        break;
                    
                    case SDLK_k:
                        //press 'k' to increase lerping rate
                        if (config->color_lerp_rate < 1.0) {
                            config->color_lerp_rate += 0.1;
                        }
                        break;

                    case SDLK_o:
                        //press 'o' to reduce the volume
                        if (config->volume > 0) {
                            config->volume -= 250;
                        }
                        break;
                    
                    case SDLK_p:
                        //press 'p' to increase the volume
                        if (config->volume < INT16_MAX) {
                            config->volume += 250;
                        }
                        break;
                    
                    case SDLK_t: // 't' to toggle between sine and square wave
                        config->use_sine_wave = !config->use_sine_wave;
                        printf("Sound wave type: %s\n", config->use_sine_wave ? "Sine" : "Square");
                        break;
                        
                    case SDLK_y: // 'y' to toggle pixel outlines
                        config->pixel_outlines = !config->pixel_outlines;
                        printf("Pixel outlines: %s\n", config->pixel_outlines ? "Enabled" : "Disabled");
                        break;

                    case SDLK_1: chip8->keypad[0x1] = true; break;
                    case SDLK_2: chip8->keypad[0x2] = true; break;
                    case SDLK_3: chip8->keypad[0x3] = true; break;
                    case SDLK_4: chip8->keypad[0xC] = true; break;
                    
                    case SDLK_q: chip8->keypad[0x4] = true; break;
                    case SDLK_w: chip8->keypad[0x5] = true; break;
                    case SDLK_e: chip8->keypad[0x6] = true; break;
                    case SDLK_r: chip8->keypad[0xD] = true; break;
                    
                    case SDLK_a: chip8->keypad[0x7] = true; break;
                    case SDLK_s: chip8->keypad[0x8] = true; break;
                    case SDLK_d: chip8->keypad[0x9] = true; break;
                    case SDLK_f: chip8->keypad[0xE] = true; break;
                    
                    case SDLK_z: chip8->keypad[0xA] = true; break;
                    case SDLK_x: chip8->keypad[0x0] = true; break;
                    case SDLK_c: chip8->keypad[0xB] = true; break;
                    case SDLK_v: chip8->keypad[0xF] = true; break;

                       // Arrow key mappings
                    case SDLK_UP:    chip8->keypad[0x5] = true; break; // Map UP to 0x5
                    case SDLK_DOWN:  chip8->keypad[0x8] = true; break; // Map DOWN to 0x8
                    case SDLK_LEFT:  chip8->keypad[0x4] = true; break; // Map LEFT to 0x4
                    case SDLK_RIGHT: chip8->keypad[0x6] = true; break; // Map RIGHT to 0x6
                    
                    default:
                        break;
                }
                break;

            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_1: chip8->keypad[0x1] = false; break;
                    case SDLK_2: chip8->keypad[0x2] = false; break;
                    case SDLK_3: chip8->keypad[0x3] = false; break;
                    case SDLK_4: chip8->keypad[0xC] = false; break;
    
                    case SDLK_q: chip8->keypad[0x4] = false; break;
                    case SDLK_w: chip8->keypad[0x5] = false; break;
                    case SDLK_e: chip8->keypad[0x6] = false; break;
                    case SDLK_r: chip8->keypad[0xD] = false; break;
    
                    case SDLK_a: chip8->keypad[0x7] = false; break;
                    case SDLK_s: chip8->keypad[0x8] = false; break;
                    case SDLK_d: chip8->keypad[0x9] = false; break;
                    case SDLK_f: chip8->keypad[0xE] = false; break;
    
                    case SDLK_z: chip8->keypad[0xA] = false; break;
                    case SDLK_x: chip8->keypad[0x0] = false; break;
                    case SDLK_c: chip8->keypad[0xB] = false; break;
                    case SDLK_v: chip8->keypad[0xF] = false; break;

                      // Arrow key mappings
                    case SDLK_UP:    chip8->keypad[0x5] = false; break; // Map UP to 0x5
                    case SDLK_DOWN:  chip8->keypad[0x8] = false; break; // Map DOWN to 0x8
                    case SDLK_LEFT:  chip8->keypad[0x4] = false; break; // Map LEFT to 0x4
                    case SDLK_RIGHT: chip8->keypad[0x6] = false; break; // Map RIGHT to 0x6
                    
                    default:
                        break;
                }
                break;

            default:
                break;
        }
    }
}

#ifdef DEBUG
void print_debug_info(chip8_t *chip8){

    printf("Address: 0x%04X and Opcode: 0x%04X Desc: ", chip8->PC-2, chip8->inst.opcode);
    switch ((chip8->inst.opcode >> 12) & 0X0F ) {
        case 0X00: 
            if(chip8->inst.nn == 0XE0){
                //0X00E0 --> clear screen
                printf("clear screen\n");
            }
            else if(chip8->inst.nn == 0XEE){
                //0X00EE --> returns from a sub routine
                //Grabs the last address from the stack 
                printf("return from a subroutine to address 0x%04X\n", *(chip8->stack_ptr - 1));
                //chip8->PC = *--chip8->stack_ptr;
            }
            else{
                printf("Unimplemented code\n");
            }
        break;

        case 0x01:
            // 0x1NNN: Jumps to address NNN
            printf("Jumps to address NNN: 0x%04x\n", chip8->inst.nnn); // Jump to subroutine address
            break;

        case 0X02:
            printf("calls a subroutine at address: 0X%04x\n", chip8->inst.nnn);
            //0X2NNN calls a subroutine AT address NNN 
            //*chip8->stack_ptr++ = chip8->PC; //store current address in PC
            //chip8->PC = chip8->inst.nnn;
        break;

        case 0x03:
            // 0x3XNN: Check if VX == NN, if so, skip the next instruction
            printf("Check if V%X (0x%02X) == NN (0x%02X), skip next instruction if true\n",
                   chip8->inst.x, chip8->V[chip8->inst.x], chip8->inst.nn);
            break;

        case 0x04:
            // 0x4XNN: Check if VX != NN, if so, skip the next instruction
            printf("Check if V%X (0x%02X) != NN (0x%02X), skip next instruction if true\n",
                   chip8->inst.x, chip8->V[chip8->inst.x], chip8->inst.nn);
            break;

        case 0x05:
            // 0x5XY0: Check if VX == VY, if so, skip the next instruction
            printf("Check if V%X (0x%02X) == V%X (0x%02X), skip next instruction if true\n",
                   chip8->inst.x, chip8->V[chip8->inst.x], 
                   chip8->inst.y, chip8->V[chip8->inst.y]);
            break;

        case 0x06:
            ///0x6XNN sets the register Vx to nn
            printf("Set register V%x to NN 0x%02x\n",
            chip8->inst.x, chip8->inst.nn);
            //chip8->V[chip8->inst.x] = chip8->inst.nn;
            break;
        
         case 0x07:
            //0x7XNN sets the register Vx+= nn
            printf("Set register V%X (0x%02X) += NN (0x%02X). Result: 0x%02X\n",
                   chip8->inst.x, chip8->V[chip8->inst.x], chip8->inst.nn,
                   chip8->V[chip8->inst.x] + chip8->inst.nn);
            break;
        
        case 0x08:
            switch(chip8->inst.n) {
                case 0:
                    // 0x8XY0: Set register VX = VY
                    printf("Set register V%X = V%X (0x%02X)\n",
                           chip8->inst.x, chip8->inst.y, chip8->V[chip8->inst.y]);
                    break;

                case 1:
                    // 0x8XY1: Set register VX |= VY
                    printf("Set register V%X (0x%02X) |= V%X (0x%02X); Result: 0x%02X\n",
                           chip8->inst.x, chip8->V[chip8->inst.x],
                           chip8->inst.y, chip8->V[chip8->inst.y],
                           chip8->V[chip8->inst.x] | chip8->V[chip8->inst.y]);
                    break;

                case 2:
                    // 0x8XY2: Set register VX &= VY
                    printf("Set register V%X (0x%02X) &= V%X (0x%02X); Result: 0x%02X\n",
                           chip8->inst.x, chip8->V[chip8->inst.x],
                           chip8->inst.y, chip8->V[chip8->inst.y],
                           chip8->V[chip8->inst.x] & chip8->V[chip8->inst.y]);
                    break;

                case 3:
                    // 0x8XY3: Set register VX ^= VY
                    printf("Set register V%X (0x%02X) ^= V%X (0x%02X); Result: 0x%02X\n",
                           chip8->inst.x, chip8->V[chip8->inst.x],
                           chip8->inst.y, chip8->V[chip8->inst.y],
                           chip8->V[chip8->inst.x] ^ chip8->V[chip8->inst.y]);
                    break;

                case 4:
                    // 0x8XY4: Set register VX += VY, set VF to 1 if carry
                    printf("Set register V%X (0x%02X) += V%X (0x%02X), VF = 1 if carry; Result: 0x%02X, VF = %X\n",
                           chip8->inst.x, chip8->V[chip8->inst.x],
                           chip8->inst.y, chip8->V[chip8->inst.y],
                           chip8->V[chip8->inst.x] + chip8->V[chip8->inst.y],
                           ((uint16_t)(chip8->V[chip8->inst.x] + chip8->V[chip8->inst.y]) > 255));
                    break;

                case 5:
                    // 0x8XY5: Set register VX -= VY, set VF to 1 if there is not a borrow (result is positive/0)
                    printf("Set register V%X (0x%02X) -= V%X (0x%02X), VF = 1 if no borrow; Result: 0x%02X, VF = %X\n",
                           chip8->inst.x, chip8->V[chip8->inst.x],
                           chip8->inst.y, chip8->V[chip8->inst.y],
                           chip8->V[chip8->inst.x] - chip8->V[chip8->inst.y],
                           (chip8->V[chip8->inst.y] <= chip8->V[chip8->inst.x]));
                    break;

                case 6:
                    // 0x8XY6: Set register VX >>= 1, store shifted off bit in VF
                    printf("Set register V%X (0x%02X) >>= 1, VF = shifted off bit (%X); Result: 0x%02X\n",
                           chip8->inst.x, chip8->V[chip8->inst.x],
                           chip8->V[chip8->inst.x] & 1,
                           chip8->V[chip8->inst.x] >> 1);
                    break;

                case 7:
                    // 0x8XY7: Set register VX = VY - VX, set VF to 1 if there is not a borrow (result is positive/0)
                    printf("Set register V%X = V%X (0x%02X) - V%X (0x%02X), VF = 1 if no borrow; Result: 0x%02X, VF = %X\n",
                           chip8->inst.x, chip8->inst.y, chip8->V[chip8->inst.y],
                           chip8->inst.x, chip8->V[chip8->inst.x],
                           chip8->V[chip8->inst.y] - chip8->V[chip8->inst.x],
                           (chip8->V[chip8->inst.x] <= chip8->V[chip8->inst.y]));
                    break;
                // note, Vx is V%, chip8->int.x
                //value in vx is chip8->V[chip8->int.x]
                case 0xE:
                    // 0x8XYE: Set register VX <<= 1, store shifted off bit in VF
                    printf("Set register V%X (0x%02X) <<= 1, VF = shifted off bit (%X); Result: 0x%02X\n",
                           chip8->inst.x, chip8->V[chip8->inst.x],
                           (chip8->V[chip8->inst.x] & 0X80) >> 7,
                           chip8->V[chip8->inst.x] << 1);
                    break;

                default:
                    // Wrong/unimplemented opcode
                    break;
            }
            break;
        
        case 0x09:
            // 0x9XY0: Check if VX != VY; Skip next instruction if so
            printf("Check if V%X (0x%02X) != V%X (0x%02X), skip next instruction if true\n",
                   chip8->inst.x, chip8->V[chip8->inst.x], 
                   chip8->inst.y, chip8->V[chip8->inst.y]);
            break;

        case 0x0A:
            //0xANN sets index register to address NNN
            printf("Sets the I register to NNN (0x%04X)\n",
            chip8->inst.nnn);
            //chip8->I = chip8->inst.nnn;
            break;
        
        case 0x0B:
            // 0xBNNN: Jump to V0 + NNN
            printf("Set PC to V0 (0x%02X) + NNN (0x%04X); Result PC = 0x%04X\n",
                   chip8->V[0], chip8->inst.nnn, chip8->V[0] + chip8->inst.nnn);
            break;
        
        case 0x0C:
            // 0xCXNN: Sets register VX = rand() % 256 & NN (bitwise AND)
            printf("Set V%X = rand() %% 256 & NN (0x%02X)\n",
                   chip8->inst.x, chip8->inst.nn);
            break;

        case 0x0D:
            printf("Draw N(%u)height sprite t coords V%0X (0x%02X) and V%X (0x%02x) from memory location I (%04x)\n",
            chip8->inst.n, chip8->inst.x, chip8->V[chip8->inst.x], chip8->inst.y, chip8->V[chip8->inst.y], chip8->I);

        //default: printf("Unimplemented Opcode\n");
        break;
        
        case 0x0E:
            if (chip8->inst.nn == 0x9E) {
                // 0xEX9E: Skip next instruction if key in VX is pressed
                printf("Skip next instruction if key in V%X (0x%02X) is pressed; Keypad value: %d\n",
                       chip8->inst.x, chip8->V[chip8->inst.x], chip8->keypad[chip8->V[chip8->inst.x]]);

            } else if (chip8->inst.nn == 0xA1) {
                // 0xEX9E: Skip next instruction if key in VX is not pressed
                printf("Skip next instruction if key in V%X (0x%02X) is not pressed; Keypad value: %d\n",
                       chip8->inst.x, chip8->V[chip8->inst.x], chip8->keypad[chip8->V[chip8->inst.x]]);
            }
            break;
        
        case 0x0F:
            switch (chip8->inst.nn) {
                case 0x0A:
                    // 0xFX0A: VX = get_key(); Await until a keypress, and store in VX
                    printf("Await until a key is pressed; Store key in V%X\n",
                           chip8->inst.x);
                    break;

                case 0x1E:
                    // 0xFX1E: I += VX; Add VX to register I. For non-Amiga CHIP8, does not affect VF
                    printf("I (0x%04X) += V%X (0x%02X); Result (I): 0x%04X\n",
                           chip8->I, chip8->inst.x, chip8->V[chip8->inst.x],
                           chip8->I + chip8->V[chip8->inst.x]);
                    break;
                
                case 0x07:
                    // 0xFX07: VX = delay timer
                    printf("Set V%X = delay timer value (0x%02X)\n",
                           chip8->inst.x, chip8->delay_timer);
                    break;

                case 0x15:
                    // 0xFX15: delay timer = VX 
                    printf("Set delay timer value = V%X (0x%02X)\n",
                           chip8->inst.x, chip8->V[chip8->inst.x]);
                    break;

                case 0x18:
                    // 0xFX18: sound timer = VX 
                    printf("Set sound timer value = V%X (0x%02X)\n",
                           chip8->inst.x, chip8->V[chip8->inst.x]);
                    break;
                
                case 0x29:
                    // 0xFX29: Set register I to sprite location in memory for character in VX (0x0-0xF)
                    printf("Set I to sprite location in memory for character in V%X (0x%02X). Result(VX*5) = (0x%02X)\n",
                           chip8->inst.x, chip8->V[chip8->inst.x], chip8->V[chip8->inst.x] * 10);
                    break;
                
                case 0x33:
                    // 0xFX33: Store BCD representation of VX at memory offset from I;
                    //   I = hundred's place, I+1 = ten's place, I+2 = one's place
                    printf("Store BCD representation of V%X (0x%02X) at memory from I (0x%04X)\n",
                           chip8->inst.x, chip8->V[chip8->inst.x], chip8->I);
                    break;
                
                case 0x55:
                    // 0xFX55: Register dump V0-VX inclusive to memory offset from I;
                    //   SCHIP does not inrement I, CHIP8 does increment I
                    printf("Register dump V0-V%X (0x%02X) inclusive at memory from I (0x%04X)\n",
                           chip8->inst.x, chip8->V[chip8->inst.x], chip8->I);
                    break;

                case 0x65:
                    // 0xFX65: Register load V0-VX inclusive from memory offset from I;
                    //   SCHIP does not inrement I, CHIP8 does increment I
                    printf("Register load V0-V%X (0x%02X) inclusive at memory from I (0x%04X)\n",
                           chip8->inst.x, chip8->V[chip8->inst.x], chip8->I);
                    break;

                default:
                    break; //unimplemented codes

    }
    
 }
}
#endif

//emulate chip8 instructions
    void emu_instr(chip8_t *chip8, const config_t config) {
        bool carry; //carry flag for arithmetic operations

        // Get next opcode (Big Endian)
        chip8->inst.opcode = (chip8->ram[chip8->PC] << 8) | chip8->ram[chip8->PC + 1];
        chip8->PC += 2; // Increment PC

        // Fill current instruction format
        chip8->inst.nnn = chip8->inst.opcode & 0x0FFF; // 12-bit address
        chip8->inst.nn  = chip8->inst.opcode & 0x00FF; // 8-bit constant
        chip8->inst.n   = chip8->inst.opcode & 0x000F; // 4-bit constant
        chip8->inst.x   = (chip8->inst.opcode >> 8) & 0x000F; // X register
        chip8->inst.y   = (chip8->inst.opcode >> 4) & 0x000F; // Y register

    #ifdef DEBUG
        print_debug_info(chip8); // Debug output
    #endif

    switch ((chip8->inst.opcode >> 12) & 0x0F) {
        case 0x00: 
            if (chip8->inst.nn == 0xE0) {
                // 0x00E0: Clear screen
                memset(chip8->display, 0, sizeof(chip8->display));
                chip8->draw = true;
            } else if (chip8->inst.nn == 0xEE) {
                // 0x00EE: Return from subroutine
                chip8->PC = *--chip8->stack_ptr; // Pop address from stack
            }
            break;

        case 0x01:
            // 0x1NNN: Jumps to address NNN
            chip8->PC = chip8->inst.nnn; // Jump to subroutine address
            break;

        case 0x02:
            // 0x2NNN: Call subroutine at address NNN
            *chip8->stack_ptr++ = chip8->PC; // Push current address to stack
            chip8->PC = chip8->inst.nnn; // Jump to subroutine address
            break;

        case 0x03:
            //0x3xnn Skip next instruction if Vx == kk. 
            //The interpreter compares register Vx to kk, and if they are equal, increments the program counter by 2.
            if(chip8->V[chip8->inst.x] == chip8->inst.nn)
            {
                chip8->PC += 2;
            }
            break;

        case 0x04:
            //Skip next instruction if Vx != kk.
            //The interpreter compares register Vx to kk, and if they are not equal, increments the program counter by 2.
            if(chip8->V[chip8->inst.x] != chip8->inst.nn)
            {
                chip8->PC += 2;
            }
            break;        

        case 0x05:
            //0x5XY0
            //Skip next instruction if Vx == Vy.
            if(chip8->inst.n != 0) break; //wrong opcode format if n!=0
            if(chip8->V[chip8->inst.x] == chip8->V[chip8->inst.y]){
                chip8->PC += 2;
            }
            break;

        case 0x06:
            // 0x6XNN: Set register Vx to NN
            chip8->V[chip8->inst.x] = chip8->inst.nn;
            break;

        case 0x07:
            // 0x7XNN: Add NN to register Vx
            chip8->V[chip8->inst.x] += chip8->inst.nn;
            break;

        case 0x08:
            switch (chip8->inst.n)
            {
                case 0:
                    // 0x8XY0: Set register VX = VY
                    chip8->V[chip8->inst.x] = chip8->V[chip8->inst.y];
                    break;

                case 1:
                    //0x8XY1,  bitwise OR, Vx |= VY
                    chip8->V[chip8->inst.x] |= chip8->V[chip8->inst.y];
                    //Chip8 ONLY quirk --> VF is changed, set to zero
                    chip8->V[0XF] = 0;
                    break;

                case 2:
                    //0x8XY2, bitwise and, vx &= vy
                    chip8->V[chip8->inst.x] &= chip8->V[chip8->inst.y];
                    //chip8 only quirk --> VF is changed, set to zero
                    chip8->V[0XF] = 0;
                    break;

                case 3:
                    //0x8XY3, bitwise xor, vx ^= vy
                    chip8->V[chip8->inst.x] ^= chip8->V[chip8->inst.y];
                    //chip8 only quirk --> VF is changed, set to zero
                    chip8->V[0XF] = 0;
                    break;

                case 4: // 0x8XY4: ADD Vx, Vy
                    {
                        carry = ((uint16_t)(chip8->V[chip8->inst.x] + chip8->V[chip8->inst.y]) > 255);
                        chip8->V[chip8->inst.x] += chip8->V[chip8->inst.y];
                        chip8->V[0XF] = carry ? 1 : 0;
                    }
                    break;

                case 5: // 0x8XY5: SUB Vx, Vy, Vx = Vx - Vy
                //VF is set to 0 when there's a borrow, and 1 when there isn't
                    {
                        carry = (chip8->V[chip8->inst.x] >= chip8->V[chip8->inst.y]);
                        chip8->V[chip8->inst.x] -= chip8->V[chip8->inst.y];
                        chip8->V[0xF] = carry ? 1 : 0;
                    }
                    break;

                case 6: {
                    // 0x8XY6: Set register VX >>= 1, store shifted off bit in VF
                    if (config.current_extension == CHIP8) {
                        carry = chip8->V[chip8->inst.y] & 1;    // Use VY
                        chip8->V[chip8->inst.x] = chip8->V[chip8->inst.y] >> 1; // Set VX = VY result
                    } else {
                        carry = chip8->V[chip8->inst.x] & 1;    // Use VX
                        chip8->V[chip8->inst.x] >>= 1;          // Use VX
                    }
                    chip8->V[0xF] = carry; }
                    break;

                case 7:
                    //0x8XY7 Vx= Vy - Vx
                    carry = (chip8->V[chip8->inst.y] >= chip8->V[chip8->inst.x]);
                    chip8->V[chip8->inst.x] = chip8->V[chip8->inst.y] - chip8->V[chip8->inst.x];
                    chip8->V[0xF] = carry ? 1 : 0;
                    break;

                    case 0xE: {
                    // 0x8XYE: Set register VX <<= 1, store shifted off bit in VF
                    if (config.current_extension == CHIP8) { 
                        carry = (chip8->V[chip8->inst.y] & 0x80) >> 7; // Use VY
                        chip8->V[chip8->inst.x] = chip8->V[chip8->inst.y] << 1; // Set VX = VY result
                    } else {
                        carry = (chip8->V[chip8->inst.x] & 0x80) >> 7;  // VX
                        chip8->V[chip8->inst.x] <<= 1;                  // Use VX
                    }
                    chip8->V[0xF] = carry;
                        }
                    break;

                default:
                    break;
            }
            break;

        case 0x09:
            //0x9XY0, if Vx != Vy, skip next instruction
            if(chip8->V[chip8->inst.x] != chip8->V[chip8->inst.y]){
                chip8->PC += 2;
            }
            break;

        case 0x0A:
            // 0xANN: Set index register to address NNN
            chip8->I = chip8->inst.nnn;
            break;  

        case 0x0B:
            //Bnnn jump to V[0] + nnn address
            chip8->PC = chip8->V[0] + chip8->inst.nnn;
            break;  

        case 0x0C:
            //BXNN use rand() to generate a random number from 0-255 and bitwise and with inst.nn to store result in Vx
            chip8->V[chip8->inst.x] = ((rand() % 256) & chip8->inst.nn);
            break;

        case 0x0D: {
    // Get coordinates from registers
    uint8_t x_start = chip8->V[chip8->inst.x];
    uint8_t y_start = chip8->V[chip8->inst.y];
    //chip8->inst.n = 10;
    uint8_t height = chip8->inst.n;
    
    // Log the instruction details
    //SDL_Log("Drawing sprite: x=%d, y=%d, height=%d, I=0x%04X", 
            //x_start, y_start, height, chip8->I);
    
    // Reset collision flag
    chip8->V[0xF] = 0;
    
    // Draw each row of the sprite
    for (uint8_t row = 0; row < height; row++) {
        // Calculate Y position (with wrapping)
        uint8_t y_pos = (y_start + row) % config.window_height;
        
        // Get the sprite row data from memory
        uint8_t sprite_byte = chip8->ram[chip8->I + row];
        
        // Log the sprite data
        //SDL_Log("  Row %d: Sprite data = 0x%02X", row, sprite_byte);
        
        // Draw each pixel in this row
        for (uint8_t col = 0; col < 8; col++) {
            // Calculate X position (with wrapping)
            uint8_t x_pos = (x_start + col) % config.window_width;
            
            // Check if the current bit is set (MSB first)
            bool sprite_bit = (sprite_byte & (0x80 >> col)) != 0;
            
            // Skip if bit is not set (optional optimization)
            if (!sprite_bit) continue;
            
            // Calculate display buffer index
            uint16_t display_idx = y_pos * config.window_width + x_pos;
            
            // Check for collision
            if (chip8->display[display_idx]) {
                chip8->V[0xF] = 1;
            }
            
            // XOR the pixel
            chip8->display[display_idx] ^= true;
        }
    }
    // Set draw flag when implemented
    chip8->draw = true;
    break;
}

        case 0x0E:
            if(chip8->inst.nn == 0x9E){
                // 0xEX9E: Skip next instruction if key in VX is pressed
                if(chip8->keypad[chip8->V[chip8->inst.x]])
                    chip8->PC += 2;
            }
            else if (chip8->inst.nn == 0xA1){
                // 0xEX9E: Skip next instruction if key in VX is not pressed
                if(!chip8->keypad[chip8->V[chip8->inst.x]]){
                    chip8->PC += 2;
                }
            }
            break;

        case 0x0F: {
            switch (chip8->inst.nn) {
                case 0x0A: {
                    bool any_key_pressed = false;
                    for (uint8_t i = 0; i < sizeof(chip8->keypad); i++) {
                        if (chip8->keypad[i]) {
                            chip8->V[chip8->inst.x] = i; // Store the pressed key in V[x]
                            any_key_pressed = true;
                            break; // Exit the loop as soon as a key is found
                        }
                    }
                    if (! any_key_pressed) {
                        chip8->PC -= 2; // Decrement PC if no key is pressed
                    }
                }
                    break;

                case 0x1E: {
                    //Adds VX to I. VF is not affected.
                    chip8->I += chip8->V[chip8->inst.x] ;
                }
                    break;

                case 07: {
                    //set Vx = delay timer
                    chip8->V[chip8->inst.x] = chip8->delay_timer;
                }
                    break;

                case 0x15:{
                    //Set delaytimer = Vx
                    chip8->delay_timer = chip8->V[chip8->inst.x];
                }
                    break;

                case 0x18:{
                    //set the sound timer to Vx
                    chip8->sound_timer = chip8->V[chip8->inst.x];
                }
                    break;

                case 0x29:{
                    //Set I = location of sprite for digit Vx.
                    chip8->I = chip8->V[chip8->inst.x] * 5;
                }
                break;

                case 0x33:{
                    //The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at location in I, the tens digit at 
                    //location I+1, and the ones digit at location I+2.
                    uint8_t bcd = chip8->V[chip8->inst.x];
                    chip8->ram[chip8->I+2] = bcd % 10;
                    bcd = bcd/10;
                    chip8->ram[chip8->I+1] = bcd % 10;
                    bcd = bcd/10;
                    chip8->ram[chip8->I] = bcd;
                    } break;
                
                case 0x55: {
                    //0xFx55 --> The interpreter copies the values of registers V0 through Vx into memory, starting at the address in I.
                    //I itself is incremented in chip8 and chip48, but not in SCHIP
                    for (uint8_t i = 0; i <= chip8->inst.x; i++) {
                        if (config.current_extension == CHIP8) 
                            chip8->ram[chip8->I++] = chip8->V[i]; // Increment I each time
                        else
                            chip8->ram[chip8->I + i] = chip8->V[i]; // I doesn't change
                    }
                }
                break;

                case 0x65: {
                                    // 0xFX65: Register load V0-VX inclusive from memory offset from I
                    for (uint8_t i = 0; i <= chip8->inst.x; i++) {
                        if (config.current_extension == CHIP8) 
                            chip8->V[i] = chip8->ram[chip8->I++]; // Increment I each time
                        else
                            chip8->V[i] = chip8->ram[chip8->I + i]; // I doesn't change
                        } 
                    }
                     break;

                default:
                    // Handle unimplemented opcodes
                    break;
            }
            break;
        }
        default:
            // Handle unimplemented opcodes
            break;
    } 
}

//update the timers every 60hz
void update_timers(const sdl_t sdl, chip8_t *chip8){
    if(chip8->delay_timer > 0)
        chip8->delay_timer --;
    
    if(chip8->sound_timer > 0) {
        chip8->sound_timer --;
        SDL_PauseAudioDevice(sdl.dev, 0); // Unpause audio device
    }
    else {
        SDL_PauseAudioDevice(sdl.dev, 1); // Pause audio device
    }
}

// MAIN function block
int main(int argc, char **argv) 
{
    //default usage
    if(argc<2){
        fprintf(stderr,"Useage: %s <rom_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Initialize emulator configuration/options
    config_t config = {0};
    if (!set_config_from_args(&config, argc, argv)) exit(EXIT_FAILURE);

    // Initialize chip8 machine
    chip8_t chip8 = {0};
    const char *rom_name = argv[1];
    if (!init_chip8(&chip8, config, rom_name)) {
        exit(EXIT_FAILURE);
    }

    // Initialize SDL
    sdl_t sdl = {0};
    if (!init_sdl(&sdl, &config)) {
        exit(EXIT_FAILURE);
    }

    // Initial screen clear
    clear_screen(sdl, config);

    //Seed random number generator so that it generates different sequence of numbers
    srand(time(NULL));

    // Main emulator loop
    while (chip8.state != QUIT) {
        // Handle input
        handle_input(&chip8, &config);

        if(chip8.state == PAUSED) continue;

        //Get_time();
        const uint64_t start_frame_time = SDL_GetPerformanceCounter();
        //emulate chip8 instructions
        // Emulate CHIP8 Instructions for this emulator "frame" (60hz)
        for (uint32_t i = 0; i < config.instr_per_sec / 60; i++) {
            emu_instr(&chip8, config);

            // If drawing on CHIP8, only draw 1 sprite this frame (display wait)
            // This matches original CHIP8's behavior where sprite drawing takes time
            if ((config.current_extension == CHIP8) && 
                ((chip8.inst.opcode >> 12) == 0xD)) 
                break;
        }
        // Clear the screen to the background color on every frame
        const uint64_t end_frame_time = SDL_GetPerformanceCounter();

        const double time_lapsed = (double)((end_frame_time - start_frame_time) * 1000)/SDL_GetPerformanceFrequency();

        //clear_screen(sdl, config);

        // Delay for approximately 60Hz/60FPS
        SDL_Delay(16.67f > time_lapsed ? 16.67 - time_lapsed : 0);

        //update screen window with changes
        if(chip8.draw){
            update_screen(sdl, config, &chip8);
            chip8.draw = false;
        }

        update_timers(sdl, &chip8);
    }

    // Final cleanup
    final_cleanup(sdl);
    exit(EXIT_SUCCESS);
}