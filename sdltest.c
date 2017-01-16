// Compile with:
//   gcc -I/usr/local/include -L/usr/local/lib -lSDL2 -lSDL2_ttf theremingame.c

#include <SDL2/SDL.h>
#include <SDL2/SDL_TTF.h>
#include <math.h>
#ifndef M_PI
  #define M_PI 3.1415926535897932384
#endif

/* Wavedata/userdata struct containing:
 *  phase:      Sine phase for callback function to continue where it left off
 *              such that there isn't any clicking
 *  freq_pitch: Frequency of the pitch given (arbitrary)
 */

typedef struct {
  double carrier_phase;
  double modulator_phase;
  double modulator_amplitude;
  int carrier_pitch;
  int modulator_pitch;
} wavedata;

/*==<< Sine wave generator -- Callback function >>==*/
void generateWaveform(void *userdata, Uint8 *stream, int len) {
  float *dest = (float*)stream;       // Destination of values generated
  int size = len/sizeof(float);       // Buffer size

  wavedata *wave_data = (wavedata*)userdata;  // Get info from wavedata
  int carrier_pitch = wave_data->carrier_pitch; // the wave that actually plays
  double carrier_phase = wave_data->carrier_phase;
  int modulator_pitch = wave_data->modulator_pitch; //the wave that modulates the carrier
  double modulator_phase = wave_data->modulator_phase;
  double modulator_amplitude = wave_data->modulator_amplitude;

  /* Fill audio buffer w/ glorious FM synth!
   * We take a sine wave and modulate it with another sine wave (the modulator)
   * This can be seen as the two nested sine functions
   * e.g. sin(sin(t+p1) + t + p2) where p1 and p2 are phases
   * This can create complex sounds with simple waveforms!
   * You hear the "outer" (carrier) sine wave, as expected, but you also hear the wave produced
   * by the modulation of the carrier. Think of a big sine wave, but when you zoom in, you
   * find that the line forming the sine is itself sinusoidal.
   */
  for (int i=0; i<size; i++) {
    dest[i] = sin(modulator_amplitude*sin(modulator_pitch*(2*M_PI)*i/48000
                                          + modulator_phase)
                  + carrier_pitch*(2*M_PI)*i/48000 + carrier_phase);
  }

  // Update phase s.t. next frame of audio starts at same point in wave
  wave_data->carrier_phase = fmod(carrier_pitch*(2*M_PI)*size/48000 +
                                  carrier_phase, 2*M_PI);
  wave_data->modulator_phase = fmod(modulator_pitch*(2*M_PI)*size/48000 +
                                    modulator_phase, 2*M_PI);
  
  // Swept sine wave; increment freq. by 1 each frame
  //wave_data->carrier_pitch += 1;

  // Change modulator frequency to show difference in sound
  //wave_data->modulator_pitch += 1;

  /* Change modulator amplitude to vary the amount of modulation
   * A decay of 1 second means 0.4/60 = 0.066 repeating
   * 0.4 is the max amplitude (completely arbitrary, it just sounds good)
   * 60 is frames per second
   */
  if(modulator_amplitude > 0) wave_data->modulator_amplitude -= 0.0066666666;
  else wave_data->modulator_amplitude = 0.4; //reset if we hit 0
}

/*=======<< Main >>=======*/
int main(int argc, char* argv[]) {
  // Rendering vars
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Event event;

  // Audio vars
  SDL_AudioSpec want, have;
  SDL_AudioDeviceID dev;
  wavedata my_wavedata;

  /*******<Initial Settings>*******/

  // Initialize with appropriate flags
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0)
    return 1;

  // Set exit function so that all SDL resources are deallocated on quit
  atexit(SDL_Quit);

  // Set audio settings
  SDL_memset(&want, 0, sizeof(want));

  /*******<Set Audio Settings>*******/
  want.freq = 48000;        // Sample rate of RasPi's sound system
  want.format = AUDIO_F32;  // 32-bit floating point samples, little-endian
  want.channels = 1;
  want.samples = 800;   // (48000 samples/sec)/(60 frames/sec) = 800 samp/frame
  want.callback = generateWaveform;

  wavedata *userdata = &my_wavedata;  // Set info in wavedata struct
  userdata->carrier_pitch = 1000;
  userdata->modulator_pitch = 500;
  userdata->modulator_phase = 0.0;
  userdata->carrier_phase = 0.0;
  userdata->modulator_amplitude = 0.4;
  want.userdata = userdata;

  // Alright audio is a go
  dev = SDL_OpenAudioDevice(NULL, 0, &want, &have,
                            SDL_AUDIO_ALLOW_FORMAT_CHANGE);
  if (dev == 0) 
    printf("Error opening audio device: %s\n", SDL_GetError());
  SDL_PauseAudioDevice(dev, 0);

  /*******<Rendering/Drawing>*******/

  // Create window in which we will draw
  window = SDL_CreateWindow("SDL_RenderClear",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 512, 512, 0);

  // Create renderer
  renderer = SDL_CreateRenderer(window, -1, 0);

  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red

  // Set screen to red
  SDL_RenderClear(renderer);

  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
  SDL_RenderDrawLine(renderer, 5, 5, 300, 300);

  // Move to foreground
  SDL_RenderPresent(renderer);

  while(1) {
    if(SDL_PollEvent(&event)) {
      if(event.type == SDL_KEYDOWN)
        break;
    }
  }

  // CLEAN YO' ROOM
  SDL_CloseAudioDevice(dev);
  SDL_Quit();

  return 0;
}
