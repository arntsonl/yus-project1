#include "famisets.h"

SDL_Surface *LoadIMG2RGBA(char *filename)
{
    SDL_Surface *orig,*image;

    SDL_PixelFormat RGBAFormat;
	RGBAFormat.palette = 0;
	RGBAFormat.BitsPerPixel = 32;
	RGBAFormat.BytesPerPixel = 4;
	RGBAFormat.Rmask = RMASK;
	RGBAFormat.Rshift = 0;
	RGBAFormat.Rloss = 0;
	RGBAFormat.Gmask = GMASK;
	RGBAFormat.Gshift = 8;
	RGBAFormat.Gloss = 0;
	RGBAFormat.Bmask = BMASK;
	RGBAFormat.Bshift = 16;
	RGBAFormat.Bloss = 0;
	RGBAFormat.Amask = AMASK;
	RGBAFormat.Ashift = 24;
	RGBAFormat.Aloss = 0;

    char cCurrentPath[FILENAME_MAX];
    GetCurrentDir(cCurrentPath, sizeof(cCurrentPath));
    char * fullPath = new char[strlen(filename) + strlen(cCurrentPath) + 1];
    strcpy(fullPath,cCurrentPath);
    strcat(fullPath,filename);
    fullPath[strlen(filename)+strlen(cCurrentPath)] = '\0';
    orig = IMG_Load(fullPath);
    if ( orig == NULL ) {
        return(NULL);
    }

    image = SDL_ConvertSurface(orig, &RGBAFormat, NULL); // Converts all surfaces to RGBA
    SDL_FreeSurface(orig);

    return(image);
}
