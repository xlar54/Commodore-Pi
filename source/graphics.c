#include "graphics.h"

unsigned int gFrameBufferId=1; // 1 or 2

void SetVirtualFrameBuffer(int bufferId)
{
	gFrameBufferId = bufferId;
}

inline void DrawPixel(unsigned int x, unsigned int y, CSIZE color)
{
	if(gFrameBufferId == 1)
	{
		CSIZE* ptr = (CSIZE*)(gFbAddr + (y * gPitch) + (x * 2));
		*ptr = color;
	}
	else
	{
		CSIZE* ptr = (CSIZE*)(gFbAddr + ( (y+gScreenHeight) * gPitch ) + (x * 2));
		*ptr = color;
	}
}

void DrawLine(int x0, int y0, int x1, int y1, CSIZE color) 
{
 
  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
  int err = (dx>dy ? dx : -dy)/2, e2;
 
  for(;;){
    DrawPixel(x0,y0, color);
    if (x0==x1 && y0==y1) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x0 += sx; }
    if (e2 < dy) { err += dx; y0 += sy; }
  }
}

void DrawRectangle(int x0, int y0, int x1, int y1,  CSIZE color)
{
	DrawLine(x0, y0, x1, y0, color);
	DrawLine(x0, y0, x0, y1, color);
	DrawLine(x0, y1, x1, y1, color);
	DrawLine(x1, y1, x1, y0, color);
}

void DrawFilledRectangle(int x0, int y0, int x1, int y1, CSIZE color)
{
	while(x0 <= x1 && y0 <= y1)
		DrawRectangle(x0++, y0++, x1, y1, color);
}

void DrawCircle(int x0, int y0, int radius,  CSIZE color)
{
  int x = radius, y = 0;
  int radiusError = 1-x;
 
  while(x >= y)
  {
    DrawPixel(x + x0, y + y0, color);
    DrawPixel(y + x0, x + y0, color);
    DrawPixel(-x + x0, y + y0, color);
    DrawPixel(-y + x0, x + y0, color);
    DrawPixel(-x + x0, -y + y0, color);
    DrawPixel(-y + x0, -x + y0, color);
    DrawPixel(x + x0, -y + y0, color);
    DrawPixel(y + x0, -x + y0, color);
 
    y++;
        if(radiusError<0)
                radiusError+=2*y+1;
        else
        {
                x--;
                radiusError+=2*(y-x+1);
        }
  }
}

void DrawCharacterAt(unsigned int ch, unsigned int x, unsigned int y,  CSIZE color)
{
	// Ensure valid char table lookup
	ch = ch < 32 ? 0 : ch > 127 ? 0 : ch - 32;
	
	int col;
	unsigned int row;
	for(row = 0; row < CHAR_HEIGHT; row++)
	{
		unsigned int i = 0;
		for(col = CHAR_HEIGHT - 2; col >= 0 ; col--)
		{
			if(row < (CHAR_HEIGHT - 1) && (teletext[ch][row] & (1 << col)))
			{
				DrawPixel(x + i, y + row, color);
			}
			else
			{
				DrawPixel(x + i, y + row, 0x0000);
			}
			i++;
		}
	}
}

void PutsAt(const char *s, unsigned int x, unsigned int y, CSIZE color)
{
	unsigned int i = 0;
	unsigned int c = 0;
	
	while(s[i] !=0)
	{
		c = s[i++];
		DrawCharacterAt(c, x,y, color);
		x += 8;
	};
	
}