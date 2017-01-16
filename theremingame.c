/*=====================*
 |    Theremin Hero    |
 |  fseidel, jocelynh  |
 |     03/14/2016      |
 *=====================*/

/* Have you ever wanted to be a theremin-playing superhero? Well, you still
 * can't, but at least you can pretend to play a theremin.
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <math.h>

#include "theremin.h"

#ifndef M_PI
  #define M_PI 3.1415926535897932384
#endif

#define TAU (2*M_PI)

#define PIANO 2
#define GUITAR 0.5

#define WIDTH 1024
#define HEIGHT 768

/*==========<< GLOBALS >>===========*/

int quit = 0;         /* Did the user hit quit? */
float instr = PIANO;  /* Chosen instrument */

float pitches[] = {
  261.63, // C4
  293.66, // D4
  329.63, // E4
  349.23, // F4
  392.00, // G4
  440.00, // A4
  493.88, // B4
  523.25  // C5
};
char* pitchNames[] = {
  "C4",
  "D4",
  "E4",
  "F4",
  "G4",
  "A4",
  "B4",
  "C5"
};

// Settings
int colorblind = 0;
int mute = 0;

/* AUDIO wavedata/userdata struct */
typedef struct {
  double carrier_phase;       // Sine phase for callback to continue w.o clicks
  double modulator_phase;
  double modulator_amplitude; // Amount of modulation
  //int carrier_pitch;          // Frequency of carrier that determines pitch
  int pitchindex;
  int modulator_pitch;        // Frequency of modulator
} wavedata;

/* Functions */
void createWant(SDL_AudioSpec *wantpoint, wavedata *userdata);
void updateWavedata(wavedata *userdata, int newPitch);

/*=========<< END GLOBALS >>=========*/

/********<< Helper Functions >>*********/

/*=======<< generateWaveform (Callback Function) >>=======*
 * Fill audio buffer w/ glorious FM synth!                *
 * We take a sine wave (the carrier) and modulate it with *
 * another sine wave (the modulator).                     *
 *                                                        *
 * => sin(sin(t + p1) + t + p2) where p1, p2 are phases   *
 *                                                        *
 * This can create complex sounds with simple waveforms!  *
 * You hear the "outer" (carrier) sine wave, as expected, *
 * but you also hear the wave produced by the modulation  *
 * of the carrier.                                        *
 *========================================================*/
void generateWaveform(void *userdata, Uint8 *stream, int len) {
  short *dest = (short*)stream;       // Destination of values generated
  int size = len/sizeof(short);       // Buffer size

  /* Get info from wavedata */
  wavedata *wave_data = (wavedata*)userdata;
  double c_pitch = pitches[wave_data->pitchindex];  // Wave that actually plays
  double c_phase = wave_data->carrier_phase;
  double m_pitch = instr*c_pitch;                // Wave that modulates carrier
  double m_phase = wave_data->modulator_phase;
  double m_amplitude = wave_data->modulator_amplitude;

  // Fill buffer
  for (int i=0; i<size; i++) {
    dest[i] =
      sin( m_amplitude * sin( m_pitch*TAU*i/48000 + m_phase )
           + c_pitch*TAU*i/48000 + c_phase)*32767;  // <- Modulation
    //converts from float audio to signed short
  }

  // Update phase s.t. next frame of audio starts at same point in wave
  wave_data->carrier_phase =
    fmod(c_pitch*TAU*size/48000 + c_phase, TAU);
  wave_data->modulator_phase =
    fmod(m_pitch*TAU*size/48000 + m_phase, TAU);

  /* Change modulator amplitude to vary the amount of modulation.
   * A decay of 1 second means 0.4/60 = 0.066 repeating.
   * 0.4 is the max amplitude (completely arbitrary, it just sounds good).
   * 60 is frames per second.
   */
  //if(m_amplitude > 0) wave_data->modulator_amplitude -= 0.0066666666;
  //else wave_data->modulator_amplitude = 0.4; //reset if we hit 0
}


/*==========< createWant >===========*
 * Initialize the "want" Audiospec,  *
 * and set its values appropriately  *
 *===================================*/

void createWant(SDL_AudioSpec *wantpoint, wavedata *userdata) {

  wantpoint->freq = 48000;        // Sample rate of RasPi's sound system
  wantpoint->format = AUDIO_S16SYS;  // 32-bit floating pt samples, little-endian
  wantpoint->channels = 1;
  wantpoint->samples = 800;       // (48000 samples/sec)/(60 frames/sec)
                                  //  = 800 samp/frame
  wantpoint->callback = generateWaveform;

  // Set info in wavedata struct
  userdata->pitchindex = 0;          // Start at C4
  userdata->modulator_pitch = instr*440;
  userdata->modulator_phase = 0.0;
  userdata->carrier_phase = 0.0;
  userdata->modulator_amplitude = 0.4;

  wantpoint->userdata = userdata;
}



/*================< updateWavedata >================*
 * Update the wavedata (userdata) with values from  *
 * the theremin.                                    *
 *==================================================*/

void updateWavedata(wavedata *userdata, int newPitch) {
  userdata->pitchindex = newPitch;
}


/*================< checkKey >=================*
 * Check the key that was pressed, and         *
 * respond appropriately.                      *
 *=============================================*/
void checkKey(SDL_Keycode key, wavedata* wavedata_ptr) {
  int pitchindex = wavedata_ptr->pitchindex;

  /* Quit */
  if (key == SDLK_ESCAPE || key == SDLK_q) {
    quit = 1;
  }
  /* Raise pitch by one note */
  else if (key == SDLK_UP && pitchindex < 7) {
    updateWavedata(wavedata_ptr, pitchindex+1);
    printf("%d\n", pitchindex);
  }
  /* Lower pitch by one note */
  else if (key == SDLK_DOWN && pitchindex > 0) {
    updateWavedata(wavedata_ptr, pitchindex-1);
    printf("%d\n", pitchindex);
  }
  /* Change to colorblind mode */
  else if (key == SDLK_BACKSPACE) {
    colorblind = (colorblind+1)%2;
  }
  /* Change instruments */
  else if (key == SDLK_i) {
    instr = (instr == PIANO) ? GUITAR : PIANO;
    updateWavedata(wavedata_ptr, pitchindex);
  }
  /* Mute */
  else if (key == SDLK_m) {
    mute = (mute+1)%2;
  }
}

void drawNoteRectangle(int index, SDL_Renderer* renderer) {
    SDL_Rect r;
    r.x = index*50+50;
    r.y = (int)(5.0/6.0*HEIGHT);
    r.w = 50;
    r.h = 50;
    SDL_SetRenderDrawColor(renderer, 0,0,255,255);
    SDL_RenderFillRect(renderer, &r);
}



/*=============<< main >>==============*
 * Get that party started!             *
 * Initialize for rendering and audio. *
 *=====================================*/
int main(int argc, char* argv[]) {
  // Audio vars
  SDL_AudioSpec want, have;
  SDL_AudioDeviceID dev;
  wavedata my_wavedata;
  
  // Rendering vars
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Event event;

  // Text vars
  TTF_Font* font;
  SDL_Surface *surfaceMessage;
  SDL_Texture *message;
  SDL_Rect message_rect;

  SDL_Surface *noteMessage;
  SDL_Texture *nmessage;
  SDL_Rect nmessage_rect;
  
  // Keycode for key presses
  SDL_Keycode key;

  /*******<Initial Settings>*******/

  // Initialize with appropriate flags
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0 ||
      TTF_Init() < 0)
    return 1;
  atexit(SDL_Quit); // Set exit function s.t. SDL resources deallocated on quit



  /* ======<< AUDIO SETTINGS >>======= */
  SDL_memset(&want, 0, sizeof(want));
  createWant(&want, &my_wavedata);    // Call function to initialize vals
  dev = SDL_OpenAudioDevice(NULL, 0, &want, &have,
                            SDL_AUDIO_ALLOW_FORMAT_CHANGE);
  if (dev == 0) 
    printf("Error opening audio device: %s\n", SDL_GetError());



  /* ======<< RENDERING SETTINGS >====== */

  // Create window and renderer
  window = SDL_CreateWindow("SDL_RenderClear",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
  renderer = SDL_CreateRenderer(window, -1, 0);

  /* Text */

  // Opens font
  font = TTF_OpenFont("/Library/Fonts/Impact.ttf", 72);
  if(font == NULL) {
    printf("Font not found\n");
    return 1;
  }
  SDL_Color normalFontColor = {50, 170, 255};   // Darker blue
  SDL_Color cbFontColor = {54, 79, 60};        // Weird green
  SDL_Color fontColor = normalFontColor;
  

  /*********< Okay, game time! >***********/
  while (!quit) {

    // Get theremin input
    readFromTheremin(); // Dummy Function ###############!!!!!!!!!!!##########
    /* ==========<< Poll for events >>============ */
    while (SDL_PollEvent(&event)) {
      switch (event.type) {

        /* Key pressed */
        case SDL_KEYDOWN:
          key = event.key.keysym.sym;
          checkKey(key, &my_wavedata);
          break;
        /* Exit */
        case SDL_QUIT:
          quit = 1;
          break;
        default:
          break;
      }
    }

    /* ========<< Text >>======== */

    // Set font color
    fontColor = normalFontColor;
    if (colorblind) {
      fontColor = cbFontColor;
    }
    
    // Create surface and convert it to texture
    surfaceMessage =
      TTF_RenderText_Solid(font, "Theremin Hero!", fontColor);
    if (colorblind) {
      surfaceMessage =
        TTF_RenderText_Solid(font, "Colorblind Mode ;D", fontColor);
    }
    message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

    // {xPos, yPos, width, height}
    message_rect.x = 150;
    message_rect.y = 200;
    message_rect.w = 200;
    message_rect.h = 80;

    /* Shows note on screen */
    noteMessage =
      TTF_RenderText_Solid(font, pitchNames[my_wavedata.pitchindex], fontColor);
    nmessage = SDL_CreateTextureFromSurface(renderer, noteMessage);

    nmessage_rect.x = 210;
    nmessage_rect.y = 350;
    nmessage_rect.w = 100;
    nmessage_rect.h = 50;


    /* ========<< Background >>========= */

    // Choose background color
    SDL_SetRenderDrawColor(renderer, 170, 200, 215, 255);   // Light blue
    if (colorblind) {
      SDL_SetRenderDrawColor(renderer, 79, 54, 58, 255);    // Dark brown
    }

    // Set background color
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 54, 79, 60, 255);     // Green
    SDL_RenderDrawLine(renderer, 5, 5, 340, 340);     // Awkward diagonal line

    // Render message texture
    SDL_RenderCopy(renderer, message, NULL, &message_rect);
    SDL_RenderCopy(renderer, nmessage, NULL, &nmessage_rect);

    /* =======<< Rectangle Showing Note >>======= */
    drawNoteRectangle(my_wavedata.pitchindex, renderer);

    // Move to foreground
    SDL_RenderPresent(renderer);

    /* =========<< Audio >>========== */
    SDL_PauseAudioDevice(dev, mute);
  }

  // CLEAN YO' ROOM (Cleanup)
  TTF_CloseFont(font);
  SDL_CloseAudioDevice(dev);
  SDL_Quit();

  return 0;
}




