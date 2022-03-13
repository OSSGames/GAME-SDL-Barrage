/***************************************************************************
    copyright            : (C) 2003 by Michael Speck
    email                : http://lgames.sf.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "defs.h"
#include "shots.h"
#include "units.h"
#include "data.h"
#include "chart.h"
#include "bfield.h"
#include "menu.h"

SDL_Surface *vsurf;
SDL_Surface *screen;
int use_shadow_surface = 0;
int video_scale = 0; /* factors: 0 = 1, 1 = 1.5, 2 = 2 */
int video_xoff = 0, video_yoff = 0; /* offset of scaled surface in display */
int video_sw = 0, video_sh = 0; /* size of scaled shadow surface */
int display_w = 0, display_h = 0; /* original size of display */
int video_forced_w = 0, video_forced_h = 0; /* given by command line */
#define BITDEPTH 16

int delay = 0;
int fullscreen = 1;
int audio_on = 1;
int audio_freq = 22050;
int audio_format = AUDIO_S16LSB;
int audio_channels = 2;
int audio_mix_channel_count = 0;

extern SDL_Surface 
	*img_ammo, *background, *img_ground, *img_logo, *img_icons, *img_black, *img_cursors; 
extern SDL_Font *ft_main, *ft_chart, *ft_chart_highlight, *ft_help, *ft_copyright;
extern SDL_Cursor *cr_empty;
extern int ammo_w, ammo_h;
extern int cursor_w, cursor_x_offset;
extern char *optarg;

Menu main_menu;
enum {
	ID_GAME = 101,
	ID_HELP,
	ID_CHART,
	ID_QUIT
};

Vector gun_base = { 20, 240, 0 };
Vector gun_img_offset = { -9, -20, 0 };

int player_score = 0;
int player_ammo = 0;
char player_name[20] = "Michael"; /* is updated when entering highscore */

int game_start = 0, game_end = 0; /* time in global secs when game started and has to end */

int timer_blink_delay = 0; /* timer starts to blink when time runs out */
int timer_visible = 1; /* used to blink timer */

enum {
	STATE_MENU,
	STATE_HELP,
	STATE_CHART,
	STATE_GAME
};
int state = STATE_MENU; /* state of game */

/* Lets try to get name from environment */
static void set_player_name_from_env() 
{
	char * user_name = getenv("USER");
	if (user_name!=NULL)
	{
		strncpy(player_name, user_name, 19);
	}
};

static void select_best_video_mode(int *best_w, int *best_h)
{
	SDL_Rect **modes;
	SDL_Rect wanted_mode;
	int i;
	int dratio;

	dratio = 100*display_w/display_h;
	wanted_mode.w = display_w;
	wanted_mode.h = display_h;

	/* Get available fullscreen/hardware modes */
	modes=SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);
	/* Check if there are any modes available */
	if(modes == (SDL_Rect **)0){
		printf("No modes available!\n");
		exit(-1);
	}
	/* Check if our resolution is restricted */
	if(modes == (SDL_Rect **)-1){
		printf("All resolutions available.\n");
	} else {
		/* Print valid modes */
		printf("Available modes: ");
		for(i=0;modes[i];++i)
			printf("%d x %d, ", modes[i]->w, modes[i]->h);
		printf("\n");

		/* select lowest mode with same ratio as display */
		for(i=0;modes[i];++i)
			if (100*modes[i]->w/modes[i]->h == dratio) {
				if (modes[i]->h < wanted_mode.h)
					wanted_mode = *modes[i];
			}
		printf("Best mode: %d x %d\n",wanted_mode.w,wanted_mode.h);
	}

	*best_w = wanted_mode.w;
	*best_h = wanted_mode.h;
}

static void set_video_mode()
{
	int w, h;
	int vflags = (fullscreen?SDL_FULLSCREEN:0) | SDL_HWSURFACE;

	if (use_shadow_surface && screen) {
		SDL_FreeSurface(screen);
		screen = NULL;
		use_shadow_surface = 0;
	}

	if (video_forced_w > 0 && video_forced_h > 0) {
		w = video_forced_w;
		h = video_forced_h;
	} else if (!fullscreen) {
		w = 640;
		h = 480;
	} else
		select_best_video_mode(&w, &h);

	/* for 640x480 no shadow surface is used */
	video_scale = 0;
	video_xoff = video_yoff = 0;
	if (w == 640 && h == 480)
		vflags |= SDL_DOUBLEBUF; /* use fast double buffering */
	else {
		printf("Using shadow surface\n");
		use_shadow_surface = 1;
		if (2*480 <= h) {
			video_scale = 2;
			video_sw = 2*640;
			video_sh = 2*480;
		} else if (3*480/2 <= h) {
			video_scale = 1;
			video_sw = 3*640/2;
			video_sh = 3*480/2;
		} else {
			video_sw = 640;
			video_sh = 480;
		}
		printf("Using scale factor %s\n",
				(video_scale==2)?"2":((video_scale==1)?"1.5":0));
		video_xoff = (w - video_sw) / 2;
		video_yoff = (h - video_sh) / 2;
	}

	if (SDL_SetVideoMode(w, h, BITDEPTH, vflags) < 0 ) {
		printf( "%s\n", SDL_GetError() );
		exit(1);
	}

	/* with no shadow surface screen is doubled buffered video surface */
	if (!use_shadow_surface) {
		screen = SDL_GetVideoSurface();
		return;
	}

	/* create shadow screen surface as 640x480 */
	vsurf = SDL_GetVideoSurface();
	screen = SDL_CreateRGBSurface( SDL_SWSURFACE, 640, 480,
			vsurf->format->BitsPerPixel,
			vsurf->format->Rmask, vsurf->format->Bmask,
			vsurf->format->Gmask, vsurf->format->Amask );

}

static void scale_surface(SDL_Surface *src, SDL_Surface *dst)
{
	int i,j;
	int bpp = src->format->BytesPerPixel; /* should equal dst */
	int sxoff, syoff, dxoff, dyoff;
	Uint32 pixel = 0;

	if (bpp != dst->format->BytesPerPixel) {
		printf("Oops, pixel size does not match, no scaling\n");
		SDL_BlitSurface(screen, 0, vsurf, 0);
		return;
	}

	if (SDL_MUSTLOCK(src))
		SDL_LockSurface(src);
	if (SDL_MUSTLOCK(dst))
		SDL_LockSurface(dst);

	sxoff = syoff = 0;
	dxoff = video_xoff * bpp;
	dyoff = video_yoff * dst->pitch;
	for (j = 0; j < src->h; j++) {
		for (i = 0; i < src->w; i++) {
			memcpy(&pixel, src->pixels + syoff + sxoff, bpp);
			memcpy(dst->pixels + dyoff + dxoff, &pixel, bpp);
			if (video_scale) {
				if (video_scale==2 || (i&1))
					memcpy(dst->pixels + dyoff + dxoff + bpp, &pixel, bpp);
				if (video_scale==2 || (j&1)) {
					memcpy(dst->pixels + dyoff + dst->pitch + dxoff, &pixel, bpp);
					if (video_scale==2 || (i&1))
						memcpy(dst->pixels + dyoff + dst->pitch + dxoff + bpp, &pixel, bpp);
				}
				if (video_scale==2 || (i&1))
					dxoff += bpp;
			}
			sxoff += bpp;
			dxoff += bpp;
		}
		sxoff = 0;
		syoff += src->pitch;
		dxoff = video_xoff*bpp;
		dyoff += dst->pitch;
		if (video_scale && (video_scale==2 || (j&1)))
			dyoff += dst->pitch;
	}

	if (SDL_MUSTLOCK(src))
		SDL_UnlockSurface(src);
	if (SDL_MUSTLOCK(dst))
		SDL_UnlockSurface(dst);
}

void refresh_screen()
{
	if (!use_shadow_surface)
		SDL_Flip(screen);
	else {
		scale_surface(screen,vsurf);
		SDL_UpdateRect(vsurf, 0, 0, 0, 0);
	}
}

static void fade_screen( int type, int time )
{
	SDL_Surface *buffer = 0;
	float alpha;
	float alpha_change; /* per ms */
	int leave = 0;
	int ms;
	int cur_ticks, prev_ticks;

	/* get screen contents */
	buffer = SDL_CreateRGBSurface( 
			SDL_SWSURFACE, screen->w, screen->h, 
			screen->format->BitsPerPixel,
			screen->format->Rmask, screen->format->Bmask,
			screen->format->Gmask, screen->format->Amask );
	if ( buffer == 0 ) return;
	SDL_BlitSurface( screen, 0, buffer, 0 );

	/* compute alpha and alpha change */
	if ( type == FADE_OUT ) {
		alpha = 255;
		alpha_change = -255.0 / time;
	}
	else {
		alpha = 0;
		alpha_change = 255.0 / time;
	}

	/* fade */
	cur_ticks = prev_ticks = SDL_GetTicks();
	while ( !leave ) {
		prev_ticks = cur_ticks;
		cur_ticks = SDL_GetTicks();
		ms = cur_ticks - prev_ticks;
		
		alpha += alpha_change * ms;
		if ( type == FADE_IN && alpha >= 255 ) break;
		if ( type == FADE_OUT && alpha <= 0 ) break;
		
		/* update */
		SDL_FillRect( screen, 0, 0x0 );
		SDL_SetAlpha( buffer, SDL_SRCALPHA | SDL_RLEACCEL, (int)alpha );
		SDL_BlitSurface( buffer, 0, screen, 0 );
		refresh_screen(screen);
		if (delay>0) SDL_Delay(10);
	}

	/* update screen */
	SDL_SetAlpha( buffer, 0, 0 );
	if ( type == FADE_IN )
		SDL_BlitSurface( buffer, 0, screen, 0 );
	else
		SDL_FillRect( screen, 0, 0x0 );
	refresh_screen(screen);
	SDL_FreeSurface( buffer );
}

static int all_filter( const SDL_Event *event ) { return 0; }

static void wait_for_input( void )
{
	SDL_EventFilter filter;
	SDL_Event event;
	
	filter = SDL_GetEventFilter();
	SDL_SetEventFilter(all_filter);
	while ( SDL_PollEvent(&event) ); /* clear queue */
	SDL_SetEventFilter(filter);
	
	while ( 1 ) {
		SDL_WaitEvent(&event);
		if (event.type == SDL_QUIT) {
			return;
		}
		if (event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN)
			return;
	}
}

static void draw_score()
{
	char str[8];

	if ( player_score < 0 ) player_score = 0;
	snprintf( str, 8, "%06i", player_score ); str[7] = 0;
	SDL_WriteText( ft_main, screen, 10, 10, str );
}

static void draw_time( int ms )
{
	static int old_secs = 0;
	char str[8];
	int secs = game_end - time(0);
	
	/* blink timer when time is running out in such a way
	 * that it becomes visible everytime a new second started */
	if ( secs < 30 ) {
		if ( timer_blink_delay )
		if ( (timer_blink_delay-=ms) <= 0 )
			timer_visible = 0;
		if ( secs != old_secs ) {
			timer_visible = 1;
			timer_blink_delay = 500;
			old_secs = secs;
		}
	}
	
	if ( !timer_visible ) return;
	snprintf( str, 8, "%i:%02i", secs/60, secs%60 ); str[7] = 0;
	SDL_WriteTextRight( ft_main, screen, 630, 10, str );
}

static void draw_ammo()
{
	int i, full, rest;
	SDL_Rect drect, srect;
	
	full = player_ammo / AMMO_BOMB; /* one tile represents a bomb ammo which
					   contains of AMMO_BOMB single grenade shots */
	rest = player_ammo % AMMO_BOMB; /* first ammo may contain less than AMMO_BOMB ammo */
	
	drect.w = srect.w = ammo_w;
	drect.h = srect.h = ammo_h;
	srect.x = 0; srect.y = 0;
	drect.y = 480 - 10 - drect.h;
	drect.x = 10;

	for ( i = 0; i < full; i++ ) { 
		SDL_BlitSurface( img_ammo, &srect, screen, &drect );
		drect.y -= drect.h;
	}

	if ( rest > 0 ) {
		srect.y = (AMMO_BOMB - rest) * ammo_h;
		SDL_BlitSurface( img_ammo, &srect, screen, &drect );
	}
}

extern BField bfield;
/* draw cursor centered at x,y */
static void draw_cursor( SDL_Surface *dest, int x, int y )
{
	SDL_Rect srect,drect;

	drect.w = srect.w = cursor_w;
	drect.h = srect.h = img_cursors->h;
	srect.x = cursor_x_offset; srect.y = 0;
	/* XXX: within gun strip, gun cannot be fired; to indicate this use
	 * disabled cursor instead of current selection. (bad hack, state
	 * should be properly set somewhere) */
	if ( x < STRIP_OFFSET && !bfield.demo )
		srect.x = CURSOR_DISABLED * cursor_w;
	drect.x = x - (cursor_w>>1); drect.y = y - (img_cursors->h>>1);
	SDL_BlitSurface( img_cursors, &srect, dest, &drect );
}

/* add copyright */
static void draw_logo_centered( SDL_Surface *dest, int y )
{
	SDL_Rect drect;

	drect.x = (dest->w-img_logo->w)/2;
	drect.y = y;
	drect.w = dest->w; drect.h = dest->h;

	SDL_BlitSurface( img_logo, 0, dest, &drect );

	SDL_WriteText( ft_copyright, dest, 4, 454, "(C) 2003-2019 Michael Speck" );
	SDL_WriteTextRight( ft_copyright, dest, 636, 454, "http://lgames.sf.net" );
}

#define LINE(y,text) SDL_WriteText(ft_help,dest,x,y,text)
static void draw_help( SDL_Surface *dest )
{
	int x = 70, target_y = 60, gun_y = 180, hint_y = 350;
	SDL_Rect srect, drect;

	SDL_BlitSurface( img_black, 0, dest, 0 );
	
	/* objective */
	LINE(10,"Your objective is to destroy as many dummy" );
	LINE(30,"targets as possible within 3 minutes." );
	
	/* icons and points */
	SDL_WriteText( ft_help, dest, x,target_y, "Target Scores:" );
	srect.w = drect.w = 30;
	srect.h = drect.h = 30;
	srect.y = 0; drect.y = target_y+20;
	srect.x = 0; drect.x = x; 
	SDL_BlitSurface( img_icons, &srect, dest, &drect );
	SDL_WriteText( ft_help, dest, x + 34,target_y+24, "Soldier: 5 Pts" );
	srect.x = 30; drect.x = x+180;
	SDL_BlitSurface( img_icons, &srect, dest, &drect );
	SDL_WriteText( ft_help, dest, x+214,target_y+24, "Jeep: 20 Pts" );
	srect.x = 60; drect.x = x+350;
	SDL_BlitSurface( img_icons, &srect, dest, &drect );
	SDL_WriteText( ft_help, dest, x+384,target_y+24, "Tank: 50 Pts" );
	SDL_WriteText( ft_help, dest, x,target_y+50, 
			"If a jeep makes it safe through the screen you'll" );
	SDL_WriteText( ft_help, dest, x,target_y+70, 
			"loose 10 pts. For a tank you'll loose 25 pts. For" );
	SDL_WriteText( ft_help, dest, x,target_y+90,
			"soldiers there is no such penalty." );

	/* gun handling */
	LINE(gun_y,    "Gun Handling:");
	LINE(gun_y+20, "The gun is controlled by the mouse. Move the");
	LINE(gun_y+40, "crosshair at the target and left-click to fire a" );
	LINE(gun_y+60, "large grenade. (costs 6 rounds, 0.5 secs fire rate)");
	LINE(gun_y+80, "Middle-click to fire a small grenade. (1 round)" );
	LINE(gun_y+100,"Use the right mouse button to reload your gun" );
	LINE(gun_y+120,"to the full 36 rounds. Reloading always costs" );
	LINE(gun_y+140,"36 points so do this as late as possible." );

	/* hint */
	LINE(hint_y,    "NOTE: A tank cannot be damaged by small" ); 
	LINE(hint_y+20, "grenades which you should only use at soldiers" );
	LINE(hint_y+40, "and jeeps when you are experienced enough in" );
	LINE(hint_y+60, "aiming. For rookies it is much better to only" );
	LINE(hint_y+80, "use the large grenades (left-click), count the" );
	LINE(hint_y+100,"shots and reload after six times. (right-click)" );
}

static void take_screenshot()
{
	char buf[32];
	static int i;
	sprintf( buf, "screenshot%i.bmp", i++ );
	SDL_SaveBMP( screen, buf );

}

static void game_init()
{
	bfield_finalize(); /* clear battlefield from menu */
	bfield_init( BFIELD_GAME, &gun_base, &gun_img_offset );
	player_score = 0;
	player_ammo = MAX_AMMO;

	SET_CURSOR( CURSOR_CROSSHAIR );
	fade_screen( FADE_OUT, 500 );
	SDL_BlitSurface( background, 0, screen, 0 );
	fade_screen( FADE_IN, 500 );

	game_start = time(0); game_end = game_start + GAME_TIME; timer_visible = 1;
}

static void game_finalize( int check_chart )
{
	int rank;
	char buf[64];
	
	bfield_finalize();
	SET_CURSOR( CURSOR_ARROW );

	fade_screen( FADE_OUT, 500 );
	bfield_init( BFIELD_DEMO, &gun_base, &gun_img_offset ); /* new battlefield for menu */
	bfield_draw( screen, 0, 0 );
	fade_screen( FADE_IN, 500 );
	
	/* check wether highscore was entered */
	if ( check_chart ) {
		SDL_WriteTextCenter( ft_chart, screen, 320, 130, "The time is up!" );
		sprintf( buf, "Your result is %i points.", player_score );
		SDL_WriteTextCenter( ft_chart, screen, 320, 160, buf );
		if ( ( rank = chart_get_rank( player_score ) ) >= 0 ) {
			SDL_WriteTextCenter( 
					ft_chart, screen, 320, 200, "Good job! This makes you" );
			sprintf( buf, "Topgunner #%i", rank+1 );
			SDL_WriteTextCenter( ft_chart, screen, 320, 230, buf );
			SDL_WriteTextCenter( ft_chart, screen, 320, 290, "Sign here:" );
			refresh_screen(screen);
			SDL_EnterTextCenter( ft_chart, screen, 320, 320, 18, player_name );
			chart_add_entry( player_name, player_score );
		}
		else {
			SDL_WriteTextCenter( 
					ft_chart, screen, 320, 290, "Not enough to be a topgunner!" );
			SDL_WriteTextCenter( 
					ft_chart, screen, 320, 320, "Go, try again! Dismissed." );
			refresh_screen(screen);
			wait_for_input();
		}
		chart_save();
	}
}
	
static void main_loop()
{
	Uint8 *keystate, empty_keys[SDLK_LAST];
	int x = 0, y = 0;
	int cur_ticks, prev_ticks;
	int ms;
	int buttonstate;
	int reload_clicked = 0;
	int reload_key = SDLK_SPACE;
	int frame_time = 0, frames = 0;
	Vector dest;
	int quit = 0;
	int input_delay = 0; /* while >0 no clicks are accepted */
	SDLMod modstate = 0;
	int shot_type;
	SDL_Event event;
	
	frame_time = SDL_GetTicks(); /* for frame rate */
	memset( empty_keys, 0, sizeof( empty_keys ) ); /* to block input */
		
	state = STATE_MENU;
	bfield_init( BFIELD_DEMO, &gun_base, &gun_img_offset );
	
	cur_ticks = prev_ticks = SDL_GetTicks();
	while ( !quit ) {
	  if (delay>0) SDL_Delay(delay);
	  
		prev_ticks = cur_ticks;
		cur_ticks = SDL_GetTicks();
		ms = cur_ticks - prev_ticks;

		/* gather events in queue and get mouse/keyboard states */
		SDL_PumpEvents();
		if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
			quit = 1;
		buttonstate = SDL_GetMouseState( &x, &y );
		keystate = SDL_GetKeyState( 0 );
		modstate = SDL_GetModState();

		/* translate cursor position if shadow surface is used */
		if (use_shadow_surface) {
			//printf("%d x %d ->",x,y);
			x -= video_xoff;
			y -= video_yoff;
			if (x < 0)
				x = 0;
			if (y < 0)
				y = 0;
			if (x >= video_sw)
				x = video_sw - 1;
			if (y >= video_sh)
				y = video_sh - 1;
			x = 640 * x / video_sw;
			y = 480 * y / video_sh;
			//printf(" %d x %d (w=%d)\n",x,y,video_sw);
		}
	
		/* update the battlefield (particles,units,new cannonfodder) */
		bfield_update( ms );

		/* update input_delay */
		if ( input_delay > 0 ) 
		if ( (input_delay-=ms) < 0 ) input_delay = 0;
		
		/* draw ground, trees, units, particles */
		bfield_draw( screen, 0, 0 );
		
		/* according to state draw add-ons */
		switch ( state ) {
			case STATE_GAME:
				draw_score();
				draw_time( ms );
				draw_ammo();
				break;
			case STATE_MENU:
				draw_logo_centered( screen, 70 );
				menu_draw( &main_menu, screen );
				break;
			case STATE_CHART:
				chart_draw( screen );
				break;
			case STATE_HELP:
				draw_help( screen );
				break;
		}
		
		/* add cursor */
		draw_cursor( screen, x, y );
		
		/* udpate screen */
		refresh_screen(screen);
		frames++;

		/* end game? */
		if ( state == STATE_GAME )
		if ( time(0) >= game_end ) {
			game_finalize( 1 /*check highscore*/ );
			state = STATE_CHART;
			input_delay = INPUT_DELAY;
			cur_ticks = prev_ticks = SDL_GetTicks(); /* reset timer */
		}
		
		/* after this point only input is handled */
		if ( input_delay > 0 ) continue;
		
		/* handle input */
		switch ( state ) {
		    case STATE_HELP:
		    case STATE_CHART:
			if ( keystate[SDLK_ESCAPE] ||
			     keystate[SDLK_SPACE] || 
			     keystate[SDLK_RETURN] || 
			     keystate[SDLK_SPACE] ||
			     buttonstate ) {
				input_delay = INPUT_DELAY;
				state = STATE_MENU;
				/* clear the selection of the menu entry */
				menu_handle_motion( &main_menu, -1, -1 );
			}
			break;
		    case STATE_GAME:
			/* abort game */
			if ( keystate[SDLK_ESCAPE] ) {
				game_finalize( 0/*don't check highscore*/ );
				cur_ticks = prev_ticks = SDL_GetTicks(); /* reset timer */
				state = STATE_CHART;
				input_delay = INPUT_DELAY;
			}
			/* set direction of gun */
			bfield_update_gun_dir( x, y );
			/* fire gun */
			if ( bfield_gun_is_ready() ) {
				dest.x = x; dest.y = y; dest.z = 0;
				shot_type = ST_NONE;
				if ( buttonstate & SDL_BUTTON(1) ) {
					if ( modstate & (KMOD_LCTRL|KMOD_RCTRL) )
						shot_type = ST_GRENADE;
					else
						shot_type = ST_BOMB;
				}
				if ( buttonstate & SDL_BUTTON(2) )
					shot_type = ST_GRENADE;
				if ( shot_type != ST_NONE )
					bfield_fire_gun( shot_type, &dest, 60, 0 );
			}
			/* reload */
			if ( buttonstate & SDL_BUTTON(3) || keystate[reload_key] ) {
				if ( !reload_clicked ) {
					reload_clicked = 1;
					bfield_reload_gun();
				}
			}
			else
				reload_clicked = 0;
			break;
		    case STATE_MENU:
			if ( keystate[SDLK_ESCAPE] )
				quit = 1;
			if ( buttonstate & SDL_BUTTON(1) ) {
				switch (menu_handle_click( &main_menu, x, y ) ) {
				    case ID_QUIT: 
					quit = 1; 
					break;
				    case ID_HELP:
					state = STATE_HELP;
					input_delay = INPUT_DELAY;
					break;
				    case ID_CHART: 
					state = STATE_CHART; 
					input_delay = INPUT_DELAY;
					break;
				    case ID_GAME:
					state = STATE_GAME; 
					game_init();
					break;
				}
			}
			else
				menu_handle_motion( &main_menu, x, y );
			break;
		}
		
		/* switch fullscreen/window anywhere */
		if ( keystate[SDLK_f] ) {
			fullscreen = !fullscreen;
			set_video_mode();
			input_delay = INPUT_DELAY;
		}
		/* enabe/disable sound anywhere */
		if ( keystate[SDLK_s] && audio_on != -1 ) {
			audio_on = !audio_on;
			input_delay = INPUT_DELAY;
		}
		/* take screenshots anywhere */
		if ( keystate[SDLK_TAB] )
			take_screenshot();
	}

	frame_time = SDL_GetTicks() - frame_time;
	printf( "Time: %.2f, Frames: %i\n", (double)frame_time/1000, frames );
	printf( "FPS: %.2f\n", 1000.0 * frames / frame_time );

	fade_screen( FADE_OUT, 500 );
	
}

int main( int argc, char **argv )
{
  int c;
  const SDL_VideoInfo* info;

  printf( "BARRAGE v%s\n", VERSION );
  printf( "Copyright 2003-2019 Michael Speck (http://lgames.sf.net)\n" );
  printf( "Released under GNU GPL\n---\n" );

  while ( ( c = getopt( argc, argv, "d:wsr:" ) ) != -1 )
    {
      switch (c)
	{
	case 'd': delay = atoi(optarg); break;
	case 'w': fullscreen=0; break;
	case 's': audio_on = 0; break;
	case 'r':
		sscanf(optarg, "%dx%d", &video_forced_w, &video_forced_h);
		printf("Trying to force resolution %dx%d\n",
				video_forced_w, video_forced_h);
		break;
	}
    }
  printf("main loop delay: %d ms\n",delay);
	
  srand( time(0) );

  set_player_name_from_env();
	
  if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER ) < 0 ) {
    printf( "%s\n", SDL_GetError() );
    exit(1);
  }
  else if ( SDL_InitSubSystem( SDL_INIT_AUDIO ) < 0 ) {
    printf( "%s\n", SDL_GetError() );
    printf( "Disabling sound and continuing...\n" );
    audio_on = -1;
  }

  /* get real display size now because when switching
   * it becomes wrong window size */
  info = SDL_GetVideoInfo();
  printf("Display resolution: %d x %d\n", info->current_w, info->current_h);
  display_w = info->current_w;
  display_h = info->current_h;
  if (video_forced_h > 0) {
	  if (video_forced_w > display_w || video_forced_h > display_h) {
		  printf("Forced resolution out of bounds, ignoring it\n");
		  video_forced_h = video_forced_w = 0;
	  }
	  if (video_forced_w < 640 || video_forced_h < 480) {
		  video_forced_w = 0;
		  video_forced_h = 0;
	  }
  }


  set_video_mode();

  atexit(SDL_Quit);
  SDL_WM_SetCaption( "LBarrage", 0 );
	
#ifdef AUDIO_ENABLED	
  if ( Mix_OpenAudio( audio_freq, audio_format, audio_channels, 1024 ) < 0 ) {
    printf( "%s\n", SDL_GetError() );
    printf("Disabling sound and continuing\n");
    audio_on = -1;
  }
  audio_mix_channel_count = Mix_AllocateChannels( 16 );
#endif
	
  atexit(data_delete); data_load();
  chart_load();

  SDL_SetCursor( cr_empty );
  /* create simple menu */
  menu_init( &main_menu, 230, 40 );
  menu_add_entry( &main_menu, "Enter Shooting Range", ID_GAME );
  menu_add_entry( &main_menu, "Receive Briefing", ID_HELP );
  menu_add_entry( &main_menu, "Topgunners", ID_CHART );
  menu_add_entry( &main_menu, "", 0 );
  menu_add_entry( &main_menu, "Quit", ID_QUIT );
	
  /* run game */
  main_loop();

#ifdef AUDIO_ENABLED
  Mix_CloseAudio();
#endif
	
  return (0);
}
