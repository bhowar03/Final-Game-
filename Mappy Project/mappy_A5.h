/* Header file for mappyAL V1.0 */
/* (C)2001 Robin Burrows  -  rburrows@bigfoot.com */
#pragma once
#define ALLEGRO_HAVE_STDBOOL_H
#ifdef __cplusplus
extern "C" {
#endif

#include "allegro5/allegro.h"
typedef struct 
{				/* Structure for data blocks */
	long int bgoff, fgoff;			/* offsets from start of graphic blocks */
	long int fgoff2, fgoff3; 		/* more overlay blocks */
	unsigned long int user1, user2;	/* user long data */
	unsigned short int user3, user4;	/* user short data */
	unsigned char user5, user6, user7;	/* user byte data */
	unsigned char tl : 1;				/* bits for collision detection */
	unsigned char tr : 1;
	unsigned char bl : 1;
	unsigned char br : 1;
	unsigned char trigger : 1;			/* bit to trigger an event */
	unsigned char unused1 : 1;
	unsigned char unused2 : 1;
	unsigned char unused3 : 1;
} BLKSTR;

typedef struct 
{		/* Animation control structure */
	signed char antype;	/* Type of anim, AN_? */
	signed char andelay;	/* Frames to go before next frame */
	signed char ancount;	/* Counter, decs each frame, till 0, then resets to andelay */
	signed char anuser;	/* User info */
	long int ancuroff;	/* Points to current offset in list */
	long int anstartoff;	/* Points to start of blkstr offsets list, AFTER ref. blkstr offset */
	long int anendoff;	/* Points to end of blkstr offsets list */
} ANISTR;

/* map file management/initialisation */
int MapLoad (char *filename, const int convertMagicPink);
int MapDecode (unsigned char *,const int );
int MapLoadMAR (char *, int);
int MapDecodeMAR (unsigned char *, int,int );
void MapFreeMem (void);
void MapInitAnims (void);		//called by mapload so not required
int MapGenerateYLookup (void);	//called by mapload so not required
void MapRestore (void);
void MapFreeMem (void);

/* map manipulation */
int MapChangeLayer (int);
void MapUpdateAnims (void);

/* drawing operations */
void MapDrawBG (int mapxoffset, int mapyoffset, int x, int y,int w, int h);
void MapDrawFG (int mapxoffset, int mapyoffset, int x, int y,int w, int h, int foregroundid);
void MapDrawRow (int mapxoffset, int mapyoffset , int x, int y , int w , int h, int row, void (*cellcall) (int cx, int cy, int dx, int dy));
ALLEGRO_BITMAP * MapMakeParallaxBitmap (ALLEGRO_BITMAP *);
void MapDrawParallax (ALLEGRO_BITMAP *parallaxbitmap, int mapxoffset, int mapyoffset, int x, int y,int w, int h);

/* block manipulation */
int MapGetXOffset (int x, int y);
int MapGetYOffset (int x, int y);
BLKSTR * MapGetBlockInPixels (int x, int y);
BLKSTR * MapGetBlock (int x, int y);
void MapSetBlockInPixels (int x, int y, int blockid);
void MapSetBlock (int x, int y, int blockid);
int MapGetBlockID (int blockid, int userid);

/* All global variables used bt Mappy playback are here */
extern int maperror;								/* Set to a MER_ error if something wrong happens */
extern short int mapwidth, mapheight;				/* width/height in tiles */
extern short int mapblockwidth, mapblockheight;	/* size in pixels of each tile */
extern short int mapdepth;							/* colour depth of map - not necessarily the target bitmap */
extern int mapblockgapx, mapblockgapy, mapblockstaggerx, mapblockstaggery; /* for computing offsets for block locations */
/* End of Mappy globals */

#define MER_NONE 0		/* All the horrible things that can go wrong */
#define MER_OUTOFMEM 1
#define MER_MAPLOADERROR 2
#define MER_NOOPEN 3
#define MER_NOSCREEN 4
#define MER_NOACCELERATION 5
#define MER_CVBFAILED 6
#define MER_MAPTOONEW 7
#define MER_NOTSUPPORTED 8

#define AN_END -1			/* Animation types, AN_END = end of anims */
#define AN_NONE 0			/* No anim defined */
#define AN_LOOPF 1		/* Loops from start to end, then jumps to start etc */
#define AN_LOOPR 2		/* As above, but from end to start */
#define AN_ONCE 3			/* Only plays once */
#define AN_ONCEH 4		/* Only plays once, but holds end frame */
#define AN_PPFF 5			/* Ping Pong start-end-start-end-start etc */
#define AN_PPRR 6			/* Ping Pong end-start-end-start-end etc */
#define AN_PPRF 7			/* Used internally by playback */
#define AN_PPFR 8			/* Used internally by playback */
#define AN_ONCES 9		/* Used internally by playback */


/* internal forward declaration functions */
static int MapRelocate (const int convertMagicPink, const short int numblockgfx,const short int numblockstr, const char* mapcmappt);
static int MapRealLoad (char * mname, const int convertMagicPink);	//declaration
static int MapPreRealDecode (unsigned char * mapmempt, const int convertMagicPink);	//declaration
static int MapRealDecode (const int convertMagicPink, void* mfpt, unsigned char * mmpt, long int mpfilesize); //declaration
static int MapDecodeMPHD (unsigned char * mdatpt);
static int MapDecodeCMAP (unsigned char * mdatpt);
static int MapDecodeBKDT (unsigned char * mdatpt);
static int MapDecodeANDT (unsigned char * mdatpt);
static int MapDecodeAGFX (unsigned char * mdatpt);
static int MapDecodeBGFX (unsigned char * mdatpt);
static int MapDecodeNOVC (unsigned char * mdatpt);
static int MapDecodeLayer (unsigned char * mdatpt, int lnum);
static int MapDecodeNULL (unsigned char * mdatpt);
static ALLEGRO_BITMAP* PushTargetDisplayAndBitmap(ALLEGRO_BITMAP* );
static void PopTargetDisplayAndBitmap();
static void itoC8(int colour, int *r, int* g, int* b);
static void itoC15(int colour, int* r, int* g, int* b);
static void itoC16(int colour, int*r, int*g, int*b);
static void* packed_fopen(const char *path, const char *mode);
static size_t packed_fread(void *ptr, size_t size,void *f);
static void packed_fclose(void *f);
static long packed_ftell(void *f);


#ifdef __cplusplus
}
#endif
