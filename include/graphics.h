#include "font.h"
#include "math.h"

#define CHAR_HEIGHT 9
#define CHAR_WIDTH 9
#define CSIZE unsigned short int

extern unsigned int gFbAddr;
extern unsigned int gPitch;
extern unsigned int gScreenWidth, gScreenHeight;


void DrawLine(int x0, int y0, int x1, int y1, CSIZE color);
void DrawRectangle(int x0, int y0, int x1, int y1,  CSIZE color);
void DrawCircle(int x0, int y0, int radius,  CSIZE color);
void DrawPixel(unsigned int x, unsigned int y,  CSIZE color);
void DrawCharacterAt(unsigned int c, unsigned int x, unsigned int y,  CSIZE color);
void DrawFilledRectangle(int x0, int y0, int x1, int y1, CSIZE color);
void PutsAt(const char *s, unsigned int x, unsigned int y, CSIZE color);
void SetVirtualFrameBuffer(int bufferId);
void DisplayVirtualFramebuffer(int bufferId);