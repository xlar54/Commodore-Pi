#define CHAR_HEIGHT 8
#define CHAR_WIDTH 8
#define CHAR_VSPACING 0
#define CHAR_HSPACING 0
#define TERMINAL_WIDTH (SCREEN_WIDTH / (CHAR_WIDTH + CHAR_HSPACING)) - 1 // 191 @ 1920
#define TERMINAL_HEIGHT (SCREEN_HEIGHT / (CHAR_HEIGHT + CHAR_VSPACING)) - 1 // 76 @ 1080
#define BUFFER_HEIGHT TERMINAL_HEIGHT
#define BUFFER_WIDTH TERMINAL_WIDTH

#include "framebuffer.h"

// TODO 0: Make buffer larger than display to allow for some history to get saved
// TODO 1: Change to int buffers and embed colors in value
char gBuffer[BUFFER_HEIGHT][BUFFER_WIDTH];
char gTerminal[TERMINAL_HEIGHT][TERMINAL_WIDTH];
int gBufferCaretRow; 		// The current row of the caret - where  text will be written to
int gBufferCaretCol;		// The current column of the caret - where text will be written to
int gFirstVisibleBufferRow; // The row in the first buffer that is currently the first row on screen

void PresentBufferToScreen(void)
{	
		
	unsigned int row;
	unsigned int col;
	for(row = 0; row < TERMINAL_HEIGHT; row++)
	{
		for(col = 0; col < TERMINAL_WIDTH; col++)
		{
			if(gTerminal[row][col] != gBuffer[row][col])
			{
				gTerminal[row][col] = gBuffer[row][col];
				DrawCharacterAt(gTerminal[row][col], col * (CHAR_WIDTH + CHAR_HSPACING), row * (CHAR_HEIGHT + CHAR_VSPACING), 0x07FF);
			}
		}
	}
}

void terminal_clear(void)
{
	gFirstVisibleBufferRow = 0;
	gBufferCaretRow = 0;
	gBufferCaretCol = 0;
	
	unsigned int row;
	for(row = 0; row < BUFFER_HEIGHT; row++)
	{
		unsigned int col;
		for(col = 0; col < BUFFER_WIDTH; col++)
		{
			gBuffer[row][col] = ' ';
		}
	}
	
	// Sync to display
	PresentBufferToScreen();
}

int terminal_init(void)
{
	if(InitializeFramebuffer() != 0)
	{
		return -1;
	}

	// Initial setup of buffers etc
	terminal_clear();		
	
	return(0);
}

void terminal_back(void)
{
	if(gBufferCaretCol == 0)
	{
		gBufferCaretRow--;
		gBufferCaretCol = BUFFER_WIDTH - 1;
		
		// We have to go back up a row
		gBuffer[gBufferCaretRow][gBufferCaretCol] = ' ';
	}
	else
	{
		gBufferCaretCol--;
		gBuffer[gBufferCaretRow][gBufferCaretCol] = ' ';
	}
	
	PresentBufferToScreen();
}

void print(char* string, unsigned int length)
{
	// 1. "Draw" everything to the buffer 
	unsigned int i;
	for(i = 0; i < length; i++)
	{	
		if(string[i] == '\n')
		{
			gBuffer[gBufferCaretRow][gBufferCaretCol] = ' ';
			gBufferCaretRow++;
			gBufferCaretCol = 0;
			continue;
		}
		
		if(gBuffer[gBufferCaretRow][gBufferCaretCol] != string[i])
			gBuffer[gBufferCaretRow][gBufferCaretCol] = string[i];
		
		if(gBufferCaretCol < BUFFER_WIDTH - 1)
		{
			gBufferCaretCol++;
		}
		else
		{
			// Reached the end of this row
			gBufferCaretRow++;
			gBufferCaretCol = 0;
		}
		
		if(gBufferCaretRow >= BUFFER_HEIGHT)
		{
			terminal_clear();
			gBufferCaretRow = 0;
		}
	}
	
	// 2. Flip buffer to screen
	PresentBufferToScreen();
}
