/*
 * @(#) graphicsenv.h 96/04/05 1.1
 */

#define MAX_BITMAPS       5  /* Overkill, since only 3 (2FB + 1Z) are needed */

/* Display just handles the View */
typedef struct Display {
    uint32    width;
    uint32    height;
    uint32    depth;
    uint32    numScreens;
    int32     signal;
    bool      interpolated;
    Item      view;
} Display;

/* GraphicsEnv contains info about the graphical environment */
typedef struct GraphicsEnv {
    GState*   gs;
    Display*  d;
    Item      bitmaps[MAX_BITMAPS];
} GraphicsEnv;


Display*      Display_Create       (void);
Err           Display_CreateView   (Display* d);
void          Display_Delete       (Display* d);
GraphicsEnv*  GraphicsEnv_Create   (void);
void          GraphicsEnv_Delete   (GraphicsEnv* genv);
Err           GraphicsEnv_Init     (GraphicsEnv* genv, uint32 width,
				    uint32 height, uint32 depth);



