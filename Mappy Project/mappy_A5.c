#ifdef __cplusplus
#error "Hey, stop compiling mappyal.c as C++!, see MappyAL readme.txt"
#endif
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mappy_A5.h"

/* NOTES: 
	1. all drawing uses the current target bitmap using, set beforehand if this is to change
	2. all drawing uses standard create bitmaps which are video by default in A5 unles flags have been set: 
		call al_set_new_bitmap_flags, etc to alter this
*/

/* public facing functions
	MapRestore				: puts pixels back into bitmaps. Useful if video memory is wiped, e.g. alt-tabbing. but usually A5 handles this
	MapMakeParallaxBitmap	: 
*/


/* ordering of data loading/conversion

	MapLoad:	MapRealLoad			-> MapRealDecode -> MapRelocate -> MapRelocate2
	MapDecode:	MapPreRealDecode	-> MapRealDecode -> MapRelocate -> MapRelocate2

	RealLoad/RealDecode		: only read in data and verify, they do no other work

	1. mapblockgfxpt stores loaded integers representing each pixel (MapRealLoad/Decode)
	2. integers are converted to the correct bitmap depth and mapblockgfxpt updated (MapRelocate)
	3. bitmaps are created from mapblockgfxpt. mapblockgfxpt is kept for map restore (MapRelocate2)

	MapRestore: updates all bitmaps with data from mapblockgfxpt

*/

/* public stuff for public access that might be useful */
int maperror;								/* Set to a MER_ error if something wrong happens */
short int mapwidth, mapheight;				/* width/height in tiles */
short int mapblockwidth, mapblockheight;	/* size in pixels of each tile */
short int mapdepth;							/* colour depth of map - not necessarily the target bitmap */
short int ** maparraypt = NULL;				/* map data, i.e. represents the tiles shown for each location - can be used to locate tiles directly, e.g. maparraypt[x][y] */
int mapblockgapx, mapblockgapy, mapblockstaggerx, mapblockstaggery; /* for computing offsets for block locations */
/* */

/* internal variables */
static short int mapblockstrsize, mapnumblockstr, mapnumblockgfx;
static int mapaltdepth;						/* not really used. was AGFX (8 bit in 24 bit screen), but not supported in A5 version */
static int maptype, mapislsb;

static ALLEGRO_BITMAP ** abmTiles = NULL;
static ALLEGRO_STATE state;
static bool stateSet=false;

static char * mapcmappt = NULL;				/* if map is 8bit this is the raw palette (i.e. 256 integers representing colours) */
static ANISTR * mapanimstrpt = NULL;		/* animation data */
static char mapnovctext[80];
static char * mapblockgfxpt = NULL;										/* the raw graphics stored as sequential pixel data */
static char * mapblockstrpt = NULL;										/* the tile data */
static int * mapanimseqpt = NULL;											/* the animation data */
static ANISTR * mapanimstrendpt;											/* end of animation data */
static short int * mappt = NULL;											/* pointer to the current map layer */
static short int * mapmappt[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };	/* all layers of map, use mappt normally */
static short int ** mapmaparraypt[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }; /* pointers to layers, use mapmappt normally */
/* end of internal variables */


/* MAP LOADING/INIT/SHUTDOWN FUNCTIONS */

/*	LOAD map, uses al_fopen so may use physfs 
	supports 8,15,16,24,32 bpp maps and translates to current screen format after loading
	if convertToAlpha then 
		if 8bpp any pixel matching black or magic pink will be changed to alpha
		if 15,16,24,32bpp any black pixel will be changed to alpha
*/
int MapLoad (char * mapname, const int convertToAlpha)
{
	int retVal;
	retVal=MapRealLoad (mapname,convertToAlpha);

	MapInitAnims();	//should be ok to call even if faulre

	return retVal;
}

/* as MapLoad but takes data from memory rather than file/physfs location */
int MapDecode (unsigned char * mapmempt, const int convertMagicPink)
{
	int retVal;
	retVal=MapPreRealDecode (mapmempt,convertMagicPink);

	MapInitAnims();	//should be ok to call even if faulre

	return retVal;
}

/* if video memory is used and may be corrupted after switching (e.g. Windows) this
	recreates all bitmaps
*/
void MapRestore (void)
{
	int i, j, k;
	int r,g,b;
	unsigned char * newgfxpt;

	if (abmTiles == NULL) return;
	i = 0; newgfxpt = (unsigned char*)mapblockgfxpt;
	while (abmTiles[i]!=NULL) 
	{
		//not needed for real-time stuff so don't bother locking

		PushTargetDisplayAndBitmap(abmTiles[i]);
		al_lock_bitmap(abmTiles[i],al_get_bitmap_format(abmTiles[i]),ALLEGRO_LOCK_WRITEONLY);
		for (k=0;k<mapblockheight;k++) 
		{
			for (j=0;j<mapblockwidth;j++) 
			{
				switch (mapdepth) 
				{
					case 8:
						itoC8((int)newgfxpt,&r,&g,&b);
						al_put_pixel(j, k, al_map_rgb(r,g,b));
						newgfxpt++;
						break;
					case 15:
					case 16:
						itoC16((int)newgfxpt,&r,&g,&b);
						al_put_pixel(j, k, al_map_rgb(r,g,b));
						newgfxpt+=2;
						break;
					case 24:
						al_put_pixel (j, k, al_map_rgb(newgfxpt[0], newgfxpt[1], newgfxpt[2]));
						newgfxpt+=3;
						break;
					case 32:
						al_put_pixel (j, k, al_map_rgb (newgfxpt[1], newgfxpt[2], newgfxpt[3]));
						newgfxpt+=4;
						break;
				} 
			} 
		}
		al_unlock_bitmap(abmTiles[i]);
		PopTargetDisplayAndBitmap();
		i++;
	}
}

/* mar files just contain tile data, i.e. no graphics or animations and work 
	equivalent to nonMAR functions, e.g. MapLoadMar -> MapLoad
*/
int MapLoadMAR (char * mname, int marlyr)
{
	int i, j;
	short int * mymarpt;
	//PACKFILE * marfpt;
	void* marfpt;

	if (marlyr < 0 || marlyr > 7)return -1;

	marfpt = packed_fopen (mname, "rb");
	if (marfpt==NULL)
	{ 
		return -1; 
	} 

	if (mapmappt[marlyr] == NULL)
		mapmappt[marlyr] = malloc (mapwidth*mapheight*sizeof(short int));

	if (packed_fread (mapmappt[marlyr], (mapwidth*mapheight*sizeof(short int)), marfpt) !=(mapwidth*mapheight*sizeof(short int))) 
	{
		packed_fclose (marfpt); 
		return -1; 
	}

	packed_fclose (marfpt);

	mymarpt = mapmappt[marlyr];
	j = 0; for (i=0;i<(mapwidth*mapheight);i++) 
	{ 
		if (mymarpt[i]&0xF) 
			j = 1; 
	}
	if (j == 0) 
	{
		for (i=0;i<(mapwidth*mapheight);i++) 
		{
			if (mymarpt[i] >= 0)
				mymarpt[i] /= 32;
			else 
				mymarpt[i] /= 16;
		}
	}
	MapGenerateYLookup();
	MapInitAnims();	//should be ok to call even if failure

	return 0;
}

int MapDecodeMAR (unsigned char * mrpt, int marlyr, int initAnims)
{
int i, j;
short int * mymarpt;

	if (marlyr < 0 || marlyr > 7)
		return -1;

	if (mapmappt[marlyr] == NULL)
		mapmappt[marlyr] = malloc (mapwidth*mapheight*sizeof(short int));

	memcpy (mapmappt[marlyr], mrpt, (mapwidth*mapheight*sizeof(short int)));
	mymarpt = mapmappt[marlyr];
	j = 0; 

	for (i=0;i<(mapwidth*mapheight);i++) 
	{
		if (mymarpt[i]&0xF) 
			j = 1; 
	}

	if (j == 0) 
	{
		for (i=0;i<(mapwidth*mapheight);i++)
		{
			if (mymarpt[i] >= 0)
				mymarpt[i] /= 32;
			else 
				mymarpt[i] /= 16;
		}
	}

	if(initAnims) MapInitAnims();	//should be ok to call even if faulre

	return 0;
}

/* initialise any animations in the map. This is called automatically so 
	only use is to reset everything
*/
void MapInitAnims (void)
{
	ANISTR * myanpt;
	if (mapanimstrpt==NULL) 
		return;
	myanpt = (ANISTR *) mapanimstrendpt; myanpt--;

	while (myanpt->antype!=-1)
	{
		if (myanpt->antype==AN_PPFR) myanpt->antype = AN_PPFF;
		if (myanpt->antype==AN_PPRF) myanpt->antype = AN_PPRR;
		if (myanpt->antype==AN_ONCES) myanpt->antype = AN_ONCE;
		if ((myanpt->antype==AN_LOOPR) || (myanpt->antype==AN_PPRR))
		{
			myanpt->ancuroff = myanpt->anstartoff;
			if ((myanpt->anstartoff)!=(myanpt->anendoff)) 
				myanpt->ancuroff=(myanpt->anendoff)-1;
		} 
		else 
		{
			myanpt->ancuroff = myanpt->anstartoff;
		}
		myanpt->ancount = myanpt->andelay;
		myanpt--;
	}
}

/* clear up memory, i.e. call this when finished with map
	called automatically during mapload */
void MapFreeMem (void)
{
	int i;
	for (i=0;i<8;i++) 
	{ 
		if (mapmappt[i]!=NULL) 
		{ 
			free (mapmappt[i]); mapmappt[i] = NULL;
		} 
	}
	mappt = NULL;

	for (i=0;i<8;i++) 
	{ 
		if (mapmaparraypt[i]!=NULL) 
		{
			free (mapmaparraypt[i]); 
			mapmaparraypt[i] = NULL;
		}
	}
	maparraypt = NULL;

	if (mapcmappt!=NULL) 
	{ 
		free (mapcmappt); 
		mapcmappt = NULL; 
	}
	if (mapblockgfxpt!=NULL) 
	{ 
		free (mapblockgfxpt);
		mapblockgfxpt = NULL; 
	}
	if (mapblockstrpt!=NULL)
	{ 
		free (mapblockstrpt);
		mapblockstrpt = NULL; 
	}
	if (mapanimseqpt!=NULL) 
	{ 
		free (mapanimseqpt);
		mapanimseqpt = NULL; 
	}
	if (mapanimstrpt!=NULL)
	{
		free (mapanimstrpt); 
		mapanimstrpt = NULL; 
	}
	if (abmTiles != NULL) 
	{
		i = 0; 
		while (abmTiles[i]!=NULL)
		{ 
			al_destroy_bitmap (abmTiles[i]); 
			i++; 
		}
		free (abmTiles);
		abmTiles = NULL;
	}

	mapnovctext[0] = 0;
}

/* MAP MANIPULATION FUNCTION*/
/* change to the new layer (0 being first, 7 being last) */
int MapChangeLayer (int newlyr)
{
	if (newlyr<0 || newlyr>7 || mapmappt[newlyr] == NULL) 
		return -1;
	mappt = mapmappt[newlyr]; 
	maparraypt = mapmaparraypt[newlyr];
	return newlyr;
}

/* call at every logic update to update each animation in the map */
void MapUpdateAnims (void)
{
	ANISTR * myanpt;

	if (mapanimstrpt==NULL) 
		return;

	myanpt = (ANISTR *) mapanimstrendpt; myanpt--;

	while (myanpt->antype!=-1)
	{
		if (myanpt->antype!=AN_NONE) 
		{ 
			myanpt->ancount--; 
			if (myanpt->ancount<0) 
			{
				myanpt->ancount = myanpt->andelay;
				if (myanpt->antype==AN_LOOPF)
				{
					if (myanpt->anstartoff!=myanpt->anendoff) 
					{ 
						myanpt->ancuroff++;
						if (myanpt->ancuroff==myanpt->anendoff) 
							myanpt->ancuroff = myanpt->anstartoff;
					} 
				}

				//
				if (myanpt->antype==AN_LOOPR)
				{
					if (myanpt->anstartoff!=myanpt->anendoff) 
					{ 
						myanpt->ancuroff--;
						if (myanpt->ancuroff==((myanpt->anstartoff)-1))
							myanpt->ancuroff = (myanpt->anendoff)-1;
					} 
				}

				//
				if (myanpt->antype==AN_ONCE)
				{
					if (myanpt->anstartoff!=myanpt->anendoff)
					{ myanpt->ancuroff++;
						if (myanpt->ancuroff==myanpt->anendoff)
						{ 
							myanpt->antype = AN_ONCES;
							myanpt->ancuroff = myanpt->anstartoff; 
						}
					} 
				}

				if (myanpt->antype==AN_ONCEH)
				{
					if (myanpt->anstartoff!=myanpt->anendoff) 
					{
						if (myanpt->ancuroff!=((myanpt->anendoff)-1)) 
							myanpt->ancuroff++;
					} 
				}

				//
				if (myanpt->antype==AN_PPFF)
				{
					if (myanpt->anstartoff!=myanpt->anendoff) 
					{ 
						myanpt->ancuroff++;
						if (myanpt->ancuroff==myanpt->anendoff)
						{ 
							myanpt->ancuroff -= 2;
							myanpt->antype = AN_PPFR;
							if (myanpt->ancuroff<myanpt->anstartoff) 
								myanpt->ancuroff++; 
						}
					} 
				} 
				else
				{
					if (myanpt->antype==AN_PPFR)
					{
						if (myanpt->anstartoff!=myanpt->anendoff) 
						{
							myanpt->ancuroff--;
							if (myanpt->ancuroff==((myanpt->anstartoff)-1)) 
							{ 
								myanpt->ancuroff += 2;
								myanpt->antype = AN_PPFF;
								if (myanpt->ancuroff>myanpt->anendoff) 
									myanpt->ancuroff --; 
							}
						}
					} 
				}

				//
				if (myanpt->antype==AN_PPRR)
				{
					if (myanpt->anstartoff!=myanpt->anendoff) 
					{ 
						myanpt->ancuroff--;
						if (myanpt->ancuroff==((myanpt->anstartoff)-1)) 
						{ 
							myanpt->ancuroff += 2;
							myanpt->antype = AN_PPRF;
							if (myanpt->ancuroff>myanpt->anendoff) 
								myanpt->ancuroff--; 
						}
					} 
				} 
				else
				{
					if (myanpt->antype==AN_PPRF)
					{
						if (myanpt->anstartoff!=myanpt->anendoff) 
						{ myanpt->ancuroff++;
							if (myanpt->ancuroff==myanpt->anendoff) 
							{ 
								myanpt->ancuroff -= 2;
								myanpt->antype = AN_PPRR;
								if (myanpt->ancuroff<myanpt->anstartoff) 
									myanpt->ancuroff++; 
							}
						} 
					}
				}
			}
		} 
		myanpt--;
	}
}


/* BLOCK/TILE FUNCTIONS */
/* to help convert pixels to blocks/tiles */
int MapGetXOffset (int xpix, int ypix)
{
	int xb;

	if (mapblockstaggerx || mapblockstaggery) 
	{
		xpix += (mapblockstaggerx);
		ypix += (mapblockstaggery);
	}
	xb = xpix/mapblockgapx;

	if (xb < 0) xb = 0;
	if (xb >= mapwidth) xb = mapwidth-1;
	return xb;
}

/* to help convert pixels to blocks/tiles */
int MapGetYOffset (int xpix, int ypix)
{
int yb;

	if (mapblockstaggerx || mapblockstaggery) 
	{
		xpix += (mapblockstaggerx);
		ypix += (mapblockstaggery);
	}
	yb = ypix/mapblockgapy;

	if (yb < 0) yb = 0;
	if (yb >= mapheight) yb = mapheight-1;
	return yb;
}

/* get the block at x,y pixel location from the top-left of the map */
BLKSTR * MapGetBlockInPixels (int x, int y)
{
int xp, yp;
short int * mymappt;
ANISTR * myanpt;

	if (x < 0 || y < 0 || x >= (mapwidth*mapblockwidth) || y >= (mapheight*mapblockheight))
		return NULL;

	xp = x; yp = y;
	x = MapGetXOffset (xp, yp);
	y = MapGetYOffset (xp, yp);

	if (maparraypt!= NULL)
	{
		mymappt = maparraypt[y]+x;
	} 
	else
	{
		mymappt = mappt;
		mymappt += x;
		mymappt += y*mapwidth;
	}
	if (*mymappt>=0)
		return ((BLKSTR*) mapblockstrpt) + *mymappt;
	else
	{
		myanpt = mapanimstrendpt + *mymappt;
		return ((BLKSTR *) mapblockstrpt) + mapanimseqpt[myanpt->ancuroff]; 
	}
}

/* get block at x,y block/tile location from the top-left of the map */
BLKSTR * MapGetBlock (int x, int y)
{
	short int * mymappt;
	ANISTR * myanpt;

	if (maparraypt!= NULL)
	{
		mymappt = maparraypt[y]+x;
	} 
	else
	{
		mymappt = mappt;
		mymappt += x;
		mymappt += y*mapwidth;
	}

	if (*mymappt>=0)
		return ((BLKSTR*) mapblockstrpt) + *mymappt;
	else 
	{ 
		myanpt = mapanimstrendpt + *mymappt;
		return ((BLKSTR *) mapblockstrpt) + mapanimseqpt[myanpt->ancuroff]; 
	}
}

/* as MapGetBlockInPixels but sets a new block */
void MapSetBlockInPixels (int x, int y, int strvalue)
{
int xp, yp;
short int * mymappt;

	if (x < 0 || y < 0 || x >= (mapwidth*mapblockwidth) || y >= (mapheight*mapblockheight))
		return;

	xp = x; yp = y;
	x = MapGetXOffset (xp, yp);
	y = MapGetYOffset (xp, yp);

	if (maparraypt!= NULL)
	{
		mymappt = maparraypt[y]+x;
	} 
	else 
	{
		mymappt = mappt;
		mymappt += x;
		mymappt += y*mapwidth;
	}
	*mymappt = strvalue;
}

/* as MapSetBlockInPixels but sets a block at pixel location */
void MapSetBlock (int x, int y, int strvalue)
{
short int * mymappt;

	if (maparraypt!= NULL) 
	{
		mymappt = maparraypt[y]+x;
	} 
	else 
	{
		mymappt = mappt;
		mymappt += x;
		mymappt += y*mapwidth;
	}
	*mymappt = strvalue;
}


/* get first block that matches value 'blid' in the user defined fields 'usernum' */
int MapGetBlockID (int blid, int usernum)
{
int i;
BLKSTR * myblkpt;

	myblkpt = (BLKSTR *) mapblockstrpt;
	if (myblkpt == NULL) return -1;

	for (i=0;i<mapnumblockstr;i++) 
	{
		switch (usernum) 
		{
			case 1:
				if (myblkpt[i].user1 == blid) return i;
				break;
			case 2:
				if (myblkpt[i].user2 == blid) return i;
				break;
			case 3:
				if (myblkpt[i].user3 == blid) return i;
				break;
			case 4:
				if (myblkpt[i].user4 == blid) return i;
				break;
			case 5:
				if (myblkpt[i].user5 == blid) return i;
				break;
			case 6:
				if (myblkpt[i].user6 == blid) return i;
				break;
			case 7:
				if (myblkpt[i].user7 == blid) return i;
				break;
		}
	}

	return -1;
}


/* BITMAP FUNCTIONS */

/* if using parallax scrolling, call this to convert bitmap ready for parallax scrolling */
ALLEGRO_BITMAP * MapMakeParallaxBitmap (ALLEGRO_BITMAP * sourcebm)
{
	ALLEGRO_BITMAP * newbm;
	ALLEGRO_BITMAP* temp;

	int w,h;
	if (mappt == NULL) return NULL;

	w=al_get_bitmap_width(sourcebm);
	h=al_get_bitmap_height(sourcebm);
	newbm = al_create_bitmap (w+mapblockwidth,h+mapblockheight);

	if (newbm == NULL) 
		return NULL;

	PushTargetDisplayAndBitmap(newbm);
	al_draw_bitmap(sourcebm,0,0,0);	//whole bitmap copy
	al_draw_bitmap_region(sourcebm,0,0,w,mapblockheight,0,h,0);

	al_set_target_bitmap(newbm);
	al_draw_bitmap(sourcebm, 0,0,0);

	temp=al_create_bitmap(mapblockwidth,h+mapblockheight);
	al_set_target_bitmap(temp);
	al_draw_bitmap(newbm,0,0,0);

	al_set_target_bitmap(newbm);
	al_draw_bitmap_region(temp,0,0,mapblockwidth,h+mapblockheight,w,0,0);
	al_destroy_bitmap(temp);

	PopTargetDisplayAndBitmap();
	return newbm;
}

/* draw parallax graphic */
void MapDrawParallax (ALLEGRO_BITMAP * parbm, int mapxo, int mapyo, int mapx, int mapy, int mapw, int maph)
/* requires target bitmap to be set, usually backbuffer
 * parbm = standard allegro bitmap. 
 * mapxo = offset, in pixels, from the left edge of the map.
 * mapyo = offset, in pixels, from the top edge of the map.
 * mapx  = offset, in pixels, from the left edge of the BITMAP.
 * mapy  = offset, in pixels, from the top edge of the BITMAP.
 * mapw  = width, in pixels, of drawn area.
 * maph  = height, in pixels, of drawn area.
 */
{
	int mycl, mycr, myct, mycb;
	int i, i2, j;
	int paraxo, paraxo2, parayo;
	short int * mymappt, * mymappt2;
	int w,h;
	BLKSTR * blkdatapt;
	ANISTR * myanpt;
	ALLEGRO_BITMAP* mapdestpt;

	mapdestpt=PushTargetDisplayAndBitmap(NULL);

	if (mapblockstaggerx || mapblockstaggery) return;
	al_get_clipping_rectangle(&mycl,&mycr,&myct,&mycb);
	al_set_clipping_rectangle(mapx,mapy,mapw,maph);//	set_clip (mapdestpt, mapx, mapy, mapx+mapw-1, mapy+maph-1);

	mymappt = (short int *) mappt;
	mymappt += (mapxo/mapblockwidth)+((mapyo/mapblockheight)*mapwidth);

	w=al_get_bitmap_width(parbm);
	h=al_get_bitmap_height(parbm);

	paraxo = ((mapxo-(mapxo%mapblockwidth))%(w-mapblockwidth))-((mapxo/2)%(w-mapblockwidth));
	parayo = ((mapyo-(mapyo%mapblockheight))%(h-mapblockheight))-((mapyo/2)%(h-mapblockheight));
	while (paraxo < 0) paraxo += (w-mapblockwidth);
	while (parayo < 0) parayo += (h-mapblockheight);

	i = mapx-(mapxo%mapblockwidth);
	j = mapy-(mapyo%mapblockheight);

	i2 = i; paraxo2 = paraxo; mymappt2 = mymappt;
	while (j < (mapy+maph)) 
	{
		while (i < (mapx+mapw)) 
		{
			if (*mymappt>=0) 
				blkdatapt = ((BLKSTR*) mapblockstrpt) + *mymappt;
			else 
			{ 
				myanpt = mapanimstrendpt + *mymappt;
				blkdatapt = ((BLKSTR *) mapblockstrpt) + mapanimseqpt[myanpt->ancuroff]; 
			}
			if (blkdatapt->trigger)
				al_draw_bitmap_region(parbm,paraxo,parayo,mapblockwidth,mapblockheight,i,j,0); //blit (parbm, mapdestpt, paraxo, parayo, i, j, mapblockwidth, mapblockheight);
			
			paraxo += mapblockwidth;
			if (paraxo >= (w-mapblockwidth)) 
				paraxo -= (w-mapblockwidth);

			i += mapblockwidth; 
			mymappt++;
		}
		parayo += mapblockheight;
		if (parayo >= (h-mapblockheight)) 
			parayo -= (h-mapblockheight);
		i = i2; paraxo = paraxo2; mymappt2 += mapwidth; mymappt = mymappt2;
		j += mapblockheight;
	}
	al_set_clipping_rectangle(mycl,mycr,myct,mycb);
	PopTargetDisplayAndBitmap();
}

/* draw the background graphics on the current layer to the target bitmap, usually backbuffer */
void MapDrawBG (int mapxo, int mapyo, int mapx, int mapy,int mapw, int maph)
{
	int i, j, mycl, mycr, myct, mycb, mapvclip, maphclip;
	int mbgx, mbgy;
	short int *mymappt;
	short int *mymap2pt;
	BLKSTR *blkdatapt;
	ANISTR *myanpt;
	ALLEGRO_BITMAP* mapdestpt;

	mapdestpt=PushTargetDisplayAndBitmap(NULL);
	al_get_clipping_rectangle(&mycl,&mycr,&myct,&mycb);
	al_set_clipping_rectangle(mapx,mapy,mapw,maph);//	set_clip (mapdestpt, mapx, mapy, mapx+mapw-1, mapy+maph-1);

	mapxo -= mapblockstaggerx;
	mapyo -= mapblockstaggery;
	mymappt = (short int *) mappt;
	if (mapblockstaggerx || mapblockstaggery) 
	{
		mymappt += (mapxo/mapblockgapx)+((mapyo/mapblockgapy)*mapwidth*2);
		mbgx = mapblockgapx;
		mbgy = mapblockgapy;
	}
	else 
	{
		mymappt += (mapxo/mapblockgapx)+((mapyo/mapblockgapy)*mapwidth);
		mbgx = 0;
		mbgy = 0;
	}
	mapvclip = mapyo%mapblockgapy;
	maphclip = mapxo%mapblockgapx;

	mymap2pt = mymappt;
	for (j=((mapy-mapvclip)-mbgy);j<((mapy+maph));j+=mapblockgapy)
	{
		for (i=((mapx-maphclip)-mbgx);i<((mapx+mapw));i+=mapblockgapx) 
		{
			if (*mymappt>=0) 
				blkdatapt = ((BLKSTR*) mapblockstrpt) + *mymappt;
			else
			{ 
				myanpt = mapanimstrendpt + *mymappt;
				blkdatapt = ((BLKSTR *) mapblockstrpt) + mapanimseqpt[myanpt->ancuroff]; 
			}
			if (mapblockstaggerx || mapblockstaggery) 
			{
			if (abmTiles[0] != (ALLEGRO_BITMAP *) blkdatapt->bgoff)
				//masked_blit ((ALLEGRO_BITMAP *) blkdatapt->bgoff, mapdestpt, 0, 0, i, j, mapblockwidth, mapblockheight);
				al_draw_bitmap((ALLEGRO_BITMAP *) blkdatapt->bgoff,i,j,0);	//size should be the same
			} 
			else 
			{
				al_draw_bitmap_region((ALLEGRO_BITMAP *) blkdatapt->bgoff,0,0,mapblockwidth,mapblockheight,i,j,0); //blit ((BITMAP *) blkdatapt->bgoff, mapdestpt, 0, 0, i, j, mapblockwidth, mapblockheight);
			}
			mymappt++;
		}
		if (mapblockstaggerx || mapblockstaggery) 
		{
			mymap2pt += mapwidth;
			mymappt = mymap2pt;
			for (i=(((mapx-maphclip)-mbgx)+mapblockstaggerx);i<((mapx+mapw));i+=mapblockgapx)
			{
				if (*mymappt>=0) 
					blkdatapt = ((BLKSTR*) mapblockstrpt) + *mymappt;
				else 
				{ 
					myanpt = mapanimstrendpt + *mymappt;
					blkdatapt = ((BLKSTR *) mapblockstrpt) + mapanimseqpt[myanpt->ancuroff]; 
				}
				if (abmTiles[0] != (ALLEGRO_BITMAP *) blkdatapt->bgoff)
					//masked_blit ((ALLEGRO_BITMAP *) blkdatapt->bgoff, mapdestpt, 0, 0, i, j+mapblockstaggery, mapblockwidth, mapblockheight);
					al_draw_bitmap((ALLEGRO_BITMAP *) blkdatapt->bgoff,i,j+mapblockstaggery,0);	//size should be the same

				mymappt++;
			}
		}
		mymap2pt += mapwidth;
		mymappt = mymap2pt;
	}
	al_set_clipping_rectangle(mycl,mycr,myct,mycb);
	PopTargetDisplayAndBitmap();
}

/* draw foreground graphics on the current layer to the target bitmap, usually backbuffer
	there are three foreground tiles available (0,1,2) for stacking (e.g. items on top of others)
*/
void MapDrawFG (int mapxo, int mapyo, int mapx, int mapy, int mapw, int maph, int mapfg)
{
	int i, j, mycl, mycr, myct, mycb, mapvclip, maphclip;
	int mbgx, mbgy;
	short int *mymappt;
	short int *mymap2pt;
	BLKSTR *blkdatapt;
	ANISTR *myanpt;
	ALLEGRO_BITMAP *mapgfxpt;
	ALLEGRO_BITMAP* mapdestpt;

	mapdestpt=PushTargetDisplayAndBitmap(NULL);

		al_get_clipping_rectangle(&mycl,&mycr,&myct,&mycb);
		al_set_clipping_rectangle(mapx,mapy,mapw,maph);//	set_clip (mapdestpt, mapx, mapy, mapx+mapw-1, mapy+maph-1);
		mapxo -= mapblockstaggerx;
		mapyo -= mapblockstaggery;
		mymappt = (short int *) mappt;
		if (mapblockstaggerx || mapblockstaggery) 
		{
			mymappt += (mapxo/mapblockgapx)+((mapyo/mapblockgapy)*mapwidth*2);
			mbgx = mapblockgapx;
			mbgy = mapblockgapy;
		} 
		else 
		{
			mymappt += (mapxo/mapblockgapx)+((mapyo/mapblockgapy)*mapwidth);
			mbgx = 0;
			mbgy = 0;
		}
		mapvclip = mapyo%mapblockgapy;
		maphclip = mapxo%mapblockgapx;

		mymap2pt = mymappt;
		for (j=((mapy-mapvclip)-mbgy);j<((mapy+maph));j+=mapblockgapy)
		{
			for (i=((mapx-maphclip)-mbgx);i<((mapx+mapw));i+=mapblockgapx)
			{
				if (*mymappt>=0) 
					blkdatapt = ((BLKSTR*) mapblockstrpt) + *mymappt;
				else 
				{ 
					myanpt = mapanimstrendpt + *mymappt;
					blkdatapt = ((BLKSTR *) mapblockstrpt) + mapanimseqpt[myanpt->ancuroff]; 
				}
				if (!mapfg)
					mapgfxpt = (ALLEGRO_BITMAP *) blkdatapt->fgoff;
				else if (mapfg == 1)
					mapgfxpt = (ALLEGRO_BITMAP *) blkdatapt->fgoff2;
				else
					mapgfxpt = (ALLEGRO_BITMAP *) blkdatapt->fgoff3;

				if (((int)mapgfxpt) != 0)
					//masked_blit (mapgfxpt, mapdestpt, 0, 0, i, j, mapblockwidth, mapblockheight);
					al_draw_bitmap(mapgfxpt,i,j,0);	//size should be the same

				mymappt++;
			}
			if (mapblockstaggerx || mapblockstaggery)
			{
				mymap2pt += mapwidth;
				mymappt = mymap2pt;
				for (i=(((mapx-maphclip)-mbgx)+mapblockstaggerx);i<((mapx+mapw));i+=mapblockgapx) 
				{
					if (*mymappt>=0)
						blkdatapt = ((BLKSTR*) mapblockstrpt) + *mymappt;
					else 
					{ 
						myanpt = mapanimstrendpt + *mymappt;
						blkdatapt = ((BLKSTR *) mapblockstrpt) + mapanimseqpt[myanpt->ancuroff]; 
					}
					if (!mapfg) 
						mapgfxpt = (ALLEGRO_BITMAP *) blkdatapt->fgoff;
					else if (mapfg == 1)
						mapgfxpt = (ALLEGRO_BITMAP *) blkdatapt->fgoff2;
					else 
						mapgfxpt = (ALLEGRO_BITMAP *) blkdatapt->fgoff3;
					if (((int)mapgfxpt) != 0)
						//masked_blit (mapgfxpt, mapdestpt, 0, 0, i, j+mapblockstaggery, mapblockwidth, mapblockheight);
						al_draw_bitmap(mapgfxpt,i,j+mapblockstaggery,0);	//size should be the same

					mymappt++;
				}
			}
			mymap2pt += mapwidth;
			mymappt = mymap2pt;
		}

	al_set_clipping_rectangle(mycl,mycr,myct,mycb);
	PopTargetDisplayAndBitmap();
}

/* draw row at a time calling own hook. used for isometric drawing */
void MapDrawRow (int mapxo, int mapyo, int mapx, int mapy, int mapw, int maph, int maprw, void (*cellcall) (int cx, int cy, int dx, int dy))
{
	int i, j, mycl, mycr, myct, mycb, mapvclip, maphclip;
	int mbgx, mbgy, bfield, bysub;
	int cx, cy;
	short int *mymappt;
	short int *mymap2pt;
	BLKSTR *blkdatapt;
	ANISTR *myanpt;
	ALLEGRO_BITMAP *mapgfxpt;
	ALLEGRO_BITMAP* mapdestpt;

	mapdestpt=PushTargetDisplayAndBitmap(NULL);

	if (((mapyo/mapblockgapy)+maprw) >= mapheight) return;
	if (mapblockstaggerx || mapblockstaggery)
	{
		mapxo -= mapblockstaggerx;
		mapyo -= mapblockstaggery;
		if ((((mapyo/mapblockgapy)*2)+maprw) >= (mapheight-1)) return;
	}
	al_get_clipping_rectangle(&mycl,&mycr,&myct,&mycb);
	al_set_clipping_rectangle(mapx,mapy,mapw,maph);//	set_clip (mapdestpt, mapx, mapy, mapx+mapw-1, mapy+maph-1);
	mymappt = (short int *) mappt;
	mapvclip = mapyo%mapblockgapy;
	maphclip = mapxo%mapblockgapx;
	j = (mapy-mapvclip); i = 0;
	if (mapblockstaggerx || mapblockstaggery) 
	{
		cx = mapxo/mapblockgapx;
		cy = (((mapyo/mapblockgapy)*2)+maprw);
		mymappt += (cx)+(cy*mapwidth);
		mbgx = mapblockgapx;
		mbgy = mapblockgapy;
		j -= mbgy;
		j += ((maprw/2)*mapblockgapy);
		if (maprw&1) 
		{ 
			j += mapblockstaggery; 
			i = mapblockstaggerx; 
		}
	}
	else
	{
		cx = mapxo/mapblockgapx;
		cy = ((mapyo/mapblockgapy)+maprw);
		mymappt += (cx)+(cy*mapwidth);
		mbgx = 0;
		mbgy = 0;
		j += (maprw*mapblockgapy);
	}

	mymap2pt = mymappt;
	for (i+=((mapx-maphclip)-mbgx);i<((mapx+mapw));i+=mapblockgapx)
	{
		if (cellcall != NULL) 
			cellcall (cx, cy, i, j);
		if (*mymappt>=0) 
			blkdatapt = ((BLKSTR*) mapblockstrpt) + *mymappt;
		else
		{ 
			myanpt = mapanimstrendpt + *mymappt;
			blkdatapt = ((BLKSTR *) mapblockstrpt) + mapanimseqpt[myanpt->ancuroff];
		}
		bfield = 1; bysub = 0;
		do 
		{
			if (!bfield) blkdatapt++;
			for (;bfield<4;bfield++)
			{
			switch (bfield) 
			{
				case 0: mapgfxpt = (ALLEGRO_BITMAP *) blkdatapt->bgoff; break;
				case 1: mapgfxpt = (ALLEGRO_BITMAP *) blkdatapt->fgoff; break;
				case 2: mapgfxpt = (ALLEGRO_BITMAP *) blkdatapt->fgoff2; break;
				default:
				case 3: mapgfxpt = (ALLEGRO_BITMAP *) blkdatapt->fgoff3; break;
			}
			if (((int)mapgfxpt) != 0) 
			{
				if (blkdatapt->unused2 && !blkdatapt->unused3)
				{
					//masked_blit (mapgfxpt, mapdestpt, 0, 0, i, j-bysub, mapblockwidth/2, mapblockheight);
					al_draw_bitmap_region((ALLEGRO_BITMAP *) blkdatapt->bgoff,0,0,mapblockwidth/2,mapblockheight,i,j-bysub,0);	//size should be the same
				} 
				else
				{
					if (!blkdatapt->unused2 && blkdatapt->unused3)
					{
						//masked_blit (mapgfxpt, mapdestpt, mapblockwidth/2, 0, i, j-bysub, mapblockwidth/2, mapblockheight);
						al_draw_bitmap_region((ALLEGRO_BITMAP *) blkdatapt->bgoff,mapblockwidth/2,0,mapblockwidth/2,mapblockheight,i,j-bysub,0);	//size should be the same
					} 
					else 
					{
						//masked_blit (mapgfxpt, mapdestpt, 0, 0, i, j-bysub, mapblockwidth, mapblockheight);
						al_draw_bitmap(mapgfxpt,i,j-bysub,0);
					}
				}
			}
			bysub += mapblockheight;
			}
			bfield = 0;
		} while (blkdatapt->unused1);
		mymappt++; 
		cx++;
	}

	al_set_clipping_rectangle(mycl,mycr,myct,mycb);
	PopTargetDisplayAndBitmap();
}




// STATIC FUNCTIONS NOT TO BE MADE PUBLIC 
static int MapGetchksz (unsigned char * locpt)
{
	int ret=((((int) (locpt[0]))<<24)|(((int) (locpt[1]))<<16)|
		(((int) (locpt[2]))<<8)|((int) (locpt[3])));

	return ret;
}

static int MapGetshort (unsigned char * locpt)
{
int rval;

	if (mapislsb)
		rval = ((((int) (locpt[1]))<<8)|((int) (locpt[0])));
	else
		rval = ((((int) (locpt[0]))<<8)|((int) (locpt[1])));

	if (rval & 0x8000) 
		rval -= 0x10000;

	return rval;
}

static int MapGetlong (unsigned char * locpt)
{
	if (mapislsb)
		return ((((int) (locpt[3]))<<24)|(((int) (locpt[2]))<<16)|
			(((int) (locpt[1]))<<8)|((int) (locpt[0])));
	else
		return ((((int) (locpt[0]))<<24)|(((int) (locpt[1]))<<16)|
			(((int) (locpt[2]))<<8)|((int) (locpt[3])));
}

static int MapRealLoad (char * mname, const int convertMagicPink)
{
	int mretval;
	char idtag[4];
	unsigned char tempc;
	int mapfilesize = 0;

	void * mapfilept;

	maperror = MER_NONE;
	mapfilept = packed_fopen (mname, "rb");
	if(mapfilept==NULL)
	{
		maperror = MER_NOOPEN; 
		return -1; 
	} 

	maperror = MER_MAPLOADERROR;
	if ( (packed_fread (&idtag[0], 1, mapfilept) == 1) && (packed_fread (&idtag[1], 1, mapfilept) == 1) && (packed_fread (&idtag[2], 1, mapfilept) == 1) && (packed_fread (&idtag[3], 1, mapfilept) == 1) && (packed_fread (&tempc, 1, mapfilept) == 1))
	{
		mapfilesize = (((int) tempc)<<24);
		if (packed_fread (&tempc, 1, mapfilept) == 1) 
		{
			mapfilesize |= (((int) tempc)<<16);
			if (packed_fread (&tempc, 1, mapfilept) == 1) 
			{
				mapfilesize |= (((int) tempc)<<8);
				if (packed_fread (&tempc, 1, mapfilept) == 1) 
				{
					mapfilesize |= (((int) tempc));
					mapfilesize += 8;
					if (!strncmp (idtag, "FORM", 4)) 
					{
						if ( (packed_fread (&idtag[0], 1, mapfilept) == 1) && (packed_fread (&idtag[1], 1, mapfilept) == 1) && (packed_fread (&idtag[2], 1, mapfilept) == 1) && (packed_fread (&idtag[3], 1, mapfilept) == 1) )
						{
							if (!strncmp (idtag, "FMAP", 4)) 
								maperror = MER_NONE;
						} 
					}
				} 
			}
		}
	}

	if (maperror != MER_NONE) 
	{ 
		packed_fclose (mapfilept); 
		return -1; 
	}

	mretval = MapRealDecode (convertMagicPink, mapfilept, NULL, mapfilesize);
	packed_fclose (mapfilept);

	return mretval;
}

static int MapPreRealDecode (unsigned char * mapmempt, const int convertMagicPink)
{
long int maplength;

	MapFreeMem ();
	maperror = 0;

	if (*mapmempt!='F') 
		maperror = MER_MAPLOADERROR;
	if (*(mapmempt+1)!='O') 
		maperror = MER_MAPLOADERROR;
	if (*(mapmempt+2)!='R') 
		maperror = MER_MAPLOADERROR;
	if (*(mapmempt+3)!='M') 
		maperror = MER_MAPLOADERROR;

	mapmempt += 4;
	maplength = (MapGetchksz (mapmempt))+8;

	if (maperror) 
		return -1;

	mapmempt += 4;

	if (*mapmempt!='F') 
		maperror = MER_MAPLOADERROR;
	if (*(mapmempt+1)!='M') 
		maperror = MER_MAPLOADERROR;
	if (*(mapmempt+2)!='A') 
		maperror = MER_MAPLOADERROR;
	if (*(mapmempt+3)!='P') 
		maperror = MER_MAPLOADERROR;

	mapmempt+=4;

	if (maperror) 
		return -1;
	else
		return MapRealDecode (convertMagicPink, NULL, mapmempt, maplength);
}

static int MapRealDecode (const int convertMagicPink, void* mfpt, unsigned char * mmpt, long int mpfilesize)
{
	int chkdn;
	int size1, size2,pos;
	unsigned char * fmappospt;
	char mphdr[8];
	int retval;

	MapFreeMem ();
	mpfilesize -= 12;
	while (mpfilesize > 0) 
	{
		if (mfpt != NULL)
		{
			pos=packed_ftell(mfpt);
			if (packed_fread (mphdr, 8, mfpt) != 8) 
			{
				maperror = MER_MAPLOADERROR;
				MapFreeMem ();
				return -1;
			}
			fmappospt = malloc (MapGetchksz((unsigned char*)mphdr+4)+8);

			if (fmappospt == NULL) 
			{
				maperror = MER_OUTOFMEM;
				MapFreeMem ();
				return -1;
			}

			memcpy (fmappospt, mphdr, 8);
			size1=MapGetchksz(mphdr+4);
			pos=packed_ftell(mfpt);
			size2=packed_fread (fmappospt+8, size1, mfpt);
			pos=packed_ftell(mfpt);
			if (size2 != size1)
			{
				maperror = MER_MAPLOADERROR;
				MapFreeMem ();
				return -1;
			}
		} 
		else 
		{
			fmappospt = mmpt;
			mmpt += MapGetchksz(mmpt+4);
			mmpt += 8;
		}
		
		chkdn = 0;

		if (!strncmp ((char*)fmappospt, "MPHD", 4)) 
		{ 
			chkdn = 1; MapDecodeMPHD (fmappospt); 
		}
		if (!strncmp ((char*)fmappospt, "CMAP", 4)) 
		{ 
			chkdn = 1; MapDecodeCMAP (fmappospt); 
		}
		if (!strncmp ((char*)fmappospt, "BKDT", 4)) 
		{ 
			chkdn = 1; MapDecodeBKDT (fmappospt); 
		}
		if (!strncmp ((char*)fmappospt, "ANDT", 4)) 
		{ 
			chkdn = 1; MapDecodeANDT (fmappospt); 
		}
		if (!strncmp ((char*)fmappospt, "AGFX", 4))
		{ 
			chkdn = 1; MapDecodeAGFX (fmappospt); 
		}
		if (!strncmp ((char*)fmappospt, "BGFX", 4))
		{ 
			chkdn = 1; MapDecodeBGFX (fmappospt); 
		}
		if (!strncmp ((char*)fmappospt, "NOVC", 4))
		{ 
			chkdn = 1; MapDecodeNOVC (fmappospt); 
		}
		if (!strncmp ((char*)fmappospt, "BODY", 4))
		{ 
			chkdn = 1; MapDecodeLayer (fmappospt, 0); 
		}
		if (!strncmp ((char*)fmappospt, "LYR1", 4)) 
		{ 
			chkdn = 1; MapDecodeLayer (fmappospt, 1);
		}
		if (!strncmp ((char*)fmappospt, "LYR2", 4))
		{ 
			chkdn = 1; MapDecodeLayer (fmappospt, 2);
		}
		if (!strncmp ((char*)fmappospt, "LYR3", 4))
		{ 
			chkdn = 1; MapDecodeLayer (fmappospt, 3); 
		}
		if (!strncmp ((char*)fmappospt, "LYR4", 4))
		{ 
			chkdn = 1; MapDecodeLayer (fmappospt, 4);
		}
		if (!strncmp ((char*)fmappospt, "LYR5", 4))
		{ 
			chkdn = 1; MapDecodeLayer (fmappospt, 5); 
		}
		if (!strncmp ((char*)fmappospt, "LYR6", 4))
		{ 
			chkdn = 1; MapDecodeLayer (fmappospt, 6); 
		}
		if (!strncmp ((char*)fmappospt, "LYR7", 4))
		{ 
			chkdn = 1; MapDecodeLayer (fmappospt, 7); 
		}
		
		if (!chkdn) 
			MapDecodeNULL (fmappospt);

		mpfilesize -= 8;
		mpfilesize -= MapGetchksz (fmappospt+4);
		if (mfpt != NULL) 
			free (fmappospt);

		if (maperror != MER_NONE) 
		{ 
			MapFreeMem (); 
			return -1; 
		}
	}

	mapdepth = mapaltdepth;
	retval=MapRelocate(convertMagicPink, mapnumblockgfx,mapnumblockstr, mapcmappt);

	//put locking in to speed up bitmap generation
	if(retval==0)
		MapGenerateYLookup();	//always do this as it adds no burden. this is the common last call made by load and decode
		
	return retval;
}

static int MapDecodeMPHD (unsigned char * mdatpt)
{
	mdatpt += 8;
	if (mdatpt[0] > 1)
	{ 
		maperror = MER_MAPTOONEW; 
		return -1;
	}
	if (mdatpt[2] == 1)
		mapislsb = 1;
	else 
		mapislsb = 0;

	maptype = (int) mdatpt[3];
	if (maptype > 3) 
	{ 
		maperror = MER_MAPTOONEW; 
		return -1;
	}
	mapwidth = (short) MapGetshort (mdatpt+4);
	mapheight = (short) MapGetshort (mdatpt+6);
	mapblockwidth = (short) MapGetshort (mdatpt+12);
	mapblockheight = (short) MapGetshort (mdatpt+14);
	mapdepth = (short) MapGetshort (mdatpt+16);
	mapaltdepth = mapdepth;
	mapblockstrsize = (short) MapGetshort (mdatpt+18);
	mapnumblockstr = (short) MapGetshort (mdatpt+20);
	mapnumblockgfx = (short) MapGetshort (mdatpt+22);

	if (MapGetchksz (mdatpt-4) > 28) 
	{
		mapblockgapx = (int) MapGetshort (mdatpt+28);
		mapblockgapy = (int) MapGetshort (mdatpt+30);
		mapblockstaggerx = (int) MapGetshort (mdatpt+32);
		mapblockstaggery = (int) MapGetshort (mdatpt+34);
	} 
	else 
	{
		mapblockgapx = (int) mapblockwidth;
		mapblockgapy = (int) mapblockheight;
		mapblockstaggerx = 0;
		mapblockstaggery = 0;
	}

	return 0;
}

static int MapDecodeCMAP (unsigned char * mdatpt)
{
	mdatpt += 8;
	mapcmappt = (unsigned char *) malloc (MapGetchksz (mdatpt-4));
	if (mapcmappt==NULL) 
	{ 
		maperror = MER_OUTOFMEM; 
		return -1; 
	}
	memcpy (mapcmappt, mdatpt, MapGetchksz (mdatpt-4));
	return 0;
}

static int MapDecodeBKDT (unsigned char * mdatpt)
{
int i, j;
BLKSTR * myblkpt;

	mdatpt += 8;
	mapblockstrpt = malloc (mapnumblockstr*sizeof(BLKSTR));
	if (mapblockstrpt==NULL) 
	{ 
		maperror = MER_OUTOFMEM; 
		return -1; 
	}

	myblkpt = (BLKSTR *) mapblockstrpt;
	j = MapGetchksz (mdatpt-4);
	i = 0; 
	while (i < (mapnumblockstr*mapblockstrsize)) 
	{
		myblkpt->bgoff = (int) MapGetlong (mdatpt+i);
		myblkpt->fgoff = (int) MapGetlong (mdatpt+i+4);
		myblkpt->fgoff2 = (int) MapGetlong (mdatpt+i+8);
		myblkpt->fgoff3 = (int) MapGetlong (mdatpt+i+12);
		if (maptype == 0) 
		{
			myblkpt->bgoff /= (mapblockwidth*mapblockheight*((mapdepth+1)/8));
			myblkpt->fgoff /= (mapblockwidth*mapblockheight*((mapdepth+1)/8));
			myblkpt->fgoff2 /= (mapblockwidth*mapblockheight*((mapdepth+1)/8));
			myblkpt->fgoff3 /= (mapblockwidth*mapblockheight*((mapdepth+1)/8));
		}
		myblkpt->user1 = (unsigned int) MapGetlong (mdatpt+i+16);
		myblkpt->user2 = (unsigned int) MapGetlong (mdatpt+i+20);
		myblkpt->user3 = (unsigned short int) MapGetshort (mdatpt+i+24);
		myblkpt->user4 = (unsigned short int) MapGetshort (mdatpt+i+26);
		myblkpt->user5 = mdatpt[i+28];
		myblkpt->user6 = mdatpt[i+29];
		myblkpt->user7 = mdatpt[i+30];
		if (mdatpt[i+31]&0x80) myblkpt->unused3 = 1; else myblkpt->unused3 = 0;
		if (mdatpt[i+31]&0x40) myblkpt->unused2 = 1; else myblkpt->unused2 = 0;
		if (mdatpt[i+31]&0x20) myblkpt->unused1 = 1; else myblkpt->unused1 = 0;
		if (mdatpt[i+31]&0x10) myblkpt->trigger = 1; else myblkpt->trigger = 0;
		if (mdatpt[i+31]&0x08) myblkpt->br = 1; else myblkpt->br = 0;
		if (mdatpt[i+31]&0x04) myblkpt->bl = 1; else myblkpt->bl = 0;
		if (mdatpt[i+31]&0x02) myblkpt->tr = 1; else myblkpt->tr = 0;
		if (mdatpt[i+31]&0x01) myblkpt->tl = 1; else myblkpt->tl = 0;
		i += mapblockstrsize;
		myblkpt++;
	}
	return 0;
}

static int MapDecodeANDT (unsigned char * mdatpt)
{
int numani, i, ancksz;
unsigned char * mdatendpt;

	mdatpt += 8;
	ancksz = MapGetchksz(mdatpt-4);
	mdatendpt = mdatpt+ancksz;

	numani = 0;
	while (1) 
	{
		mdatendpt -= 16;
		numani++;
		if (*mdatendpt == 255) 
			break;
	}

	mapanimseqpt = malloc (((mdatendpt-mdatpt)/4)*sizeof(int));
	if (mapanimseqpt == NULL) 
	{ 
		maperror = MER_OUTOFMEM; 
	return -1; 
	}
	i = 0; while (mdatpt != mdatendpt) 
	{
		mapanimseqpt[i] = MapGetlong (mdatpt);
		if (maptype == 0) mapanimseqpt[i] /= mapblockstrsize;
		mdatpt += 4; i++;
	}

	mapanimstrpt = malloc (numani*sizeof(ANISTR));
	if (mapanimstrpt == NULL) { maperror = MER_OUTOFMEM; return -1; }
	mapanimstrendpt = mapanimstrpt;
	mapanimstrendpt += numani;

	i = 0; 
	while (i<numani) 
	{
		mapanimstrpt[i].antype = mdatendpt[0];
		mapanimstrpt[i].andelay = mdatendpt[1];
		mapanimstrpt[i].ancount = mdatendpt[2];
		mapanimstrpt[i].anuser = mdatendpt[3];
		if (maptype == 0) 
		{
			mapanimstrpt[i].ancuroff = (int) ((MapGetlong (mdatendpt+4)+ancksz)/4);
			mapanimstrpt[i].anstartoff = (int) ((MapGetlong (mdatendpt+8)+ancksz)/4);
			mapanimstrpt[i].anendoff = (int) ((MapGetlong (mdatendpt+12)+ancksz)/4);
		} 
		else 
		{
			mapanimstrpt[i].ancuroff = (int) MapGetlong (mdatendpt+4);
			mapanimstrpt[i].anstartoff = (int) MapGetlong (mdatendpt+8);
			mapanimstrpt[i].anendoff = (int) MapGetlong (mdatendpt+12);
		}
		mdatendpt += 16; i++;
	}

	MapInitAnims ();
	return 0;
}

static int MapDecodeAGFX (unsigned char * mdatpt)
{
	maperror=MER_NOTSUPPORTED;
	return MER_NOTSUPPORTED;	//shouldn't get here
/*	if (bitmap_color_depth (screen) > 8) return 0;
	if (mapblockgfxpt != NULL) free (mapblockgfxpt);
	mapblockgfxpt = malloc (MapGetchksz (mdatpt+4));
	if (mapblockgfxpt==NULL) { maperror = MER_OUTOFMEM; return -1; }
	memcpy (mapblockgfxpt, mdatpt+8, MapGetchksz(mdatpt+4));
	mapaltdepth = 8;
	return 0;*/
}

static int MapDecodeBGFX (unsigned char * mdatpt)
{
	if (mapblockgfxpt != NULL) return 0;
	mapblockgfxpt = malloc (MapGetchksz (mdatpt+4));
	if (mapblockgfxpt==NULL) 
	{ 
		maperror = MER_OUTOFMEM; 
		return -1; 
	}
	memcpy (mapblockgfxpt, mdatpt+8, MapGetchksz(mdatpt+4));
	return 0;
}

static int MapDecodeNOVC (unsigned char * mdatpt)
{
	memset (mapnovctext, 0, 70);
	if (MapGetchksz (mdatpt+4) < 70) strcpy (mapnovctext, (char*)mdatpt+8);
	return 0;
}

static int MapDecodeLayer (unsigned char * mdatpt, int lnum)
{
	int i, j, k, l;
	short int * mymappt, * mymap2pt;

	mapmappt[lnum] = malloc (mapwidth*mapheight*sizeof(short int));
	if (mapmappt[lnum] == NULL) 
	{ 
		maperror = MER_OUTOFMEM; 
		return -1; 
	}

	mdatpt += 8;
	mymappt = mapmappt[lnum];
	if (maptype == 0) 
	{
		for (j=0;j<mapheight;j++)
			{
				for (i=0;i<mapwidth;i++) 
				{
					*mymappt = (short int) MapGetshort (mdatpt);
					if (*mymappt >= 0) 
					{ 
						*mymappt /= mapblockstrsize; 
					}
					else 
					{ 
						*mymappt /= 16; 
					}
					mdatpt+=2; mymappt++;
				}
			}
		}
		else 
		{
			if (maptype == 1) 
			{
			for (j=0;j<mapheight;j++) 
			{
				for (i=0;i<mapwidth;i++)
				{
					*mymappt = (short int) MapGetshort (mdatpt);
					mdatpt+=2; mymappt++;
				}
			}
		} 
		else 
		{
			if (maptype == 2)
			{
			for (j=0;j<mapheight;j++)
			{
				for (i=0;i<mapwidth;)
				{
					k = (int) MapGetshort (mdatpt);
					mdatpt += 2;
					if (k > 0)
					{
						while (k) 
						{
							*mymappt = (short int) MapGetshort (mdatpt);
							mymappt++; mdatpt += 2;
							i++; k--;
						}
					} 
					else 
					{
					if (k < 0) 
					{
						l = (int) MapGetshort (mdatpt); mdatpt += 2;
						while (k)
						{
							*mymappt = (short int) l;
							mymappt++;
							i++; k++;
						}
					} 
					else 
					{
					} 
					}
				}
			}
		} 
		else 
		{
			if (maptype == 3) 
			{
				for (j=0;j<mapheight;j++)
				{
					for (i=0;i<mapwidth;)
					{
						k = (int) MapGetshort (mdatpt);
						mdatpt += 2;
						if (k > 0)
						{
							while (k)
							{
								*mymappt = (short int) MapGetshort (mdatpt);
								mymappt++; mdatpt += 2;
								i++; k--;
							}
						} 
						else
						{
						if (k < 0)
						{
							mymap2pt = mymappt + (int) MapGetshort (mdatpt); mdatpt += 2;
							while (k)
							{
								*mymappt = *mymap2pt;
								mymappt++; mymap2pt++;
								i++; k++;
							}
						}
						else 
						{
						} 
						}
					}
				}
			}
		} 
	} 
}

	if (lnum == 0) 
	{ 
		mappt = mapmappt[lnum]; 
	}
	return 0;
}

static int MapDecodeNULL (unsigned char * mdatpt)
{
	return 0;
}







//PRIVATE FUNCTIONS
static int MapGenerateYLookup (void)
{
	int i, j;
	for (i=0;i<8;i++)
	{
		if (mapmaparraypt[i]!=NULL)
		{ 
			free (mapmaparraypt[i]);
			mapmaparraypt[i] = NULL; 
		}

		if (mapmappt[i]!=NULL) 
		{
			mapmaparraypt[i] = malloc (mapheight*sizeof(short int *));

			if (mapmaparraypt[i] == NULL) 
				return -1;

			for (j=0;j<mapheight;j++) 
				mapmaparraypt[i][j] = (mapmappt[i]+(j*mapwidth));

			if (mapmappt[i] == mappt) 
				maparraypt = mapmaparraypt[i];
		}
	}
	return 0;
}

//convert integer in 8/15/16 bit RGB to separates
static void itoC8(int colour, int *r, int* g, int* b)
{
	unsigned char * mycmappt;
	int j;
	mycmappt=(unsigned char *) mapcmappt;
	j = colour*3;
	*r=mycmappt[j];
	*g=mycmappt[j+1];
	*b=mycmappt[j+2];
}

static void itoC15(int colour, int* r, int* g, int* b)
{
/*
15 bit to RGB
colour=(r << 10)+(g << 5)+b -> 0-31 r,g, b
r=(colour & 0x7c00) >> 10
g=(colour & 0x03e0) >> 5
b=(colour & 0x001f)
*/
	*r=(colour & 0x7c00) >> 10;
	*g=(colour & 0x03e0) >> 5;
	*b=(colour & 0x001f);
}
static void itoC16(int colour, int*r, int*g, int*b)
{
	/*
	16bit to RGB
colour=(r << 11)+(g << 5)+b -> 0-31 r,b, g 0-63
r=(colour & 0xf800) >> 11
g=(colour & 0x03e0) >> 5
b=(colour & 0x001f)
*/
	*r=(colour & 0xf800) >> 11;
	*g=(colour & 0x03e0) >> 5;
	*b=(colour & 0x001f);
}
static int C15toi(ALLEGRO_COLOR colour)
{
	return ((int)colour.r << 10)+((int)colour.g << 5)+(int)colour.b;
}
static int C16toi(ALLEGRO_COLOR colour)
{
	return ((int)colour.r << 11)+((int)colour.g << 5)+(int)colour.b;
}
static int C32toi(ALLEGRO_COLOR colour)
{
	return ((int)(colour.r*255)<<16)+((int)(colour.g*255)<<8)+(int)(colour.b*255);
}
static int C24toi(ALLEGRO_COLOR colour)
{
	return ((int)colour.r<<16)+((int)colour.g<<8)+(int)colour.b;
}

static int MapRelocate2 (int convertMagicPink, const short int numblockgfx,const short int numblockstr)
{
	//bitmaps are created from mapblockgfxpt. mapblockgfxpt is kept for map restore (MapRelocate2)
	//in integer format in the current screen/bitmap colour depth

	int i, j, k;
	int r,g,b;
	BLKSTR * myblkstrpt;
	char * newgfxpt, * novcarray;
	char ascnum[80];

	i = numblockstr;
	myblkstrpt = (BLKSTR *) mapblockstrpt;

	novcarray = malloc (numblockgfx);
	memset (novcarray, 0, numblockgfx);
	i = 0; 

	//get text stored in map
	//create bitmaps for each stored in raw pixels
	//loop every raw pixel (in correct format as relocate did the conversion)
	//for every pixel in the bitmap update the colour
	while (mapnovctext[i] != 0)
	{
		j = 0; while (mapnovctext[i] >= '0' && mapnovctext[i] <= '9')
		{
			ascnum[j] = mapnovctext[i];
			i++; j++;
		}

		ascnum[j] = 0;
		k = atoi(ascnum);
		if (k < 0 || k >= numblockgfx) 
			break;
		if (mapnovctext[i] == '-') 
		{
			i++;
			j = 0; while (mapnovctext[i] >= '0' && mapnovctext[i] <= '9')
			{
				ascnum[j] = mapnovctext[i];
				i++; j++;
			}
			ascnum[j] = 0;
			j = atoi(ascnum);

			if (j < k || j >= numblockgfx)
				break;

			while (k <= j) 
			{
				novcarray[k] = 1; k++;
			}
		} 
		else 
		{
			novcarray[k] = 1;
		}

		if (mapnovctext[i] == ',') 
			i++;
	}

	//create bitmaps for each stored in raw pixels
	if (abmTiles == NULL) abmTiles = malloc ((sizeof (ALLEGRO_BITMAP *))*(numblockgfx+2));
	abmTiles[0] = NULL;
	i = 0; 
	newgfxpt = mapblockgfxpt; 
	
	//loop every raw pixel (in correct format as relocate did the conversion)
	while (i<numblockgfx) 
	{
		//create bitmap - using current flags
		abmTiles[i+1] = NULL;
		abmTiles[i] = al_create_bitmap(mapblockwidth, mapblockheight);
		if (abmTiles[i] == NULL)
		{
			MapFreeMem (); 
			maperror = MER_CVBFAILED ; 
			return -1; 
		}
		//for every pixel in the bitmap update the colour
		PushTargetDisplayAndBitmap(abmTiles[i]);
		al_lock_bitmap(abmTiles[i],al_get_bitmap_format(abmTiles[i]),ALLEGRO_LOCK_WRITEONLY);
		for (k=0;k<mapblockheight;k++) 
		{
			for (j=0;j<mapblockwidth;j++) 
			{
				switch (mapdepth)
				{
					case 8:
						itoC8((int)newgfxpt,&r,&g,&b);
						al_put_pixel(j, k, al_map_rgb(r,g,b));
						newgfxpt++;
						break;
					case 15:
							//merge with 16 if this fails and resorting to allegro colours
						itoC15(*((unsigned short int *) newgfxpt),&r,&g,&b);
						al_put_pixel(j, k, al_map_rgb(r,g,b));
						newgfxpt+=2;
						break;
					case 16:
						itoC16(*((unsigned short int *) newgfxpt),&r,&g,&b);
						al_put_pixel(j, k, al_map_rgb(r,g,b));
						newgfxpt+=2;
						break;
					case 24:
						al_put_pixel (j, k, al_map_rgb (newgfxpt[0], newgfxpt[1], newgfxpt[2]));
						newgfxpt+=3;
						break;
					case 32:
						al_put_pixel (j, k, al_map_rgb(newgfxpt[1], newgfxpt[2], newgfxpt[3]));
						newgfxpt+=4;
						break;
				} 
			} 
		}
		al_unlock_bitmap(abmTiles[i]);
		if(convertMagicPink)
			al_convert_mask_to_alpha(abmTiles[i], al_map_rgb(255,0,255));

		PopTargetDisplayAndBitmap();

		i++;
	}

	i = numblockstr; 
	while (i)
	{
		((ALLEGRO_BITMAP *) myblkstrpt->bgoff) = abmTiles[myblkstrpt->bgoff];
		if (myblkstrpt->fgoff!=0)
			((ALLEGRO_BITMAP *) myblkstrpt->fgoff) = abmTiles[myblkstrpt->fgoff];
		if (myblkstrpt->fgoff2!=0)
			((ALLEGRO_BITMAP *) myblkstrpt->fgoff2) = abmTiles[myblkstrpt->fgoff2];
		if (myblkstrpt->fgoff3!=0)
			((ALLEGRO_BITMAP *) myblkstrpt->fgoff3) = abmTiles[myblkstrpt->fgoff3];
		myblkstrpt++; i--;
	}

	free (novcarray);
	return 0;
}


int MapRelocate (const int convertMagicPink, const short int numblockgfx,const short int numblockstr, const char* mapcmappt)
{
	//mapblockgfxpt stores list of integers representing colours (at start of function they are in depth of saved map
	//integers are then converted to the correct bitmap depth and mapblockgfxpt updated
	int i, j, cdepth, pixcol, ccr, ccg, ccb;
	int pixelFormat;
	ALLEGRO_COLOR _pixcol;
	//BLKSTR * myblkstrpt;
	unsigned char * oldgfxpt;
	unsigned char * mycmappt;
	unsigned char * newgfxpt;
	unsigned char * newgfx2pt;

	if(!al_is_system_installed()) 
	{ 
		MapFreeMem (); 
		maperror = MER_NOSCREEN; 
		return -1; 
	}

	//A5 doesn't have 8bit so we are always converting it so remove code that checks for 8bpp
	pixelFormat=al_get_bitmap_format(al_get_backbuffer(al_get_current_display()));
	cdepth=al_get_pixel_format_bits(pixelFormat);

	//now referred to as old (existing pixels as integers from map file), new (integers converted to current depth)
	oldgfxpt = (unsigned char *) mapblockgfxpt;
	newgfxpt =  malloc (mapblockwidth*mapblockheight*((mapdepth+1)/8)*numblockgfx*((cdepth+1)/8));

	if (newgfxpt==NULL)
	{ 
		MapFreeMem (); 
		maperror = MER_OUTOFMEM; 
		return -1; 
	}

	//loop every pixel in every bitmap
	//extract colour from the stored bitmap (format is as per map depth)
	//update raw graphics to colour in the current bitmap depth
	newgfx2pt = newgfxpt;
	mycmappt = (unsigned char *) mapcmappt; pixcol = 0;
	for (i=0;i<(mapblockwidth*mapblockheight*numblockgfx);i++)
	{
		//extract colour from the stored bitmap (format is as per map depth)
		switch (mapdepth) 
		{
		case 8:
			j = (*oldgfxpt)*3;
			_pixcol = al_map_rgb(mycmappt[j], mycmappt[j+1], mycmappt[j+2]);
			if(convertMagicPink && j == 0 && cdepth!=8) 
				_pixcol = al_map_rgb (255, 0, 255); 
			oldgfxpt++;
			break;
		case 15:
			ccr = ((((int) *oldgfxpt)&0x7C)<<1);
			ccg = ((((((int) *oldgfxpt)&0x3)<<3)|(((int) *(oldgfxpt+1))>>5))<<3);
			ccb = (((int) *(oldgfxpt+1)&0x1F)<<3);
			ccr |= ((ccr>>5)&0x07);
			ccg |= ((ccg>>5)&0x07);
			ccb |= ((ccb>>5)&0x07);
			_pixcol = al_map_rgb (ccr, ccg, ccb);
			if(ccr+ccg+ccb==0)
				_pixcol=al_map_rgb(255,0,255);
			oldgfxpt += 2;
			break;
		case 16:
			ccr = (((int) *oldgfxpt)&0xF8);
			ccg = ((((((int) *oldgfxpt)&0x7)<<3)|(((int) *(oldgfxpt+1))>>5))<<2);
			ccb = (((int) *(oldgfxpt+1)&0x1F)<<3);
			ccr |= ((ccr>>5)&0x07);
			ccg |= ((ccg>>6)&0x03);
			ccb |= ((ccb>>5)&0x07);
			_pixcol = al_map_rgb (ccr, ccg, ccb);
			if(ccr+ccg+ccb==0)
				_pixcol=al_map_rgb(255,0,255);
			oldgfxpt += 2;
			break;
		case 24:
			_pixcol = al_map_rgb (*oldgfxpt, *(oldgfxpt+1), *(oldgfxpt+2));
			if((*oldgfxpt)+(*(oldgfxpt+1))+(*(oldgfxpt+2))==0)
				_pixcol=al_map_rgb(255,0,255);
			oldgfxpt += 3;
			break;
		case 32:
			_pixcol = al_map_rgb (*(oldgfxpt+1), *(oldgfxpt+2), *(oldgfxpt+3));

			if((*(oldgfxpt+1))+ (*(oldgfxpt+2))+ (*(oldgfxpt+3)) ==0 && convertMagicPink)
				_pixcol=al_map_rgb(255,0,255);

			oldgfxpt += 4;
			break;
		}

		//update raw graphics to colour in the current bitmap depth
		switch (cdepth) 
		{
		case 8:
			//should never get here
			MapFreeMem (); 
			maperror = MER_NOTSUPPORTED; 
			return -1; 
			break;
		case 15:
			//this was just the same for 15/16 but different for method. if scrap to use colour array then probably merge back
			pixcol=C15toi(_pixcol);
			*((unsigned short int *) newgfxpt) = (unsigned short int) pixcol;
			newgfxpt+=2;
			break;
		case 16:
			pixcol=C16toi(_pixcol);
			*((unsigned short int *) newgfxpt) = (unsigned short int) pixcol;
			newgfxpt+=2;
			break;
		case 24:
			pixcol=C24toi(_pixcol);
			*newgfxpt = (unsigned char) (pixcol>>16)&0xFF;
			*(newgfxpt+1) = (unsigned char) (pixcol>>8)&0xFF;
			*(newgfxpt+2) = (unsigned char) pixcol&0xFF;
			newgfxpt+=3;
			break;
		case 32:
			pixcol=C32toi(_pixcol);
			*newgfxpt = 0;
			*(newgfxpt+1) = (unsigned char) (pixcol>>16)&0xFF;
			*(newgfxpt+2) = (unsigned char) (pixcol>>8)&0xFF;
			*(newgfxpt+3) = (unsigned char) pixcol&0xFF;
			newgfxpt+=4;
			break;
		}
	}
	free (mapblockgfxpt); 
	mapblockgfxpt = (char *) newgfx2pt;

	//both are now the same
	mapdepth = cdepth;

	return MapRelocate2 (convertMagicPink, numblockgfx,numblockstr);
}


//allegro packed functions so that we can call them and cast from core
//translate methods to use those in mappy file to avoid re doing them all

//ALLEGRO_FILE *al_fopen(const char *path, const char *mode)

void* packed_fopen(const char *path, const char *mode)
{
	return al_fopen(path,mode);
}

long packed_ftell(void *f)
{
	return al_ftell(f);
}

//size_t al_fread(ALLEGRO_FILE *f, void *ptr, size_t size)
size_t packed_fread(void *ptr, size_t size,void *f)
{

	return al_fread(f,ptr,size);
}

//void al_fclose(ALLEGRO_FILE *f)
void packed_fclose(void *f)
{
	al_fclose(f);
}

//helper for all methods to save repeating any require state change
//if supplied bitmap then set that as the target, otherwise keep current. Then return the current active target
static ALLEGRO_BITMAP* PushTargetDisplayAndBitmap(ALLEGRO_BITMAP* mapdestpt)
{
	al_store_state(&state,ALLEGRO_STATE_TARGET_BITMAP | ALLEGRO_STATE_DISPLAY);
	stateSet=true;
	if(mapdestpt)
	{
		al_set_target_bitmap(mapdestpt);
		return mapdestpt;
	}
	else	
		return al_get_target_bitmap();
}

static void PopTargetDisplayAndBitmap()
{
	if(stateSet)
		al_restore_state(&state);
	stateSet=false;
}
