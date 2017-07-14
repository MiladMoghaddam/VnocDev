#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <SDL/SDL.h>
#include "NOC_global.h"
//#include "mbuffer.h"

#define WIDTH 1280
#define HEIGHT 720
#define BPP 4
#define DEPTH 32


static inline int clamp_and_scale(int i) {

  i>>=20;
  if(i<0)i=0; if(i>255)i=255;

  return i;
}


void setpixel(SDL_Surface *screen, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    Uint32 *pixmem32;
    Uint32 colour;  
 
    colour = SDL_MapRGB( screen->format, r, g, b );
  
    pixmem32 = (Uint32*) screen->pixels  + y + x;
    *pixmem32 = colour;
}


void DrawScreen(SDL_Surface* screen, StorablePicture *p )
{ 
    int x, y, ytimesw;
	int R,G,B; 
	int Y,U,V;
	int Pb, Pr, Cb,Cr,Y1;
 
    if(SDL_MUSTLOCK(screen)) 
    {
        if(SDL_LockSurface(screen) < 0) return;
    }

    for(y = 0; y < screen->h; y++ ) 
    {
        ytimesw = y*screen->pitch/BPP;
        for( x = 0; x < screen->w; x++ ) 
        {
			Y=p->imgY[y][x];
			U=p->imgUV[0][y/2][x/2];
			V=p->imgUV[1][y/2][x/2];

			Cb=p->imgUV[0][y/2][x/2];
			Cr=p->imgUV[1][y/2][x/2];

			B= 1.164*(Y-16)+2.018*(U-128);			
			G= 1.164*(Y-16) - 0.813*(V-128) - 0.391*(U-128);
			R= 1.164*(Y-16) - 1.596*(V-128);

			Y1= (Y-16) *1225732;
			Pb=(Cb-128)*1170;
	        Pr=(Cr-128)*1170;

            R=clamp_and_scale(Y1        +1436*Pr);
            G=clamp_and_scale(Y1- 352*Pb -731*Pr);
            B=clamp_and_scale(Y1+1815*Pb);


            //setpixel(screen, x, ytimesw,p->imgY[y][x],p->imgUV[0][y/2][x/2],p->imgUV[0][y/2][x/2]);
            //setpixel(screen, x, ytimesw,Y,U,V);            
			setpixel(screen, x, ytimesw,R,G,B);
        }
    }

    if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
  
    SDL_Flip(screen); 
}


int show_one_frame(StorablePicture *p)
{
    SDL_Surface *screen;
    SDL_Event event;
  	int frame_show_time;
	int wait;
	int delay=0;
  
    //if (SDL_Init(SDL_INIT_VIDEO) < 0 ) return 1;

  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)) {
    return 1;
  }
  atexit(SDL_Quit);

    if (!(screen = SDL_SetVideoMode(p->size_x, p->size_y, DEPTH, SDL_HWSURFACE)))//SDL_FULLSCREEN|SDL_HWSURFACE)))
    {
        SDL_Quit();
        return 1;
    }

	frame_show_time=SDL_GetTicks();
    DrawScreen(screen,p); 
	wait = (frame_show_time+delay)-SDL_GetTicks();
	if (wait > 0)	
		SDL_Delay(wait);

/*
    while(!keypress) 
    {
        DrawScreen(screen,h++,p);
         while(SDL_PollEvent(&event)) 
         {      
              switch (event.type) 
              {
                  case SDL_QUIT:
	              keypress = 1;
	              break;
                  case SDL_KEYDOWN:
                       keypress = 1;
                       break;
              }
         }

    }
*/    
    //SDL_Quit();
    
    return 0;
}

