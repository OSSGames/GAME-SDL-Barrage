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

#include "defs.h"
#include "data.h"

SDL_Surface *img_particles = 0;
SDL_Surface *img_ground = 0;
SDL_Surface *img_crater = 0;
SDL_Surface *img_small_crater = 0;
SDL_Surface *img_units = 0;
SDL_Surface *img_trees = 0;
SDL_Surface *img_shots = 0;
SDL_Surface *img_ammo = 0;
SDL_Surface *img_logo = 0;
SDL_Surface *img_icons = 0;
SDL_Surface *img_black = 0;
SDL_Surface *background = 0;
SDL_Surface *img_cursors = 0;
SDL_Surface *img_gun = 0;

SDL_Font *ft_main = 0;
SDL_Font *ft_chart = 0, *ft_chart_highlight = 0;
SDL_Font *ft_menu = 0, *ft_menu_highlight = 0;
SDL_Font *ft_help = 0;
SDL_Font *ft_copyright = 0;

SDL_Sound *wav_expl1 = 0; /* grenade explosion */
SDL_Sound *wav_expl2 = 0; /* (cluster)bomb explosion */
SDL_Sound *wav_expl3 = 0; /* tank explosion */
SDL_Sound *wav_cannon1 = 0; /* grenade autocannon */
SDL_Sound *wav_cannon2 = 0;
SDL_Sound *wav_click = 0;
SDL_Sound *wav_highlight = 0;

SDL_Cursor *cr_empty = 0;

int img_count = 0, wav_count = 0, ft_count = 0;
int ammo_w = 0, ammo_h = 0; /* size of an ammo tile */
int cursor_w = 0, cursor_x_offset = 0;
int gun_w = 0, gun_h = 0;

extern int audio_on;

extern SDL_Surface *screen; /* display */

#define SET_ALPHA( surf, alpha ) SDL_SetAlpha( surf, SDL_SRCALPHA | SDL_RLEACCEL, alpha )
#define SET_CKEY( surf, ckey ) SDL_SetColorKey( surf, SDL_SRCCOLORKEY, ckey )

static void data_free_cursor( SDL_Cursor **cursor )
{
	if ( *cursor ) {
		SDL_FreeCursor( *cursor );
		*cursor = 0;
	}
}

static SDL_Cursor* data_create_cursor( int width, int height, int hot_x, int hot_y, char *source )
{
	unsigned char *mask = 0, *data = 0;
	SDL_Cursor *cursor = 0;
	int i, j, k;
	unsigned char data_byte, mask_byte;
	int pot;

	/* meaning of char from source: b : black, w: white, ' ':transparent */

	mask = malloc( width * height * sizeof ( char ) / 8 );
	data = malloc( width * height * sizeof ( char ) / 8 );

	k = 0;
	for (j = 0; j < width * height; j += 8, k++) {
		pot = 1;
		data_byte = mask_byte = 0;
		/* create byte */
		for (i = 7; i >= 0; i--) {
			switch ( source[j + i] ) {
				case 'b': data_byte += pot;
				case 'w': mask_byte += pot; break;
			}
			pot *= 2;
		}
		/* add to mask */
		data[k] = data_byte;
		mask[k] = mask_byte;

	}
	/* create and return cursor */
	cursor = SDL_CreateCursor( data, mask, width, height, hot_x, hot_y );
	free( mask );
	free( data );
	return cursor;
}

static void data_free_image( SDL_Surface **surf )
{
	if ( *surf ) {
		SDL_FreeSurface( *surf );
		*surf = 0;
	}
}

static SDL_Surface *data_create_image( int w, int h )
{
	SDL_Surface *surf = 0;
	printf( "creating %ix%i bitmap ... ", w, h );
	surf = SDL_CreateRGBSurface( 
			SDL_SWSURFACE, w, h, 
			screen->format->BitsPerPixel,
			screen->format->Rmask, screen->format->Bmask,
			screen->format->Gmask, screen->format->Amask );
	if ( surf == 0 ) {
		printf( "%s\n", SDL_GetError() );
		exit(1);
	}
	img_count++; /* statistics */
	printf( "ok\n" );
	return surf;
}

static SDL_Surface *data_load_image( char *name )
{
	char path[128];
	SDL_Surface *tmp = 0, *surf = 0;
	snprintf( path, 128, "%s/gfx/%s", SRC_DIR, name ); path[127] = 0;
	printf( "loading %s ... ", path );
	tmp = SDL_LoadBMP( path );
	if ( tmp == 0 || (surf = SDL_DisplayFormat(tmp)) == 0 ) {
		printf( "%s\n", SDL_GetError() );
		data_free_image( &tmp );
		exit(1);
	}
	data_free_image( &tmp );
	img_count++; /* statistics */
	printf( "ok\n" );
	return surf;
}

static void data_free_font( SDL_Font **font )
{
	if ( *font ) {
		data_free_image( &(*font)->Surface );
		free( *font );
		*font = 0;
	}
}

static SDL_Font* data_load_font( char *name )
{
	SDL_Font *font = calloc( 1, sizeof( SDL_Font ) );
	if ( font == 0 ) { printf( "out of memory\n" ); exit(1); }
	font->Surface = data_load_image( name );
	InitFont2( font );
	ft_count++; /* statistics */
	return font;
}

#ifdef AUDIO_ENABLED
static void data_free_sound( SDL_Sound **wav )
{
	if ( *wav ) {
		Mix_FreeChunk( *wav );
		*wav = 0;
	}
}

static SDL_Sound *data_load_sound( char *name )
{
	char path[128];
	SDL_Sound *wav = 0;
	snprintf( path, 128, "%s/sounds/%s", SRC_DIR, name ); path[127] = 0;
	printf( "loading %s ... ", path );
	wav = Mix_LoadWAV( path ); /* convets already */
	if ( wav == 0 ) {
		printf( "%s\n", SDL_GetError() );
		exit(1);
	}
	wav_count++; /* statistics */
	printf( "ok\n" );
	return wav;
}
#endif

void data_load()
{
	img_ground = data_load_image( "ground.bmp" );
	img_particles = data_load_image( "particles.bmp" );
	SET_CKEY( img_particles, 0x0 );
	img_crater = data_load_image("crater.bmp" );
	SET_CKEY( img_crater, 0x0 ); SET_ALPHA( img_crater, 196 );
	img_small_crater = data_load_image( "small_crater.bmp" );
	SET_CKEY( img_small_crater, 0x0 ); SET_ALPHA( img_small_crater, 196 );
	img_units = data_load_image( "units.bmp" );
	SET_CKEY( img_units, 0x0 );
	img_trees = data_load_image( "trees.bmp" );
	SET_CKEY( img_trees, 0x0 );
	img_shots = data_load_image( "shots.bmp" );
	SET_CKEY( img_shots, 0x0);
	img_ammo = data_load_image( "ammo.bmp" );
	SET_CKEY( img_ammo, 0x0 );
	ammo_w = img_ammo->w; ammo_h = img_ammo->h / AMMO_BOMB;
	img_logo = data_load_image( "logo.bmp" );
	SDL_LockSurface( img_logo );
	SET_CKEY( img_logo, *(Uint16*)(img_logo->pixels) );
	SDL_UnlockSurface( img_logo );
	img_icons = data_load_image( "icons.bmp" );
	img_black = data_create_image( screen->w, screen->h );
	SET_ALPHA( img_black, 128 );
	background = data_create_image( screen->w, screen->h ); 
	img_cursors = data_load_image( "cursors.bmp" );
	SET_CKEY( img_cursors, 0x0 );
	cursor_w = img_cursors->w / CURSOR_COUNT;
	img_gun = data_load_image( "gun.bmp" );
	SDL_LockSurface( img_gun );
	SET_CKEY( img_gun, *(Uint16*)(img_gun->pixels) );
	SDL_UnlockSurface( img_gun );
	gun_w = img_gun->w / 9; gun_h = img_gun->h;
	
	ft_main = data_load_font( "ft_red.bmp" );
	ft_chart = data_load_font( "ft_red.bmp" );
	ft_chart_highlight = data_load_font( "ft_yellow.bmp" );
	ft_help = data_load_font( "ft_red14.bmp" );
	ft_copyright = data_load_font( "ft_neon.bmp" );
	/* use the same fonts for menu */
	ft_menu = ft_chart; ft_menu_highlight = ft_chart_highlight;
	
#ifdef AUDIO_ENABLED
    if ( audio_on != -1 ) {
	wav_expl1 = data_load_sound( "expl1.wav" );
	wav_expl2 = data_load_sound( "expl2.wav" );
	wav_expl3 = data_load_sound( "expl3.wav" );
	wav_cannon1 = data_load_sound( "autocannon.wav" );
	wav_cannon2 = data_load_sound( "gunfire.wav" );
	wav_click = data_load_sound( "click.wav" );
	wav_highlight = data_load_sound( "highlight.wav" );
    }
#endif

	cr_empty = data_create_cursor( 16, 16, 8, 8,
			"                "
			"                "
			"                "
			"                "
			"                "
			"                "
			"                "
			"                "
			"                "
			"                "
			"                "
			"                "
			"                "
			"                "
			"                "
			"                " );
}

void data_delete()
{
	data_free_image( &img_particles );
	data_free_image( &img_ground );
	data_free_image( &img_crater );
	data_free_image( &img_small_crater );
	data_free_image( &img_units );
	data_free_image( &img_trees );
	data_free_image( &img_shots );
	data_free_image( &img_ammo );
	data_free_image( &img_logo );
	data_free_image( &img_icons );
	data_free_image( &img_black );
	data_free_image( &background );
	data_free_image( &img_cursors );
	data_free_image( &img_gun );
	printf( "%i images deleted\n", img_count );

	data_free_font( &ft_main );
	data_free_font( &ft_chart );
	data_free_font( &ft_chart_highlight );
	data_free_font( &ft_help );
	data_free_font( &ft_copyright );
	printf( "%i fonts deleted\n", ft_count );
	
#ifdef AUDIO_ENABLED
    if ( audio_on != -1 ) {
	data_free_sound( &wav_expl1 );
	data_free_sound( &wav_expl2 );
	data_free_sound( &wav_expl3 );
	data_free_sound( &wav_cannon1 );
	data_free_sound( &wav_click );
	data_free_sound( &wav_highlight );
	printf( "%i sounds deleted\n", wav_count );
    }
#endif

	data_free_cursor( &cr_empty );
}
