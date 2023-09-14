// vid_sdl.h -- sdl video driver 

#include <SDL2/SDL.h>
#include "quakedef.h"
#include "d_local.h"

viddef_t    vid;                // global video state
unsigned short  d_8to16table[256];

#define BASEWIDTH           320
#define BASEHEIGHT          240

//ATSB: Global resolution for correct display of various things in the game like intermissions.
//ATSB: This goes along with the scaling to ensure it all looks nice.
unsigned screenWidth = 320;
unsigned screenHeight = 200;

int    VGA_width, VGA_height, VGA_rowbytes, VGA_bufferrowbytes = 0;
byte    *VGA_pagebase;

static int	lockcount;
static qboolean	vid_initialized = false;
static SDL_Surface *screen;
static qboolean	palette_changed;
static unsigned char	vid_curpal[256*3];	/* save for mode changes */
static qboolean mouse_avail;
static float   mouse_x, mouse_y;
static int mouse_oldbuttonstate = 0;
int format;

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Surface* paletteSurface;
static SDL_Texture *screen_tex;

// No support for option menus
void (*vid_menudrawfn)(void) = NULL;
void (*vid_menukeyfn)(int key) = NULL;

void VID_SetPalette(unsigned char* palette)
{
    int i;
    SDL_Color colors[256];

    palette_changed = true;

    if (palette != vid_curpal)
        memcpy(vid_curpal, palette, sizeof(vid_curpal));

    for (i = 0; i < 256; ++i)
    {
        colors[i].r = *palette++;
        colors[i].g = *palette++;
        colors[i].b = *palette++;
        colors[i].a = SDL_ALPHA_OPAQUE;  // Set the alpha value, assuming fully opaque
    }
    
    SDL_CreateRGBSurfaceWithFormat(0, 256, 1, 8, SDL_PIXELFORMAT_INDEX8);
    if (!paletteSurface)
    {
        // Handle error
        return;
    }

    // Create a palette surface with SDL_CreateRGBSurface
    screen = SDL_CreateRGBSurface(0, vid.width, vid.height, 8, 0, 0, 0, 0);
    if (!paletteSurface)
    {
        // Handle error
        return;
    }

    // Set the color palette for the palette surface
    SDL_SetPaletteColors(paletteSurface->format->palette, colors, 0, 256);

    // Set the color key for transparency (if needed)
    SDL_SetColorKey(paletteSurface, SDL_TRUE, 0);

    // Apply the palette to the screen or destination surface
    SDL_BlitSurface(paletteSurface, NULL, screen, NULL);
    SDL_FreeSurface(paletteSurface);  // Free the palette surface if you're done with it
}

void    VID_ShiftPalette (unsigned char *palette)
{
    VID_SetPalette(palette);
}

void VID_LockBuffer (void)
{
	lockcount++;

	if (lockcount > 1)
		return;

	SDL_LockSurface(screen);

	// Update surface pointer for linear access modes
	vid.buffer = vid.conbuffer = vid.direct = (pixel_t *) screen->pixels;
	vid.rowbytes = vid.conrowbytes = screen->pitch;

	if (r_dowarp)
		d_viewbuffer = r_warpbuffer;
	else
		d_viewbuffer = vid.buffer;

	if (r_dowarp)
		screenwidth = WARP_WIDTH;
	else
		screenwidth = vid.rowbytes;
}

void VID_UnlockBuffer (void)
{
	lockcount--;

	if (lockcount > 0)
		return;

	if (lockcount < 0)
		return;

	SDL_UnlockSurface (screen);

// to turn up any unlocked accesses
	//vid.buffer = vid.conbuffer = vid.direct = d_viewbuffer = NULL;
}

void VID_Init(unsigned char* palette)
{
    int pnum, chunk, pitch;
    void *pixels;
    byte* cache;
    int cachesize;
    Uint8 video_bpp;
    Uint16 video_w, video_h;
    Uint32 flags;
    char caption[50];

    // Initialize SDL
    SDL_version v;
    SDL_version *sdl_version = &v;
    SDL_GetVersion(&v);

    Sys_Printf("Found SDL version %i.%i.%i\n",sdl_version->major,sdl_version->minor,sdl_version->patch);
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        Sys_Error("VID: Couldn't initialise SDL: %s\n", SDL_GetError());
    }

    // Set up display mode (width and height)
    vid.width = BASEWIDTH;
    vid.height = BASEHEIGHT;
    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;

    // Check for command-line window size
    if ((pnum = COM_CheckParm("-winsize")))
    {
        if (pnum >= com_argc - 2)
            Sys_Error("VID: -winsize <width> <height>\n");
        vid.width = Q_atoi(com_argv[pnum + 1]);
        vid.height = Q_atoi(com_argv[pnum + 2]);
        if (!vid.width || !vid.height)
            Sys_Error("VID: Bad window width/height\n");
    }
    if ((pnum = COM_CheckParm("-width"))) {
        if (pnum >= com_argc - 1)
            Sys_Error("VID: -width <width>\n");
        vid.width = Q_atoi(com_argv[pnum + 1]);
        if (!vid.width)
            Sys_Error("VID: Bad window width\n");
    }
    if ((pnum = COM_CheckParm("-height"))) {
        if (pnum >= com_argc - 1)
            Sys_Error("VID: -height <height>\n");
        vid.height = Q_atoi(com_argv[pnum + 1]);
        if (!vid.height)
            Sys_Error("VID: Bad window height\n");
    }

    // Set video width, height, and flags
    flags = (SDL_SWSURFACE | SDL_RENDERER_PRESENTVSYNC | SDL_WINDOW_FULLSCREEN);

    if (COM_CheckParm("-fullscreen"))
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    if (COM_CheckParm("-window")) {
        flags &= ~SDL_WINDOW_BORDERLESS;
    }

    if (vid.width > 1280 || vid.height > 1024)
    {
        Sys_Error("Maximum Resolution is 1280 width and 1024 height");
    }
    
    //ATSB: Adjust W/H depending on current screen resolution
    SDL_DisplayMode DispMode;
    SDL_GetCurrentDisplayMode(0, &DispMode);

    //Adjust height so the screen is 4:3 aspect ratio
    //ATSB: Stuck in my DisplayMode as local variables for the SDL window and video scaling only
    //ATSB: So that it looks better upscaled to a decent resolution, however all scaling
    //ATSB: Remains at 4:3 320x200 so that nothing is stretched on widescreen resolutions.
    
    //ATSB: No idea why, but Windows vs Linux behaves differently...
#ifdef _WIN32
    screenWidth = DispMode.h * 3.0 / 4.0;
    screenHeight = DispMode.w * 3.0 / 4.0;
#else
    screenWidth = DispMode.w * 3.0 / 4.0;
    screenHeight = DispMode.h * 3.0 / 4.0;
#endif

    // Initialize display 
    window = SDL_CreateWindow("Good Looking Quake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, flags);
    if (window == NULL) {
        Sys_Error("VID: Couldn't create window: %s\n", SDL_GetError());
        }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        Sys_Error("VID: Couldn't create renderer: %s\n", SDL_GetError());
	}

    VID_SetPalette(palette);
    
    format = SDL_GetWindowPixelFormat(window);
    if (format == SDL_PIXELFORMAT_UNKNOWN)
        format = SDL_PIXELFORMAT_RGBA32;
    screen_tex = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, vid.width, vid.height);
    if (!screen_tex)
    {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
        SDL_DestroyWindow(window);
        window = NULL;
    }

    sprintf(caption, "Good Looking Quake - Version %4.2f", VERSION);
    SDL_SetWindowTitle(window, caption);

    // Now know everything we need to know about the buffer
    VGA_width = vid.conwidth = vid.width;
    VGA_height = vid.conheight = vid.height;
    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int*)vid.colormap + 2048));

    // Allocate z buffer and surface cache
    chunk = vid.width * vid.height * sizeof(*d_pzbuffer);
    cachesize = D_SurfaceCacheForRes(vid.width, vid.height);
    chunk += cachesize;
    d_pzbuffer = Hunk_HighAllocName(chunk, "video");
    if (d_pzbuffer == NULL)
        Sys_Error("Not enough memory for video mode\n");

    // Initialize the cache memory
    cache = (byte*)d_pzbuffer + vid.width * vid.height * sizeof(*d_pzbuffer);
    D_InitCaches(cache, cachesize);

    // Initialize the mouse
    SDL_ShowCursor(SDL_DISABLE);

    vid_initialized = true;
}


void    VID_Shutdown (void)
{
	if (vid_initialized)
	{
		if (screen != NULL && lockcount > 0)
			SDL_UnlockSurface (screen);

		vid_initialized = 0;
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}
}

void VID_Update(vrect_t* rects)
{
    SDL_Rect* sdlrects;
    int n, i;
    vrect_t* rect;

    // Two-pass system, since Quake doesn't do it the SDL way...

    // First, count the number of rectangles
    n = 0;
    for (rect = rects; rect; rect = rect->pnext)
        ++n;

    // Second, copy them to SDL rectangles and render
    if (!(sdlrects = (SDL_Rect*)alloca(n * sizeof(*sdlrects))))
        Sys_Error("Out of memory");
    i = 0;
    for (rect = rects; rect; rect = rect->pnext)
    {
        sdlrects[i].x = rect->x;
        sdlrects[i].y = rect->y;
        sdlrects[i].w = rect->width;
        sdlrects[i].h = rect->height;
        ++i;
    }

    // Assuming you have a renderer available, render the rectangles
    for (i = 0; i < n; ++i)
    {
        SDL_RenderFillRect(renderer, &sdlrects[i]);
    }

    // Update the screen
    SDL_RenderPresent(renderer);
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
    Uint8 *offset;


    if (!screen) return;
    if ( x < 0 ) x = screen->w+x-1;
    offset = (Uint8 *)screen->pixels + y*screen->pitch + x;
    while ( height-- )
    {
        memcpy(offset, pbitmap, width);
        offset += screen->pitch;
        pbitmap += width;
    }
}


/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect(int x, int y, int width, int height)
{
    if (!screen) return;
    if (x < 0) x = screen->w + x - 1;

    // Assuming you have a renderer initialized
    SDL_Rect rect = { x, y, width, height };
    SDL_RenderSetViewport(renderer, &rect);
    SDL_RenderPresent(renderer);
}


/*
================
Sys_SendKeyEvents
================
*/

void Sys_SendKeyEvents(void)
{
    SDL_Event event;
    int sym, state;
    int modstate;

    while (SDL_PollEvent(&event))
    {
        switch (event.type) {

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            sym = event.key.keysym.sym;
            state = event.key.state;
            modstate = SDL_GetModState();
            switch (sym)
            {
            case SDLK_DELETE: sym = K_DEL; break;
            case SDLK_BACKSPACE: sym = K_BACKSPACE; break;
            case SDLK_F1: sym = K_F1; break;
            case SDLK_F2: sym = K_F2; break;
            case SDLK_F3: sym = K_F3; break;
            case SDLK_F4: sym = K_F4; break;
            case SDLK_F5: sym = K_F5; break;
            case SDLK_F6: sym = K_F6; break;
            case SDLK_F7: sym = K_F7; break;
            case SDLK_F8: sym = K_F8; break;
            case SDLK_F9: sym = K_F9; break;
            case SDLK_F10: sym = K_F10; break;
            case SDLK_F11: sym = K_F11; break;
            case SDLK_F12: sym = K_F12; break;
            case SDLK_PAUSE: sym = K_PAUSE; break;
            case SDLK_UP: sym = K_UPARROW; break;
            case SDLK_DOWN: sym = K_DOWNARROW; break;
            case SDLK_RIGHT: sym = K_RIGHTARROW; break;
            case SDLK_LEFT: sym = K_LEFTARROW; break;
            case SDLK_INSERT: sym = K_INS; break;
            case SDLK_HOME: sym = K_HOME; break;
            case SDLK_END: sym = K_END; break;
            case SDLK_PAGEUP: sym = K_PGUP; break;
            case SDLK_PAGEDOWN: sym = K_PGDN; break;
            case SDLK_RSHIFT:
            case SDLK_LSHIFT: sym = K_SHIFT; break;
            case SDLK_RCTRL:
            case SDLK_LCTRL: sym = K_CTRL; break;
            case SDLK_RALT:
            case SDLK_LALT: sym = K_ALT; break;
            case SDLK_KP_0:
                if (modstate & KMOD_NUM) sym = K_INS;
                else sym = SDLK_0;
                break;
            case SDLK_KP_1:
                if (modstate & KMOD_NUM) sym = K_END;
                else sym = SDLK_1;
                break;
            case SDLK_KP_2:
                if (modstate & KMOD_NUM) sym = K_DOWNARROW;
                else sym = SDLK_2;
                break;
            case SDLK_KP_3:
                if (modstate & KMOD_NUM) sym = K_PGDN;
                else sym = SDLK_3;
                break;
            case SDLK_KP_4:
                if (modstate & KMOD_NUM) sym = K_LEFTARROW;
                else sym = SDLK_4;
                break;
            case SDLK_KP_5: sym = SDLK_5; break;
            case SDLK_KP_6:
                if (modstate & KMOD_NUM) sym = K_RIGHTARROW;
                else sym = SDLK_6;
                break;
            case SDLK_KP_7:
                if (modstate & KMOD_NUM) sym = K_HOME;
                else sym = SDLK_7;
                break;
            case SDLK_KP_8:
                if (modstate & KMOD_NUM) sym = K_UPARROW;
                else sym = SDLK_8;
                break;
            case SDLK_KP_9:
                if (modstate & KMOD_NUM) sym = K_PGUP;
                else sym = SDLK_9;
                break;
            case SDLK_KP_PERIOD:
                if (modstate & KMOD_NUM) sym = K_DEL;
                else sym = SDLK_PERIOD;
                break;
            case SDLK_KP_DIVIDE: sym = SDLK_SLASH; break;
            case SDLK_KP_MULTIPLY: sym = SDLK_ASTERISK; break;
            case SDLK_KP_MINUS: sym = SDLK_MINUS; break;
            case SDLK_KP_PLUS: sym = SDLK_PLUS; break;
            case SDLK_KP_ENTER: sym = SDLK_RETURN; break;
            case SDLK_KP_EQUALS: sym = SDLK_EQUALS; break;
            }
            // If we're not directly handled and still above 255
            // just force it to 0
            if (sym > 255) sym = 0;
            Key_Event(sym, state);
            break;

        case SDL_MOUSEMOTION:
            if ((event.motion.x != (vid.width / 2)) ||
                (event.motion.y != (vid.height / 2))) {
                mouse_x = event.motion.xrel * 10;
                mouse_y = event.motion.yrel * 10;
                if ((event.motion.x < ((vid.width / 2) - (vid.width / 4))) ||
                    (event.motion.x > ((vid.width / 2) + (vid.width / 4))) ||
                    (event.motion.y < ((vid.height / 2) - (vid.height / 4))) ||
                    (event.motion.y > ((vid.height / 2) + (vid.height / 4)))) {
                    SDL_WarpMouseInWindow(NULL, vid.width / 2, vid.height / 2);
                }
            }
            break;

        case SDL_QUIT:
            CL_Disconnect();
            Host_ShutdownServer(false);
            Sys_Quit();
            break;

        default:
            break;
        }
    }
}

void IN_Init (void)
{
    if ( COM_CheckParm ("-nomouse") )
        return;
    mouse_x = mouse_y = 0.0;
    mouse_avail = 1;
}

void IN_Shutdown (void)
{
    mouse_avail = 0;
}

void IN_Commands (void)
{
    int i;
    int mouse_buttonstate;
   
    if (!mouse_avail) return;
   
    i = SDL_GetMouseState(NULL, NULL);
    /* Quake swaps the second and third buttons */
    mouse_buttonstate = (i & ~0x06) | ((i & 0x02)<<1) | ((i & 0x04)>>1);
    for (i=0 ; i<3 ; i++) {
        if ( (mouse_buttonstate & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)) )
            Key_Event (K_MOUSE1 + i, true);

        if ( !(mouse_buttonstate & (1<<i)) && (mouse_oldbuttonstate & (1<<i)) )
            Key_Event (K_MOUSE1 + i, false);
    }
    mouse_oldbuttonstate = mouse_buttonstate;
}

void IN_Move (usercmd_t *cmd)
{
    if (!mouse_avail)
        return;
   
    mouse_x *= sensitivity.value;
    mouse_y *= sensitivity.value;
   
    if ( (in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1) ))
        cmd->sidemove += m_side.value * mouse_x;
    else
        cl.viewangles[YAW] -= m_yaw.value * mouse_x;
    if (in_mlook.state & 1)
        V_StopPitchDrift ();
   
    if ( (in_mlook.state & 1) && !(in_strafe.state & 1)) {
        cl.viewangles[PITCH] += m_pitch.value * mouse_y;
        if (cl.viewangles[PITCH] > 80)
            cl.viewangles[PITCH] = 80;
        if (cl.viewangles[PITCH] < -70)
            cl.viewangles[PITCH] = -70;
    } else {
        if ((in_strafe.state & 1) && noclip_anglehack)
            cmd->upmove -= m_forward.value * mouse_y;
        else
            cmd->forwardmove -= m_forward.value * mouse_y;
    }
    mouse_x = mouse_y = 0.0;
}

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
    return 0;
}
