#include "NOC_global.h"
#include <SDL/SDL.h>
static inline int clamp_and_scale(int i);
void setpixel(SDL_Surface *screen, int x, int y, Uint8 r, Uint8 g, Uint8 b);

void DrawScreen(SDL_Surface* screen, StorablePicture *p);

int show_one_frame(StorablePicture *p);
