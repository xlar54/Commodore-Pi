#define SCREEN_WIDTH 360 //320
#define SCREEN_HEIGHT 240 //200
#define CSIZE unsigned short int

int GetScreenSizeFromTags();
int SetupScreen();
int GetPitch();
int InitializeFramebuffer();
void DisplayVirtualFramebuffer(int bufferId);

