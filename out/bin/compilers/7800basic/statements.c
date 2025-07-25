// Provided under the GPL v2 license. See the included LICENSE.txt for details.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <libgen.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include "statements.h"
#include "keywords.h"
#include "atarivox.h"
#include "minitar.h"

// liblzsa headers...
#include <shrink_inmem.h>
#include <lib.h>

#ifndef TRUE
#define TRUE (1==1)
#endif
#ifndef FALSE
#define FALSE (1==0)
#endif

#define MAXINCBASIC 50
#define MAXINCBASICSTR 100
int savelines[MAXINCBASIC];
char savelinesname[MAXINCBASIC][MAXINCBASIC];

extern char stdoutfilename[256];
extern FILE *stdoutfilepointer;
extern FILE *preprocessedfd;
extern char backupname[256];
extern int maxpasses;

FILE *stderrfilepointer = NULL;

int passes;

char redefined_variables[80000][100];

char includespath[500];
char incbasepath[500];
char user_includes[1000];

char forvar[50][50];
char forlabel[50][50];
char forstep[50][50];
char forend[50][50];
char fixpoint44[2][500][50];
char fixpoint88[2][500][50];
char multtablename[100][100];
int multtablewidth[100];
int multtableheight[100];

unsigned char graphicsdata[16][256][100];
char graphicslabels[16][256][80];
unsigned char graphicsinfo[16][256];
unsigned char graphicsmode[16][256];
char currentcharset[256];
int graphicsdatawidth[16];
char charactersetchars[257];

char constants[MAXCONSTANTS][CONSTANTLEN];
char bannerfilenames[1000][100];
int bannerheights[1000];
int bannerwidths[1000];
int bannerpixelwidth[1000];
char palettefilenames[1000][100];
char Areg[SIZEOFSTATEMENT];
int graphicfilepalettes[1000];
int graphicfilemodes[1000];

unsigned char graphiccolorindex[16];
unsigned char graphic7800colors[16];
unsigned char graphiccolormode;

int savelevel = 0;
int dmaplain = 0;
int templabel = 0;
int plotlabel = 0;
int doublewide = 0;
int zoneheight = 16;
int zonelocking = 0;
int optimization = 0;
int multtableindex = 0;
int numfixpoint44 = 0;
int numfixpoint88 = 0;

int numredefvars = 0;
int numconstants = 0;
int line = 0;
int numjsrs = 0;
int doingfunction = 0;
int branchtargetnumber = 0;

int smartbranching = 1;
int banksetrom = 0;
int collisionwrap = 1;
int romat4k = 0;
int bankcount = 0;
int romsize = 0;
int currentbank = 0;
int doublebufferused = 0;
int boxcollisionused = 0;
int tallspritemode = 1;
int multibutton = 0;

int ongosub = 0;
int condpart = 0;
int currentdmahole = 0;
int decimal = 0;
int includesfile_already_done = 0;
int numfors = 0;
int extra = 0;
int extralabel = 0;
int extraactive = 0;
int macroactive = 0;
int dumpgraphics_index = 0;

int firstfourbyte = 1;
int firstcompress = 1;

int changedmaholescalled = 0;

int romsize_already_set = 0;

int TIGHTPACKBORDER = 0;

#define TALLSPRITEMAX 2048
char tallspritelabel[TALLSPRITEMAX][1024];
int tallspriteheight[TALLSPRITEMAX];
int tallspritecount;

int fourbitfade_alreadyused = 0;

int backupflag = FALSE;
int backupcount = 0;

#define BANKSETASM "banksetrom.asm"
#define BANKSETSTRINGSASM "banksetstrings.asm"

int deprecatedframeheight = 0;
int deprecated160bindexes = 0;
int deprecatedboxcollision = 0;

int dumpgraphics = 0;
int dumpgraphicsaddr = 0;

#define PNG_DEBUG 3
#include <png.h>


void currdir_foundmsg (char *foundfile)
{
    prinfo ("User-defined '%s' found in the current directory", foundfile);
}

void assertminimumargs (char **statement, char *commandname, int argcount)
{
    // statement index: 1=command, 2=arg1, 3=arg2, ...

    int t;

    for (t = 2; t < argcount + 2; t++)
    {
	if ((statement[t] == NULL) || (statement[t][0] == 0)
	    || (statement[t][0] == '\n') || (statement[t][0] == '\r') || (statement[t][0] == ':'))
	{
	    t = 0;
	    break;
	}

    }

    if (t < (argcount + 2))
    {
	prerror ("command %s doesn't contain %d arguments", commandname, argcount);
    }
}

void checkvalidfilename (char *filename)
{
    int t;
    filename = ourbasename (filename);
    if (!isalpha (filename[0]))
	prerror ("'%s' filename must begin with a letter", filename);
    for (t = 0; filename[t] != 0; t++)
    {
	if ((!isalpha (filename[t])) && (!isdigit (filename[t])) && (filename[t] != '.') && (filename[t] != '_'))
	    prerror ("'%s' filename must only contain letters and digits", filename);
    }
}

void backupthisfile(char *filename)
{
    if((!backupflag)||(passes>0))
        return;
    removeCR (filename);
    if(AddToArchive(filename,TRUE)==FALSE)
	prerror ("unable to backup '%s'",filename);
    backupcount++;
}


void backup(char **statement)
{
    assertminimumargs (statement, "backup", 1);
    removeCR (statement[2]);
    char *backupstr=statement[2];
    if (*backupstr == '\'')
        backupstr++;
    int t;
    for(t=0;t<SIZEOFSTATEMENT;t++)
    {
        if(backupstr[t]==0)
            break;
        if(backupstr[t]=='^')
            backupstr[t]=' ';
    }
    if(backupstr[t-1]=='\'')
        backupstr[t-1]=0;

    backupthisfile(backupstr);
}


void doreboot ()
{
    printf ("	JMP START\n");
}


int linenum ()
{
    // returns current line number in source
    return line;
}

void jsr (char *location)
{
    printf (" jsr %s\n", location);
    return;
}

int switchjoy (char *input_source)
{
    // place joystick/console switch reading code inline instead of as a subroutine
    // standard routines needed for pretty much all games
    // read switches, joysticks now compiler generated (more efficient)

    // returns 0 if we need beq/bne, 1 if bvc/bvs, and 2 if bpl/bmi

    invalidate_Areg ();		// do we need this?

    if (!strncmp (input_source, "switchreset\0", 11))
    {
	printf (" jsr checkresetswitch\n");
	return 0;
    }
    if (!strncmp (input_source, "switchselect\0", 12))
    {
	printf (" jsr checkselectswitch\n");
	return 0;
    }
    if (!strncmp (input_source, "switchleftb\0", 11))
    {
	printf (" bit SWCHB\n");
	return 1;
    }
    if (!strncmp (input_source, "switchrightb\0", 12))
    {
	printf (" bit SWCHB\n");
	return 2;
    }
    if (!strncmp (input_source, "switchpause\0", 11))
    {
	printf (" lda #8\n");
	printf (" bit SWCHB\n");
	return 0;
    }
    if (!strncmp (input_source, "joy0up\0", 6))
    {
	printf (" lda #$10\n");
	printf (" bit sSWCHA\n");
	return 0;
    }
    if (!strncmp (input_source, "softreset\0", 9))
    {
	printf (" lda sSWCHA\n");
	printf (" and #%%01110000 ;_LDU\n");
	return 0;
    }
    if (!strncmp (input_source, "softselect\0", 10))
    {
	printf (" lda sSWCHA\n");
	printf (" and #%%10110000 ;R_DU\n");
	return 0;
    }
    if (!strncmp (input_source, "softswitches\0", 12))
    {
	printf (" lda sSWCHA\n");
	printf (" and #%%00110000 ;R_DU\n");
	return 0;
    }
    if (!strncmp (input_source, "joy0down\0", 8))
    {
	printf (" lda #$20\n");
	printf (" bit sSWCHA\n");
	return 0;
    }
    if (!strncmp (input_source, "joy0left\0", 8))
    {
	printf (" bit sSWCHA\n");
	return 1;
    }
    if (!strncmp (input_source, "joy0right\0", 9))
    {
	printf (" bit sSWCHA\n");
	return 2;
    }
    if (!strncmp (input_source, "joy0any\0", 7))
    {
	printf (" lda sSWCHA\n");
	printf (" and #$F0\n");
	printf (" eor #$F0\n");
	return 4;
    }

    if (!strncmp (input_source, "joy1up\0", 6))
    {
	printf (" lda #1\n");
	printf (" bit sSWCHA\n");
	return 0;
    }
    if (!strncmp (input_source, "joy1down\0", 8))
    {
	printf (" lda #2\n");
	printf (" bit sSWCHA\n");
	return 0;
    }
    if (!strncmp (input_source, "joy1left\0", 8))
    {
	printf (" lda #4\n");
	printf (" bit sSWCHA\n");
	return 0;
    }
    if (!strncmp (input_source, "joy1right\0", 9))
    {
	printf (" lda #8\n");
	printf (" bit sSWCHA\n");
	return 0;
    }
    if (!strncmp (input_source, "joy1any\0", 7))
    {
	printf (" lda sSWCHA\n");
	printf (" and #$0F\n");
	printf (" eor #$0F\n");
	return 4;
    }
    if (!strncmp (input_source, "joy0fire0\0", 9))
    {
	printf (" bit sINPT1\n");
	return 5;
    }
    if (!strncmp (input_source, "joy0fire1\0", 9))
    {
	printf (" bit sINPT1\n");
	return 3;
    }
    if (!strncmp (input_source, "joy0fire2\0", 9))
    {
	printf (" lda sINPT1\n");
	printf (" and #%%00100000\n");
	return 0;
    }
    if (!strncmp (input_source, "joy0fire3\0", 9))
    {
	printf (" lda sINPT1\n");
	printf (" and #%%00010000\n");
	return 0;
    }
    if (!strncmp (input_source, "joy0fire4\0", 9))
    {
	printf (" lda sINPT1\n");
	printf (" and #%%00001000\n");
	return 0;
    }
    if (!strncmp (input_source, "joy0fire5\0", 9))
    {
	printf (" lda sINPT1\n");
	printf (" and #%%00000100\n");
	return 0;
    }
    if (!strncmp (input_source, "joy0select\0", 10))
    {
	printf (" lda sINPT1\n");
	printf (" and #%%00000010\n");
	return 0;
    }
    if (!strncmp (input_source, "joy0start\0", 9))
    {
	printf (" lda sINPT1\n");
	printf (" and #%%00000001\n");
	return 0;
    }
    if (!strncmp (input_source, "joy1fire0\0", 9))
    {
	printf (" bit sINPT3\n");
	return 5;
    }
    if (!strncmp (input_source, "joy1fire1\0", 9))
    {
	printf (" bit sINPT3\n");
	return 3;
    }
    if (!strncmp (input_source, "joy1fire2\0", 9))
    {
	printf (" lda sINPT3\n");
	printf (" and #%%00100000\n");
	return 0;
    }
    if (!strncmp (input_source, "joy1fire3\0", 9))
    {
	printf (" lda sINPT3\n");
	printf (" and #%%00010000\n");
	return 0;
    }
    if (!strncmp (input_source, "joy1fire4\0", 9))
    {
	printf (" lda sINPT3\n");
	printf (" and #%%00001000\n");
	return 0;
    }
    if (!strncmp (input_source, "joy1fire5\0", 9))
    {
	printf (" lda sINPT3\n");
	printf (" and #%%00000100\n");
	return 0;
    }
    if (!strncmp (input_source, "joy1select\0", 10))
    {
	printf (" lda sINPT3\n");
	printf (" and #%%00000010\n");
	return 0;
    }
    if (!strncmp (input_source, "joy1start\0", 9))
    {
	printf (" lda sINPT3\n");
	printf (" and #%%00000001\n");
	return 0;
    }
    if (!strncmp (input_source, "joy0fire\0", 8))
    {
	printf (" bit sINPT1\n");
	return 3;
    }
    if (!strncmp (input_source, "joy1fire\0", 8))
    {
	printf (" bit sINPT3\n");
	return 3;
    }

    // SNES2ATARI byte bits...
    // At the end:      7      6      5      4      3      2      1      0
    // snes2atari0lo:   A      X     LSH    RSH     -      -      -      -
    // snes2atari0hi:   B      Y    SELECT START    UP    DOWN   LEFT  RIGHT

    if (!strncmp (input_source, "snes0any\0", 9))
    {
	printf (" lda snes2atari0hi\n");
	printf (" and snes2atari0lo\n");
	printf (" eor #$FF\n");
	return 4;
    }
    if (!strncmp (input_source, "snes0anyABXY\0", 12))
    {
	printf (" lda snes2atari0hi\n");
	printf (" and snes2atari0lo\n");
	printf (" ora #%%00111111\n");
	printf (" eor #$FF\n");
	return 4;
    }
    if (!strncmp (input_source, "snes0anymove\0", 12))
    {
	printf (" lda snes2atari0hi\n");
	printf (" ora #%%11110000\n");
	printf (" eor #$FF\n");
	return 4;
    }

    if (!strncmp (input_source, "snes0up\0", 7))
    {
	printf (" lda snes2atari0hi\n");
	printf (" and #%%00001000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes0down\0", 9))
    {
	printf (" lda snes2atari0hi\n");
	printf (" and #%%00000100\n");
	return 0;
    }
    if (!strncmp (input_source, "snes0left\0", 9))
    {
	printf (" lda snes2atari0hi\n");
	printf (" and #%%00000010\n");
	return 0;
    }
    if (!strncmp (input_source, "snes0right\0", 10))
    {
	printf (" lda snes2atari0hi\n");
	printf (" and #%%00000001\n");
	return 0;
    }
    if (!strncmp (input_source, "snes0A\0", 6))
    {
	printf (" lda snes2atari0lo\n");
	printf (" and #%%10000000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes0X\0", 6))
    {
	printf (" lda snes2atari0lo\n");
	printf (" and #%%01000000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes0lsh\0", 8))
    {
	printf (" lda snes2atari0lo\n");
	printf (" and #%%00100000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes0rsh\0", 8))
    {
	printf (" lda snes2atari0lo\n");
	printf (" and #%%00010000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes0B\0", 6))
    {
	printf (" lda snes2atari0hi\n");
	printf (" and #%%10000000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes0Y\0", 6))
    {
	printf (" lda snes2atari0hi\n");
	printf (" and #%%01000000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes0select\0", 11))
    {
	printf (" lda snes2atari0hi\n");
	printf (" and #%%00100000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes0start\0", 10))
    {
	printf (" lda snes2atari0hi\n");
	printf (" and #%%00010000\n");
	return 0;
    }

    // SNES2ATARI byte bits...
    // At the end:      7      6      5      4      3      2      1      0
    // snes2atari0lo:   A      X     LSH    RSH     -      -      -      -
    // snes2atari0hi:   B      Y    SELECT START    UP    DOWN   LEFT  RIGHT

    if (!strncmp (input_source, "snes1any\0", 9))
    {
	printf (" lda snes2atari1hi\n");
	printf (" and snes2atari1lo\n");
	printf (" eor #$FF\n");
	return 4;
    }
    if (!strncmp (input_source, "snes1anyABXY\0", 12))
    {
	printf (" lda snes2atari1hi\n");
	printf (" and snes2atari1lo\n");
	printf (" ora #%%00111111\n");
	printf (" eor #$FF\n");
	return 4;
    }
    if (!strncmp (input_source, "snes1anymove\0", 12))
    {
	printf (" lda snes2atari1hi\n");
	printf (" ora #%%11110000\n");
	printf (" eor #$FF\n");
	return 4;
    }
    if (!strncmp (input_source, "snes1up\0", 7))
    {
	printf (" lda snes2atari1hi\n");
	printf (" and #%%00001000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes1down\0", 9))
    {
	printf (" lda snes2atari1hi\n");
	printf (" and #%%00000100\n");
	return 0;
    }
    if (!strncmp (input_source, "snes1left\0", 9))
    {
	printf (" lda snes2atari1hi\n");
	printf (" and #%%00000010\n");
	return 0;
    }
    if (!strncmp (input_source, "snes1right\0", 10))
    {
	printf (" lda snes2atari1hi\n");
	printf (" and #%%00000001\n");
	return 0;
    }
    if (!strncmp (input_source, "snes1A\0", 6))
    {
	printf (" lda snes2atari1lo\n");
	printf (" and #%%10000000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes1X\0", 6))
    {
	printf (" lda snes2atari1lo\n");
	printf (" and #%%01000000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes1lsh\0", 8))
    {
	printf (" lda snes2atari1lo\n");
	printf (" and #%%00100000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes1rsh\0", 8))
    {
	printf (" lda snes2atari1lo\n");
	printf (" and #%%00010000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes1B\0", 6))
    {
	printf (" lda snes2atari1hi\n");
	printf (" and #%%10000000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes1Y\0", 6))
    {
	printf (" lda snes2atari1hi\n");
	printf (" and #%%01000000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes1select\0", 11))
    {
	printf (" lda snes2atari1hi\n");
	printf (" and #%%00100000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes1start\0", 10))
    {
	printf (" lda snes2atari1hi\n");
	printf (" and #%%00010000\n");
	return 0;
    }


    // SNES2ATARI byte bits...
    // At the end:      7      6      5      4      3      2      1      0
    // snes2atari0lo:   A      X     LSH    RSH     -      -      -      -
    // snes2atari0hi:   B      Y    SELECT START    UP    DOWN   LEFT  RIGHT

    if (!strncmp (input_source, "snes#any\0", 9))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0hi,x\n");
	printf (" and snes2atari0lo,x\n");
	printf (" eor #$FF\n");
	return 4;
    }
    if (!strncmp (input_source, "snes#anyABXY\0", 12))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0hi,x\n");
	printf (" and snes2atari0lo,x\n");
	printf (" ora #%%00111111\n");
	printf (" eor #$FF\n");
	return 4;
    }
    if (!strncmp (input_source, "snes#anymove\0", 12))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0hi,x\n");
	printf (" ora #%%11110000\n");
	printf (" eor #$FF\n");
	return 4;
    }

    if (!strncmp (input_source, "snes#up\0", 7))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0hi,x\n");
	printf (" and #%%00001000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes#down\0", 9))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0hi,x\n");
	printf (" and #%%00000100\n");
	return 0;
    }
    if (!strncmp (input_source, "snes#left\0", 9))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0hi,x\n");
	printf (" and #%%00000010\n");
	return 0;
    }
    if (!strncmp (input_source, "snes#right\0", 10))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0hi,x\n");
	printf (" and #%%00000001\n");
	return 0;
    }
    if (!strncmp (input_source, "snes#A\0", 6))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0lo,x\n");
	printf (" and #%%10000000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes#X\0", 6))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0lo,x\n");
	printf (" and #%%01000000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes#lsh\0", 8))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0lo,x\n");
	printf (" and #%%00100000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes#rsh\0", 8))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0lo,x\n");
	printf (" and #%%00010000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes#B\0", 6))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0hi,x\n");
	printf (" and #%%10000000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes#Y\0", 6))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0hi,x\n");
	printf (" and #%%01000000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes#select\0", 11))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0hi,x\n");
	printf (" and #%%00100000\n");
	return 0;
    }
    if (!strncmp (input_source, "snes#start\0", 10))
    {
	printf (" ldx snesport\n");
	printf (" lda snes2atari0hi,x\n");
	printf (" and #%%00010000\n");
	return 0;
    }

    // mega7800data0 button bits: CBSAYZMX
    if (!strncmp (input_source, "mega0any\0", 8))
    {
	printf (" lda mega7800data0\n");
	printf (" eor #$FF\n");
	return 4;
    }
    if (!strncmp (input_source, "mega0start\0", 10))
    {
	printf (" lda mega7800data0\n");
	printf (" and #%%00100000\n");
	return 0;
    }
    if (!strncmp (input_source, "mega0A\0", 6))
    {
	printf (" lda mega7800data0\n");
	printf (" and #%%00010000\n");
	return 0;
    }
    if (!strncmp (input_source, "mega0C\0", 6))
    {
	printf (" lda mega7800data0\n");
	printf (" and #%%10000000\n");
	return 0;
    }
    if (!strncmp (input_source, "mega0B\0", 6))
    {
	printf (" lda mega7800data0\n");
	printf (" and #%%01000000\n");
	return 0;
    }
    // mega7800data0 button bits: CBSAYZMX
    if (!strncmp (input_source, "mega0mode\0", 9))
    {
	printf (" lda mega7800data0\n");
	printf (" and #%%00000010\n");
	return 0;
    }
    if (!strncmp (input_source, "mega0X\0", 6))
    {
	printf (" lda mega7800data0\n");
	printf (" and #%%00000001\n");
	return 0;
    }
    if (!strncmp (input_source, "mega0Y\0", 6))
    {
	printf (" lda mega7800data0\n");
	printf (" and #%%00001000\n");
	return 0;
    }
    if (!strncmp (input_source, "mega0Z\0", 6))
    {
	printf (" lda mega7800data0\n");
	printf (" and #%%00000100\n");
	return 0;
    }
    // mega7800data0 button bits: CBSAYZMX
    if (!strncmp (input_source, "mega1any\0", 8))
    {
	printf (" lda mega7800data1\n");
	printf (" eor #$FF\n");
	return 4;
    }
    if (!strncmp (input_source, "mega1start\0", 10))
    {
	printf (" lda mega7800data1\n");
	printf (" and #%%00100000\n");
	return 0;
    }
    if (!strncmp (input_source, "mega1A\0", 6))
    {
	printf (" lda mega7800data1\n");
	printf (" and #%%00010000\n");
	return 0;
    }
    if (!strncmp (input_source, "mega1C\0", 6))
    {
	printf (" lda mega7800data1\n");
	printf (" and #%%10000000\n");
	return 0;
    }
    if (!strncmp (input_source, "mega1B\0", 6))
    {
	printf (" lda mega7800data1\n");
	printf (" and #%%01000000\n");
	return 0;
    }
    // mega7800data0 button bits: CBSAYZMX
    if (!strncmp (input_source, "mega1mode\0", 9))
    {
	printf (" lda mega7800data1\n");
	printf (" and #%%00000010\n");
	return 0;
    }
    if (!strncmp (input_source, "mega1X\0", 6))
    {
	printf (" lda mega7800data1\n");
	printf (" and #%%00000001\n");
	return 0;
    }
    if (!strncmp (input_source, "mega1Y\0", 6))
    {
	printf (" lda mega7800data1\n");
	printf (" and #%%00001000\n");
	return 0;
    }
    if (!strncmp (input_source, "mega1Z\0", 6))
    {
	printf (" lda mega7800data1\n");
	printf (" and #%%00000100\n");
	return 0;
    }
    if (!strncmp (input_source, "keypad", 6))
    {
	// 1 2 3   keypad layout
	// 4 5 6
	// 7 8 9
	// s 0 h

	int port, keynum;
	int keymask[3] = { 0x04, 0x02, 0x01 };
	int rowletter[4] = { 'a', 'b', 'c', 'd' };
	char keynumlut[13] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', 's', '0', 'h',
	    'X'
	};
	port = input_source[6] - '0';
	if ((port != 0) && (port != 1))
	    prerror ("unsupported keypad port value");
	for (keynum = 0; keynum < 13; keynum++)
	    if (keynumlut[keynum] == input_source[10])
		break;
	if (keynum == 13)
	    prerror ("unsupported keypad key value");
	// if we're here, the input_source in the form "keypadNkeyT", where N=port #, K=key (0-9,s or n)
	printf (" lda keypadmatrix%d%c\n", port, rowletter[keynum / 3]);
	printf (" and #$%02x\n", keymask[keynum % 3]);
	return 4;
    }
    prerror ("invalid console switch/controller reference");
    return 0;			// to shut-up gcc warning about non-return-value exit.
}

float immed_fixpoint (char *fixpointval)
{
    int i = findpoint (fixpointval);
    if (i == 500)
	return 0;		// failsafe
    char decimalpart[50];
    fixpointval[i] = '\0';
    sprintf (decimalpart, "0.%s", fixpointval + i + 1);
    return atof (decimalpart);
}

int findpoint (char *item)	// determine if fixed point var
{
    int i;
    for (i = 0; i < 500; ++i)
    {
	if (item[i] == '\0')
	    return 500;
	if (item[i] == '.')
	    return i;
    }
    return i;
}

void freemem (char **statement)
{
    int i;
    for (i = 0; i < STATEMENTCOUNT; ++i)
	free (statement[i]);
    free (statement);
}

void printfrac (char *item)
{				// prints the fractional part of a declared 8.8 fixpoint variable
    char getvar[50];
    int i;
    for (i = 0; i < numfixpoint88; ++i)
    {
	strcpy (getvar, fixpoint88[1][i]);
	if (!strcmp (fixpoint88[0][i], item))
	{
	    printf ("%s\n", getvar);
	    return;
	}
    }
    // must be immediate value
    if (findpoint (item) < 500)
	printf ("#%d\n", (int) (immed_fixpoint (item) * 256.0));
    else
	printf ("#0\n");
}

int isfixpoint (char *item)
{
    // determines if a variable is fixed point, and if so, what kind
    int i;
    removeCR (item);
    for (i = 0; i < numfixpoint88; ++i)
	if (!strcmp (item, fixpoint88[0][i]))
	    return 8;
    for (i = 0; i < numfixpoint44; ++i)
	if (!strcmp (item, fixpoint44[0][i]))
	    return 4;
    if (findpoint (item) < 500)
	return 12;
    return 0;
}

void set_romsize (char *size)
{
    if (romsize_already_set)
	prerror ("rom size was specified more than once.");
    romsize_already_set = 1;

    if (!strncmp (size, "32k\0", 3))
    {
	romsize = 32;
	strcpy (redefined_variables[numredefvars++], "ROM32K = 1");
	if (strncmp (size + 3, "RAM", 3) == 0)
	{
	    append_a78info ("set ram@4000");
	    strcpy (redefined_variables[numredefvars++], "SGRAM = 1");
	}
    }

    else if (!strncmp (size, "16k\0", 3))
    {
	romsize = 16;
	strcpy (redefined_variables[numredefvars++], "ROM16K = 1");
    }
    else if (!strncmp (size, "8k\0", 2))
    {
	romsize = 8;
	strcpy (redefined_variables[numredefvars++], "ROM8K = 1");
    }

    else if (!strncmp (size, "48k\0", 3))
    {
	romsize = 48;
	strcpy (redefined_variables[numredefvars++], "ROM48K = 1");
    }
    else if (!strncmp (size, "52k\0", 3))
    {
	romsize = 52;
	strcpy (redefined_variables[numredefvars++], "ROM52K = 1");
    }
    else if (!strncmp (size, "144k\0", 4))
    {
	romsize = 144;
	strcpy (redefined_variables[numredefvars++], "ROM144K = 1");
	strcpy (redefined_variables[numredefvars++], "ROMAT4K = 1");
	strcpy (redefined_variables[numredefvars++], "bankswitchmode = 9");
	bankcount = 9;
	currentbank = 0;
	romat4k = 1;
	append_a78info ("set supergame");
	append_a78info ("set rom@4000");
    }
    else if (!strncmp (size, "128k\0", 4))
    {
	romsize = 128;
	strcpy (redefined_variables[numredefvars++], "ROM128K = 1");
	strcpy (redefined_variables[numredefvars++], "bankswitchmode = 8");
	bankcount = 8;
	currentbank = 0;
	append_a78info ("set supergame");
	if (strncmp (size + 4, "BANKRAM", 7) == 0)
	{
	    append_a78info ("set bankram");
	    strcpy (redefined_variables[numredefvars++], "SGRAM = 1");
	    strcpy (redefined_variables[numredefvars++], "BANKRAM = 1");
	}
	else if (strncmp (size + 4, "RAM", 3) == 0)
	{
	    if (banksetrom)
		append_a78info ("set hram@4000");
	    else
		append_a78info ("set supergameram");
	    strcpy (redefined_variables[numredefvars++], "SGRAM = 1");
	}
    }
    else if (!strncmp (size, "256k\0", 4))
    {
	romsize = 256;
	strcpy (redefined_variables[numredefvars++], "ROM256K = 1");
	strcpy (redefined_variables[numredefvars++], "bankswitchmode = 16");
	bankcount = 16;
	currentbank = 0;
	append_a78info ("set supergame");
	if (strncmp (size + 4, "BANKRAM", 7) == 0)
	{
	    append_a78info ("set supergamebankram");
	    strcpy (redefined_variables[numredefvars++], "SGRAM = 1");
	    strcpy (redefined_variables[numredefvars++], "BANKRAM = 1");
	}
	else if (strncmp (size + 4, "RAM", 3) == 0)
	{
	    if (banksetrom)
		append_a78info ("set hram@4000");
	    else
		append_a78info ("set supergameram");
	    strcpy (redefined_variables[numredefvars++], "SGRAM = 1");
	}
	else
	    append_a78info ("set supergame");
    }
    else if (!strncmp (size, "272k\0", 4))
    {
	romsize = 272;
	strcpy (redefined_variables[numredefvars++], "ROM272K = 1");
	strcpy (redefined_variables[numredefvars++], "ROMAT4K = 1");
	strcpy (redefined_variables[numredefvars++], "bankswitchmode = 17");
	bankcount = 17;
	currentbank = 0;
	romat4k = 1;
	append_a78info ("set supergame");
	append_a78info ("set rom@4000");
    }
    else if (!strncmp (size, "512k\0", 4))
    {
	romsize = 512;
	strcpy (redefined_variables[numredefvars++], "ROM512K = 1");
	strcpy (redefined_variables[numredefvars++], "bankswitchmode = 32");
	bankcount = 32;
	currentbank = 0;
	append_a78info ("set supergame");
	if (strncmp (size + 4, "BANKRAM", 7) == 0)
	{
	    append_a78info ("set supergamebankram");
	    strcpy (redefined_variables[numredefvars++], "SGRAM = 1");
	    strcpy (redefined_variables[numredefvars++], "BANKRAM = 1");
	}
	else if (strncmp (size + 4, "RAM", 3) == 0)
	{
	    append_a78info ("set supergameram");
	    strcpy (redefined_variables[numredefvars++], "SGRAM = 1");
	}
	else
	    append_a78info ("set supergame");
    }
    else if (!strncmp (size, "528k\0", 4))
    {
	romsize = 528;
	strcpy (redefined_variables[numredefvars++], "ROM528K = 1");
	strcpy (redefined_variables[numredefvars++], "ROMAT4K = 1");
	strcpy (redefined_variables[numredefvars++], "bankswitchmode = 33");
	bankcount = 33;
	currentbank = 0;
	romat4k = 1;
	append_a78info ("set supergame");
	append_a78info ("set rom@4000");
    }
    else
    {
	prerror ("unsupported ROM size");
    }
}

void lockzone (char **statement)
{
    //  1       2
    // lockzone #

    assertminimumargs (statement, "lockzone", 1);
    removeCR (statement[2]);
    int zone = strictatoi (statement[2]);

    printf (" ldx #%d\n", zone);
    printf (" jsr lockzonex\n");

    if (zonelocking == 0)
    {
	strcpy (redefined_variables[numredefvars++], "CHECKOVERWRITE = 1");
	strcpy (redefined_variables[numredefvars++], "ZONELOCKS = 1");
	zonelocking = 1;
    }
}

void unlockzone (char **statement)
{
    //  1         2
    // unlockzone #

    assertminimumargs (statement, "unlockzone", 1);
    removeCR (statement[2]);
    int zone = strictatoi (statement[2]);

    printf (" ldx #%d\n", zone);
    printf (" jsr unlockzonex\n");
    if (zonelocking == 0)
    {
	strcpy (redefined_variables[numredefvars++], "CHECKOVERWRITE = 1");
	strcpy (redefined_variables[numredefvars++], "ZONELOCKS = 1");
	zonelocking = 1;
    }
}

void shakescreen (char **statement)
{
    //  1          2
    // shakescreen lo|med|hi|off

    int shakeamount = (zoneheight-1);

    assertminimumargs (statement, "shakescreen", 1);

    removeCR (statement[2]);
    if (statement[2][0] == 'l')
	shakeamount = shakeamount/4;
    else if (statement[2][0] == 'm')
	shakeamount = shakeamount/2;
    else if (statement[2][0] == 'h')
	shakeamount = shakeamount;
    else if (statement[2][0] == 'o')
    {
	printf ("    lda DLLMEM+9\n");
	printf ("    and #%%11110000\n");
	printf ("    ora #%d\n",zoneheight-1);
	printf ("    sta DLLMEM+9\n");
	printf ("  ifconst DOUBLEBUFFER\n");
	printf ("    ldy doublebufferstate\n");
	printf ("    beq [.+5]\n");
	printf ("    sta.w DLLMEM+DBOFFSET+9\n");
	printf ("  endif ; DOUBLEBUFFER\n");

	return;
    }
    else
	prerror ("unsupported shakescreen argument");

    printf ("    jsr randomize\n");
    printf ("    and #%d\n", shakeamount);
    printf ("    eor DLLMEM+9\n");
    printf ("    sta DLLMEM+9\n");
    printf ("  ifconst DOUBLEBUFFER\n");
    printf ("    ldy doublebufferstate\n");
    printf ("    beq [.+5]\n");
    printf ("    sta.w DLLMEM+DBOFFSET+9\n");
    printf ("  endif ; DOUBLEBUFFER\n");


}

void bank (char **statement)
{
    //  1   2
    // bank #

    assertminimumargs (statement, "bank", 1);

    removeCR (statement[2]);

    int requestedbank = strictatoi (statement[2]) - 1;

    if (bankcount == 0)
	prerror ("bank statement encountered, but non-banking rom format was selected");

    if (requestedbank == 0)	//we don't care about the first "bank" definition
	return;

    if (requestedbank >= bankcount)
	prerror ("bank statement exceedes total banks in rom format");

    // 1. dump and clear any incgraphics from the current bank before changing
    barf_graphic_file ();

    orgprintf (" if START_OF_ROM = . ; avoid dasm empty start-rom truncation.\n");
    orgprintf ("     .byte 0\n");
    orgprintf (" endif\n");
    orgprintf ("START_OF_ROM SET 0 ; scuttle so we always fail subsequent banks\n");

    // 2.issue ORG,RORG
    currentbank = requestedbank;
    if (romat4k == 1)
	orgprintf (" ORG $%04X,0\n", (currentbank + 1) * 0x4000);
    else
	orgprintf (" ORG $%04X,0\n", (currentbank + 2) * 0x4000);
    if (currentbank == (bankcount - 1))	//last bank
	orgprintf (" RORG $C000\n");
    else
	orgprintf (" RORG $8000\n");

    // a bit kludgey, but we need this module in the first bit of the last bankset bank
    // instead of the last 4k, where it goes normally.
    if ((banksetrom == 1) && (currentbank == (bankcount - 1)))
    {
	// gfxprint only outputs to the bankset bank, when the bankset scheme is used.
	gfxprintf ("     ifnconst included.hiscore.asm\n");
	gfxprintf ("         include hiscore.asm\n");
	gfxprintf ("     endif ; included.hiscore.asm\n");
    }
}

void dmahole (char **statement)
{
    //  1      2   3
    // dmahole # [noflow]

    int requestedhole;
    int noflow = FALSE;

    assertminimumargs (statement, "dmahole", 1);

    if (banksetrom == 1)
    {
	prwarn ("the dmahole command was ignored. dmahole isn't supported with banksets.");
	return;
    }

    removeCR (statement[2]);
    removeCR (statement[3]);

    if ((statement[3] != 0) && (strncmp (statement[3], "noflow", 8) == 0))
	noflow = TRUE;

    requestedhole = strictatoi (statement[2]);

    if (!noflow)
    {
	if (bankcount == 0)
	    printf (" jmp dmahole_%d\n", requestedhole);
	else
	    printf (" jmp dmahole_%d_%d\n", requestedhole, currentbank);
    }

    if (requestedhole > 0)
	printf ("DMAHOLEEND%d SET .\n", requestedhole - 1);

    sprintf (stdoutfilename, "7800hole.%d.asm", requestedhole);
    if ((stdoutfilepointer = freopen (stdoutfilename, "w", stdout)) == NULL)
    {
	prerror ("couldn't create the %s file", stdoutfilename);
    }

    if (!noflow)
    {
	if (bankcount == 0)
	    printf ("dmahole_%d\n", requestedhole);
	else
	    printf ("dmahole_%d_%d\n", requestedhole, currentbank);
    }

    currentdmahole = requestedhole;
    printf ("DMAHOLESTART%d SET .\n", requestedhole);
}

void voice (char **statement)
{
    //  1      2
    // voice on/off

    assertminimumargs (statement, "voice", 1);

    removeCR (statement[2]);

    if (strcmp (statement[2], "off") == 0)
	printf ("    lda #$ff\n");
    else
	printf ("    lda #0\n");
    printf ("    sta avoxenable\n");
}


void characterset (char **statement)
{
    assertminimumargs (statement, "characterset", 1);

    removeCR (statement[2]);
    printf ("    lda #>%s\n", statement[2]);
    printf ("    sta sCHARBASE\n\n");
    printf ("    sta CHARBASE\n");

    printf ("    lda #(%s_mode | %%01100000)\n", statement[2]);
    printf ("    sta charactermode\n\n");

    strcpy (currentcharset, statement[2]);
}

void displaymode (char **statement)
{
    assertminimumargs (statement, "displaymode", 1);

    removeCR (statement[2]);
    if ((strncmp (statement[2], "160A", 4) == 0) || (strncmp (statement[2], "160B", 4) == 0))
    {
	if (doublewide == 1)
	{
	    printf ("    lda #%%01010000 ;Enable DMA, mode=160x2/160x4, 2x character width\n");
	}
	else
	{
	    printf ("    lda #%%01000000 ;Enable DMA, mode=160x2/160x4\n");
	}
    }
    else if ((strncmp (statement[2], "320A", 4) == 0) || (strncmp (statement[2], "320C", 4) == 0))
    {
	if (doublewide == 1)
	{
	    printf ("    lda #%%01010011 ;Enable DMA, mode=160x2/160x4, 2x character width\n");
	}
	else
	{
	    printf ("    lda #%%01000011 ;Enable DMA, mode=160x2/160x4\n");
	}
    }
    else if ((strncmp (statement[2], "320B", 4) == 0) || (strncmp (statement[2], "320D", 4) == 0))
    {
	if (doublewide == 1)
	{
	    printf ("    lda #%%01010010 ;Enable DMA, mode=160x2/160x4, 2x character width\n");
	}
	else
	{
	    printf ("    lda #%%01000010 ;Enable DMA, mode=160x2/160x4\n");
	}
    }
    else
    {
	prerror ("illegal '%s' display mode specified", statement[2]);
    }


    printf ("    sta CTRL\n\n");
    printf ("    sta sCTRL\n\n");
}

int gettallspriteindex (char *needle)
{

    int t;

    if (tallspritemode == 0)
	return (-1);
    for (t = 0; t < TALLSPRITEMAX ; t++)
    {
	if (strcmp (tallspritelabel[t], needle) == 0)
	{
	    return (t);
	}
    }
    return (-1);
}


void plotsprite (char **statement, int fourbytesprite)
{
    //    1          2         3    4 5   6        7
    //plotsprite spritename palette x y [frame] [tallheight]

    // temp1 = lowbyte_of_sprite (adjusted for "frame")
    // temp2 = hibyte_of_sprite
    // temp3 = palette | width
    // temp4 = x
    // temp5 = y


    assertminimumargs (statement, "plotsprite", 4);

    int len = strlen(statement[2]);
    char *pagedelim = strchr(statement[2],'@');
    int rampage = 0;

    if (pagedelim != NULL)
    {
        pagedelim[0]=0; // split the pre-delimiter and post-delimiter into 2 strings

        if (pagedelim[1]==0)
            rampage = 0x40;
        else
            rampage = strictatoi (pagedelim+1);
        if (rampage > 255)
            prerror ("plotsprite graphic redirection '%s' is >255 (%s)", statement[2],pagedelim+1);

        // note: if rampage<0 then the user is using a variable for the ram page
    }

    if(fourbytesprite && firstfourbyte)
    {
	strcpy (redefined_variables[numredefvars++], "PLOTSP4 = 1");
	sprintf (constants[numconstants++], "PLOTSP4");
        firstfourbyte = 0;
    }
    if(fourbytesprite)
    {
        printf(" if %s_width = 32\n echo \"*** ERROR: plotsprite4 encountered sprite 32 bytes wide (%s)\"\n",statement[2],statement[2]); 
        printf(" echo \"*** plotsprite4 is limited to sprites 31 bytes wide or less.\"\n ERR\n endif\n");
    }

    int tsi = gettallspriteindex (statement[2]);

    if ((statement[6][0] != 0) && (statement[6][0] != ':') && (statement[6][0] != '0'))
    {
	removeCR (statement[6]);

	printf ("    lda #<%s\n", statement[2]);
	if ((tsi >= 0) && (tallspritemode != 2) && (deprecatedframeheight == 0))
	    printf ("    ldy #(%s_width*%d)\n", statement[2], tallspriteheight[tsi]);
	else if ((statement[6][0] != 0) && (statement[6][0] != ':')
		 && (statement[7][0] != 0) && (statement[7][0] != ':') && (deprecatedframeheight == 0))
	{
	    removeCR (statement[7]);
	    printf ("    ldy #(%s_width*%s)\n", statement[2], statement[7]);
	}
	else
	    printf ("    ldy #%s_width\n", statement[2]);
	printf ("    beq plotspritewidthskip%d\n", templabel);
	printf ("plotspritewidthloop%d\n", templabel);
	printf ("      clc\n");
	printf ("      adc ");
	printimmed (statement[6]);
	printf ("%s\n", statement[6]);
	printf ("      dey\n");
	printf ("      bne plotspritewidthloop%d\n", templabel);
	printf ("plotspritewidthskip%d\n", templabel);
	printf ("    sta temp1\n\n");

	templabel++;

    }
    else
    {
	printf ("    lda #<%s\n", statement[2]);
	printf ("    sta temp1\n\n");
    }

    if (pagedelim != NULL)
    {
        if (rampage < 0)
            printf ("    lda %s\n", pagedelim+1);
        else
            printf ("    lda #%d\n", rampage);
    }
    else
        printf ("    lda #>%s\n", statement[2]);

    printf ("    sta temp2\n\n");

    if ((statement[3][0] >= '0') && (statement[3][0] <= '9'))
    {
	int palette;
	palette = strictatoi (statement[3]);
	printf ("    lda #(%d|%s_width_twoscompliment)\n", palette << 5, statement[2]);
	printf ("    sta temp3\n\n");
    }
    else
    {
	printf ("    lda ");
	printimmed (statement[3]);
	printf ("%s\n", statement[3]);
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    ora #%s_width_twoscompliment\n", statement[2]);
	printf ("    sta temp3\n\n");
    }

    printf ("    lda ");
    printimmed (statement[4]);
    printf ("%s\n", statement[4]);	//X
    printf ("    sta temp4\n\n");

    printf ("    lda ");
    printimmed (statement[5]);
    printf ("%s\n", statement[5]);	//Y
    printf ("    sta temp5\n\n");

    if(!fourbytesprite)
    {
        printf ("    lda #(%s_mode|%%01000000)\n", statement[2]);
        printf ("    sta temp6\n\n");
        jsr ("plotsprite");
    }
    else
        jsr ("plotsprite4");

    if ((statement[6][0] != 0) && (statement[6][0] != ':') && (statement[7][0] != 0) && (statement[7][0] != ':'))
    {
	int tsheight, t;
	removeCR (statement[7]);
	tsheight = atoi (statement[7]);
	for (t = 1; t < tsheight; t++)
	{
	    printf ("    ; +tall sprite replot\n");
	    printf ("    clc\n");
	    printf ("    lda temp1\n");
	    printf ("    adc #%s_width\n", statement[2]);
	    printf ("    sta temp1\n");
	    printf ("    lda temp5\n");
	    printf ("    adc #WZONEHEIGHT\n");
	    printf ("    sta temp5\n");
            if(!fourbytesprite)
	        printf ("    jsr plotsprite\n");
            else
	        printf ("    jsr plotsprite4\n");
	}
    }
    else if ((tsi >= 0) && (tallspritemode != 2))
    {
	int t;

	printf ("  ifconst TALLCLIP\n");
	printf ("      lda #0\n");
	printf ("      ldy temp5\n");
	printf ("      cpy #(WSCREENHEIGHT)\n");
	printf ("      adc #$FF\n");
	printf ("      sta temp7 ; on-screen: temp7=0, off-screen: temp7=$ff\n");
	printf ("  endif ; TALLCLIP\n");

	for (t = 1; t < tallspriteheight[tsi]; t++)
	{
	    printf ("    ; +tall sprite replot\n");
	    printf ("    clc\n");
	    printf ("    lda temp1\n");
	    printf ("    adc #%s_width\n", statement[2]);
	    printf ("    sta temp1\n");
	    printf ("    lda temp5\n");
	    printf ("    adc #WZONEHEIGHT\n");
	    printf ("    sta temp5\n");
	    printf ("  ifconst TALLCLIP\n");
	    printf ("      ora temp7\n");
	    printf ("      cmp #(WSCREENHEIGHT)\n");
	    printf ("      bcs .plotexit_%d\n",plotlabel);
	    printf ("  endif ; TALLCLIP\n");
            if(!fourbytesprite)
	        printf ("    jsr plotsprite\n");
            else
	        printf ("    jsr plotsprite4\n");
	}
	    printf (".plotexit_%d\n",plotlabel);
            plotlabel++;
    }
}

void PLOTSPRITE (char **statement, int fourbytesprite)
{
    //    1          2         3    4 5    6    
    //plotsprite spritename palette x y [frame]

    //a wrapper to the PLOTSPRITE* family of macros
    assertminimumargs (statement, "PLOTSPRITE", 4);
    printf(" if %s_width > 16\n echo \"*** ERROR: PLOTSPRITE encountered sprite wider than 16 bytes. (%s)\"\n",statement[2],statement[2]); 
    printf(" echo \"*** PLOTSPRITE/PLOTSPRITE4 is limited to sprites 16 bytes wide or less.\"\n ERR\n endif\n");

    if(fourbytesprite)
    {
        if (isimmed (statement[3])) // palette is a constant
            printf ("MACARG2CONST SET 1\n");
        else // palette is a variable
            printf ("MACARG2CONST SET 0\n");
        if (isimmed (statement[4])) // x is a constant
            printf ("MACARG3CONST SET 1\n");
        else // x is a variable
            printf ("MACARG3CONST SET 0\n");
        if (isimmed (statement[5])) // y is a constant
            printf ("MACARG4CONST SET 1\n");
        else // y is a variable
            printf ("MACARG4CONST SET 0\n");
	if (optionalargused(statement[6]) && !isimmed (statement[6])) // const
            printf ("MACARG5CONST SET 0\n");
        else // frame is a variable
            printf ("MACARG5CONST SET 1\n");

        printf (" PLOTSPRITE4 %s,%s,%s,%s,%s\n",statement[2],statement[3],statement[4],statement[5],statement[6]);

    }
    else //!fourbytesprite
    {
        if (isimmed (statement[3])) // palette is a constant
            printf ("MACARG2CONST SET 1\n");
        else // palette is a variable
            printf ("MACARG2CONST SET 0\n");
        if (isimmed (statement[4])) // x is a constant
            printf ("MACARG3CONST SET 1\n");
        else // x is a variable
            printf ("MACARG3CONST SET 0\n");
        if (isimmed (statement[5])) // y is a constant
            printf ("MACARG4CONST SET 1\n");
        else // y is a variable
            printf ("MACARG4CONST SET 0\n");
	if (optionalargused(statement[6]) && !isimmed (statement[6])) // const
            printf ("MACARG5CONST SET 0\n");
        else // frame is a variable
            printf ("MACARG5CONST SET 1\n");

        printf (" PLOTSPRITE %s,%s,%s,%s,%s\n",statement[2],statement[3],statement[4],statement[5],statement[6]);
    }
}

int optionalargused(char *statement)
{
    removeCR (statement);
    return ((statement[0] != 0) && (statement[0] != ':'));
}

void plotbanner (char **statement)
{
    //    1          2         3    4 5
    //plotbanner bannername palette x y

    // temp1 = lowbyte_of_sprite
    // temp2 = hibyte_of_sprite
    // temp3 = palette | width
    // temp4 = x
    // temp5 = y

    assertminimumargs (statement, "plotbanner", 4);

    int q, t;
    int zonecount;

    int width;
    int palette;

    int wi;

    //get the banner name, and look up the zone count for it
    for (q = 0; q < 1000; q++)
    {
	if ((q == 999) || (bannerfilenames[q][0] == 0))
	{
	    if (passes==0)
                return;
	    else
	        prerror ("plotbanner didn't find a banner height for %s", statement[2]);
	}

	if (strcmp (bannerfilenames[q], statement[2]) == 0)
	    break;
    }

    zonecount = bannerheights[q];
    width = bannerwidths[q] + 1;	//width of image in 32-byte chunks.

    for (wi = 0; wi < width; wi++)
    {
	if (wi == (width - 1))
	{
	    if (((statement[3][0] >= '0') && (statement[3][0] <= '9'))
		|| (statement[3][0] == '$') || (statement[3][0] == '%'))
	    {
		palette = strictatoi (statement[3]);
		printf ("    lda #(%d|%s00_width_twoscompliment)\n", palette << 5, statement[2]);
		printf ("    sta temp3\n\n");
	    }
	    else
		prerror ("plotbanner only works with a numeric palette argument");
	}
	else
	{
	    palette = strictatoi (statement[3]);
	    printf ("    lda #(%d)\n", palette << 5);
	    printf ("    sta temp3\n\n");

	}

	printf ("    lda ");
	printimmed (statement[4]);
	printf ("%s\n", statement[4]);	//X
	if (wi > 0)
	{
	    printf ("    clc\n");
	    printf ("    adc #%d\n", bannerpixelwidth[q]);	// add x-offset for new column of banner
	}
	printf ("    sta temp4\n\n");

	printf ("    lda ");
	printimmed (statement[5]);
	printf ("%s\n", statement[5]);	//Y
	printf ("    sta temp5\n\n");

	printf ("    lda #(%s00_mode|%%01000000)\n", statement[2]);
	printf ("    sta temp6\n\n");

	for (t = 0; t < zonecount; t++)
	{
	    printf ("    lda #<(%s%02d + %d)\n", statement[2], t, wi * 32);
	    printf ("    sta temp1\n\n");

	    printf ("    lda #>(%s%02d + %d)\n", statement[2], t, wi * 32);
	    printf ("    sta temp2\n\n");

	    jsr ("plotsprite");

	    if (t != (zonecount - 1))
	    {
		//advance the Y coordinate for the next line of graphics...
		printf ("    clc\n");
		printf ("    lda #%d\n", zoneheight);
		printf ("    adc temp5\n");
		printf ("    sta temp5\n");
	    }
	}
    }

}


void sinedata (char **statement)
{

    int t;
    // sinedata generates a sine data table

    //      1          2        3            [   4.0      [    5.0      [ 6  [     7    ]]]]
    //   sinedata    label  wave-len(bytes)  [wave-cycles [phase-offset [amp [amp-offset]]]]
    assertminimumargs (statement, "sidedata", 2);
    if (!(optimization & 4))
	printf ("	JMP .skip%s\n", statement[0]);
    printf ("%s\n", statement[2]);

    for (t = 3; t < 8; t++)
	removeCR (statement[t]);

    int wavelength, waveamplitude, waveamplitudeoffset;
    double waveindex, wavecycles, wavephaseoffset;
    int value;

    wavelength = strictatoi (statement[3]);
    if ((statement[4] != NULL) && (statement[4][0] != 0))
	wavecycles = atof (statement[4]);
    else
	wavecycles = 1.0;

    if ((statement[5] != NULL) && (statement[5][0] != 0))
	wavephaseoffset = atof (statement[5]);
    else
	wavephaseoffset = 0.0;

    if ((statement[6] != NULL) && (statement[6][0] != 0))
	waveamplitude = strictatoi (statement[6]);
    else
	waveamplitude = 127;

    if ((statement[7] != NULL) && (statement[7][0] != 0))
	waveamplitudeoffset = strictatoi (statement[7]);
    else
	waveamplitudeoffset = 0;

    if ((wavelength < 1) || (wavelength > 256))
	prerror ("invalid wavelength used for sinedata");

    for (t = 0; t < wavelength; t++)
    {
	if ((t % 16) == 0)
	{
	    printf ("\n");
	    if (t != wavelength)
		printf (" .byte ");
	}
	waveindex = (((double) t * wavecycles * 2.0 * M_PI) / (double) wavelength) + (wavephaseoffset * 2.0 * M_PI);
	//value = (sin(waveindex) * ((double) waveamplitude + 0.5)) + (double) waveamplitudeoffset;
	value = (sin (waveindex) * ((double) waveamplitude) + 0.5) + (double) waveamplitudeoffset;
	printf ("$%02x", (value & 0xff));
	if ((t % 16 != 15))
	    printf (",");
    }
    printf ("\n");
    printf (".skip%s\n", statement[0]);

}


int inlinealphadata (char **statement)
{
    // utility function. takes statement[2] inline quoted text and fixes the space characters.
    // it then replaces the statement[2] literal string with the new data statement

    //      1          2             3            4
    //   alphadata uniquelabel graphicslabel [extrawide|singlewide]
    //  'mytext strings here'

    int t, charoffset;
    int quotelen = 0;

    if (currentcharset[0] == '\0')
	prerror ("the characterset statement needs to precede a command with an inline string");


    for (t = 0; statement[2][t] != '\0'; t++)
	if (statement[2][t] == '^')
	    statement[2][t] = ' ';

    if (banksetrom == 0)
    {
	printf ("	JMP skipalphadata%d\n", templabel);
    }
    gfxprintf ("alphadata%d\n", templabel);

    for (t = 1; (statement[2][t] != '\'') && (statement[2][t] != '\0'); t++)
    {
	charoffset = lookupcharacter (statement[2][t]);
	quotelen++;
	if (charoffset < 0)
	{
	    prerror ("plotchars character '%c' is missing from alphachars", statement[2][t]);
	}
	if ((doublewide == 1) && (strncmp (statement[6], "singlewide", 10) != 0))
	    charoffset = charoffset * 2;
	if (strncmp (statement[6], "extrawide", 9) == 0)
	{
	    quotelen++;
	    charoffset = charoffset * 2;
	}
	gfxprintf (" .byte (<%s + $%02x)\n", currentcharset, charoffset);
	if (strncmp (statement[6], "extrawide", 9) == 0)
	{
	    gfxprintf (" .byte (<%s + $%02x)\n", currentcharset, charoffset + 1);
	}
    }
    if (banksetrom == 0)
    {
	printf ("skipalphadata%d\n", templabel);
    }
    sprintf (statement[2], "alphadata%d", templabel);
    templabel++;
    return (quotelen);
}

void dostrcpy(char **statement)
{
    //           1        2             3
    //        strcpy   destination 'string literal'


    int autotextwidth = 0;

    assertminimumargs (statement, "strcpy", 2);

    if (statement[3][0] == '\'')
	autotextwidth = inlinealphadata (statement+1);
    else
	prerror ("strcpy requires a string literal");

    printf(" ldy #%d\n",autotextwidth);
    printf ("copystr%d\n", templabel);
    printf(" lda [%s-1],y\n",statement[3]);
    printf(" sta [%s-1],y\n",statement[2]);
    printf(" dey\n");
    printf (" bne copystr%d\n", templabel++);
}


void plotchars (char **statement)
{
    //            1        2          3        4     5                   6
    //        plotchars text_data palette screen_x screen_y output_text_width|"extrawide"


    // temp1 = lowbyte of character map
    // temp2 = hibyte of character map
    // temp3 = palette | width
    // temp4 = x
    // temp5 = y

    int paletteval;
    int widthval = 0;
    int overflowval = 0;
    int autotextwidth = 0;

    assertminimumargs (statement, "plotchars", 4);

    if (statement[2][0] == '\'')
    {
	autotextwidth = inlinealphadata (statement);
    }

    removeCR (statement[6]);

    //check if width is a decimal constant. If so, we can avoid opcodes and cycles.
    if (autotextwidth > 0)
	widthval = autotextwidth;
    else if (((statement[6][0] >= '0') && (statement[6][0] <= '9'))
	     || (statement[6][0] == '$') || (statement[6][0] == '%'))
	widthval = strictatoi (statement[6]);
    else
    {
	//width
	printf ("    lda #0\n");
	printf ("    sec\n");
	printf ("    sbc ");
	printimmed (statement[6]);
	printf ("%s\n", statement[6]);
	printf ("    and #%%00011111\n");
        widthval=-1;
    }
    if (widthval==0)
	    prerror ("width value can not be 0");
    else if (widthval>0)
    {
        if(widthval>32)
        {
            overflowval = widthval-32;
            widthval=32;
        }
	printf ("    lda #%d ; width in two's complement\n", ((0 - widthval) & 31));
    }

    //check if palette is  a decimal constant. If so, we can avoid opcodes and cycles.
    if (((statement[3][0] >= '0') && (statement[3][0] <= '9')) || (statement[3][0] == '$') || (statement[3][0] == '%'))
    {
	paletteval = strictatoi (statement[3]);
	if (paletteval > 7)
	{
	    prerror ("palette value must range from 0 to 7");
	}

	printf ("    ora #%d ; palette left shifted 5 bits\n", paletteval << 5);
	printf ("    sta temp3\n");
    }
    else
    {
	//palette as a variable
	printf ("    sta temp3\n");
	printf ("    lda ");
	printimmed (statement[3]);
	printf ("%s\n", statement[3]);
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    ora temp3\n");
	printf ("    sta temp3\n");
    }

    printf ("    lda #<%s\n", statement[2]);
    printf ("    sta temp1\n\n");

    printf ("    lda #>%s\n", statement[2]);
    printf ("    sta temp2\n\n");

    printf ("    lda ");
    printimmed (statement[4]);
    printf ("%s\n", statement[4]);
    printf ("    sta temp4\n\n");

    printf ("    lda ");
    printimmed (statement[5]);
    printf ("%s\n", statement[5]);
    printf ("    sta temp5\n\n");

    jsr ("plotcharacters");

    if (overflowval)
    {
	printf ("    ; more than 32 chars were plotted, so plot the rest\n");
	printf ("    lda temp3\n");
	printf ("    eor #($%02x) ; fix width\n", ((0 - overflowval) & 31)^((0 - widthval) & 31));
	printf ("    sta temp3\n\n");

        printf ("    lda #<(%s+32) ; fix string offset\n", statement[2]);
        printf ("    sta temp1\n\n");

        printf ("    lda #>(%s+32) ; fix string offset\n", statement[2]);
        printf ("    sta temp2\n\n");

	if ((doublewide == 1) && (strncmp (statement[6], "singlewide", 10) != 0))
	    prerror ("plotchars can't plot more than 32 doublewide characters.");
	else if (strncmp (statement[6], "extrawide", 9) == 0)
	    prerror ("plotchars can't plot more than 32 extrawide characters.");
        else
            printf ("    lda #%d ; fix X\n",32*4); 

        printf ("    clc\n");
        printf ("    adc temp4\n");
        printf ("    sta temp4\n\n");

        jsr ("plotcharacters");
    }

}

void plotvalue (char **statement)
{
    //            1        2          3          4             5             6      7          8
    //        plotvalue digit_gfx palette variable/data number_of_digits screen_x screen_y [extrawide]


    // asm sub arguments:
    // temp1=lo charactermap
    // temp2=hi charactermap
    // temp3=palette | width byte
    // temp4=x
    // temp5=y
    // temp6=number of digits
    // temp7=lo variable
    // temp8=hi variable

    int paletteval;
    int widthval;

    assertminimumargs (statement, "plotvalue", 6);

    printf ("    lda #<%s\n", statement[2]);
    printf ("    sta temp1\n\n");

    printf ("    lda #>%s\n", statement[2]);
    printf ("    sta temp2\n\n");

    printf ("    lda charactermode\n");
    printf ("    sta temp9\n");
    printf ("    lda #(%s_mode | %%01100000)\n", statement[2]);
    printf ("    sta charactermode\n");

    removeCR (statement[7]);
    removeCR (statement[8]);

    //check if width is a decimal constant. If so, we can avoid opcodes and cycles.
    if (((statement[5][0] >= '0') && (statement[5][0] <= '9')) || (statement[5][0] == '$') || (statement[5][0] == '%'))
    {
	widthval = strictatoi (statement[5]);
	if (strncmp (statement[8], "extrawide", 9) == 0)
	    widthval = widthval * 2;
	if ((widthval > 32) || (widthval == 0))
	{
	    prerror ("width value must range from 1 to 32, and '%s' was used", statement[5]);
	}
	widthval = ((0 - widthval) & 31);
	printf ("    lda #%d ; width in two's complement\n", widthval);
    }
    else			//width is a label of some sort...
    {
	printf ("    lda #0\n");
	printf ("    sec\n");
	printf ("    sbc ");
	printimmed (statement[5]);
	printf ("%s\n", statement[5]);
	printf ("    and #%%00011111\n");
    }


    //check if palette is a decimal constant. If so, we can avoid opcodes and cycles.
    if (((statement[3][0] >= '0') && (statement[3][0] <= '9')) || (statement[3][0] == '$') || (statement[3][0] == '%'))
    {
	paletteval = strictatoi (statement[3]);
	if (paletteval > 7)
	{
	    prerror ("palette value must range from 0 to 7");
	}

	printf ("    ora #%d ; palette left shifted 5 bits\n", paletteval << 5);
	printf ("    sta temp3\n");
    }
    else
    {
	//palette
	printf ("    sta temp3\n");
	printf ("    lda ");
	printimmed (statement[3]);
	printf ("%s\n", statement[3]);
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    ora temp3\n");
	printf ("    sta temp3\n");
    }

    removeCR (statement[7]);

    printf ("    lda ");
    printimmed (statement[6]);
    printf ("%s\n", statement[6]);
    printf ("    sta temp4\n\n");

    printf ("    lda ");
    printimmed (statement[7]);
    printf ("%s\n", statement[7]);
    printf ("    sta temp5\n\n");

    printf ("    lda ");
    printimmed (statement[5]);
    printf ("%s\n", statement[5]);
    printf ("    sta temp6\n\n");

    printf ("    lda #<%s\n", statement[4]);
    printf ("    sta temp7\n\n");

    printf ("    lda #>%s\n", statement[4]);
    printf ("    sta temp8\n\n");

    if (strncmp (statement[8], "extrawide", 9) == 0)
    {
	jsr ("plotvalueextra");
	printf ("USED_PLOTVALUEEXTRA = 1\n");
    }
    else
    {
	jsr ("plotvalue");
	printf ("USED_PLOTVALUE = 1\n");
    }

    printf ("    lda temp9\n");
    printf ("    sta charactermode\n");

}


void plotmapfile (char **statement)
{
    //         1           2             3           4        5          6             7
    //     plotmapfile map_file.tmx  display_mem  screen_x screen_y screen_width screen_height

    // Overview
    // 1. open map file
    // 2. create an int array of widthxheight
    // 3. set each cell of the int according to the palette of the character there
    // 4. run through the array and build up the data structure for plotcharloop

    char datavalues[256][256];
    int s, t;
    int x, y, width, height;
    int tileindex = 0;

    int palettevalues[2048];
    int graphicmodes[2048];

    assertminimumargs (statement, "plotmapfile", 6);

    x = strictatoi (statement[4]);
    y = strictatoi (statement[5]);
    width = strictatoi (statement[6]);
    removeCR (statement[7]);
    height = strictatoi (statement[7]);

    fixfilename (statement[2]);

    //open file...
    FILE *fp = fopen (statement[2], "rb");
    if (!fp)
    {
	prerror ("plotmapfile couldn't open map file '%s' for reading", statement[2]);
    }


    //our default data label...
    for (t = 0; t < 256; t++)
	sprintf (datavalues[t], "palette0");
//datavalues
    for (;;)
    {
	char line[1024];
	char *keyword;
	char *firstgid;
	char *tilename;
	int gid;
	int layer = 0;

	if (fgets (line, 1024, fp) == NULL)
	    break;

	keyword = strstr (line, "tileset firstgid=\"");
	if (keyword != NULL)
	{
	    firstgid = strstr (line, "firstgid=\"");
	    tilename = strstr (line, "name=\"");
	    if ((tilename != NULL) && (firstgid != NULL))
	    {
		gid = atoi (firstgid + 10);
		tilename = tilename + 6;
		for (t = 0; t < 1024; t++)
		{
		    if (line[t] == '"')
			line[t] = 0;
		    if (line[t] == '.')
			line[t] = 0;
		}
		s = 0;
		for (t = gid; t < 256; t++)
		{
		    sprintf (datavalues[t], "%s", tilename);

		    s = s + 1;
		    if (doublewide == 1)
			s = s + 1;
		}
	    }
	}

	keyword = strstr (line, "tile gid=\"");
	if (keyword != NULL)
	{
	    int q;

	    //get the tile value...
	    gid = atoi (keyword + 10);

	    if (gid == 0)
		gid = 1;	//kludge - to work around empty characters

	    //get the image name, and look up the palette value for that image
	    for (q = 0; q < 1000; q++)
	    {
		if ((q == 999) || (palettefilenames[q][0] == 0))
		{
                    if (passes==0)
                        return;
                    else
		        prerror ("plotmapfile didn't find a palette for %s", datavalues[gid]);
		}

		if (strcmp (palettefilenames[q], datavalues[gid]) == 0)
		    break;
	    }

	    palettevalues[tileindex] = graphicfilepalettes[q];
	    graphicmodes[tileindex] = graphicfilemodes[q];

	    tileindex++;
	}

	keyword = strstr (line, "<data encoding=\"");
	if (keyword != NULL)
	{
	    fclose (fp);
	    keyword = keyword + 15;
	    for (t = 0; t < 1024; t++)
		if (line[t] == '"')
		    line[t] = 0;
	    prerror ("map file '%s' is %s encoded. XML is required", statement[2], keyword);
	}
	keyword = strstr (line, "<layer name=\"");
	if (keyword != NULL)
	{
	    layer = layer + 1;
	    if (layer > 1)
	    {
		fclose (fp);
		prerror ("map file '%s' contains multiple layers", statement[2]);
	    }
	}
    }
    fclose (fp);


    int startp;
    int pmwidth, charwidth, pmx, pmy, pmemoffset, pmwidthandpalette;
    int ix, iy;

    // convert palettevalues[] to actual data values for plotcharloop
    //      then add code to set (temp8) and jsr plotcharloop

    printf (" lda #(<plotmapfiledata%d)\n", templabel);
    printf (" sta temp8\n");
    printf (" lda #(>plotmapfiledata%d)\n", templabel);
    printf (" sta temp9\n");
    printf (" jsr plotcharloop\n");

    printf (" jmp skipplotmapfiledata%d\n", templabel);
    printf ("plotmapfiledata%d\n", templabel);

    // we now have a 2d array of the palette each character should reference
    // we need to build up data with sets of: lo_data, hi_data, palette|width, x, y
    // format ends with lo_data=0, hi_data=0
    for (iy = 0; iy < height; iy++)
    {
	ix = 0;
	for (;;)
	{
	    if (ix >= width)
		break;
	    startp = palettevalues[(iy * width) + ix];

	    // 320 mode character widths are divided by 2 because MARIA uses 160 pixels res for positioning...
	    if (graphicmodes[(iy * width) + ix] == MODE160A)
		charwidth = 4;
	    else if (graphicmodes[(iy * width) + ix] == MODE160B)
		charwidth = 2;
	    else if (graphicmodes[(iy * width) + ix] == MODE320A)
		charwidth = 8 / 2;
	    else if (graphicmodes[(iy * width) + ix] == MODE320B)
		charwidth = 4 / 2;
	    else if (graphicmodes[(iy * width) + ix] == MODE320C)
		charwidth = 4 / 2;
	    else if (graphicmodes[(iy * width) + ix] == MODE320D)
		charwidth = 8 / 2;
	    if (doublewide == 1)
		charwidth = charwidth * 2;

	    for (t = ix; (t < width) && (palettevalues[(iy * width) + t] == startp) && (t < (ix + 32)); t++);
	    t = t - 1;
	    pmwidth = t - ix + 1;
	    pmwidthandpalette = ((0 - pmwidth) & 31) | (startp << 5);
	    pmy = iy + y;

	    pmx = (ix * charwidth) + x;

	    pmemoffset = (iy * width) + ix;
	    printf (" .byte <(%s+%d), >(%s+%d), $%02x, %d, %d\n",
		    statement[3], pmemoffset, statement[3], pmemoffset, pmwidthandpalette, pmx, pmy);
	    ix = t + 1;
	}
    }

    printf (" .byte 0,0\n");

    printf ("skipplotmapfiledata%d\n", templabel);
    templabel = templabel + 1;
}

void plotmap (char **statement)
{

    //         1      2          3        4        5          6             7       [      8                9                 10       ]
    //     plotmap map_data    palette screen_x screen_y screen_width screen_height [text_map_x_offset text_map_y_offset text_map_width]

    // temp1 = lowbyte of map data
    // temp2 = hibyte of map data
    // temp3 = palette | width
    // temp4 = screen_x
    // temp5 = screen_y
    // ...

    int paletteval;
    int widthval;

    assertminimumargs (statement, "plotmap", 6);

    //check if palette is decimal constant. If so, we can avoid opcodes and cycles.
    if (((statement[6][0] >= '0') && (statement[6][0] <= '9')) || (statement[6][0] == '$') || (statement[6][0] == '%'))
    {
	widthval = strictatoi (statement[6]);
	if ((widthval > 32) || (widthval == 0))
	{
	    prerror ("width value must range from 1 to 32, and '%s' was used", statement[6]);
	}

	widthval = ((0 - widthval) & 31);
	printf ("    lda #%d ; width in two's complement\n", widthval);
    }
    else
    {
	//width
	printf ("    lda #0\n");
	printf ("    sec\n");
	printf ("    sbc ");
	printimmed (statement[6]);
	printf ("%s\n", statement[6]);
	printf ("    and #%%00011111\n");
    }

    printf ("    sta temp3\n");

    //check if palette is  a decimal constant. If so, we can avoid opcodes and cycles.
    if (((statement[3][0] >= '0') && (statement[3][0] <= '9')) || (statement[3][0] == '$') || (statement[3][0] == '%'))
    {
	paletteval = strictatoi (statement[3]);
	if (paletteval > 7)
	{
	    prerror ("palette value must range from 0 to 7");
	}

	printf ("    ora #%d ; palette left shifted 5 bits\n", paletteval << 5);
	printf ("    sta temp3\n");
    }
    else
    {
	//palette
	printf ("    sta temp3\n");
	printf ("    lda ");
	printimmed (statement[3]);
	printf ("%s\n", statement[3]);
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    asl\n");
	printf ("    ora temp3\n");
	printf ("    sta temp3\n");
    }




    printf ("    lda ");
    printimmed (statement[4]);
    printf ("%s\n", statement[4]);
    printf ("    sta temp4\n\n");

    printf ("    lda ");
    printimmed (statement[5]);
    printf ("%s\n", statement[5]);
    printf ("    sta temp5\n\n");

    //         1      2          3        4        5          6             7       [      8                9                 10       ]
    //     plotmap map_data    palette screen_x screen_y screen_width screen_height [text_map_x_offset text_map_y_offset text_map_width]
    // temp1 = lowbyte of map data
    // temp2 = hibyte of map data
    // temp3 = palette | width
    // temp4 = screen_x
    // temp5 = screen_y
    // temp6 = screen_height
    // ...

    printf ("    lda ");
    printimmed (statement[7]);
    printf ("%s\n", statement[7]);
    printf ("    sta temp6\n");

    if ((statement[8][0] == 0) || (statement[8][0] == ':'))
    {

	printf ("    lda #<%s\n", statement[2]);
	printf ("    sta temp1\n\n");
	printf ("    lda #>%s\n", statement[2]);
	printf ("    sta temp2\n\n");

	printf ("plotcharactersloop%d\n", templabel);
	//the map is the same size as the character block we're displaying...
	jsr ("plotcharacters");
	printf ("    clc\n");
	printf ("    lda ");
	printimmed (statement[6]);
	printf ("%s\n", statement[6]);
	printf ("    adc temp1\n");
	printf ("    sta temp1\n");
	printf ("    lda #0\n");
	printf ("    adc temp2\n");
	printf ("    sta temp2\n");
	printf ("    inc temp5\n");
	printf ("    dec temp6\n");
	printf ("    bne plotcharactersloop%d\n", templabel);
	templabel++;
    }
    else
    {
	//the map is a larger size compared to the character block we're displaying...


	printf ("    lda #<%s\n", statement[2]);
	printf ("    sta temp1\n\n");
	printf ("    lda #>%s\n", statement[2]);
	printf ("    sta temp2\n\n");

	// multiply text_map_y_offset*text_map_width
	printf ("    ldy ");
	printimmed (statement[9]);
	printf ("%s\n", statement[9]);
	printf ("    cpy #0\n");
	printf ("    beq skipmapyadjust%d\n", templabel);
	printf ("    lda ");
	printimmed (statement[10]);
	printf ("%s\n", statement[10]);
	printf ("    jsr mul16\n");
	printf (".calledfunction_mul16 = 1\n");	
	printf ("    ;result is in A, temp1 contains overflow\n");
	printf ("    sta temp2\n");

	printf ("    clc\n");
	printf ("    lda #<%s\n", statement[2]);
	printf ("    adc temp1\n");
	printf ("    sta temp1\n");
	printf ("    lda #>%s\n", statement[2]);
	printf ("    adc temp2\n");
	printf ("    sta temp2\n");

	printf ("skipmapyadjust%d\n", templabel);
	templabel++;

	//adjust for the x-offset...
	printf ("    clc\n");
	printf ("    lda ");
	printimmed (statement[8]);
	printf ("%s\n", statement[8]);
	printf ("    adc temp1\n");
	printf ("    sta temp1\n");
	printf ("    lda #0\n");
	printf ("    adc temp2\n");
	printf ("    sta temp2\n");

	printf ("plotcharactersloop%d\n", templabel);

	jsr ("plotcharacters");

	//add the map width to get to the next row...
	printf ("    clc\n");
	printf ("    lda ");
	printimmed (statement[10]);
	printf ("%s\n", statement[10]);
	printf ("    adc temp1\n");
	printf ("    sta temp1\n");
	printf ("    lda #0\n");
	printf ("    adc temp2\n");
	printf ("    sta temp2\n");

	printf ("    inc temp5\n");	//new row on the screen

	//reduce the line count, and continue if we're non-zero...
	printf ("    dec temp6\n");
	printf ("    bne plotcharactersloop%d\n", templabel);
	templabel++;
    }

}

void domemcpy (char **statement)
{
    //   1      2     3   4
    // memcpy dest  src  count

    int count, page;

    assertminimumargs (statement, "memcpy", 3);

    removeCR (statement[4]);
    count = strictatoi (statement[4]);

    if (count > 255)
    {
	// chip away at count a page at a time.
	// this may be a bit less rom-efficient for large ranges compared to a "proper"
	// indirect addressing solution, but this should be a lot faster and smaller
	// for ranges that aren't a lot bigger than a couple of pages.
	page = 0;
	while (count > 255)
	{
	    printf (" ldy #0\n");
	    printf ("memcpyloop%d\n", templabel);
	    printf (" lda %s+%d,y\n", statement[3], page * 256);
	    printf (" sta %s+%d,y\n", statement[2], page * 256);
	    printf (" dey\n");
	    printf (" bne memcpyloop%d\n", templabel);
	    templabel++;
	    page = page + 1;
	    count = count - 256;
	}
	if (count > 0)
	{
	    printf (" ldy #%d\n", count);
	    printf ("memcpyloop%d\n", templabel);
	    printf (" lda %s-1+%d,y\n", statement[3], page * 256);
	    printf (" sta %s-1+%d,y\n", statement[2], page * 256);
	    printf (" dey\n");
	    printf (" bne memcpyloop%d\n", templabel);
	    templabel++;
	}

    }

    if (count == 0)
	return;
    if (count == 256)
    {
	printf (" ldy #0\n");
	printf ("memcpyloop%d\n", templabel);
	printf (" lda %s,y\n", statement[3]);
	printf (" sta %s,y\n", statement[2]);
	printf (" dey\n");
	printf (" bne memcpyloop%d\n", templabel);
	templabel++;
    }
    else
    {
	printf (" ldy #%d\n", count);
	printf ("memcpyloop%d\n", templabel);
	printf (" lda %s-1,y\n", statement[3]);
	printf (" sta %s-1,y\n", statement[2]);
	printf (" dey\n");
	printf (" bne memcpyloop%d\n", templabel);
	templabel++;
    }

}

void domemset (char **statement)
{
    //   1      2     3   4
    // memset dest   val  count

    int count, val;

    assertminimumargs (statement, "memset", 3);

    removeCR (statement[4]);

    if (statement[3][0] == '\'')
    {
	if (statement[4][0] != '\'')
	{
	    val = lookupcharacter (statement[3][1]);
	    count = strictatoi (statement[4]);
	    if (doublewide == 1)
		val = val * 2;
	}
	else
	{
	    val = lookupcharacter (' ');
	    count = strictatoi (statement[5]);
	    if (doublewide == 1)
		val = val * 2;
	}
    }
    else
    {

	if (isalpha (statement[3][0]))
	{
	    val = -1;		// flag as a variable or constant
	    count = strictatoi (statement[4]);
	}
	else
	{
	    val = strictatoi (statement[3]);
	    count = strictatoi (statement[4]);
	}
    }

    if (count > 255)
    {

	printf (" lda #%d\n", count / 256);
	printf (" sta temp1\n");
	printf (" lda #<(%s)\n", statement[2]);
	printf (" sta temp2\n");
	printf (" lda #>(%s)\n", statement[2]);
	printf (" sta temp3\n");
	if (val > -1)
	{
	    printf (" lda #%d\n", val);
	}
	else
	{
	    if (isimmed (statement[3]))
		printf (" lda #%s\n", statement[3]);
	    else
		printf (" lda %s\n", statement[3]);
	}
	printf (" ldy #0\n");
	printf ("memsetloop%d\n", templabel);
	printf (" sta (temp2),y\n");
	printf (" dey\n");
	printf (" bne memsetloop%d\n", templabel);
	printf (" inc temp3\n");
	printf (" dec temp1\n");
	printf (" bne memsetloop%d\n", templabel);
	if ((count % 256) > 0)
	{
	    templabel++;
	    printf (" ldy #%d\n", (count % 256) - 1);
	    printf ("memsetloop%d\n", templabel);
	    printf (" sta (temp2),y\n");
	    printf (" dey\n");
	    printf (" bne memsetloop%d\n", templabel);
	    printf (" sta (temp2),y\n");
	}
	templabel++;
	return;
    }

    if (count == 0)
	return;
    else
    {
	printf (" ldy #%d\n", count);
	printf (" lda #%d\n", val);
	printf ("memsetloop%d\n", templabel);
	printf (" sta %s-1,y\n", statement[2]);
	printf (" dey\n");
	printf (" bne memsetloop%d\n", templabel);
	templabel++;
    }
}



void savemultiplicationtable (char *tablename, int width, int height)
{
    int t;
    //check so we don't save the same table twice...
    for (t = 0; t < multtableindex; t++)
    {
	if (strcmp (tablename, multtablename[t]) == 0)
	    return;
    }
    if (multtableindex > 98)
	return;
    //save multiplication table info. we'll later need to barf it.
    strncpy (multtablename[multtableindex], tablename, 49);
    multtablewidth[multtableindex] = width;
    multtableheight[multtableindex] = height;
    multtableindex = multtableindex + 1;
}

void barfmultiplicationtables (void)
{
    int s, t, incvalue;
    if (multtableindex > 99)
	return;			// TODO: THIS SHOULD BE A HARD ERROR
    for (t = 0; t < multtableindex; t++)
    {
	printf ("%s_mult_lo\n", multtablename[t]);
	incvalue = 0;
	for (s = 0; s < multtableheight[t]; s++)
	{
	    printf ("  .byte <(%s+%d)\n", multtablename[t], incvalue);
	    incvalue = incvalue + multtablewidth[t];
	}
	printf ("%s_mult_hi\n", multtablename[t]);
	incvalue = 0;
	for (s = 0; s < multtableheight[t]; s++)
	{
	    printf ("  .byte >(%s+%d)\n", multtablename[t], incvalue);
	    incvalue = incvalue + multtablewidth[t];
	}
    }

}

void getfade (char **statement)
{

    //   4       5     6   7   8     9
    // getfade   (   value , "black" )

    if ((statement[7][0] != ')') && (statement[9][0] != ')'))
	prerror ("bad argument count for getfade");

    if (!fourbitfade_alreadyused)
    {
	fourbitfade_alreadyused = 1;
	strcpy (redefined_variables[numredefvars++], "FOURBITFADE = 1");
    }

    templabel++;
    if ((statement[7][0] != ')') && (statement[9][0] == ')') && (statement[8][0] == 'b'))
    {
	printf ("    lda fourbitfadevalue\n");
	printf ("    beq .fadezeroskip%d\n", templabel);
    }
    printf ("    lda ");
    printimmed (statement[6]);
    printf ("%s\n", statement[6]);
    printf ("    jsr fourbitfade\n");
    printf (".fadezeroskip%d\n", templabel);
    strcpy (Areg, "invalid");

}

void setfade (char **statement)
{

    //   1      2
    // setfade  value

    assertminimumargs (statement, "setfade", 1);

    if (!fourbitfade_alreadyused)
    {
	fourbitfade_alreadyused = 1;
	strcpy (redefined_variables[numredefvars++], "FOURBITFADE = 1");
    }

    printf ("    lda ");
    printimmed (statement[2]);
    printf ("%s\n", statement[2]);
    printf ("    asl\n");
    printf ("    asl\n");
    printf ("    asl\n");
    printf ("    asl\n");
    printf ("    sta fourbitfadevalue\n");
    strcpy (Areg, "invalid");
}

void peekchar (char **statement)
{
    //   4      5   6   7 8 9  10 11 12    13 14     15
    // peekchar (   mem , x ,  y  ,  width ,  height )

    int width, height;


    if (statement[15][0] != ')')
	prerror ("bad argument count for peekchar");

    width = strictatoi (statement[12]);
    height = strictatoi (statement[14]);

    if ((width * height < 1) || (width * height > 65536))
	prerror ("bad width or height with peekchar");

    savemultiplicationtable (statement[6], width, height);

    printf ("    ldy ");
    printimmed (statement[10]);
    printf ("%s\n", statement[10]);
    printf ("    lda %s_mult_lo,y\n", statement[6]);
    printf ("    sta temp1\n");
    printf ("    lda %s_mult_hi,y\n", statement[6]);
    printf ("    sta temp2\n");
    printf ("    ldy ");
    printimmed (statement[8]);
    printf ("%s\n", statement[8]);
    printf ("    lda (temp1),y\n");
    strcpy (Areg, "invalid");

}

void pokechar (char **statement)
{
    //   1      2   3  4 5     6      7
    // pokechar mem x  y width height value

    int width, height;

    assertminimumargs (statement, "pokechar", 6);

    removeCR (statement[7]);

    width = strictatoi (statement[5]);
    height = strictatoi (statement[6]);

    if ((width * height < 1) || (width * height > 65536))
	prerror ("bad width or height with pokechar");

    savemultiplicationtable (statement[2], width, height);

    printf ("    ldy ");
    printimmed (statement[4]);
    printf ("%s\n", statement[4]);
    printf ("    lda %s_mult_lo,y\n", statement[2]);
    printf ("    sta temp1\n");
    printf ("    lda %s_mult_hi,y\n", statement[2]);
    printf ("    sta temp2\n");
    printf ("    ldy ");
    printimmed (statement[3]);
    printf ("%s\n", statement[3]);
    printf ("    lda ");
    printimmed (statement[7]);
    printf ("%s\n", statement[7]);
    printf ("    sta (temp1),y\n");

    strcpy (Areg, "invalid");

}

int strictatoi (char *numstring)
{
    if (numstring[0] == '\0')
    {
	prwarn ("bad non-variable value");
	return (-1);
    }
    //check if its a plain decimal argument...
    if (numstring[0] == '-')
	return (256 - atoi (numstring + 1));
    if ((numstring[0] >= '0') && (numstring[0] <= '9'))
	return (atoi (numstring));
    if ((numstring[0] == '$') && (numstring[1] != '\0'))
	return ((int) strtol (numstring + 1, NULL, 16));
    if ((numstring[0] == '%') && (numstring[1] != '\0'))
	return ((int) strtol (numstring + 1, NULL, 2));
    prwarn ("bad non-variable value");
    return (-1);
}

void tsound (char **statement)
{
    //   1      2     3   4
    // tsound channel , [freq] , [waveform] , [volume]

    int nextindex;
    int channel = strictatoi (statement[2]);

    if ((channel < 0) || (channel > 1))
	prerror ("illegal channel for tsound");
    nextindex = 4;
    printf (" ifnconst NOTIALOCKMUTE\n");
    if (statement[nextindex][0] == ',')
	nextindex = nextindex + 1;
    else
    {
	printf (" lda ");
	printimmed (statement[nextindex]);
	printf ("%s\n", statement[nextindex]);
	printf (" sta AUDF%d\n", channel);
	nextindex = nextindex + 2;
    }
    if (statement[nextindex][0] == ',')
	nextindex = nextindex + 1;
    else
    {
	printf (" lda ");
	printimmed (statement[nextindex]);
	printf ("%s\n", statement[nextindex]);
	printf (" sta AUDC%d\n", channel);
	nextindex = nextindex + 2;
    }
    removeCR (statement[nextindex]);
    if (statement[nextindex][0] == '\0')
    {
	printf (" endif ; NOTIALOCKMUTE\n");
	return;
    }
    if (statement[nextindex][0] == ',')
	nextindex = nextindex + 1;
    else
    {
	printf (" lda ");
	printimmed (statement[nextindex]);
	printf ("%s\n", statement[nextindex]);
	printf (" sta AUDV%d\n", channel);
	nextindex = nextindex + 2;
    }
    printf (" endif ; NOTIALOCKMUTE\n");
}

void psound (char **statement)
{
    //   1      2     3   4
    // psound channel , [freq] [ , waveform , volume ]

    int nextindex;
    int channel = 255;

    if (((statement[2][0] >= '0') && (statement[2][0] <= '9')) || (statement[2][0] == '$') || (statement[2][0] == '%'))
    {
	channel = strictatoi (statement[2]);
	if ((channel < 0) || (channel > 3))
	    prerror ("illegal channel for psound");
    }

    nextindex = 4;
    if (statement[nextindex][0] == ',')
	nextindex = nextindex + 1;
    else
    {
	if (channel != 255)
	    printf (" ldy #%d\n", (channel * 2) + 0);	//FREQ
	else
	{
	    printf (" lda ");
	    printimmed (statement[2]);
	    printf ("%s\n", statement[2]);
	    printf (" asl\n");
	    printf (" tay\n");
	}
	printf (" lda ");
	printimmed (statement[nextindex]);
	printf ("%s\n", statement[nextindex]);
	printf (" sta (pokeybase),y\n");
	nextindex = nextindex + 2;
    }

    removeCR (statement[nextindex]);
    removeCR (statement[nextindex + 1]);
    removeCR (statement[nextindex + 2]);

    if ((statement[nextindex] == 0) || (statement[nextindex + 1] == 0))
	prerror ("malformed psound statement");

    if (statement[nextindex][0] == ',')
	return;
    else
    {

	printf (" iny\n");	//WAVE/CTRL

	if ((((statement[nextindex][0] >= '0')
	      && (statement[nextindex][0] <= '9'))
	     || (statement[nextindex][0] == '$')
	     || (statement[nextindex][0] == '%'))
	    &&
	    (((statement[nextindex + 2][0] >= '0')
	      && (statement[nextindex + 2][0] <= '9'))
	     || (statement[nextindex + 2][0] == '$') || (statement[nextindex + 2][0] == '%')))
	{
	    int wave, volume;

	    wave = strictatoi (statement[nextindex]);
	    volume = strictatoi (statement[nextindex + 2]);
	    printf (" lda #%d\n", (wave * 16) + volume);
	    printf (" sta (pokeybase),y\n");

	}
	else
	{

	    printf (" lda ");
	    printimmed (statement[nextindex]);
	    printf ("%s\n", statement[nextindex]);
	    printf (" asl\n");
	    printf (" asl\n");
	    printf (" asl\n");
	    printf (" asl\n");
	    printf (" clc\n");

	    nextindex = nextindex + 1;
	    removeCR (statement[nextindex]);
	    if (statement[nextindex][0] == '\0')
		prerror ("malformed psound... volume is required with waveform");
	    nextindex = nextindex + 1;
	    removeCR (statement[nextindex]);
	    if (statement[nextindex][0] == '\0')
		prerror ("malformed psound... volume is required with waveform");

	    printf (" adc ");
	    printimmed (statement[nextindex]);
	    printf ("%s\n", statement[nextindex]);
	    printf (" sta (pokeybase),y\n");
	    return;
	}
    }

}

void snesdetect ()
{
    printf (" jsr SNES_AUTODETECT\n");
    if (!isconstantdefined ("SNES2ATARISUPPORT"))
    {
	strcpy (redefined_variables[numredefvars++], "SNES2ATARISUPPORT = 1");
	sprintf (constants[numconstants++], "SNES2ATARISUPPORT");
    }
}

void changecontrol (char **statement)
{
    //   1            2     3
    // changecontrol 0|1 controltype

    int port;

    assertminimumargs (statement, "changecontrol", 2);
    removeCR (statement[2]);
    removeCR (statement[3]);

    port = strictatoi (statement[2]);

    /*
       port#control values...
       1 = proline
       2 = lightgun
       3 = paddle
       4 = trakball
       5 = vcs joystick
       6 = driving
       7 = keypad
       8 = st mouse/cx80
       9 = amiga mouse
       10 = atarivox
     */

    if ((port != 0) && (port != 1))
	prerror ("only ports 0 or 1 are supported");

    if (!strcmp (statement[3], "2buttonjoy"))
    {
	printf ("  lda #1 ; controller=joystick\n");
	if (port == 0)
	{
	    printf ("  sta port0control\n");
	    printf ("  ldx #0\n");
	}
	else
	{
	    printf ("  sta port1control\n");
	    printf ("  ldx #1\n");
	}
	printf ("  jsr setportforinput\n");
	printf ("  jsr settwobuttonmode\n");
    }
    if (!strcmp (statement[3], "none"))
    {
	printf ("  lda #0 ; controller=none\n");
	if (port == 0)
	{
	    printf ("  sta port0control\n");
	    printf ("  ldx #0\n");
	}
	else
	{
	    printf ("  sta port1control\n");
	    printf ("  ldx #1\n");
	}
	printf ("  jsr setportforinput\n");
	printf ("  jsr setonebuttonmode\n");
    }
    if (!strcmp (statement[3], "1buttonjoy"))
    {
	printf ("  lda #1 ; controller=joystick\n");
	if (port == 0)
	{
	    printf ("  sta port0control\n");
	    printf ("  ldx #0\n");
	}
	else
	{
	    printf ("  sta port1control\n");
	    printf ("  ldx #1\n");
	}
	printf ("  jsr setportforinput\n");
	printf ("  jsr setonebuttonmode\n");
    }
    else if (!strcmp (statement[3], "snes"))
    {
	printf ("  lda #11 ; controller=snes2atari\n");
	if (port == 0)
	{
	    printf ("  sta port0control\n");
	    printf ("  ldx #0\n");
	}
	else
	{
	    printf ("  sta port1control\n");
	    printf ("  ldx #1\n");
	}
	printf ("  jsr setportforinput\n");
	printf ("  jsr setonebuttonmode\n");

	strcpy (redefined_variables[numredefvars++], "SNES2ATARISUPPORT = 1");
	sprintf (constants[numconstants++], "SNES2ATARISUPPORT");
    }

    else if (!strcmp (statement[3], "mega7800"))
    {
	printf ("  lda #12 ; controller=mega7800\n");
	if (port == 0)
	{
	    printf ("  sta port0control\n");
	    printf ("  ldx #0\n");
	}
	else
	{
	    printf ("  sta port1control\n");
	    printf ("  ldx #1\n");
	}
	printf ("  jsr setportforinput\n");
	printf ("  jsr settwobuttonmode\n");

	strcpy (redefined_variables[numredefvars++], "MEGA7800SUPPORT = 1");
	sprintf (constants[numconstants++], "MEGA7800SUPPORT");
    }

    else if (!strcmp (statement[3], "lightgun"))
    {
	printf ("  lda #2 ; controller=lightgun\n");
	if (port == 0)
	{
	    printf ("  sta port0control\n");
	    printf ("  ldx #0\n");
	}
	else
	{
	    printf ("  sta port1control\n");
	    printf ("  ldx #1\n");
	}
	printf ("  jsr setportforinput\n");
	printf ("  jsr setonebuttonmode\n");
    }

    if (!strcmp (statement[3], "paddle"))
    {
	if (!isconstantdefined ("PADDLESUPPORT"))
	{
	    strcpy (redefined_variables[numredefvars++], "PADDLESUPPORT = 1");
	    sprintf (constants[numconstants++], "PADDLESUPPORT");
	}
	if ((port == 0) && (!isconstantdefined ("PADDLE0SUPPORT")))
	{
	    strcpy (redefined_variables[numredefvars++], "PADDLE0SUPPORT = 1");
	    sprintf (constants[numconstants++], "PADDLE0SUPPORT");
	}
	if ((port == 1) && (!isconstantdefined ("PADDLE1SUPPORT")))
	{
	    strcpy (redefined_variables[numredefvars++], "PADDLE1SUPPORT = 1");
	    sprintf (constants[numconstants++], "PADDLE1SUPPORT");
	}
	if ((isimmed ("PADDLE1SUPPORT")) && (isimmed ("PADDLE1SUPPORT")) && (!isconstantdefined ("FOURPADDLESUPPORT")))
	{
	    strcpy (redefined_variables[numredefvars++], "FOURPADDLESUPPORT = 1");	// if so, enable four paddle reads
	    sprintf (constants[numconstants++], "FOURPADDLESUPPORT");
	}
	if (!isconstantdefined ("LONGCONTROLLERREAD"))
	{
	    strcpy (redefined_variables[numredefvars++], "LONGCONTROLLERREAD = 1");
	    sprintf (constants[numconstants++], "LONGCONTROLLERREAD");
	}
	printf ("  lda #3 ; controller=paddle\n");
	if (port == 0)
	{
	    printf ("  sta port0control\n");
	    printf ("  ldx #0\n");
	}
	else
	{
	    printf ("  sta port1control\n");
	    printf ("  ldx #1\n");
	}
	printf ("  jsr setportforinput\n");
    }
    else if (!strcmp (statement[3], "trakball"))
    {
	if (!isconstantdefined ("TRAKBALLSUPPORT"))
	{
	    strcpy (redefined_variables[numredefvars++], "TRAKBALLSUPPORT = 1");
	    sprintf (constants[numconstants++], "TRAKBALLSUPPORT");
	}
	printf ("  lda #4 ; controller=trakball\n");
	if (port == 0)
	{
	    printf ("  sta port0control\n");
	    printf ("  lda #0\n");
	    printf ("  sta trakballcodex0\n");
	    printf ("  sta trakballcodey0\n");
	    printf ("  lda #2\n");
	    printf ("  sta port0resolution\n");
	    printf ("  ldx #0\n");

	    if (!isconstantdefined ("TRAKBALL0SUPPORT"))
	    {
		strcpy (redefined_variables[numredefvars++], "TRAKBALL0SUPPORT = 1");
		sprintf (constants[numconstants++], "TRAKBALL0SUPPORT");
	    }
	}
	else
	{
	    printf ("  sta port1control\n");
	    printf ("  lda #0\n");
	    printf ("  sta trakballcodex1\n");
	    printf ("  sta trakballcodey1\n");
	    printf ("  lda #2\n");
	    printf ("  sta port1resolution\n");
	    printf ("  ldx #1\n");
	    if (!isconstantdefined ("TRAKBALL1SUPPORT"))
	    {
		strcpy (redefined_variables[numredefvars++], "TRAKBALL1SUPPORT = 1");
		sprintf (constants[numconstants++], "TRAKBALL1SUPPORT");
	    }
	}
	printf ("  jsr setportforinput\n");
	printf ("  jsr settwobuttonmode\n");
	if (!isconstantdefined ("LONGCONTROLLERREAD"))
	{
	    strcpy (redefined_variables[numredefvars++], "LONGCONTROLLERREAD = 1");
	    sprintf (constants[numconstants++], "LONGCONTROLLERREAD");
	}
    }
    else if (!strcmp (statement[3], "keypad"))
    {
	if (!isconstantdefined ("KEYPADSUPPORT"))
	{
	    strcpy (redefined_variables[numredefvars++], "KEYPADSUPPORT = 1");
	    sprintf (constants[numconstants++], "KEYPADSUPPORT");
	}
	printf ("  lda #7 ; controller=keypad\n");
	if (port == 0)
	{
	    printf ("  sta port0control\n");
	    printf ("  ldx #0\n");
	}
	else
	{
	    printf ("  sta port1control\n");
	    printf ("  ldx #1\n");
	}
	printf ("  jsr setonebuttonmode\n");
    }
    else if ((!strcmp (statement[3], "stmouse"))
	     || (!strcmp (statement[3], "amigamouse")) || (!strcmp (statement[3], "driving")))
    {
	if (!isconstantdefined ("MOUSESUPPORT"))
	{
	    strcpy (redefined_variables[numredefvars++], "MOUSESUPPORT = 1");
	    sprintf (constants[numconstants++], "MOUSESUPPORT");
	}
	if ((port == 0) && (!isconstantdefined ("MOUSE0SUPPORT")))
	{
	    strcpy (redefined_variables[numredefvars++], "MOUSE0SUPPORT = 1");
	    sprintf (constants[numconstants++], "MOUSE0SUPPORT");
	}
	if ((port == 1) && (!isconstantdefined ("MOUSE1SUPPORT")))
	{
	    strcpy (redefined_variables[numredefvars++], "MOUSE1SUPPORT = 1");
	    sprintf (constants[numconstants++], "MOUSE1SUPPORT");
	}
	if (!isconstantdefined ("LONGCONTROLLERREAD"))
	{
	    strcpy (redefined_variables[numredefvars++], "LONGCONTROLLERREAD = 1");
	    sprintf (constants[numconstants++], "LONGCONTROLLERREAD");
	}
	if (!strcmp (statement[3], "stmouse"))
	{
	    printf ("  lda #8 ; controller=stmouse\n");
	    printf ("  ldx #1 ; mouse default resolution\n");
	}
	if (!strcmp (statement[3], "amigamouse"))
	{
	    printf ("  lda #9 ; controller=amigamouse\n");
	    printf ("  ldx #1 ; mouse default resolution\n");
	}
	if (!strcmp (statement[3], "driving"))
	{
	    printf ("  lda #6 ; controller=driving\n");
	    printf ("  ldx #1 ; driving default resolution\n");
	}
	if (port == 0)
	{
	    printf ("  sta port0control\n");
	    printf ("  stx port0resolution\n");
	    printf ("  ldx #0\n");
	}
	else
	{
	    printf ("  sta port1control\n");
	    printf ("  stx port1resolution\n");
	    printf ("  ldx #1\n");
	}
	printf ("  jsr setportforinput\n");
	printf ("  jsr setonebuttonmode\n");
    }
    else if (!strcmp (statement[3], "atarivox"))
    {
	printf ("  lda #10 ; controller=atarivox\n");
	if (port == 0)
	{
	    printf ("  sta port0control\n");
	    printf ("  ldx #0\n");
	}
	else
	{
	    printf ("  sta port1control\n");
	    printf ("  ldx #1\n");
	}
	printf ("  jsr setportforinput\n");
    }
}

void playsfx (char **statement)
{
    //   1       2     3
    // playsfx data [pitch]

    assertminimumargs (statement, "playsfx", 1);

    removeCR (statement[2]);
    removeCR (statement[3]);

    printf (" ifnconst NOTIALOCKMUTE\n");
    printf ("    lda #1\n");
    printf ("    sta sfxschedulelock\n");

    printf ("    lda #<%s\n", statement[2]);
    printf ("    sta sfxinstrumentlo\n");
    printf ("    lda #>%s\n", statement[2]);
    printf ("    sta sfxinstrumenthi\n");

    if ((statement[3][0] != 0) && (statement[3][0] != ':'))
    {
	printf ("    lda ");
	printimmed (statement[3]);
	printf ("%s\n", statement[3]);
	printf ("    sta sfxpitchoffset ; pitch modification\n");
	printf ("    lda #0\n");
	printf ("    sta sfxnoteindex ; not a musical note\n");
    }
    else
    {
	printf ("    lda #0\n");
	printf ("    sta sfxpitchoffset ; no pitch modification\n");
	printf ("    sta sfxnoteindex ; not a musical note\n");
    }


    printf ("    jsr schedulesfx\n");
    printf ("    lda #0\n");
    printf ("    sta sfxschedulelock\n");
    printf (" endif ; NOTIALOCKMUTE\n");
}

void incbasic (char **statement)
{
    // the inclusion is handled by the preprocessor, but we need to
    // fix the line number and record the name of the file we're working
    // with to have helpful error messages.
    removeCR(statement[2]);
    savelevel++;
    if (savelevel > MAXINCBASIC)
        prerror ("Too many nested incbas files!");
    savelines[savelevel] = line;
    strncpy(savelinesname[savelevel],statement[2],MAXINCBASICSTR);
    line = 1;
}

void incbasicend ()
{
    // the included file ended. go back a level.
    line = savelines[savelevel];
    savelevel--;
    if (savelevel < 0)
        prerror ("We somehow reached the end of more incbas files than used!");
}

void mutesfx (char **statement)
{
    //   1         2
    // mutesfx  TIA|POKEY

    assertminimumargs (statement, "mutesfx", 1);
    if (strncmp (statement[2], "pokey", 5) == 0)
    {
	printf ("    ifconst pokeysupport\n");
	printf ("         jsr mutepokey\n");
	printf ("    endif\n");
    }
    else if (strncmp (statement[2], "tia", 3) == 0)
    {
	printf (" ifnconst NOTIALOCKMUTE\n");
	printf ("         jsr mutetia\n");
	printf (" endif ; NOTIALOCKMUTE\n");
    }
}



void fixfilename (char *filename)
{
    int t;

    if ((filename == NULL) || (filename[0] == 0))
    {
	prerror ("filename missing");
    }

    removeCR (filename);	//remove CR from the filename, if present

    // ensure no bad characters in the filename...
    if ((strchr (filename, '-') != NULL) ||
	(strchr (filename, '+') != NULL) || ((filename[0] >= '0') && (filename[0] <= '9')))
    {
	prerror ("problem characters in filename '%s'", filename);
    }

    if ((filename[0] != '/') && (filename[0] != '\\') && (incbasepath[0] != 0))
    {
	char temppath[1024];

	//the path is relative, and basepath is defined. We'll add on the basepath

	sprintf (temppath, "%s/%s", incbasepath, filename);
	strcpy (filename, temppath);
    }


#ifdef WIN32
    // convert any Unix slashes to Windows.
    for (t = 0; filename[t] != '\0'; t++)
    {
	if (filename[t] == '/')
	{
	    filename[t] = '\\';
	}
    }
#else
    // convert any Win slashes to Unix.
    for (t = 0; filename[t] != '\0'; t++)
    {
	if (filename[t] == '\\')
	{
	    filename[t] = '/';
	}
    }
#endif

}

char *ourbasename (char *fullpath)
{
    //replacement of basename() that works with Unix or Windows style directory slashes

    int t;

    for (t = strlen (fullpath) - 2; t >= 0; t--)
    {
	if ((fullpath[t] == '/') || (fullpath[t] == '\\'))
	    return (fullpath + t + 1);
    }
    return (fullpath);

}

void incmapfile (char **statement)
{
    // Converts a "tiled" tmx xml file to a data statement.

    //     1        2
    // incmapfile filename

    char datalabelname[256];
    char datavalues[256][256];
    int s, t;
    int thisdatabankset;

    thisdatabankset = 0;

    assertminimumargs (statement, "incmapfile", 1);

    fixfilename (statement[2]);

    //our label is based on the filename...
    snprintf (datalabelname, 255, "%s", ourbasename (statement[2]));

    checkvalidfilename (statement[2]);

    //but remove the extension...
    for (t = (strlen (datalabelname) - 3); t > 0; t--)
	if (strcasecmp (datalabelname + t, ".tmx") == 0)
	    datalabelname[t] = 0;

    if ((banksetrom) && (strncmp (datalabelname, "bset_", 5) == 0))
	thisdatabankset = 1;

    if (!thisdatabankset)
	if (!(optimization & 4))
	    printf ("    JMP skipmapdata%d\n", templabel);

    if (!thisdatabankset)
	printf ("%s\n", datalabelname);
    else
	gfxprintf ("%s\n", datalabelname);

    //default data value is the "0" character...
    for (t = 0; t < 256; t++)
	sprintf (datavalues[t], " .byte 0\n");

    backupthisfile(statement[2]);

    //open file...
    FILE *fp = fopen (statement[2], "rb");
    if (!fp)
    {
	prerror ("incmapfile couldn't open map file '%s' for reading", statement[2]);
    }

    for (;;)
    {
	char line[1024];
	char *keyword;
	char *firstgid;
	char *tilename;
	int gid;
	int layer = 0;

	if (fgets (line, 1024, fp) == NULL)
	    break;

	keyword = strstr (line, "tileset firstgid=\"");
	if (keyword != NULL)
	{
	    firstgid = strstr (line, "firstgid=\"");
	    tilename = strstr (line, "name=\"");
	    if ((tilename != NULL) && (firstgid != NULL))
	    {
		gid = atoi (firstgid + 10);
		tilename = tilename + 6;
		for (t = 0; t < 1024; t++)
		    if (line[t] == '"')
			line[t] = 0;
		s = 0;
		for (t = gid; t < 256; t++)
		{
		    sprintf (datavalues[t], " .byte <(%s+%d)\n", tilename, s);
		    s = s + 1;
		    if (doublewide == 1)
			s = s + 1;
		}
	    }
	}

	keyword = strstr (line, "tile gid=\"");
	if (keyword != NULL)
	{
	    gid = atoi (keyword + 10);
	    if (!thisdatabankset)
		printf ("%s", datavalues[gid]);
	    else
		gfxprintf ("%s", datavalues[gid]);
	}

	keyword = strstr (line, "<data encoding=\"");
	if (keyword != NULL)
	{
	    fclose (fp);
	    keyword = keyword + 15;
	    for (t = 0; t < 1024; t++)
		if (line[t] == '"')
		    line[t] = 0;
	    prerror ("map file '%s' is %s encoded. XML is required", statement[2], keyword);
	}
	keyword = strstr (line, "<layer name=\"");
	if (keyword != NULL)
	{
	    layer = layer + 1;
	    if (layer > 1)
	    {
		fclose (fp);
		prerror ("map file '%s' contains multiple layers", statement[2]);
	    }
	}
    }
    fclose (fp);

    if (!thisdatabankset)
	printf ("skipmapdata%d\n", templabel);

    templabel = templabel + 1;
}

void convertbmp2png (char *bmpname)
{
    uint32_t BMPFileSize;
    int headersize;

#pragma pack(push, 1)
    struct BMPHeader
    {
	uint16_t BMPIdent;	//00 01
	uint32_t BMPFileSize;	//02 03 04 05
	uint16_t BMPReserved1;	//06 07
	uint16_t BMPReserved2;	//08 09
	uint32_t BMPImageOffset;	//0A 0B 0C 0D
	uint32_t BMPInfoHeaderSize;	//0E 0F 10 11
	uint32_t BMPWidthInPixels;	//12 13 14 15
	uint32_t BMPHeightInPixels;	//16 17 18 19
	uint16_t BMPColorPlanes;	//1A 1B
	uint16_t BMPBitsPerPixel;	//1C 1D
	uint32_t BMPCompressionMethod;	//1E 1F 20 21
	uint32_t BMPImageSize;	//22 23 24 25
	uint32_t BMPHorizontalResolution;	//26 27 28 29
	uint32_t BMPVerticalResolution;	//2A 2B 2C 2D
	uint32_t BMPNumberOfColors;	//2E 2F 30 31
	uint32_t BMPNumberOfImportantColors;	//32 33 34 35
    } OurBMPHeader;
#pragma pack(pop)

    prinfo ("bitmap %s is being used. converting it to  a png", bmpname);

    FILE *in;
    in = fopen (bmpname, "rb");
    if (in == NULL)
	prerror ("couldn't open %s", bmpname);

    //get the BMP file size...
    fseek (in, 0, SEEK_END);
    BMPFileSize = ftell (in);
    fseek (in, 0, SEEK_SET);
    if (BMPFileSize == 0)
	prerror ("couldn't read data from %s", bmpname);

    //read in the BMP file...
    headersize = fread (&OurBMPHeader, 1, sizeof (struct BMPHeader), in);
    if (headersize == 0)
	prerror ("couldn't read %s", bmpname);


    prinfo ("DEBUG: %d bytes of header read in", headersize);

    //some sanity checking...
    if (OurBMPHeader.BMPBitsPerPixel != 24)
	prerror ("bmp %s isn't in RGB format", bmpname);

    prinfo ("DEBUG: %04x", OurBMPHeader.BMPIdent);
    prinfo ("DEBUG: %08x", OurBMPHeader.BMPFileSize);
    prinfo ("DEBUG: %04x", OurBMPHeader.BMPNumberOfColors);
    prinfo ("DEBUG: %dx%d", OurBMPHeader.BMPWidthInPixels, OurBMPHeader.BMPHeightInPixels);

}

void add_graphic (char **statement, int incbanner)
{
    int s, t, width, height;
    int palettestatement = -1;
    char *fileextension;
    char generalname[1024];

    //160A...
    //320B...
    //320C...
    //320D...
    //incgraphic filename [mode] [color0] [color1] [color2] [color3] [default palette]
    //    1         2        3       4       5        6        7         8

    //160B...
    //incgraphic filename [mode] [color0] [color1] [color2] [color3] ... [color12] [default palette]
    //    1         2        3       4       5        6        7           16            17

    //320A...
    //incgraphic filename [mode] [color0] [color1] [default palette]
    //    1         2        3       4       5            6


    //setup our defaults...
    for (t = 0; t < 16; t++)
    {
	graphiccolorindex[t] = t;
	graphic7800colors[t] = 0;
    }

    graphiccolormode = MODE160A;

    if ((graphicsdatawidth[0] == 0) && (dmaplain == 0))
    {
	//this is the first graphics statement encountered. initialize everything.
	memset (graphicsdata, 0, 16 * 256 * 100 * sizeof (char));
	memset (graphicslabels, 0, 16 * 256 * 80 * sizeof (char));
	memset (graphicsinfo, 0, 16 * 256 * sizeof (char));
	memset (graphicsmode, 0, 16 * 256 * sizeof (char));
    }

    fixfilename (statement[2]);

    backupthisfile(statement[2]);

    fileextension = strrchr (statement[2], '.');
    if (fileextension == NULL)
	prerror ("'%s' filename extension is missing", statement[2]);
    checkvalidfilename (statement[2]);
    if (strcmp (fileextension, ".bmp") == 0)
    {
	//prwarn("'%s' bitmap isn't supported yet",statement[2]);
	convertbmp2png (statement[2]);
    }

    //check if the user is setting the color mode...
    if ((statement[3][0] != 0) && (statement[3][0] != ':') && (statement[3][0] != ';'))
    {
	removeCR (statement[3]);
	if (strcasecmp (statement[3], "160A") == 0)
	{
	    graphiccolormode = MODE160A;
	    palettestatement = 8;
	}
	else if (strcasecmp (statement[3], "160B") == 0)
	{
	    graphiccolormode = MODE160B;
	    palettestatement = 17;
	    if (deprecated160bindexes == 1)
	    {
		s = 1;
		for (t = 1; t < 12; t++)
		{
		    if (s % 4 == 0)
			s = s + 1;
		    graphiccolorindex[t] = s;
		    s = s + 1;
		}
		graphiccolorindex[12] = 15;
		graphiccolorindex[13] = 4;
		graphiccolorindex[14] = 8;
		graphiccolorindex[15] = 12;
	    }
	}
	else if (strcasecmp (statement[3], "320A") == 0)
	{
	    graphiccolormode = MODE320A;
	    palettestatement = 6;
	}
	else if (strcasecmp (statement[3], "320B") == 0)
	{
	    graphiccolormode = MODE320B;
	    palettestatement = 8;
	}
	else if (strcasecmp (statement[3], "320C") == 0)
	{
	    graphiccolormode = MODE320C;
	    palettestatement = 8;
	}
	else if (strcasecmp (statement[3], "320D") == 0)
	{
	    graphiccolormode = MODE320D;
	    palettestatement = 8;
	}
    }
    width = getgraphicwidth (statement[2]);
    if (width > 256)
    {
	prerror ("'%s' is wider than 256 bytes", statement[2]);
    }

    //check if the user is reordering the color indexes of the imported graphic
    if ((statement[4][0] != 0) && (statement[3][0] != ':') && (statement[3][0] != ';'))
    {
	for (t = 0; t < 16; t++)
	{
	    if (statement[t + 4][0] == ':')
		break;
	    if (statement[t + 4][0] != 0)
	    {
		removeCR (statement[t + 4]);
		if ((statement[t + 4][0] != 0) && (statement[t + 4][0] != '\0'))
		    graphiccolorindex[t] = strictatoi (statement[t + 4]);
	    }
	}
    }

    if ((strcasecmp (statement[3], "160B") == 0) && (deprecated160bindexes == 0))
    {
	// We need to reorder the color indexes some more. For 160B the 7800 makes color
	// indexes 0, 4, 8, and 12 transparent, but a 16 color PNG will likely have
	// non-transparent colors in slots 4, 8, and 12.
	// We assume 0 is aligned correctly and the background/transparent color.

	// move indexes 4+ to 5+
	for (t = 1; t < 16; t++)
	    if (graphiccolorindex[t] >= 4)
		graphiccolorindex[t]++;

	// move indexes 8+ to 9+
	for (t = 1; t < 16; t++)
	    if (graphiccolorindex[t] >= 8)
		graphiccolorindex[t]++;

	// move indexes 12+ to 13+
	for (t = 1; t < 16; t++)
	    if (graphiccolorindex[t] >= 12)
		graphiccolorindex[t]++;

	graphiccolorindex[13] = 4;
	graphiccolorindex[14] = 8;
	graphiccolorindex[15] = 12;



    }

    if (!incbanner)
    {

	int offset = 0;
	int istallsprite;

	height = getgraphicheight (statement[2]);
	istallsprite = (((height / zoneheight) > 1) && (tallspritemode > 0));

	if (istallsprite)
	{
	    if ((graphicsdatawidth[dmaplain] + (width * (height / zoneheight))) > 256)
	    {
		//the next graphic will overflow the DMA plain, so we skip to the next one...
		dmaplain = dmaplain + 1;
		graphicsdatawidth[dmaplain] = 0;
	    }
	}
	else			//it's a regular height sprite
	{
	    if ((graphicsdatawidth[dmaplain] + width) > 256)
	    {
		//the next graphic will overflow the DMA plain, so we skip to the next one...
		dmaplain = dmaplain + 1;
		graphicsdatawidth[dmaplain] = 0;
	    }
	}
	//our label is based on the filename...
	snprintf (generalname, 80, "%s", ourbasename (statement[2]));

	checkvalidfilename (statement[2]);

	//but remove the extension...
	for (t = (strlen (generalname) - 3); t > 0; t--)
	    if (strcasecmp (generalname + t, ".png") == 0)
		generalname[t] = 0;

	//save the label
	strcpy (graphicslabels[dmaplain][graphicsdatawidth[dmaplain]], generalname);

	if ((istallsprite)&&(passes==0))
	{
	    //remember sprite height
	    strcpy (tallspritelabel[tallspritecount], generalname);
	    tallspriteheight[tallspritecount] = height / zoneheight;
	    tallspritecount++;

	}

	if (palettestatement > 0)
	{
	    //check if a default palette was set
	    for (t = 1; t <= palettestatement; t++)
	    {
		if (statement[t] == 0)
		{
		    t = 0;
		    break;
		}
		removeCR (statement[t]);	//remove CR
		if ((statement[t][0] == 0) || (statement[t][0] == ':'))
		{
		    t = 0;
		    break;
		}
	    }
	    if (t == (palettestatement + 1))
	    {
		//the user has specified a default palette. remember it.
		for (s = 0; s < 1000; s++)
		{
		    if (palettefilenames[s][0] == 0)
			break;
	            if (strcmp (palettefilenames[s], graphicslabels[dmaplain][graphicsdatawidth[dmaplain]]) == 0)
	                break;
		}
		if (s > 998)
		    prerror ("ran out of default graphic palette entries");
		strcpy (palettefilenames[s], graphicslabels[dmaplain][graphicsdatawidth[dmaplain]]);
		graphicfilepalettes[s] = strictatoi (statement[palettestatement]);
		graphicfilemodes[s] = graphiccolormode;
	    }
	}

	if (istallsprite)
	{
	    for (t = 0; t < (height / zoneheight); t++)
	    {
		char indexstr[1124];

		// Now read the png into memory...
		incgraphic (statement[2], offset);
		offset = offset + zoneheight;

		//add on the banner zone number to the label, for our extra rows
		sprintf (indexstr, "%s_tallsprite_%02d", generalname, t);
		strcat (graphicslabels[dmaplain][graphicsdatawidth[dmaplain]], indexstr);
	    }
	}
	else
	{
	    // Now read the png into memory...
	    incgraphic (statement[2], offset);
	}
    }
    else			// incbanner called us. we're importing a multi-zoneheight tall banner
    {
	int height;
	int offset = 0;
	char indexstr[256];

	height = getgraphicheight (statement[2]);
	if ((height < zoneheight) || (height > 224))
	    prerror ("image height %d out of spec for incbanner",height);


	//our label is based on the filename...
	snprintf (generalname, 80, "%s", ourbasename (statement[2]));

	checkvalidfilename (statement[2]);

	//but remove the extension...
	for (t = (strlen (generalname) - 3); t > 0; t--)
	    if (strcasecmp (generalname + t, ".png") == 0)
		generalname[t] = 0;

	for (s = 0; s < 1000; s++)
	{
	    if (bannerfilenames[s][0] == 0)
	        break;
	    if (strcmp (bannerfilenames[s], generalname) == 0)
	        break;
	    if (s > 998)
	        prerror ("ran out of banner height entries");
        }
	strcpy (bannerfilenames[s], generalname);
	bannerheights[s] = height / zoneheight;
	bannerwidths[s] = (width - 1) / 32;

	// width of 32 bytes, in coordinates...
	//TODO: stuff this into a LUT...
	if (graphiccolormode == MODE160A)
	    bannerpixelwidth[s] = 128;
	if (graphiccolormode == MODE160B)
	    bannerpixelwidth[s] = 64;
	if (graphiccolormode == MODE320A)
	    bannerpixelwidth[s] = 128;
	if (graphiccolormode == MODE320B)
	    bannerpixelwidth[s] = 64;
	if (graphiccolormode == MODE320C)
	    bannerpixelwidth[s] = 64;
	if (graphiccolormode == MODE320D)
	    bannerpixelwidth[s] = 128;

	for (offset = 0; offset < (height - (height % zoneheight)); offset = offset + zoneheight)
	{
	    if ((graphicsdatawidth[dmaplain] + width) > 256)
	    {
		//the next graphic will overflow the DMA plain, so we skip to the next one...
		dmaplain = dmaplain + 1;
		graphicsdatawidth[dmaplain] = 0;
	    }

	    //our label is based on the filename...
	    snprintf (graphicslabels[dmaplain][graphicsdatawidth[dmaplain]], 80, "%s", ourbasename (statement[2]));

	    //but remove the extension...
	    for (t = (strlen (graphicslabels[dmaplain][graphicsdatawidth[dmaplain]]) - 3); t > 0; t--)
		if (strcasecmp (graphicslabels[dmaplain][graphicsdatawidth[dmaplain]] + t, ".png") == 0)
		    graphicslabels[dmaplain][graphicsdatawidth[dmaplain]][t] = 0;

	    //add on the banner zone number to the label
	    sprintf (indexstr, "%02d", offset / zoneheight);
	    strcat (graphicslabels[dmaplain][graphicsdatawidth[dmaplain]], indexstr);


	    // Now read the png into memory...
	    incgraphic (statement[2], offset);
	}

	//add color definitions for general banner name, because incgraphic will add them for bannername## instead...
	int modecolors;
	if (graphiccolormode == MODE160A)
	    modecolors = 3;
	else if (graphiccolormode == MODE160B)
	    modecolors = 12;
	else if (graphiccolormode == MODE320A)
	    modecolors = 1;
	else if (graphiccolormode == MODE320B)
	    modecolors = 3;
	else if (graphiccolormode == MODE320C)
	    modecolors = 3;
	else if (graphiccolormode == MODE320D)
	    modecolors = 3;

	for (t = 1; t < modecolors + 1; t++)
	{
	    sprintf (redefined_variables[numredefvars++], "%s_color%d = %s00_color%d", generalname, t, generalname, t);
	    sprintf (constants[numconstants++], "%s_color%d", generalname, t);	// record to queue
	}
    }
}

void filetolabel(char *target, char *source)
{
	int t;

	//our label is based on the filename...
	snprintf (target, 80, "%s", ourbasename (source));

	checkvalidfilename (target);

	//but remove the extension...
	for (t = (strlen (target) - 3); t > 0; t--)
	    if (strcasecmp (target + t, ".png") == 0)
		target[t] = 0;
}

void defaultpalette (char **statement)
{
    //defaultpalette filename  mode   palette
    //    1             2        3      4

    int s;
    unsigned char ourmode;

    assertminimumargs (statement, "defaultpalette", 3);
    removeCR (statement[4]);

    char imagename[256];
    filetolabel(imagename,statement[2]);

    if (strcasecmp (statement[3], "160A") == 0)
	    ourmode = MODE160A;
    else if (strcasecmp (statement[3], "160B") == 0)
	    ourmode = MODE160B;
    else if (strcasecmp (statement[3], "320A") == 0)
	    ourmode = MODE320A;
    else if (strcasecmp (statement[3], "320B") == 0)
	    ourmode = MODE320B;
    else if (strcasecmp (statement[3], "320C") == 0)
	    ourmode = MODE320C;
    else if (strcasecmp (statement[3], "320D") == 0)
	    ourmode = MODE320D;

    // scan for the palette entry, or make a new one...
    for (s = 0; s < 1000; s++)
    {
        if (palettefilenames[s][0] == 0)
             break;
	if (strcmp (palettefilenames[s], imagename) == 0)
	     break;
    }
    if (s > 998)
        prerror ("ran out of default graphic palette entries");
    strcpy (palettefilenames[s], imagename);
    graphicfilepalettes[s] = strictatoi (statement[4]);
    graphicfilemodes[s] = ourmode;
}

int getgraphicwidth (char *file_name)
{
    int width;

    png_structp png_ptr;
    png_infop info_ptr;

    //******* read file

    unsigned char header[8];	// 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen (file_name, "rb");
    if (!fp)
    {
	prerror ("couldn't open png file '%s' for reading", file_name);
    }
    if (fread (header, 1, 8, fp) == 0)
    {
	prerror ("couldn't read from png file '%s'", file_name);
    }
    if (png_sig_cmp (header, 0, 8))
    {
	prerror ("'%s' doesn't appear to be a png file", file_name);
    }
    /* initialize stuff */
    png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
    {
	prerror ("when preparing '%s', png_create_read_struct failed", file_name);
    }

    info_ptr = png_create_info_struct (png_ptr);
    if (!info_ptr)
    {
	prerror ("when preparing '%s', png_create_info_struct failed", file_name);
    }
    if (setjmp (png_jmpbuf (png_ptr)))
    {
	prerror ("libpng error during init_io");
    }

    png_init_io (png_ptr, fp);
    png_set_sig_bytes (png_ptr, 8);

    png_read_info (png_ptr, info_ptr);

    width = png_get_image_width (png_ptr, info_ptr);

    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

    fclose (fp);

    if (graphiccolormode == MODE160A)
	width = width / 4;	//4 pixels per byte
    if (graphiccolormode == MODE160B)
	width = width / 2;	//2 pixels per byte
    if (graphiccolormode == MODE320A)
	width = width / 8;	//8 pixels per byte
    if (graphiccolormode == MODE320B)
	width = width / 4;	//4 pixels per byte
    if (graphiccolormode == MODE320C)
	width = width / 4;	//4 pixels per byte
    if (graphiccolormode == MODE320D)
	width = width / 8;	//8 pixels per byte

    return (width);
}

int getgraphicheight (char *file_name)
{

    int height;

    png_structp png_ptr;
    png_infop info_ptr;

    //******* read file

    unsigned char header[8];	// 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen (file_name, "rb");
    if (!fp)
    {
	prerror ("couldn't open png file '%s' for reading", file_name);
    }
    if (fread (header, 1, 8, fp) == 0)
    {
	prerror ("couldn't read from png file '%s'", file_name);
    }
    if (png_sig_cmp (header, 0, 8))
    {
	prerror ("'%s' doesn't appear to be a png file", file_name);
    }
    /* initialize stuff */
    png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
    {
	prerror ("when preparing '%s', png_create_read_struct failed", file_name);
    }

    info_ptr = png_create_info_struct (png_ptr);
    if (!info_ptr)
    {
	prerror ("when preparing '%s', png_create_info_struct failed", file_name);
    }
    if (setjmp (png_jmpbuf (png_ptr)))
    {
	prerror ("libpng error during init_io");
    }

    png_init_io (png_ptr, fp);
    png_set_sig_bytes (png_ptr, 8);

    png_read_info (png_ptr, info_ptr);

    height = png_get_image_height (png_ptr, info_ptr);

    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

    fclose (fp);

    return (height);
}


void newblock ()
{
    //skips to the next graphic area
    dmaplain = dmaplain + 1;
    graphicsdatawidth[dmaplain] = 0;
}

void incgraphic (char *file_name, int offset)
{
    int x, y;

    int width, height, rowbytes;
    png_byte color_type;
    png_byte bit_depth;

    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;

    int num_palette;
    int num_unique_palette;
    png_colorp palette;
    int num_trans;
    png_bytep trans;

    //******* read file

    unsigned char header[8];	// 8 is the maximum size that can be checked


    /* open file and test for it being a png */
    FILE *fp = fopen (file_name, "rb");
    if (!fp)
    {
	prerror ("couldn't open png file '%s' for reading", file_name);
    }
    if (fread (header, 1, 8, fp) == 0)
    {
	prerror ("couldn't read from png file '%s'", file_name);
    }
    if (png_sig_cmp (header, 0, 8))
    {
	prerror ("'%s' doesn't appear to be a png file", file_name);
    }
    /* initialize stuff */
    png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
    {
	prerror ("when preparing '%s', png_create_read_struct failed", file_name);
    }

    info_ptr = png_create_info_struct (png_ptr);
    if (!info_ptr)
    {
	prerror ("when preparing '%s', png_create_info_struct failed", file_name);
    }
    if (setjmp (png_jmpbuf (png_ptr)))
    {
	prerror ("libpng error during init_io");
    }

    png_init_io (png_ptr, fp);
    png_set_sig_bytes (png_ptr, 8);

    png_read_info (png_ptr, info_ptr);

    width = png_get_image_width (png_ptr, info_ptr);
    height = png_get_image_height (png_ptr, info_ptr);
    color_type = png_get_color_type (png_ptr, info_ptr);
    bit_depth = png_get_bit_depth (png_ptr, info_ptr);

    if (!png_get_tRNS (png_ptr, info_ptr, &trans, &num_trans, NULL))
	num_trans = -1;

    if (png_get_PLTE (png_ptr, info_ptr, &palette, &num_palette))
    {
	double r, g, b, y, i, q, colorangle;
	int s, t;

	// first, figure out which palettes are uniquely used
	num_unique_palette = 0;
	for (t = 0; t < (num_palette - 1); t++)
	{
	    for (s = t + 1; s < num_palette; s++)
	    {
		if ((palette[t].red == palette[s].red) &&
		    (palette[t].green == palette[s].green) && (palette[t].blue == palette[s].blue))
		    break;
	    }
	    if (s == num_palette)
		num_unique_palette = num_unique_palette + 1;
	}

	if ((bit_depth > 4) && ((num_palette > 4) || (num_palette < 2)))
	{
	    prerror
		("the png file '%s' has too many colors: bit depth: %d, palette size: %d",
		 file_name, bit_depth, num_palette);
	}

	const double convertval = 7.5 / M_PI;

	int finalcolor;
	for (t = 0; t < num_palette; t++)
	{

	    r = (double) palette[t].red / 255.0;
	    g = (double) palette[t].green / 255.0;
	    b = (double) palette[t].blue / 255.0;

	    if (num_trans > 0)
	    {
		for (s = 0; s < num_trans; s++)
		    if (trans[s] == t)
		    {
			break;
		    }
	    }

	    //convert RGB to YIQ
	    y = 0.299900 * r + 0.587000 * g + 0.114000 * b;
	    i = 0.595716 * r - 0.274453 * g - 0.321264 * b;
	    q = 0.211456 * r - 0.522591 * g + 0.311350 * b;

	    //get angle of I,Q to provide colorburst phase (cartesian->polar)
	    colorangle = atan (q / i);
	    //account for i being negative, otherwise (-q/-i)==(q/i), (-q/i)==(q/-i), ...
	    if (i < 0)
		colorangle = colorangle + M_PI;
	    colorangle = (colorangle * convertval) + 3.50;

	    //clamps
	    if (colorangle < 1)
		colorangle = 1;
	    if (colorangle > 16)
		colorangle = 16;
	    if ((r == g) && (g == b))
		colorangle = 0;
	    if (y > 1.0)
		y = 1.0;

	    y = y * 15;

	    finalcolor = ((int) (colorangle * 16)) & 0xF0;
	    finalcolor = finalcolor | (int) y;

	    if (graphicslabels[dmaplain][graphicsdatawidth[dmaplain]][0] != 0)
	    {
		sprintf (redefined_variables[numredefvars++],
			 "%s_color%d = $%02x", graphicslabels[dmaplain][graphicsdatawidth[dmaplain]], t, finalcolor);
		sprintf (constants[numconstants++], "%s_color%d", graphicslabels[dmaplain][graphicsdatawidth[dmaplain]], t);	// record to queue
	    }

	}
	//deal with images that are missing color indexes...
	for (; t < (1 << bit_depth); t++)
	{
	    sprintf (redefined_variables[numredefvars++], "%s_color%d = 0",
		     graphicslabels[dmaplain][graphicsdatawidth[dmaplain]], t);
	    sprintf (constants[numconstants++], "%s_color%d", graphicslabels[dmaplain][graphicsdatawidth[dmaplain]], t);	// record to queue
	}
    }

    //TODO: accurate warning for pixel width on non-byte boundary

    //change png to one byte per pixel, rather than tight packing...
    png_set_packing (png_ptr);


    png_read_update_info (png_ptr, info_ptr);


    /* read in the rest of the png */
    if (setjmp (png_jmpbuf (png_ptr)))
    {
	prerror ("libpng error during read_image");
    }

    rowbytes = png_get_rowbytes (png_ptr, info_ptr);
    row_pointers = (png_bytep *) malloc (sizeof (png_bytep) * height);
    for (y = 0; y < height; y++)
	row_pointers[y] = (png_byte *) malloc (rowbytes);

    png_read_image (png_ptr, row_pointers);

    fclose (fp);


    //******* process file

    if ((color_type & PNG_COLOR_TYPE_PALETTE) == 0)
    {
	prerror ("the png file %s isn't a palette-type/indexed image", file_name);
    }


    //if((bit_depth<=2)&&(graphiccolormode==MODE160A))
    if (graphiccolormode == MODE160A)
    {
	if (num_unique_palette > 4)
	    prwarn ("image contains more unique colors than 160A allows");

	graphicsinfo[dmaplain][graphicsdatawidth[dmaplain]] = width / 4;	//image width in 6502 bytes
	graphicsmode[dmaplain][graphicsdatawidth[dmaplain]] = 0;
	for (x = 0; x < width; x = x + 4)
	{
	    graphicsdatawidth[dmaplain] = graphicsdatawidth[dmaplain] + 1;
	    for (y = zoneheight - 1; y > -1; y = y - 1)
	    {
		int l, val;

		if (y >= height)	//if the image is shorter than a zone, just leave zeros in those lines
		{
		    graphicsdata[dmaplain][graphicsdatawidth[dmaplain] - 1][y] = 0;
		    continue;
		}
		png_byte *row = row_pointers[y + offset];
		val = 0;
		for (l = 0; l < 4; l++)
		{
		    if ((x + l) < width)	// if the width isn't on a byte boundary for this mode, only access valid pixels
		    {
			val = val << 2;
			val = val | graphiccolorindex[row[x + l]];
		    }
		}
		graphicsdata[dmaplain][graphicsdatawidth[dmaplain] - 1][y] = val;
	    }
	}

	for (y = 0; y < height; y++)
	    free (row_pointers[y]);
	free (row_pointers);
	png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
	return;
    }

    if (graphiccolormode == MODE160B)
    {
	if (num_unique_palette > 16)
	    prwarn ("image contains more unique colors than 160B allows");
	graphicsinfo[dmaplain][graphicsdatawidth[dmaplain]] = width / 2;	//image width in 6502 bytes
	graphicsmode[dmaplain][graphicsdatawidth[dmaplain]] = 128;
	for (x = 0; x < width; x = x + 2)
	{
	    graphicsdatawidth[dmaplain] = graphicsdatawidth[dmaplain] + 1;
	    for (y = zoneheight - 1; y > -1; y = y - 1)
	    {
		int val, workval;

		if (y >= height)	//if the image is shorter than a zone, just leave zeros in those lines
		{
		    graphicsdata[dmaplain][graphicsdatawidth[dmaplain] - 1][y] = 0;
		    continue;
		}

		png_byte *row = row_pointers[y + offset];

		// official recipe for each 160B byte...
		// take two pixels, p1, p2
		// 4 bits of p1 go 3276
		// 4 bits of p2 go 1054

		val = 0;

		workval = graphiccolorindex[row[x]];
		if ((workval & 8) > 0)
		    val = val | 8;
		if ((workval & 4) > 0)
		    val = val | 4;
		if ((workval & 2) > 0)
		    val = val | 128;
		if ((workval & 1) > 0)
		    val = val | 64;

		if ((x + 1) < width)	// if the width isn't on a byte boundary for this mode, only access valid pixels
		{
		    workval = graphiccolorindex[row[x + 1]];
		    if ((workval & 8) > 0)
			val = val | 2;
		    if ((workval & 4) > 0)
			val = val | 1;
		    if ((workval & 2) > 0)
			val = val | 32;
		    if ((workval & 1) > 0)
			val = val | 16;
		}

		graphicsdata[dmaplain][graphicsdatawidth[dmaplain] - 1][y] = val;
	    }
	}
	for (y = 0; y < height; y++)
	    free (row_pointers[y]);
	free (row_pointers);
	png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
	return;
    }

    if (graphiccolormode == MODE320A)
    {
	if (num_unique_palette > 2)
	    prwarn ("image contains more unique colors than 320A allows");
	graphicsinfo[dmaplain][graphicsdatawidth[dmaplain]] = width / 8;	//image width in 6502 bytes
	graphicsmode[dmaplain][graphicsdatawidth[dmaplain]] = 0;
	for (x = 0; x < width; x = x + 8)
	{
	    graphicsdatawidth[dmaplain] = graphicsdatawidth[dmaplain] + 1;
	    for (y = zoneheight - 1; y > -1; y = y - 1)
	    {
		int l, val;

		if (y >= height)	//if the image is shorter than a zone, just leave zeros in those lines
		{
		    graphicsdata[dmaplain][graphicsdatawidth[dmaplain] - 1][y] = 0;
		    continue;
		}

		png_byte *row = row_pointers[y + offset];
		val = 0;
		for (l = 0; l < 8; l++)
		{
		    if ((x + l) < width)	// if the width isn't on a byte boundary for this mode, only access valid pixels
		    {
			val = val << 1;
			val = val | graphiccolorindex[row[x + l]];
		    }
		}
		graphicsdata[dmaplain][graphicsdatawidth[dmaplain] - 1][y] = val;
	    }
	}
	for (y = 0; y < height; y++)
	    free (row_pointers[y]);
	free (row_pointers);
	png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
	return;
    }

    if (graphiccolormode == MODE320B)
    {
	if (num_unique_palette > 4)
	    prwarn ("image contains more unique colors than 320B allows");
	graphicsinfo[dmaplain][graphicsdatawidth[dmaplain]] = width / 4;	//image width in 6502 bytes
	graphicsmode[dmaplain][graphicsdatawidth[dmaplain]] = 128;
	for (x = 0; x < width; x = x + 4)
	{
	    graphicsdatawidth[dmaplain] = graphicsdatawidth[dmaplain] + 1;
	    for (y = zoneheight - 1; y > -1; y = y - 1)
	    {
		int val, workval;

		if (y >= height)	//if the image is shorter than a zone, just leave zeros in those lines
		{
		    graphicsdata[dmaplain][graphicsdatawidth[dmaplain] - 1][y] = 0;
		    continue;
		}

		png_byte *row = row_pointers[y + offset];

		// official recipe for each 320B byte...
		// take four pixels, p1, p2, p3, p4
		// 2 bits of p1 go to 73
		// 2 bits of p2 go to 62
		// 2 bits of p3 go to 51
		// 2 bits of p4 go to 40

		val = 0;
		workval = graphiccolorindex[row[x]];
		if ((workval & 2) > 0)
		    val = val | 128;
		if ((workval & 1) > 0)
		    val = val | 8;

		if ((x + 1) < width)
		{
		    workval = graphiccolorindex[row[x + 1]];
		    if ((workval & 2) > 0)
			val = val | 64;
		    if ((workval & 1) > 0)
			val = val | 4;
		}

		if ((x + 2) < width)
		{
		    workval = graphiccolorindex[row[x + 2]];
		    if ((workval & 2) > 0)
			val = val | 32;
		    if ((workval & 1) > 0)
			val = val | 2;
		}

		if ((x + 3) < width)
		{
		    workval = graphiccolorindex[row[x + 3]];
		    if ((workval & 2) > 0)
			val = val | 16;
		    if ((workval & 1) > 0)
			val = val | 1;
		}

		graphicsdata[dmaplain][graphicsdatawidth[dmaplain] - 1][y] = val;
	    }
	}
	for (y = 0; y < height; y++)
	    free (row_pointers[y]);
	free (row_pointers);
	png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
	return;
    }


    if (graphiccolormode == MODE320C)
    {
	if (num_unique_palette > 4)
	    prwarn ("image contains more unique colors than 320C allows");
	int supresserror = 0;
	graphicsinfo[dmaplain][graphicsdatawidth[dmaplain]] = width / 4;	//image width in 6502 bytes
	graphicsmode[dmaplain][graphicsdatawidth[dmaplain]] = 128;
	for (x = 0; x < width; x = x + 4)
	{
	    graphicsdatawidth[dmaplain] = graphicsdatawidth[dmaplain] + 1;

	    for (y = zoneheight - 1; y > -1; y = y - 1)
	    {
		int val, workval1, workval2;

		if (y >= height)	//if the image is shorter than a zone, just leave zeros in those lines
		{
		    graphicsdata[dmaplain][graphicsdatawidth[dmaplain] - 1][y] = 0;
		    continue;
		}


		png_byte *row = row_pointers[y + offset];

		// official recipe for each 320C byte...
		// take four pixels, p1, p2, p3, p4
		// where p1 must be the same palette as p2,
		// and   p3 must be the same palette as p4.
		// p1 goes to 7
		// p2 goes to 6
		// p3 goes to 5
		// p4 goes to 4
		// 2 bits of palette for p1,p2 goes to 32
		// 2 bits of palette for p3,p4 goes to 10

		val = 0;

		workval1 = graphiccolorindex[row[x]] & 3;
		if (workval1 > 0)
		    val = val | 128;

		workval2 = graphiccolorindex[row[x + 1]] & 3;
		if (workval2 > 0)
		    val = val | 64;

		if ((workval1 > 0) && (workval2 > 0) && (workval1 != workval2))
		{
		    if (supresserror == 0)
		    {
			char sBuffer[400];
			sprintf (sBuffer,
				 "The png file %s doesn't appear to meet the requirements for 320C\n", file_name);
			strcat (sBuffer,
				"         Each pair of pixels must use the same color if they aren't turned off.\n");
			strcat (sBuffer, "         The colors for this sprite will be converted incorrectly.");
			prwarn (sBuffer);
			supresserror = 1;
		    }
		    val = val | (workval1 << 2);
		}
		else
		    val = val | (workval1 << 2) | (workval2 << 2);

		workval1 = graphiccolorindex[row[x + 2]] & 3;
		if (workval1 > 0)
		    val = val | 32;

		workval2 = graphiccolorindex[row[x + 3]] & 3;
		if (workval2 > 0)
		    val = val | 16;

		if ((workval1 > 0) && (workval2 > 0) && (workval1 != workval2))
		{
		    if (supresserror == 0)
		    {
			char sBuffer[400];
			sprintf (sBuffer,
				 "The png file %s doesn't appear to meet the requirements for 320C\n", file_name);
			strcat (sBuffer,
				"         Each pair of pixels must use the same color if they aren't turned off.\n");
			strcat (sBuffer, "         The colors for this sprite will be converted incorrectly.");
			prwarn (sBuffer);
			supresserror = 1;
		    }

		    val = val | workval1;
		}
		else
		    val = val | workval1 | workval2;

		graphicsdata[dmaplain][graphicsdatawidth[dmaplain] - 1][y] = val;
	    }
	}
	for (y = 0; y < height; y++)
	    free (row_pointers[y]);
	free (row_pointers);
	png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
	return;
    }

    if (graphiccolormode == MODE320D)
    {
	if (num_unique_palette > 4)
	    prwarn ("image contains more unique colors than 320D allows");
	graphicsinfo[dmaplain][graphicsdatawidth[dmaplain]] = width / 8;	//image width in 6502 bytes
	graphicsmode[dmaplain][graphicsdatawidth[dmaplain]] = 0;
	for (x = 0; x < width; x = x + 8)
	{
	    graphicsdatawidth[dmaplain] = graphicsdatawidth[dmaplain] + 1;

	    for (y = zoneheight - 1; y > -1; y = y - 1)
	    {
		int l, val, workval;

		if (y >= height)	//if the image is shorter than a zone, just leave zeros in those lines
		{
		    graphicsdata[dmaplain][graphicsdatawidth[dmaplain] - 1][y] = 0;
		    continue;
		}

		png_byte *row = row_pointers[y + offset];

		// official recipe for each 320D byte (I think) ...
		// take 8 pixels, p1, p2, p3, p4, p5, p6, p7, p8
		// where p1 color = p3, p5, p7 colors (color1/color3=on/off)
		//   and p2 color = p4, p6, p8 colors (color2/color4=on/off)

		// some error checking first...

		for (l = 0; l < 7; l = l + 1)
		{
		    if ((row[x + l] == row[x + l + 1]))	// not a perfect check, but what a weird scheme. :|
		    {
			char sBuffer[400];
			sprintf (sBuffer,
				 "The png file %s doesn't appear to meet the requirements for 320D\n", file_name);
			strcat (sBuffer, "         All even pixels must be color 0 or color 2.\n");
			strcat (sBuffer, "         All odd pixels  must be color 1 or color 3.\n");
			strcat (sBuffer, "         The colors for this sprite will be converted incorrectly.");
			prwarn (sBuffer);

			break;
		    }
		}

		val = 0;
		for (l = 0; l < 8; l++)
		{
		    val = val << 1;
		    workval = graphiccolorindex[row[x + l]];

		    //the color varies due to the bit position, so we just encode it as on or off.
		    if (workval > 1)
			val = val | 1;
		}
		graphicsdata[dmaplain][graphicsdatawidth[dmaplain] - 1][y] = val;
	    }
	}
	for (y = 0; y < height; y++)
	    free (row_pointers[y]);
	free (row_pointers);
	png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
	return;
    }

    prerror ("unexpected code path");
}


void add_includes (char *myinclude)
{
    if (includesfile_already_done)
	prwarn ("include ignored (includes should typically precede other statements)");
    strcat (user_includes, myinclude);
}

void add_inline (char *myinclude)
{
    removeCR (myinclude);
    printf (" include %s\n", myinclude);
    sprintf (redefined_variables[numredefvars++], "included.%s = 1", myinclude);
    if ((bankcount & 1) && (currentbank > 0))
	printf ("included.%s.bank = %d\n", myinclude, currentbank - 1);
    else
	printf ("included.%s.bank = %d\n", myinclude, currentbank);
    if ((banksetrom == 1) && (strncmp (myinclude, "hiscore.asm", 11) == 0))
    {
	// gfxprint only outputs to the bankset bank, when the bankset scheme is used.
	gfxprintf (" include %s\n", myinclude);
    }
}

void init_includes (char *path)
{
    if (path)
	strcpy (includespath, path);
    else
	includespath[0] = '\0';
    user_includes[0] = '\0';
}

void barf_graphic_file (void)
{
    FILE *dumpgraphics_fileout;
    char dumpgraphics_filename[256];
    int s, t, currentplain;
    int linewidth;
    int runi;
    int ADDRBASE;
    int ABADDRBASE;

    int BANKSTART, REALSTART, DMASIZE;

    if (zoneheight == 8)
	DMASIZE = 4096;
    if (zoneheight == 16)
	DMASIZE = 8192;
    if (bankcount == 0)
    {
	if ((banksetrom == 1) && (zoneheight == 8))
	    BANKSTART = 0xF000;
	else
	    BANKSTART = 0xE000;
    }
    else if (currentbank == (bankcount - 1))	// last bank
    {
	if ((banksetrom == 1) && (zoneheight == 8))
	{
	    BANKSTART = 0xF000;
	    REALSTART = 0x9000;
	}
	else
	{
	    BANKSTART = 0xE000;
	    REALSTART = 0x8000;
	}
    }
    else if ((romat4k == 1) && (currentbank == 0))
    {
	if (zoneheight == 16)
	{
	    BANKSTART = 0x6000;
	    REALSTART = 0x8000;
	}
	else			// zoneheight == 8
	{
	    BANKSTART = 0x7000;
	    REALSTART = 0x9000;
	}
    }
    else
    {
	if (zoneheight == 16)
	{
	    BANKSTART = 0xA000;
	    REALSTART = 0x8000;
	}
	else			// zoneheight == 8
	{
	    BANKSTART = 0xB000;
	    REALSTART = 0x9000;
	}
    }

    printf ("DMAHOLEEND%d SET .\n", currentdmahole);

    //if stdout is redirected, its time change it back to 7800.asm
    if (strcmp (stdoutfilename, "7800.asm") != 0)
    {
	strcpy (stdoutfilename, "7800.asm");
	if ((stdoutfilepointer = freopen (stdoutfilename, "a", stdout)) == NULL)
	{
	    prerror ("couldn't reopen the 7800.asm file");
	}
    }


    ADDRBASE = BANKSTART - (dmaplain * DMASIZE);

    if (bankcount == 0)		// non-banked
    {
	if (((graphicsdatawidth[dmaplain] > 0) || (dmaplain > 0)) && (banksetrom == 0))	//calculate from graphics area...
	{
	    printf (" echo \" \",[($%04X - gameend)]d , \"bytes of ROM space left in the main area.\"\n", ADDRBASE);
	    printf (" if ($%04X - gameend) < 0\n", ADDRBASE);
	    printf ("SPACEOVERFLOW SET (SPACEOVERFLOW+1)\n");
	    printf (" endif\n");
	}
	else
	{
	    printf (" echo \" \",[($%04X - gameend)]d , \"bytes of ROM space left in the main area.\"\n", 0xF000);
	    printf (" if ($%04X - gameend) < 0\n", 0xF000);
	    printf ("SPACEOVERFLOW SET (SPACEOVERFLOW+1)\n");
	    printf (" endif\n");
	}
    }
    else if ((currentbank + 1) == bankcount)	// 0xC000
    {

	if (((graphicsdatawidth[dmaplain] > 0) || (dmaplain > 0)) && (banksetrom == 0))	//calculate from graphics area...
	{
	    printf
		(" echo \" \",[($%04X - .)]d , \"bytes of ROM space left in the main area of bank %d.\"\n",
		 ADDRBASE, currentbank + 1);
	    printf (" if ($%04X - .) < 0\n", ADDRBASE);
	    printf ("SPACEOVERFLOW SET (SPACEOVERFLOW+1)\n");
	    printf (" endif\n");
	}
	else
	{
	    printf
		(" echo \" \",[($%04X - .)]d , \"bytes of ROM space left in the main area of bank %d.\"\n",
		 0xEFFF, currentbank + 1);
	    printf (" if ($%04X - .) < 0\n", 0xEFFF);
	    printf ("SPACEOVERFLOW SET (SPACEOVERFLOW+1)\n");
	    printf (" endif\n");
	}
    }
    else if ((romat4k == 1) && (currentbank == 0))	// 0x4000
    {

	if (((graphicsdatawidth[dmaplain] > 0) || (dmaplain > 0)) && (banksetrom == 0))	//calculate from graphics area...
	{
	    printf
		(" echo \" \",[($%04X - .)]d , \"bytes of ROM space left in the main area of bank %d.\"\n",
		 ADDRBASE, currentbank + 1);
	    printf (" if ($%04X - .) < 0\n", ADDRBASE);
	    printf ("SPACEOVERFLOW SET (SPACEOVERFLOW+1)\n");
	    printf (" endif\n");
	}
	else
	{
	    printf
		(" echo \" \",[($%04X - .)]d , \"bytes of ROM space left in the main area of bank %d.\"\n",
		 0x7FFF, currentbank + 1);
	    printf (" if ($%04X - .) < 0\n", 0x7FFF);
	    printf ("SPACEOVERFLOW SET (SPACEOVERFLOW+1)\n");
	    printf (" endif\n");
	}
    }
    else
    {
	if (((graphicsdatawidth[dmaplain] > 0) || (dmaplain > 0)) && (banksetrom == 0))	//calculate from graphics area...
	{
	    printf
		(" echo \" \",[($%04X - .)]d , \"bytes of ROM space left in the main area of bank %d.\"\n",
		 ADDRBASE, currentbank + 1);
	    printf (" if ($%04X - .) < 0\n", ADDRBASE);
	    printf ("SPACEOVERFLOW SET (SPACEOVERFLOW+1)\n");
	    printf (" endif\n");
	}
	else
	{
	    printf
		(" echo \" \",[($%04X - .)]d , \"bytes of ROM space left in the main area of bank %d.\"\n",
		 0xBFFF, currentbank + 1);
	    printf (" if ($%04X - .) < 0\n", 0xBFFF);
	    printf ("SPACEOVERFLOW SET (SPACEOVERFLOW+1)\n");
	    printf (" endif\n");
	}
    }

    if ((graphicsdatawidth[dmaplain] > 0) || (dmaplain > 0))	//only process if the incgraphic command was encountered.
    {
	if (((bankcount > 0) && (zoneheight == 16) && (dmaplain > 1) && (!TIGHTPACKBORDER)) ||
	    ((bankcount > 0) && (zoneheight == 8) && (dmaplain > 3) && (!TIGHTPACKBORDER)))
	{
	    prerror ("graphics overrun in bank %d", currentbank);
	}
	orgprintf (" if START_OF_ROM = . ; avoid dasm empty start-rom truncation.\n");
	orgprintf ("     .byte 0\n");
	orgprintf (" endif\n");
	orgprintf ("START_OF_ROM SET 0 ; scuttle so we always fail subsequent banks\n");


	int graphics_addr[256];
	int graphics_addr_absolute[256];
	int tightpacked[256];
	int dmaholeindex = 0;

	// In 7800basic, the graphics blocks are used up from back to front of the rom/bank.
	// 
	// We need to allow tight packing of graphics (no dma holes) below some game-selected
	// threshold address.
	// 
	// Dasm requires that rom data gets laid out front to back.
	// 
	// To adhere to all of these requirements, we need to:

	// 1. Setup the initial addresses for the back-to-front loop.
	ADDRBASE = BANKSTART;
	if (bankcount == 0)
	{
	    ABADDRBASE = ADDRBASE;
	}
	else
	{
	    if (romat4k == 1)
	        ABADDRBASE = REALSTART + ((currentbank - 1) * 0x4000) + 0x2000;
	    else
	        ABADDRBASE = REALSTART + (currentbank * 0x4000) + 0x2000;
	}

	// 1a. adjust if we can start in what would normally be a dma hole...
	if (ADDRBASE + (DMASIZE/2) <= TIGHTPACKBORDER)
	{
	    ADDRBASE = ADDRBASE + (DMASIZE/2);
	    ABADDRBASE = ABADDRBASE + (DMASIZE/2);
	}

	// 2. Loop back-to-front, updating and storing the graphics addresses for later.
	for (currentplain = dmaplain ; currentplain >= 0; currentplain--)
	{
	    graphics_addr[currentplain] = ADDRBASE;
	    graphics_addr_absolute[currentplain] = ABADDRBASE;
	    if (ADDRBASE > TIGHTPACKBORDER)
	    {
	        ADDRBASE = ADDRBASE - DMASIZE;
	        ABADDRBASE = ABADDRBASE - DMASIZE;
	        if (currentplain>0)
	            tightpacked[currentplain-1]=0;
	    }
	    else // (ADDRBASE <= TIGHTPACKBORDER)
	    {
	        ADDRBASE = ADDRBASE - (DMASIZE/2);
	        ABADDRBASE = ABADDRBASE - (DMASIZE/2);
	        if (currentplain>0)
	            tightpacked[currentplain-1]=1;
	    }
	}

	// 3. Loop front to back, to store the actual graphics data in the rom/bank.
	for (currentplain = 0; currentplain <= dmaplain; currentplain++)
	{
	    ADDRBASE = graphics_addr[currentplain];
	    ABADDRBASE = graphics_addr_absolute[currentplain];

	    prout ("\n");
	    if (bankcount == 0)
	    {
		prinfo ("GFX Block #%d starts @ $%04X", currentplain, ADDRBASE);
	    }
	    else
	    {
		prinfo ("bank #%d, GFX Block #%d starts @ $%04X", currentbank + 1, currentplain, ADDRBASE);
	    }
	    if (dumpgraphics)
	    {
		snprintf (dumpgraphics_filename, 255, "dump_gfx_%02d.bin", dumpgraphics_index);
		prinfo ("dumping GFX Block to \"%s\"", dumpgraphics_filename);
		dumpgraphics_fileout = fopen (dumpgraphics_filename, "wb");
		if (dumpgraphics_fileout == NULL)
		    prerror ("couldn't open file for dumping graphics block");
	    }
	    linewidth = 0;
	    prout ("       ");

	    char dumpgraphics_data[256];

	    for (s = zoneheight - 1; s >= 0; s--)
	    {
		if (bankcount == 0)
		    gfxprintf ("\n ORG $%04X,0  ; *************\n", ADDRBASE);
		else
		{
		    gfxprintf ("\n ORG $%04X,0  ; *************\n", ABADDRBASE);
		    gfxprintf ("\n RORG $%04X ; *************\n", ADDRBASE);
		}

		if (dumpgraphics)
		    memset (dumpgraphics_data, 0, 256);

		for (t = 0; t < graphicsdatawidth[currentplain]; t++)
		{
		    if (graphicslabels[currentplain][t][0] != 0)
		    {
			runi = 0;
			if (s == (zoneheight - 1))
			{
			    printf ("\n%s = $%X\n", graphicslabels[currentplain][t], t + ADDRBASE);
			    gfxprintf ("\n%s\n       HEX ", graphicslabels[currentplain][t]);
			    if ((linewidth + strlen (graphicslabels[currentplain][t])) > 60)
			    {
				linewidth = 0;
				prout ("\n       ");
			    }
			    prout (" %s", graphicslabels[currentplain][t]);
			    linewidth = linewidth + strlen (graphicslabels[currentplain][t]);
			}
			else
			    gfxprintf ("\n;%s\n       HEX ", graphicslabels[currentplain][t]);
		    }
		    gfxprintf ("%02x", graphicsdata[currentplain][t][s]);
		    if (dumpgraphics)
			dumpgraphics_data[t] = graphicsdata[currentplain][t][s];
		    runi++;
		    if ((runi % 32 == 0) && ((t + 1) < graphicsdatawidth[currentplain]))
			gfxprintf ("\n       HEX ");
		}
		gfxprintf ("\n");
		ADDRBASE = ADDRBASE + 256;
		if (dumpgraphics)
		    fwrite (dumpgraphics_data, 1, 256, dumpgraphics_fileout);
		if (bankcount > 0)
		    ABADDRBASE = ABADDRBASE + 256;
	    }
	    prout ("\n");
	    if (bankcount == 0)
		prinfo ("GFX block #%d has %d bytes left (%d x %d bytes)\n",
			currentplain,
			(256 - graphicsdatawidth[currentplain]) * zoneheight,
			(256 - graphicsdatawidth[currentplain]), zoneheight);
	    else
		prinfo
		    ("bank #%d, GFX block #%d has %d bytes left (%d x %d bytes)\n",
		     currentbank + 1, currentplain,
		     (256 - graphicsdatawidth[currentplain]) * zoneheight,
		     (256 - graphicsdatawidth[currentplain]), zoneheight);

	    if (dumpgraphics)
		fclose (dumpgraphics_fileout);

	    // if we're in a DMA hole, report on it and barf any code that was saved for it...
	    if ((tightpacked[currentplain])&&(currentplain < dmaplain))
	        prinfo ("No DMA hole here, due to tight packing");
	    else if ( (ADDRBASE != TIGHTPACKBORDER) && ( (currentplain < dmaplain) 
	    //else if ( ( (currentplain < dmaplain) 
		|| ((currentplain == dmaplain) && (bankcount > 0) && (currentbank + 1 < bankcount))) )
	    {
		prout ("\n");
		if (bankcount == 0)
		    prinfo ("DMA hole #%d starts @ $%04X", dmaholeindex, ADDRBASE);
		else
		    prinfo ("bank #%d, DMA hole #%d starts @ $%04X", currentbank + 1, dmaholeindex, ADDRBASE);

		if (bankcount == 0)
		    gfxprintf ("\n ORG $%04X,0  ; *************\n", ADDRBASE);
		else
		{
		    gfxprintf ("\n ORG $%04X,0  ; *************\n", ABADDRBASE);
		    gfxprintf ("\n RORG $%04X ; *************\n", ADDRBASE);
		}

		FILE *holefilepointer;
		char holefilename[256];
		fflush (stdout);
		sprintf (holefilename, "7800hole.%d.asm", dmaholeindex);
		holefilepointer = fopen (holefilename, "r");
		if (holefilepointer != NULL)
		{
		    prout ("        DMA hole code found and imported\n");
		    int c;
		    while ((c = getc (holefilepointer)) != EOF)
			putchar (c);
		    fclose (holefilepointer);
		    remove (holefilename);
		}
		else
		{
		    prout ("        no code defined for DMA hole\n");
		}

		if (holefilepointer != NULL)
		{
		    printf
			(" echo \"  \",\"  \",\"  \",\"  \",[(256*WZONEHEIGHT)-(DMAHOLEEND%d - DMAHOLESTART%d)]d , \"bytes of ROM space left in DMA hole %d.\"\n",
			 dmaholeindex, dmaholeindex, dmaholeindex);
		    printf
			(" if ((256*WZONEHEIGHT)-(DMAHOLEEND%d - DMAHOLESTART%d)) < 0\n", dmaholeindex, dmaholeindex);
		    printf ("SPACEOVERFLOW SET (SPACEOVERFLOW+1)\n");
		    printf (" endif\n");
		}
	        dmaholeindex++;
	    }

	    if (dumpgraphics)
	    {
		snprintf (dumpgraphics_filename, 255, "dump_gfx_%02d.asm", dumpgraphics_index);
		dumpgraphics_fileout = fopen (dumpgraphics_filename, "wb");
		if (dumpgraphics_fileout == NULL)
		    prerror ("couldn't open file for dumping graphics assembly");
	    }

	    //stick extra graphic info into variable defines
	    for (t = 0; t < 256; t++)
	    {
		if (graphicslabels[currentplain][t][0] != 0)
		{
		    sprintf (redefined_variables[numredefvars++],
			     "%s_width = $%02x", graphicslabels[currentplain][t], graphicsinfo[currentplain][t]);
		    sprintf (redefined_variables[numredefvars++],
			     "%s_width_twoscompliment = $%02x",
			     graphicslabels[currentplain][t], ((0 - (graphicsinfo[currentplain][t])) & 31));
		    sprintf (redefined_variables[numredefvars++],
			     "%s_mode = $%02x", graphicslabels[currentplain][t], graphicsmode[currentplain][t]);
		    if (dumpgraphics)
		    {
			fprintf (dumpgraphics_fileout, "%s = $%04x\n",
				 graphicslabels[currentplain][t], dumpgraphicsaddr + t);
			fprintf (dumpgraphics_fileout, "%s_width = $%02x\n",
				 graphicslabels[currentplain][t], graphicsinfo[currentplain][t]);
			fprintf (dumpgraphics_fileout,
				 "%s_width_twoscompliment = $%02x\n",
				 graphicslabels[currentplain][t], ((0 - (graphicsinfo[currentplain][t])) & 31));
			fprintf (dumpgraphics_fileout, "%s_mode = $%02x\n",
				 graphicslabels[currentplain][t], graphicsmode[currentplain][t]);
		    }
		}
	    }

	    if (dumpgraphics)
		fclose (dumpgraphics_fileout);

	    dumpgraphics_index++;

	}			// currentplain loop, 0 to dmaplain


	if (bankcount > 0)
	{
	    // clear out the structure for the next bank...
	    memset (graphicsdata, 0, 16 * 256 * 100 * sizeof (char));
	    memset (graphicslabels, 0, 16 * 256 * 80 * sizeof (char));
	    memset (graphicsinfo, 0, 16 * 256 * sizeof (char));
	    memset (graphicsmode, 0, 16 * 256 * sizeof (char));
	    for (currentplain = 0; currentplain <= dmaplain; currentplain++)
		graphicsdatawidth[currentplain] = 0;
	    dmaplain = 0;
	}

	prout ("\n");
    }
}

void create_a78info (void)
{
    FILE *out;
    out = fopen ("a78info.cfg", "w");
    if (out == NULL)
	prerror ("couldn't create a78info.cfg parameter file");
    fprintf (out, "### parameter file for 7800header, created by 7800basic\n");
    fclose (out);

}

void append_a78info (char *line)
{
    FILE *out;
    out = fopen ("a78info.cfg", "a");
    if (out == NULL)
	prerror ("couldn't add to a78info.cfg parameter file");
    fprintf (out, "%s\n", line);
    fclose (out);
}

void create_includes (char *includesfile)
{
    FILE *includesread, *includeswrite;
    char dline[500];
    char fullpath[500];
    int i;
    int writeline;
    removeCR (includesfile);
    if (includesfile_already_done)
	return;
    includesfile_already_done = 1;
    fullpath[0] = '\0';
    if (includespath[0])
    {
	strcpy (fullpath, includespath);
	if ((includespath[strlen (includespath) - 1] == '\\') || (includespath[strlen (includespath) - 1] == '/'))
	    strcat (fullpath, "includes/");
	else
	    strcat (fullpath, "/includes/");
    }
    strcat (fullpath, includesfile);
    if ((includesread = fopen (includesfile, "r")) == NULL)	// try again in current dir
    {
	if ((includesread = fopen (fullpath, "r")) == NULL)	// open file
	{
	    prerror ("can't open '%s' for reading", includesfile);
	}
    }
    else
	currdir_foundmsg (includesfile);
    if ((includeswrite = fopen ("includes.7800", "w")) == NULL)	// open file
    {

	prerror ("can't open 'includes.7800' for writing");
    }

    while (fgets (dline, 500, includesread))
    {
	for (i = 0; i < 500; ++i)
	{
	    if (dline[i] == ';')
	    {
		writeline = 0;
		break;
	    }
	    if (dline[i] == (unsigned char) 0x0A)
	    {
		writeline = 0;
		break;
	    }
	    if (dline[i] == (unsigned char) 0x0D)
	    {
		writeline = 0;
		break;
	    }
	    if (dline[i] > (unsigned char) 0x20)
	    {
		writeline = 1;
		break;
	    }
	    writeline = 0;
	}
	if (writeline)
	{
	    if (!strncasecmp (dline, "7800.asm", 8))
		if (user_includes[0] != '\0')
		    fprintf (includeswrite, "%s", user_includes);
	    fprintf (includeswrite, "%s", dline);
	}
    }
    fclose (includesread);
    fclose (includeswrite);
}

void printindex (char *mystatement, int myindex, int indirectflag)
{
    if (myindex == 0)
    {
	printimmed (mystatement);
	printf ("%s\n", mystatement);
    }
    else if (indirectflag != 0)
	printf ("(%s),y\n", mystatement);	// indexed with y!
    else
	printf ("%s,x\n", mystatement);	// indexed with x!
}

void loadindex (char *myindex, int indirectflag)
{
    if (strncmp (myindex, "TSX\0", 3))
    {
	if (indirectflag == 0)
	    printf ("	LDX ");	// get index
	else
	    printf ("	LDY ");	// get index
	printimmed (myindex);
	printf ("%s\n", myindex);
    }
}

int getindex (char *mystatement, char *myindex, int *indirectflag)
{
    int i, j, index = 0;
    *indirectflag = 0;
    for (i = 0; i < SIZEOFSTATEMENT; ++i)
    {
	if (mystatement[i] == '\0')
	{
	    i = SIZEOFSTATEMENT;
	    break;
	}
	if (mystatement[i] == '[')
	{
	    index = 1;
	    break;
	}
    }
    if ((i < (SIZEOFSTATEMENT-2)) && (mystatement[i] == '[') && (mystatement[i + 1] == '['))
	*indirectflag = 1;
    if (i < SIZEOFSTATEMENT)
    {
	if (*indirectflag == 0)
	{
	    strcpy (myindex, mystatement + i + 1);
	    myindex[strlen (myindex) - 1] = '\0';
	}
	else
	{
	    strcpy (myindex, mystatement + i + 2);
	    myindex[strlen (myindex) - 2] = '\0';
	}
	if (myindex[strlen (myindex) - 2] == ']')
	    myindex[strlen (myindex) - 2] = '\0';
	if (myindex[strlen (myindex) - 1] == ']')
	    myindex[strlen (myindex) - 1] = '\0';
	for (j = i; j < SIZEOFSTATEMENT; ++j)
	    mystatement[j] = '\0';
    }
    return index;
}

int checkmul (int value)
{
    // check to see if value can be optimized to save cycles

    if (!(value % 2))
	return 1;		// still faster than sub

    if (value < 14)
	return 1;		// always optimize these

    while (value != 1)
    {
	if (!(value % 13))
	    value /= 13;
	else if (!(value % 11))
	    value /= 11;
	else if (!(value % 9))
	    value /= 9;
	else if (!(value % 7))
	    value /= 7;
	else if (!(value % 5))
	    value /= 5;
	else if (!(value % 3))
	    value /= 3;
	else if (!(value % 2))
	    value /= 2;
	else
	    break;
	if (!(value % 2) && (optimization & 1) != 1)
	    break;		// do not optimize multplications
    }
    if (value == 1)
	return 1;
    else
	return 0;
}

int checkdiv (int value)
{
    // check to see if value is a power of two - if not, run standard div routine
    while (value != 1)
    {
	if (value % 2)
	    break;
	else
	    value /= 2;
    }
    if (value == 1)
	return 1;
    else
	return 0;
}


void mul (char **statement, int bits)
{
    // this will attempt to output optimized code depending on the multiplicand
    int multiplicand = strictatoi (statement[6]);
    int tempstorage = 0;
    // we will optimize specifically for 2,3,5,7,9,11,13
    if (bits == 16)
    {
	printf ("  ldx #0\n");
	printf ("  stx temp1\n");
    }
    while (multiplicand != 1)
    {
	if (!(multiplicand % 13))
	{
	    if (tempstorage)
	    {
		strcpy (statement[4], "temp2");
		printf ("  sta temp2\n");
	    }
	    multiplicand /= 13;

	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  clc\n");
	    printf ("  adc ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	    if (bits == 16)
	    {
		printf ("  tax\n");
		printf ("  lda temp1\n");
		printf ("  adc #0\n");
		printf ("  sta temp1\n");
		printf ("  txa\n");
	    }
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  asl\n");
	    if (bits == 16)
            {
		printf ("  rol temp1\n");
	        printf ("  clc\n");
            }
	    printf ("  adc ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	    if (bits == 16)
	    {
		printf ("  tax\n");
		printf ("  lda temp1\n");
		printf ("  adc #0\n");
		printf ("  sta temp1\n");
		printf ("  txa\n");
	    }

	    tempstorage = 1;
	}
	else if (!(multiplicand % 11))
	{
	    if (tempstorage)
	    {
		strcpy (statement[4], "temp2");
		printf ("  sta temp2\n");
	    }
	    multiplicand /= 11;
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  clc\n");
	    printf ("  adc ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	    if (bits == 16)
	    {
		printf ("  tax\n");
		printf ("  lda temp1\n");
		printf ("  adc #0\n");
		printf ("  sta temp1\n");
		printf ("  txa\n");
	    }
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  clc\n");
	    printf ("  adc ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	    if (bits == 16)
	    {
		printf ("  tax\n");
		printf ("  lda temp1\n");
		printf ("  adc #0\n");
		printf ("  sta temp1\n");
		printf ("  txa\n");
	    }
	    tempstorage = 1;
	}
	else if (!(multiplicand % 9))
	{
	    if (tempstorage)
	    {
		strcpy (statement[4], "temp2");
		printf ("  sta temp2\n");
	    }
	    multiplicand /= 9;
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  clc\n");
	    printf ("  adc ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	    if (bits == 16)
	    {
		printf ("  tax\n");
		printf ("  lda temp1\n");
		printf ("  adc #0\n");
		printf ("  sta temp1\n");
		printf ("  txa\n");
	    }
	    tempstorage = 1;
	}
	else if (!(multiplicand % 7))
	{
	    if (tempstorage)
	    {
		strcpy (statement[4], "temp2");
		printf ("  sta temp2\n");
	    }
	    multiplicand /= 7;
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  sec\n");
	    printf ("  sbc ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	    if (bits == 16)
	    {
		printf ("  tax\n");
		printf ("  lda temp1\n");
		printf ("  sbc #0\n");
		printf ("  sta temp1\n");
		printf ("  txa\n");
	    }
	    tempstorage = 1;
	}
	else if (!(multiplicand % 5))
	{
	    if (tempstorage)
	    {
		strcpy (statement[4], "temp2");
		printf ("  sta temp2\n");
	    }
	    multiplicand /= 5;
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  clc\n");
	    printf ("  adc ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	    if (bits == 16)
	    {
		printf ("  tax\n");
		printf ("  lda temp1\n");
		printf ("  adc #0\n");
		printf ("  sta temp1\n");
		printf ("  txa\n");
	    }
	    tempstorage = 1;
	}
	else if (!(multiplicand % 3))
	{
	    if (tempstorage)
	    {
		strcpy (statement[4], "temp2");
		printf ("  sta temp2\n");
	    }
	    multiplicand /= 3;
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	    printf ("  clc\n");
	    printf ("  adc ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	    if (bits == 16)
	    {
		printf ("  tax\n");
		printf ("  lda temp1\n");
		printf ("  adc #0\n");
		printf ("  sta temp1\n");
		printf ("  txa\n");
	    }
	    tempstorage = 1;
	}
	else if (!(multiplicand % 2))
	{
	    multiplicand /= 2;
	    printf ("  asl\n");
	    if (bits == 16)
		printf ("  rol temp1\n");
	}
	else
	{
	    printf ("  ldy #%d\n", multiplicand);
	    printf ("  jsr mul%d\n", bits);
	    prwarn ("there seems to be a problem. your code may not run properly. please report seeing this message.");

	}
    }
}

void divd (char **statement, int bits)
{
    int divisor = strictatoi (statement[6]);
    if (bits == 16)
    {
	printf ("  ldx #0\n");
	printf ("  stx temp1\n");
    }
    while (divisor != 1)
    {
	if (!(divisor % 2))	// div by power of two is the only easy thing
	{
	    divisor /= 2;
	    printf ("  lsr\n");
	    if (bits == 16)
		printf ("  rol temp1\n");	// I am not sure if this is actually correct
	}
	else
	{
	    printf ("  ldy #%d\n", divisor);
	    printf ("  jsr div%d\n", bits);
	    prwarn ("there seems to be a problem. your code may not run properly. please report seeing this message.");
	}
    }

}

void endfunction ()
{
    if (!doingfunction)
	prerror ("extraneous end keyword encountered");
    doingfunction = 0;
}

void function (char **statement)
{
    assertminimumargs (statement, "function", 1);

    // declares a function - only needed if function is in bB.  For asm functions, see
    // the help.html file.
    // determine number of args, then run until we get an end.
    doingfunction = 1;
    printf ("%s\n", statement[2]);
    printf ("	sta temp1\n");
    printf ("	sty temp2\n");
}

void callfunction (char **statement)
{
    // called by assignment to a variable
    // does not work as an argument in another function or an if-then... yet.
    int i, index = 0;
    int indirectflag = 0;
    char getindex0[SIZEOFSTATEMENT];
    int arguments = 0;
    int argnum[10];
    for (i = 6; statement[i][0] != ')'; ++i)
    {
	if (statement[i][0] != ',')
	{
	    argnum[arguments++] = i;
	}
	if (i > 195)
	    prerror ("missing \")\" at end of function call");
    }
    if (!arguments)
	prwarn ("function call with no arguments");
    while (arguments)
    {
	// get [index]
	index = 0;
	index |= getindex (statement[argnum[--arguments]], &getindex0[0], &indirectflag);
	if (indirectflag != 0)
	    prerror ("indirect arrays not supported as function call arguments");
	if (index)
	    loadindex (&getindex0[0], indirectflag);

	if (arguments == 1)
	    printf ("  ldy ");
	else
	    printf ("  lda ");
	printindex (statement[argnum[arguments]], index, indirectflag);

	if (arguments > 1)
	    printf ("  sta temp%d\n", arguments + 1);
    }
    printf (" jsr %s\n", statement[4]);
    printf (".calledfunction_%s = 1\n", statement[4]);

    strcpy (Areg, "invalid");

}

void incline ()
{
    line++;
}

int bbgetline ()
{
    return line;
}

void invalidate_Areg ()
{
    strcpy (Areg, "invalid");
}

int islabel (char **statement)
{				// this is for determining if the item after a "then" is a label or a statement
    // return of 0=label, 1=statement
    int i;
    // find the "then" or a "goto"
    for (i = 0; i < (STATEMENTCOUNT-2);)
	if (!strncmp (statement[i++], "then\0", 4))
	    break;
    return findlabel (statement, i);
}

int islabelelse (char **statement)
{				// this is for determining if the item after an "else" is a label or a statement
    // return of 0=label, 1=statement
    int i;
    // find the "else"
    for (i = 0; i < (STATEMENTCOUNT-2);)
	if (!strncmp (statement[i++], "else\0", 4))
	    break;
    return findlabel (statement, i);
}

int findlabel (char **statement, int i)
{
    char statementcache[SIZEOFSTATEMENT];
    // 0 if label, 1 if not
    if ((statement[i][0] > (unsigned char) 0x2F) && (statement[i][0] < (unsigned char) 0x3B))
	return 0;
    if ((statement[i + 1][0] == ':') && (strncmp (statement[i + 2], "rem\0", 3)))
	return 1;
    if (!strncmp (statement[i + 1], "else\0", 4))
	return 0;
    if (statement[i + 1][0] != '\0')
	return 1;
    // only possibilities left are: drawscreen, asm, next, return, maybe others added later?
    strcpy (statementcache, statement[i]);
    removeCR (statementcache);
    if (!strncmp (statementcache, "incgraphic\0", 10))
	return 1;
    if (!strncmp (statementcache, "incmapfile\0", 10))
	return 1;
    if (!strncmp (statementcache, "drawscreen\0", 10))
	return 1;
    if (!strncmp (statementcache, "drawwait\0", 8))
	return 1;
    if (!strncmp (statementcache, "clearscreen\0", 11))
	return 1;
    if (!strncmp (statementcache, "restorescreen\0", 13))
	return 1;
    if (!strncmp (statementcache, "savescreen\0", 10))
	return 1;
    if (!strncmp (statementcache, "speak\0", 6))
	return 1;
    if (!strncmp (statementcache, "characterset\0", 12))
	return 1;
    if (!strncmp (statementcache, "plotsprite\0", 10))
	return 1;
    if (!strncmp (statementcache, "plotchars\0", 9))
	return 1;
    if (!strncmp (statementcache, "plotmapfile\0", 11))
	return 1;
    if (!strncmp (statementcache, "plotmap\0", 7))
	return 1;
    if (!strncmp (statementcache, "plotvalue\0", 9))
	return 1;
    if (!strncmp (statementcache, "peekchar\0", 8))
	return 1;
    if (!strncmp (statementcache, "pokechar\0", 8))
	return 1;
    if (!strncmp (statementcache, "snesdetect\0", 10))
	return 1;
    if (!strncmp (statementcache, "displaymode\0", 11))
	return 1;
    if (!strncmp (statementcache, "characterset\0", 12))
	return 1;
    if (!strncmp (statementcache, "tsound\0", 6))
	return 1;
    if (!strncmp (statementcache, "psound\0", 6))
	return 1;
    if (!strncmp (statementcache, "alphachars\0", 10))
	return 1;
    if (!strncmp (statementcache, "alphadata\0", 9))
	return 1;
    if (!strncmp (statementcache, "loadrambank\0", 11))
	return 1;
    if (!strncmp (statementcache, "loadrombank\0", 11))
	return 1;
    if (!strncmp (statementcache, "loadmemory\0", 10))
	return 1;
    if (!strncmp (statementcache, "savememory\0", 10))
	return 1;
    if (!strncmp (statementcache, "drawhiscores\0", 13))
	return 1;
    if (!strncmp (statementcache, "hiscoreload\0", 12))
	return 1;
    if (!strncmp (statementcache, "hiscoreclear\0", 13))
	return 1;
    if (!strncmp (statementcache, "memcpy\0", 6))
	return 1;
    if (!strncmp (statementcache, "memset\0", 6))
	return 1;
    if (!strncmp (statementcache, "lives:\0", 6))
	return 1;
    if (!strncmp (statementcache, "asm\0", 4))
	return 1;
    if (!strncmp (statementcache, "pop\0", 4))
	return 1;
    if (!strncmp (statementcache, "stack\0", 5))
	return 1;
    if (!strncmp (statementcache, "push\0", 4))
	return 1;
    if (!strncmp (statementcache, "pull\0", 4))
	return 1;
    if (!strncmp (statementcache, "rem\0", 3))
	return 1;
    if (!strncmp (statementcache, "next\0", 4))
	return 1;
    if (!strncmp (statementcache, "reboot\0", 6))
	return 1;
    if (!strncmp (statementcache, "return\0", 6))
	return 1;
    if (!strncmp (statementcache, "callmacro\0", 9))
	return 1;
    if (!strncmp (statementcache, "playsfx\0", 8))
	return 1;
    if (!strncmp (statementcache, "adjustvisible\0", 14))
	return 1;
    if (!strncmp (statementcache, "plotvalue\0", 10))
	return 1;
    if (!strncmp (statementcache, "newblock\0", 9))
	return 1;
    if (!strncmp (statementcache, "let\0", 4))
	return 1;
    if (!strncmp (statementcache, "dec\0", 4))
	return 1;
    if (statement[i + 1][0] == '=')
	return 1;		// it's a variable assignment

    return 0;			// I really hope we've got a label !!!!
}

void sread (char **statement)
{
    // read sequential data
    printf ("  ldy #0\n");
    printf ("  lda (%s),y\n", statement[6]);
    printf ("  inc %s\n", statement[6]);
    printf ("  bne *+4\n");
    printf ("  inc %s+1\n", statement[6]);
    strcpy (Areg, "invalid");
}

void sdata (char **statement)
{
    // sequential data, works like regular basics and doesn't have the 256 byte restriction
    char data[SIZEOFSTATEMENT];
    int i;
    removeCR (statement[4]);
    sprintf (redefined_variables[numredefvars++], "%s = %s", statement[2], statement[4]);
    printf ("  lda #<%s_begin\n", statement[2]);
    printf ("  sta %s\n", statement[4]);
    printf ("  lda #>%s_begin\n", statement[2]);
    printf ("  sta %s+1\n", statement[4]);

    printf ("  jmp .skip%s\n", statement[0]);
    // not affected by noinlinedata

    printf ("%s_begin\n", statement[2]);
    while (1)
    {
	if (((!fgets (data, SIZEOFSTATEMENT, preprocessedfd))
	     || ((data[0] < (unsigned char) 0x3A) && (data[0] > (unsigned char) 0x2F))) && (data[0] != 'e'))
	{
	    prerror ("missing \"end\" keyword at end of data");
	    exit (1);
	}
	line++;
	if (!strncmp (data, "end\0", 3))
	    break;
	remove_trailing_commas (data);
	for (i = 0; i < SIZEOFSTATEMENT; ++i)
	{
	    if ((int) data[i] > 32)
		break;
	    if (((int) data[i] < 14) && ((int) data[i] != 9))
		i = SIZEOFSTATEMENT;
	}
	if (i < SIZEOFSTATEMENT)
	    printf ("	.byte %s\n", data);
    }
    printf (".skip%s\n", statement[0]);

}

void data (char **statement)
{
    char data[SIZEOFSTATEMENT];
    char **data_length;
    char **deallocdata_length;
    int i, j;

    int thisdatabankset;

    thisdatabankset = 0;

    assertminimumargs (statement, "data", 1);

    removeCR (statement[2]);

    if ((banksetrom) && (strncmp (statement[2], "bset_", 5) == 0))
	thisdatabankset = 1;

    if (!thisdatabankset)
	if (!(optimization & 4))
	    printf ("  jmp .skip%s\n", statement[0]);

    if (!thisdatabankset)
	printf ("%s\n", statement[2]);
    else
	gfxprintf ("%s\n", statement[2]);

    while (1)
    {
	if (((!fgets (data, SIZEOFSTATEMENT, preprocessedfd))
	     || ((data[0] < (unsigned char) 0x3A) && (data[0] > (unsigned char) 0x2F))) && (data[0] != 'e'))
	{
	    prerror ("missing \"end\" keyword at end of data");
	    exit (1);
	}
	line++;
	data[SIZEOFSTATEMENT-1]=0;
	if (strlen(data)==(SIZEOFSTATEMENT-1))
	{
	    prerror ("line length exceeded in data statelement.");
	    exit (1);
	}
	if (!strncmp (data, "end\0", 3))
	    break;
	remove_trailing_commas (data);
	for (i = 0; i < SIZEOFSTATEMENT; ++i)
	{
	    if ((int) data[i] > 32)
		break;
	    if (((int) data[i] < 14) && ((int) data[i] != 9))
		i = SIZEOFSTATEMENT;
	}
	if (i < SIZEOFSTATEMENT)
	{
	    if (!thisdatabankset)
		printf ("  .byte %s\n", data);
	    else
		gfxprintf ("  .byte %s\n", data);
	}
    }

    if (!thisdatabankset)
    {
	printf (".skip%s\n", statement[0]);
	printf ("%s_length = [. - %s]\n", statement[2], statement[2]);
    }
    else
    {
	gfxprintf ("%s_length = [. - %s]\n", statement[2], statement[2]);
    }
    sprintf (constants[numconstants++], "%s_length", statement[2]);

    char consthilo[SIZEOFSTATEMENT];
    snprintf (consthilo, SIZEOFSTATEMENT, "%s_lo", statement[2]);
    strcpy (constants[numconstants++], consthilo);	// record to queue
    snprintf (consthilo, SIZEOFSTATEMENT, "%s_hi", statement[2]);
    strcpy (constants[numconstants++], consthilo);	// record to queue

    if (!thisdatabankset)
    {
	printf ("%s_lo SET #<%s\n", statement[2], statement[2]);
	printf ("%s_hi SET #>%s\n", statement[2], statement[2]);
    }
    else
    {
	gfxprintf ("%s_lo SET #<%s\n", statement[2], statement[2]);
	gfxprintf ("%s_hi SET #>%s\n", statement[2], statement[2]);
    }
}

void speak (char **statement)
{
    //   1       2
    // speak   label

    assertminimumargs (statement, "speak", 1);
    removeCR (statement[2]);
    printf ("  lda #255\n");
    printf ("  sta voxlock ; disable vox output\n");
    printf ("  SPEAK %s\n", statement[2]);
    printf ("  lda #0\n");
    printf ("  sta voxlock ; re-enable vox output\n");
}

void loadmemory (char **statement)
{

    int i, k;

    // loadmemory [vars]
    // vars can be all listed: "a b c d e"  or ranged: "a-e"
    // a maximum of 25 vars can be loaded

    assertminimumargs (statement, "loadmemory", 1);

    removeCR (statement[4]);

    //skip it all if no device is attached...
    printf ("  lda hsdevice\n");
    printf ("  bne continueloadmemory%d\n", templabel);
    printf ("  jmp skiploadmemory%d\n", templabel);
    printf ("continueloadmemory%d\n", templabel);

    //ensure we invalidate the current entry...
    printf ("  lda #$ff\n");
    printf ("  sta hsdifficulty\n");

    printf ("  jsr loaddifficultytable\n");
    // ** The loaddifficultytable routine will load our bogus difficulty table.
    // ** Along the way it will setup (HSGameTableLo) to point to either
    // ** the loaded RAW load buffer (AVox) or the in-RAM RAW game table (HSC)
    // ** which is really what we're after.

    if (statement[3][0] == '-')	// range
    {
	// ** copy the memory from (HSGameTableLo),y to the specified variables
	printf ("  ldy #(%s-%s)\n", statement[4], statement[2]);
	printf ("loadmemory%d\n", templabel);
	printf ("  lda (HSGameTableLo),y\n");
	printf ("  sta %s,y\n", statement[2]);
	printf ("  dey\n");
	printf ("  bpl loadmemory%d\n", templabel);
    }
    else
    {
	i = 2;
	printf ("  ldy #0\n");
	while ((statement[i][0] != ':') && (statement[i][0] != '\0'))
	{
	    for (k = 0; k < SIZEOFSTATEMENT; ++k)
		if ((statement[i][k] == (unsigned char) 0x0A) || (statement[i][k] == (unsigned char) 0x0D))
		    statement[i][k] = '\0';
	    if (i > 2)
		printf ("  iny\n");
	    printf ("  lda (HSGameTableLo),y\n");
	    printf ("  sta %s\n", statement[i++]);
	}
    }

    //ensure we invalidate the next entry...
    printf ("  lda #$ff\n");
    printf ("  sta hsdifficulty\n");
    printf ("skiploadmemory%d\n", templabel);

    templabel++;

}


void savememory (char **statement)
{

    int i, k;

    // savememory [vars]
    // vars can be all listed: "a b c d e"  or ranged: "a-e"
    // a maximum of 25 vars can be saved

    assertminimumargs (statement, "savememory", 1);

    removeCR (statement[4]);

    //skip it all if no device is attached...
    printf ("  lda hsdevice\n");
    printf ("  bne continuesavememory%d\n", templabel);
    printf ("  jmp skipsavememory%d\n", templabel);
    printf ("continuesavememory%d\n", templabel);

    printf ("  jsr loaddifficultytable\n");
    // ** The loaddifficultytable routine will load our bogus difficulty table.
    // ** Along the way it will setup (HSGameTableLo) to point to either
    // ** the loaded RAW load buffer (AVox) or the in-RAM RAW game table (HSC)
    // ** which is really what we're after.

    if (statement[3][0] == '-')	// range
    {
	// ** copy the memory from (HSGameTableLo),y to the specified variables
	printf ("  ldy #(%s-%s)\n", statement[4], statement[2]);
	printf ("savememory%d\n", templabel);
	printf ("  lda %s,y\n", statement[2]);
	printf ("  sta (HSGameTableLo),y\n");
	printf ("  dey\n");
	printf ("  bpl savememory%d\n", templabel);
    }
    else
    {
	i = 2;
	printf ("  ldy #0\n");
	while ((statement[i][0] != ':') && (statement[i][0] != '\0'))
	{
	    for (k = 0; k < SIZEOFSTATEMENT; ++k)
		if ((statement[i][k] == (unsigned char) 0x0A) || (statement[i][k] == (unsigned char) 0x0D))
		    statement[i][k] = '\0';
	    if (i > 2)
		printf ("  iny\n");
	    printf ("  lda %s\n", statement[i++]);
	    printf ("  sta (HSGameTableLo),y\n");
	}
    }
    // if its HSC, we already wrote over the game save memory above
    // but we need to save the data if the device is an AtariVox
    printf ("  lda hsdevice\n");
    printf ("  cmp #2\n");
    printf ("  bne skipvoxsave%d\n", templabel);
    printf ("  jsr savedifficultytableAVOXskipconvert\n");
    printf ("  ifconst DOUBLEBUFFER\n");
    printf ("    lda doublebufferstate\n");
    printf ("    bne skipsavememory%d\n", templabel);
    printf ("  endif\n");
    printf ("  jsr drawscreen\n");
    printf ("  jsr drawscreen\n");
    printf ("skipvoxsave%d\n", templabel);
    printf ("skipsavememory%d\n", templabel);
    templabel++;
}


void hiscoreload (char **statement)
{
    //       1
    // hiscoreload
    printf ("  jsr loaddifficultytable\n");
}

void hiscoreclear (char **statement)
{
    //       1
    // hiscoreclear
    printf ("  jsr loaddifficultytable\n");
    printf ("  jsr cleardifficultytablemem\n");
    printf ("  jsr savedifficultytable\n");
}


void drawhiscores (char **statement)
{
    //       1        2
    // drawhiscores mode

    assertminimumargs (statement, "drawhiscores", 1);
    removeCR (statement[2]);

    if (strcmp (statement[2], "attract") == 0)
    {
	printf ("  jsr loaddifficultytable\n");
	printf ("  lda #0\n");
	printf ("  sta hsdisplaymode\n");
	printf ("  jsr hscdrawscreen\n");
    }
    else if (strcmp (statement[2], "single") == 0)
    {
	printf ("  lda #1\n");
	printf ("  sta hsdisplaymode\n");
	printf ("  jsr loaddifficultytable\n");
	printf ("  jsr hscdrawscreen\n");
	printf ("  jsr savedifficultytable\n");
    }
    else if (strcmp (statement[2], "player1") == 0)
    {
	printf ("  lda #2\n");
	printf ("  sta hsdisplaymode\n");
	printf ("  jsr loaddifficultytable\n");
	printf ("  jsr hscdrawscreen\n");
	printf ("  jsr savedifficultytable\n");
    }
    else if (strcmp (statement[2], "player2") == 0)
    {
	printf ("  lda #3\n");
	printf ("  sta hsdisplaymode\n");
	printf ("  jsr loaddifficultytable\n");
	printf ("  jsr hscdrawscreen\n");
	printf ("  jsr savedifficultytable\n");
    }
    else if (strcmp (statement[2], "player2joy1") == 0)
    {
	printf ("  lda #4\n");
	printf ("  sta hsdisplaymode\n");
	printf ("  jsr loaddifficultytable\n");
	printf ("  jsr hscdrawscreen\n");
	printf ("  jsr savedifficultytable\n");
    }
    else
	prerror ("unsupported argument for drawscores:%s", statement[2]);
}

int getnotevalue (char *command)
{
    // ** Determine if this songdata command is a note.
    // ** If so, return its code, otherwise return negative.

    int notevalue;

    const int notes[7] = { 9, 11, 0, 2, 4, 5, 7 };

    //note characters are always 'a' to 'g'...
    if ((command[0] < 'a') || (command[0] > 'g'))
	return (-1);

    //note characters are always followed by nothing, a sharp, or a digit...
    if ((command[1] != 0) && (command[1] != '#') && ((command[1] < '0') || (command[1] > '9')))
	return (-1);

    //If we're here, it seems to be a note. Lets decode it...
    notevalue = notes[command[0] - 'a'];
    if (command[1] == '#')
	notevalue++;
    return (notevalue);
}

int getnotelength (char *command)
{
    // ** Determine if this songdata command is a note.
    // ** If so, return its length, otherwise return negative.

    int t;
    int notelength;

    //note characters are always 'a' to 'g'. we also allow 'r' rests.
    if (((command[0] < 'a') || (command[0] > 'g')) && (command[0] != 'r'))
	return (-1);

    //note characters are always followed by nothing, a sharp, or a digit...
    if ((command[1] != 0) && (command[1] != '#') && ((command[1] < '0') || (command[1] > '9')))
	return (-1);

    //If we're here, it seems to be a note. Lets decode it...
    for (t = 1; command[t] != 0; t++)
	if ((command[t] >= '0') && (command[t] <= '9'))
	    break;
    if (command[t] == 0)
	return (4);		// default to 4x 16th notes, or 1/4
    notelength = strictatoi (command + t);
    if ((notelength > 16) || (notelength < 1))
	prerror ("the note length '%d' isn't valid", notelength);
    if ((16 % notelength) > 0)
	prerror ("only note lengths 1,2,4,8, and 16 are valid");
    return ((16 / notelength) - 1);
}

int getnoteoctave (char *command)
{
    // ** Return an absolute note index from a note+octave
    // ** If so, return its code, otherwise return negative.

    int notevalue;
    int octave;
    int t;

    const int notes[7] = { 9, 11, 0, 2, 4, 5, 7 };

    //note characters are always 'a' to 'g'...
    if ((command[0] < 'a') || (command[0] > 'g'))
	return (-1);

    //note characters are always followed by nothing, a sharp, or a digit...
    if ((command[1] != 0) && (command[1] != '#') && ((command[1] < '0') || (command[1] > '9')))
	return (-1);

    //If we're here, it seems to be a note. Lets decode it...
    notevalue = notes[command[0] - 'a'];
    if (command[1] == '#')
	notevalue++;

    //Now decode the octave portion...
    for (t = 1; command[t] != 0; t++)
	if ((command[t] >= '0') && (command[t] <= '9'))
	    break;

    if (command[t] == 0)
	return (-1);		// the octave info is missing!?!

    octave = strictatoi (command + t);

    return ((octave * 12) + notevalue);
}

int getpatternloops (char *patternname)
{
    char *period;
    period = strrchr (patternname, '.');
    if (period == NULL)
	return (1);
    period[0] = 0;		// drop the loop notation on returning the loop value
    return (strictatoi (period + 1));
}


void songdata (char **statement)
{
    char data[1001];		// allow for long lines
    char savepatternname[200];
    char songstatements[200][100];
    char *wordstart;
    int s;
    int t, EOL, notevalue, notelength, tempovalue;
    int loopcount;
    assertminimumargs (statement, "songdata", 1);
    removeCR (statement[2]);

    savepatternname[0] = 0;

    if (!(optimization & 4))
	printf ("  jmp .skip%d\n", templabel);
    printf ("songdatastart_%s\n", statement[2]);

    while (1)
    {
	memset (data, 0, 1001);
	if (!fgets (data, 1000, preprocessedfd))
	{
	    prerror ("missing \"end\" keyword at end of data");
	    exit (1);
	}
	line++;
	if (!strncmp (data, "end\0", 3))
	{
	    if (savepatternname[0] != 0)
	    {
		//we have a pattern in progress. end it first.
		printf ("  .byte $ff, <%s, >%s ; pattern end\n", savepatternname, savepatternname);
	    }
	    break;
	}

	wordstart = data;
	s = 0;
	for (t = 0; t < 200; t++)
	{
	    EOL = FALSE;
	    for (s = 0; s < 100; s++)
	    {
		if (wordstart[s] == 0)
		    EOL = TRUE;
		if ((wordstart[s] == ' ') || (wordstart[s] < 32) || (wordstart[s] == 0))
		{
		    wordstart[s] = 0;
		    strncpy (songstatements[t], wordstart, 99);
		    if (EOL == FALSE)
		    {
			wordstart[s] = ' ';
			wordstart = wordstart + s + 1;
		    }
		    s = 500;
		    continue;
		}
	    }
	}

	if ((songstatements[0][0] == 0) && (songstatements[1][0] == 0))
	    continue;		// it was a blank line

	printf (" ;%s\n", data);

	if (data[0] > 47)
	{
	    if (savepatternname[0] != 0)
	    {
		//we have a pattern in progress. end it first.
		printf ("  .byte 255, <%s, >%s ; pattern end\n", savepatternname, savepatternname);
	    }
	    //non-indented character. ie. the start of a label.
	    printf ("%s_%s\n", statement[2], songstatements[0]);
	    sprintf (savepatternname, "%s_%s", statement[2], songstatements[0]);
	}

	for (t = 1; t < 200; t++)
	{
	    if (songstatements[t][0] == 0)
		break;		//all done
	    if (songstatements[t][0] == ';')
		break;		//comment. ignore the rest.
	    if (savepatternname[0] == 0)
		prerror ("songdata has data without pattern label '%s'", songstatements[t]);

	    if (songstatements[t][0] == '\'')
	    {
		// this is 16th note drum notation
		s = 1;
		while ((songstatements[t][s] != 0) && (songstatements[t][s] != '\''))
		{
		    if (songstatements[t][s] != '.')	//its a beat/note
		    {
			int u = s;
			notelength = 0;
			notevalue = 0;
			songstatements[t][s] = tolower (songstatements[t][s]);
			//we either have rests or notes. This doesn't appear to be a rest...
			if ((songstatements[t][s] >= '0') && (songstatements[t][s] <= '9'))
			    notevalue = songstatements[t][s] - '0';
			else if ((songstatements[t][s] >= 'a') && (songstatements[t][s] <= 'b'))
			    notevalue = songstatements[t][s] + 10 - 'a';
			else
			    prerror ("drum pattern has unrecognized character '%s'", songstatements[t]);
			s = s + 1;
			while ((songstatements[t][s] == '.') && (notelength < 15))
			{
			    notelength = notelength + 1;
			    s = s + 1;
			}
			printf ("  .byte $%02x ; %c\n", (notevalue * 16) | notelength, songstatements[t][u]);
		    }
		    else	//its a rest
		    {
			notelength = 0;	// 1x 1/16th note length
			//conjoin any rests to save on data...
			s = s + 1;
			while ((songstatements[t][s] == '.') && (notelength < 15))
			{
			    notelength = notelength + 1;
			    s = s + 1;
			}
			printf ("  .byte $%02x ; rest %d/16th\n", (12 * 16) | notelength, notelength + 1);
		    }
		}
	    }
	    else		// regular channel data
	    {
		notevalue = getnotevalue (songstatements[t]);
		notelength = getnotelength (songstatements[t]);
		if (notevalue < 0)	// if negative, it wasn't a note value
		{
		    //its not a note. determine what it is...
		    if (notelength >= 0)	//if it doesn't have a valid value, but has a note length, its a rest
		    {
			printf ("  .byte $%02x ; %s\n", (12 * 16) | notelength, songstatements[t]);
			continue;
		    }
		    else if (songstatements[t][1] == '=')
		    {
			//its one of the absolute setting commands
			if (songstatements[t][0] == 'k')	// key/octave change...
			{
			    notevalue = getnoteoctave (songstatements[t] + 2);
			    if (notevalue > 255)
				prerror ("key change too large '%s'", songstatements[t]);
			    if (notevalue < 0)
				prerror ("key change invalid format '%s'", songstatements[t]);
			    printf ("  .byte $FB, $%02x ; %s\n", notevalue, songstatements[t]);
			    continue;
			}
			else if (songstatements[t][0] == 't')	// tempo change...
			{
			    long tempv;
			    tempv = (256 * strictatoi (songstatements[t] + 2)) / 450;
			    tempovalue = tempv;
			    if (tempovalue > 255)
				prerror ("tempo too large '%s'", songstatements[t]);
			    if (tempovalue < 0)
				prerror ("tempo change invalid format '%s'", songstatements[t]);
			    printf ("  .byte $F9, $%02x ; %s\n", tempovalue, songstatements[t]);
			    continue;
			}
			else if (songstatements[t][0] == 'i')	// instrument change...
			{
			    printf (" .byte $F8, <%s, >%s ; %s\n",
				    songstatements[t] + 2, songstatements[t] + 2, songstatements[t]);
			    continue;
			}
			else
			{
			    prerror ("songdata unsupported '=' command '%s'", songstatements[t]);
			}
		    }
		    else if ((songstatements[t][1] == '+') || (songstatements[t][1] == '-'))
		    {
			//its one of the relative setting commands
			if (songstatements[t][0] == 'k')	// relative key/octave change...
			{
			    notevalue = strictatoi (songstatements[t] + 2);
			    if (notevalue > 15)
				prerror ("relative key change too large '%s'", songstatements[t]);
			    if (notevalue < 0)
				prerror ("relative key change invalid format '%s'", songstatements[t]);
			    if (songstatements[t][1] == '+')
				printf ("  .byte $%02x ; %s\n", 0xD0 | notevalue, songstatements[t]);
			    else
				printf ("  .byte $%02x ; %s\n", 0xE0 | notevalue, songstatements[t]);
			    continue;
			}
			else if (songstatements[t][0] == 't')	// relative tempo change...
			{
			    tempovalue = strictatoi (songstatements[t] + 2);
			    if (tempovalue > 255)
				prerror ("relative tempo change too large '%s'", songstatements[t]);
			    if (tempovalue < 1)
				prerror ("relative tempo change invalid format '%s'", songstatements[t]);
			    if (songstatements[t][1] == '-')
				tempovalue = 256 - tempovalue;
			    printf ("  .byte $FA, $%02x ; %s\n", tempovalue, songstatements[t]);
			    continue;
			}

			else
			    prerror
				("songdata using relative notation for unsupported command '%s'", songstatements[t]);
		    }
		    else if (songstatements[t][0] == '>')
		    {
			printf ("  .byte $%02x ; %s\n", 0xD0 | 12, songstatements[t]);
		    }
		    else if (songstatements[t][0] == '<')
		    {
			printf ("  .byte $%02x ; %s\n", 0xE0 | 12, songstatements[t]);
		    }

		    else
		    {
			// The second character doesn't match a note or any of our commands.
			// Assume it's a pattern call.
			loopcount = getpatternloops (songstatements[t]);
			if ((loopcount > 8) || (loopcount < 1))
			    prerror ("bad loop count %d for song pattern '%s'", loopcount, songstatements[t]);
			printf ("  .byte $%02x, <%s_%s, >%s_%s\n",
				0xf0 | (loopcount - 1), statement[2],
				songstatements[t], statement[2], songstatements[t]);
		    }
		}
		else		//the statement is a note...
		    printf ("  .byte $%02x ; %s\n", (notevalue * 16) | notelength, songstatements[t]);
	    }
	}
    }
    printf ("songchanneltable_%s ; the address of the channel data\n", statement[2]);
    for (t = 0; t < 4; t++)
    {
	printf ("  ifconst %s_main%d\n", statement[2], t + 1);
	printf ("    .word %s_main%d\n", statement[2], t + 1);
	printf ("  else\n");
	printf ("    .word $0000\n");
	printf ("  endif\n");
    }
    printf
	(" echo \" \",\"(tracker song '%s' used \" ,[(. - songdatastart_%s)]d , \"bytes)\"\n",
	 statement[2], statement[2]);

    if (!(optimization & 4))
	printf (".skip%d\n", templabel);
    templabel = templabel + 1;
}

void stopsong ()
{
    printf ("  lda #0\n");
    printf ("  sta songtempo\n");
}

void playsong (char **statement)
{
    int tempovalue, paltempovalue;
    assertminimumargs (statement, "playsong", 2);
    removeCR (statement[2]);
    removeCR (statement[3]);
    removeCR (statement[4]);
    printf ("  lda #<songchanneltable_%s\n", statement[2]);
    printf ("  sta songpointerlo\n");
    printf ("  lda #>songchanneltable_%s\n", statement[2]);
    printf ("  sta songpointerhi\n");
    //printf(" ldy paldetected\n");
    printf ("  ldy #0\n");
    printf ("  lda tempoinfo_%d,y\n", templabel);
    printf ("  sta songtempo\n");
    printf ("  jsr setsongchannels\n");
    printf ("  jmp skiptempo_%d\n", templabel);
    removeCR (statement[3]);
    tempovalue = (256 * strictatoi (statement[3])) / 450;
    paltempovalue = (256 * strictatoi (statement[3])) / 375;
    if (tempovalue > 255)
	prerror ("tempo too large '%s'", statement[3]);
    if (tempovalue < 0)
	prerror ("tempo change invalid format '%s'", statement[3]);
    printf ("tempoinfo_%d\n", templabel);
    printf ("  .byte %d, %d\n", tempovalue, paltempovalue);
    printf ("skiptempo_%d\n", templabel++);
    if ((statement[4][0] != 0) && (statement[4][0] != ':'))
    {
	if (strncmp (statement[4], "repeat", 6) == 0)
	{
	    printf ("  lda #255\n");
	    printf ("  sta songloops\n");
	}
	else
	{
	    //TODO: some sanity checking...
	    printf ("  lda #%d\n", strictatoi (statement[4]) - 1);
	    printf ("  sta songloops\n");
	}
    }
    else
    {
	printf ("  lda #0\n");
	printf ("  sta songloops\n");
    }
}

void playrmt (char **statement)
{
    assertminimumargs (statement, "playrmt", 1);
    removeCR (statement[2]);
    removeCR (statement[3]);
    printf ("  lda #0\n");
    printf ("  sta rasterpause\n");
    printf ("  ldx #<%s\n",statement[2]);
    printf ("  ldy #>%s\n",statement[2]);
    printf ("  jsr RASTERMUSICTRACKER+0 ; init: returns instrument speed\n");
    printf ("  sta rmt_ispeed\n");
    printf ("  lda #1\n");
    printf ("  sta rasterpause\n");
}

void stoprmt ()
{
    printf ("  lda #0\n");
    printf ("  sta rasterpause\n");
    printf ("  jsr RASTERMUSICTRACKER+9 ; silence\n");
}

void startrmt ()
{
    printf ("  lda #1\n");
    printf ("  sta rasterpause\n");
}

void incrmtfile (char **statement)
{
    // imports data from an rmt file.

    //     1        2
    // incrmtfile filename

    char datalabelname[256];
    int t;
    long size;

    assertminimumargs (statement, "incrmtfile", 1);

    fixfilename (statement[2]);

    //our label is based on the filename...
    snprintf (datalabelname, 255, "%s", ourbasename (statement[2]));
    checkvalidfilename (statement[2]);

    //but remove the extension...
    for (t = (strlen (datalabelname) - 1); t > 0; t--)
    {
        if (datalabelname[t]=='.')
        {
	    datalabelname[t] = 0;
            break;
        }
    }

    printf("%s\n",datalabelname);

    // we handle rmta different than rmt, so we need to open the file
    // first and see which it is.

    backupthisfile(statement[2]);

    char magic[6];
    FILE *fp = fopen (statement[2], "rb");
    if(fp==NULL)
        prerror ("couldn't open %s",statement[2]);
    fread(magic,1,5,fp); 
    magic[5]=0;
    if(!strcmp(magic,";RMTA"))
    {
        // this is an RMTA, so we can just do a regular include and be done
        fclose(fp);
        printf("  include \"%s\"\n",statement[2]);
        return;
    }
    rewind(fp);

    memset(magic,0,6);
    int c;
    while ((c = fgetc(fp)) != EOF) 
    {
        magic[0]=magic[1];
        magic[1]=magic[2];
        magic[2]=magic[3];
        magic[3]=c;
        if (!strncmp(magic,"RMT4",4))
            break;
    }
    if (c == EOF)
        prerror ("file doesn't appear to contain RMT data");
    printf("  .byte \"RMT4\"\n");    
    t=0;
    size=0;
    while ((c = fgetc(fp)) != EOF) 
    {
        size++;
        if (t%16>0)
            printf(",");
        if (t%16==0)
            printf("\n  .byte ");
        printf("$%02x",c);
        t++;
        if (t%16==0)
            printf("\n");
    }
    printf("\n");
    prout ("RMT %s imported, %ld bytes\n",datalabelname,size);
    fclose(fp);
}

void decompress (char **statement)
{
    //   1          2      3
    // decompress data destination

    assertminimumargs (statement, "decompress", 2);
    removeCR (statement[3]);

    if(firstcompress)
    {
	strcpy (redefined_variables[numredefvars++], "lzsa1support = 1");
        firstcompress = 0;
    }

    printf ("    lda #<%s\n", statement[2]);
    printf ("    sta LZSA_SRC_LO\n");
    printf ("    lda #>%s\n", statement[2]);
    printf ("    sta LZSA_SRC_HI\n");
    printf ("    lda #<%s\n", statement[3]);
    printf ("    sta LZSA_DST_LO\n");
    printf ("    lda #>%s\n", statement[3]);
    printf ("    sta LZSA_DST_HI\n");
    printf ("    jsr lzsa1_unpack\n");
}


void inccompress (char **statement)
{
    // creates compresses data from a data file.
    // this shares a lot of code with incrmt, because it's supposed to work
    // with rmt files too. It will advance the stream position to the RMT4
    // signature, if the signature exists.

    //     1         2
    // inccompress filename

    char datalabelname[256];
    int t;

    unsigned char *lzsa_buf_uncomp, *lzsa_buf_comp;
    long uncompsize, compsize, position;

    assertminimumargs (statement, "inccompress", 1);

    if(firstcompress)
    {
	strcpy (redefined_variables[numredefvars++], "lzsa1support = 1");
        firstcompress = 0;
    }

    fixfilename (statement[2]);

    //our label is based on the filename...
    snprintf (datalabelname, 255, "%s", ourbasename (statement[2]));
    checkvalidfilename (statement[2]);

    //but remove the extension...
    for (t = (strlen (datalabelname) - 1); t > 0; t--)
    {
        if (datalabelname[t]=='.')
        {
	    datalabelname[t] = 0;
            break;
        }
    }

    printf("%s\n",datalabelname);

    backupthisfile(statement[2]);

    char magic[6];
    FILE *fp = fopen (statement[2], "rb");
    if(fp==NULL)
        prerror ("couldn't open %s",statement[2]);

    // setup and scan for the 'RMT4' signature in the file
    memset(magic,0,6);
    int c;
    while ((c = fgetc(fp)) != EOF) 
    {
        magic[0]=magic[1];
        magic[1]=magic[2];
        magic[2]=magic[3];
        magic[3]=c;
        if (!strncmp(magic,"RMT4",4))
            break;
    }
    if (c == EOF)              // it's not an RMT4, so rewind to the start.
        rewind(fp);
    else
        fseek(fp,-4,SEEK_CUR); // rewind to the start of the RMT4 header.

    // Get the size of the file, excluding any bytes we skipped to reach
    // the RMT4 header...
    position=ftell(fp);
    fseek(fp,0,SEEK_END);
    uncompsize=ftell(fp)-position;
    fseek(fp,position,SEEK_SET); // and then restore the stream position

    lzsa_buf_uncomp = malloc(uncompsize);
    if(lzsa_buf_uncomp==NULL)
        prerror ("couldn't allocate memory to read %s",statement[2]);
    if(fread(lzsa_buf_uncomp,1,uncompsize,fp)!=uncompsize)
        prerror ("couldn't read %s into memory",statement[2]);
    fclose(fp);

    // allocate the destination buffer
    lzsa_buf_comp = malloc(uncompsize*2); 
    if(lzsa_buf_comp==NULL)
        prerror ("couldn't allocate memory to compress %s",statement[2]);

    // call the lzsa library in-memory compression routine
    // see libs.h for argument details
    compsize=lzsa_compress_inmem(lzsa_buf_uncomp,lzsa_buf_comp,uncompsize,uncompsize*2,(LZSA_FLAG_RAW_BLOCK|LZSA_FLAG_FAVOR_RATIO),3,1);

    if (compsize<1)
        prerror ("liblzsa couldn't compress %s",statement[2]);

    if (compsize>uncompsize)
        prwarn ("compressing %s wastes more rom than the uncompressed file",statement[2]);

    for(t=0;t<compsize;t++)
    {
        if (t%16>0)
            printf(",");
        if (t%16==0)
            printf("\n  .byte ");
        printf("$%02x",lzsa_buf_comp[t]);
    }

    printf("\n");
    prout ("   %s compressed, %ld->%ld bytes, %02.2f ratio\n",statement[2],uncompsize,compsize,((float)uncompsize/(float)compsize));
    free(lzsa_buf_uncomp);
    free(lzsa_buf_comp);
}


void speechdata (char **statement)
{
    char data[501], word[501], wordphonemes[501];
    int i, j, s, t, u, argvalue;
    int inflection, lastpitch, phonemecount;

    assertminimumargs (statement, "speechdata", 1);
    removeCR (statement[2]);

    lastpitch = 88;		//atarivox pitch default

    if (!(optimization & 4))
	printf ("  jmp .skip%s\n", statement[0]);

    //get the label defined...
    printf ("%s\n", statement[2]);

    while (1)
    {
	memset (data, 0, 501);
	if (((!fgets (data, 500, preprocessedfd))
	     || ((data[0] < (unsigned char) 0x3A) && (data[0] > (unsigned char) 0x2F))) && (data[0] != 'e'))
	{
	    prerror ("missing \"end\" keyword at end of data");
	    exit (1);
	}

	line++;
	if (!strncmp (data, "end\0", 3))
	    break;

	if (strncmp (data + 1, "reset", 5) == 0)
	{
	    lastpitch = 88;	//atarivox pitch default
	    printf ("  .byte 0,31 ; reset\n");
	    continue;
	}

	else if (strncmp (data + 1, "pitch", 5) == 0)	//22
	{
	    if ((data[6] == 0) || (data[7] == 0))
		prerror ("speechdata pitch data missing value");
	    argvalue = strictatoi (data + 7);
	    lastpitch = argvalue;
	    printf ("  .byte 22, %d ; pitch\n", argvalue);
	    continue;
	}
	else if (strncmp (data + 1, "speed", 5) == 0)	//21
	{
	    if ((data[6] == 0) || (data[7] == 0))
		prerror ("speechdata speed data missing value");
	    argvalue = strictatoi (data + 7);
	    printf ("  .byte 21, %d ; speed\n", argvalue);
	    continue;
	}
	else if (strncmp (data + 1, "raw", 3) == 0)
	{
	    removeCR (data);
	    if ((data[3] == 0) || (data[4] == 0))
		prerror ("speechdata raw data missing actual data");
	    printf ("  .byte %s ; raw\n", data + 4);
	    continue;
	}
	else if ((strncmp (data + 1, "dictionary", 10) == 0) || (strncmp (data + 1, "phonetic", 8) == 0))
	{
	    removeCR (data);

	    if ((data[9] == 0) || (data[10] == 0) || (data[11] == 0))
		prerror ("speechdata dictionary/phonetic data missing word arguments");

	    if (strncmp (data + 1, "dictionary", 10) == 0)
		for (i = 11;
		     ((data[i] != 0) && (data[i] != '\r') && (data[i] != '\n') && (data[i] != '\'') && (i < 500)); i++);
	    if (strncmp (data + 1, "phonetic", 8) == 0)
		for (i = 9;
		     ((data[i] != 0) && (data[i] != '\r') && (data[i] != '\n') && (data[i] != '\'') && (i < 500)); i++);
	    if (data[i] == '\'')
	    {
		i = i + 1;
		//convert to upper-case and find end of words data...
		for (j = i;
		     ((data[j] != 0) && (data[j] != '\r') && (data[j] != '\n') && (data[j] != '\'') && (j < 500)); j++)
		{
		    data[j] = toupper (data[j]);
		    if (data[j] == '^')
			data[j] = ' ';
		}
		if (data[j] != '\'')
		    prerror ("speechdata dictionary/phoenetic data missing end quote");
		data[j] = 0;
		// at this point data+i is a null-terminated ascii phrase to translate to data...
		for (t = i; data[t] != 0; t++)
		{
		    if (data[t] == ' ')
		    {
			printf ("  .byte $05 ; pause 60ms\n");
			continue;
		    }
		    if (data[t] == ',')
		    {
			printf ("  .byte $02 ; pause 200ms\n");
			continue;
		    }
		    if (data[t] == '.')
		    {
			printf ("  .byte $03 ; pause 700ms\n");
			continue;
		    }
		    if ((data[t] >= 'A') && (data[t] <= 'Z'))
		    {
			for (s = t; ((data[s] >= 'A') && (data[s] <= 'Z')); s++)
			    word[s - t] = data[s];
			word[s - t] = 0;
			inflection = 0;
			if (data[s] == '?')
			    inflection = 1;
			if (data[s] == '.')
			    inflection = 2;

			if (strncmp (data + 1, "dictionary", 10) == 0)
			{
			    for (i = 0; dictionary[i] != 0; i = i + 2)
			    {
				if (strcmp (dictionary[i], word) == 0)
				{
				    wordphonemes[200] = 0;
				    strncpy (wordphonemes, dictionary[i + 1], 200);
				    if (inflection > 0)
				    {
					//modify data based on inflection
					printf ("  .byte ");
					phonemecount = 1;
					for (u = 0; wordphonemes[u] != 0; u++)
					    if (wordphonemes[u] == ',')
						phonemecount++;
					if (phonemecount < 3)
					{
					    if (inflection == 1)
						printf
						    ("22,%d, %s, 22,%d; %s?\n",
						     lastpitch + 16, wordphonemes, lastpitch, word);
					    if (inflection == 2)
						printf
						    ("22,%d, %s, 22,%d; %s.\n",
						     lastpitch - 8, wordphonemes, lastpitch, word);
					}
					else
					{
					    char *insertpoint, *lastcomma;
					    lastcomma = strrchr (wordphonemes, ',');
					    *lastcomma = 0;
					    insertpoint = strrchr (wordphonemes, ',');
					    *lastcomma = ',';
					    *insertpoint = 0;
					    if (inflection == 1)
						printf
						    ("%s, 22,%d, %s, 22,%d; %s? (dictionary)\n",
						     wordphonemes, lastpitch + 16, insertpoint + 1, lastpitch, word);
					    if (inflection == 2)
						printf
						    ("%s, 22,%d, %s, 22,%d; %s (dictionary)\n",
						     wordphonemes, lastpitch - 8, insertpoint + 1, lastpitch, word);
					}

				    }
				    else	//plain inflection
					printf ("  .byte %s ; %s (dictionary)\n", wordphonemes, word);
				    break;
				}
			    }
			    if (dictionary[i] != 0)
			    {
				t = s - 1;	//advance to the end of our word
				continue;
			    }
			    prwarn ("the word '%s' isn't in the speech dictionary", word);
			}
			printphonemes (word, inflection, lastpitch);
			if (inflection == 0)
			    printf (" ; %s (phonetic)\n", word);
			if (inflection == 1)
			    printf (" ; %s? (phonetic)\n", word);
			if (inflection == 2)
			    printf (" ; %s. (phonetic)\n", word);
			t = s - 1;	//advance to the end of our word
		    }
		}


	    }
	    else
		prerror ("speechdata words data missing quotes");

	}

    }
    printf ("  .byte 255 ; end of avox data\n");
    printf (".skip%s\n", statement[0]);
}


int vowelcheck (char letter)
{
    switch (letter)
    {
    case 'A':
    case 'E':
    case 'I':
    case 'O':
    case 'U':
    case 'Y':
	return (TRUE);
    default:
	return (FALSE);
    }
}


void printphonemes (char *word, int inflection, int lastpitch)
{
    // a crude phonetic breakdown of the word.
    int i;

    printf ("  .byte ");
    for (i = 0; word[i] != 0; i++)
    {
	if ((i > 0) && ((strlen (word) < 4) || (word[i] != 'E') || (word[i + 1] != 0)))
	    printf (",");
	if (((strlen (word) > 3) && (i == (strlen (word) - 3))) || ((strlen (word) < 4) && (i == 0)))
	{
	    if (inflection == 1)	//question
		printf (" 22,%d, ", lastpitch + 16);

	    if (inflection == 2)	//period
		printf (" 22,%d, ", lastpitch - 8);
	}

	switch (word[i])
	{
	case 'A':
	    if ((word[i + 1] != 0) && (word[i + 2] != 0))
	    {
		if ((word[i + 2] == 'E') && (!vowelcheck (word[i + 1])))
		{
		    printf ("154");	//EYIY
		    break;
		}
	    }

	    switch (word[i + 1])
	    {
	    case 'E':
	    case 'I':
	    case 'Y':
		printf ("154");	//EYIY
		i++;		//skip the E/I/Y
		break;
	    case 'R':
		printf ("152");	//AWRR
		i++;		//skip the R
		break;
	    case 'L':
		if (word[i + 2] == 'L')
		{
		    i++;
		    printf ("136");	//AW
		}
		else
		    printf ("132");	//AY
		break;
	    case 'W':
		printf ("135");	//OH
		i++;		//skip the W
		break;
	    default:
		printf ("132");	//AY
		break;
	    }
	    break;
	case 'B':
	    switch (word[i + 1])
	    {
	    case 'O':
	    case 'U':
		printf ("171");	//BO
		break;
	    default:
		printf ("170");	//BE
		break;
	    }
	    break;
	case 'C':
	    switch (word[i + 1])
	    {
	    case 'H':
		i++;
		printf ("182");	//CH
		break;
	    case 'E':
		i++;
	    case 'I':
	    case 'Y':
		printf ("187");	//SE
		break;
	    default:
		printf ("194");	//KE
		break;
	    }
	    break;
	case 'D':
	    switch (word[i + 1])
	    {
	    case 'O':
	    case 'U':
		printf ("175");	//DO
		break;
	    case '\0':
		printf ("177");	//OD
		break;
	    default:
		printf ("174");	//DE
		break;
	    }
	    break;
	case 'E':
	    switch (word[i + 1])
	    {
	    case 'A':
	    case 'E':
		i++;
		printf ("128");	//IY
		break;
	    case 'D':
		if ((i > 0) && (word[i + 2] == 0) && (strlen (word) > 3)
		    && ((word[i - 1] == 'K') || (word[i - 1] == 'P')))
		{
		    i++;
		    printf ("191");	//TT
		}
		else
		    printf ("131");	//EH
		break;

	    case '\0':
		//ending E in a short word is pronounced
		//ending E in a long word is likely silent
		if (strlen (word) < 4)
		{
		    printf ("128");	//IY
		}
		//else silent E
		break;
	    default:
		printf ("131");	//EH
		break;
	    }
	    break;
	case 'F':
	    switch (word[i + 1])
	    {
	    case '\0':
		printf ("166");	//VV
		break;
	    case 'F':
		i++;
		printf ("186");	//FF
		break;
	    case 'O':
		printf ("186");	//FF
		if (word[i + 2] == 'O')
		{
		    printf (",8,139");	//UW
		    i = i + 2;
		}
		break;
	    default:
		printf ("186");	//FF
		break;
	    }
	    break;
	case 'G':
	    switch (word[i + 1])
	    {
	    case '\0':
		printf ("180");	//EG
		break;
	    case 'E':
		if (word[i + 2] == 0)
		{
		    printf ("165");	//JH
		    break;
		}
	    case 'A':
		printf ("178");	//GE
		break;

	    default:
		printf ("179");	//GO
		break;
	    }
	    break;
	case 'H':
	    switch (word[i + 1])
	    {
	    case 'O':
	    case 'U':
		printf ("184");	//HO
		break;
	    default:
		printf ("183");	//HE
		break;
	    }
	    break;
	case 'I':
	    switch (word[i + 1])
	    {
	    case 'E':
		i++;
		printf ("155");	//OHIY
		break;
	    case '\0':
		if (strlen (word) < 3)	//2 letters or less, long i
		    printf ("155");	//OHIY
		else
		    printf ("129");	//IH
		break;
	    default:
		if ((word[i + 1] != 0) && (word[i + 2] == 'E'))
		    printf ("155");	//OHIY
		else
		    printf ("129");	//IH
	    }
	    break;
	case 'J':
	    printf ("165");	//JH
	    break;
	case 'K':
	    printf ("194");	//KE
	    break;
	case 'L':
	    switch (word[i + 1])
	    {
	    case 'O':
	    case 'U':
		printf ("146");	//LO
		break;
	    default:
		printf ("145");	//LE
		break;
	    }
	    break;
	case 'M':
	    printf ("140");	//MM
	    break;
	case 'N':
	    printf ("141");	//NE
	    break;
	case 'O':
	    if ((i == 0) && ((word[i + 1] != 'F') && (word[i + 2] != 'F')))
	    {
		printf ("133");	//AX
		break;
	    }
	    switch (word[i + 1])
	    {
	    case 'O':
		i++;
		printf ("162");	//IHWW
		break;
	    case 'H':
		i++;
		printf ("164");	//OWWW
		break;
	    case 'W':
		if ((i > 0) && ((word[i - 1] == 'C') || (word[i - 1] == 'H') || (word[i - 1] == 'N')))
		    printf ("163");	//AYWW
		else
		    printf ("164");	//OWWW
		break;
	    case 'U':
		if (strlen (word) < 4)
		{
		    i++;
		    printf ("139");	//UW
		}
		else
		{
		    i++;
		    printf ("134");	//UX
		}
		break;
	    case 'Y':
		i++;
		printf ("156");	//OWIY
		break;
	    case 'R':
		printf ("137");	//OW
		break;
	    case 'X':
		printf ("135");	//OH
		break;
	    case '\0':
		if ((i == 1) || (i == 2))
		{
		    printf ("164");	//OWWW
		    break;
		}
		if ((i > 0) && ((word[i - 1] == 'T') || (word[i - 1] == 'D')))
		{
		    printf ("162");	//IHWW
		    break;
		}
		printf ("164");	//OWWW
		break;
	    case 'V':
	    case 'F':
		if (strlen (word) == 2)
		{
		    printf ("134");	//UX
		    break;
		}
		printf ("137");	//OW
		break;
	    default:
		printf ("133");	//AX
		break;
	    }
	    break;
	case 'P':
	    switch (word[i + 1])
	    {
	    case 'H':
		printf ("186");	//FF
		break;
	    case 'E':
	    case 'I':
		printf ("198");	//PE
		break;
	    default:
		if (word[i + 1] == 'P')
		    i++;
		printf ("199");	//PO
		break;
	    }
	    break;
	case 'Q':
	    printf ("194,7,147");	//QU
	    if (word[i + 1] == 'U')
		i++;
	    break;
	case 'R':
	    printf ("148");	//RR
	    break;
	case 'S':
	    if (word[i + 1] == 'H')
	    {
		i++;
		printf ("8,189");	//SH
		break;
	    }
	    if (i < 3)
	    {
		printf ("187");	//SE
		break;
	    }
	    if ((vowelcheck (word[i - 1])) && (vowelcheck (word[i - 2])))
	    {
		//S is usually Z after a long vowel
		printf ("167");	//ZZ
		break;
	    }
	    printf ("187");	//SE
	    break;
	case 'T':
	    switch (word[i + 1])
	    {
	    case 'H':
		i++;
		printf ("169");	//DH
		break;
	    default:
		printf ("191");	//TT
		break;
	    }
	    break;
	case 'U':
	    if ((word[i + 1] != 0) && (word[i + 2] != 0))
	    {
		if ((word[i + 2] == 'E') && (!vowelcheck (word[i + 1])))
		{
		    printf ("160");	//IYUW
		    break;
		}
	    }
	    switch (word[i + 1])
	    {
	    case 'H':
		printf ("138");	//UH
		break;
	    default:
		printf ("134");	//UX
		break;
	    }
	    break;
	case 'V':
	    printf ("166");	//VV
	    break;
	case 'W':
	    switch (word[i + 1])
	    {
	    case 'H':
		i++;
		printf ("147");	//WW
		break;
	    case 'R':
		i++;
		printf ("148");	//RR
		break;
	    default:
		printf ("147");	//WW
		break;
	    }
	    break;
	case 'X':
	    printf ("194,187");	//EX
	    break;
	case 'Y':
	    switch (word[i + 1])
	    {
	    case '\0':
		if (strlen (word) < 4)
		    printf ("157");	//OHIH
		else
		    printf ("158");	//IY
		continue;
	    }
	    printf ("158");	//IY
	    break;
	case 'Z':
	    printf ("167");	//ZZ
	    break;
	}
    }
    if (inflection > 0)		//restore inflection
	printf (", 22,%d", lastpitch);
}

void alphachars (char **statement)
{
    //   1            2
    //alphachars 'characters'
    //N.B. if 'characters' has a space, it will be split across 2 and 3.

    //alphachars ASCII

    //this command is really a compiler directive, but couldn't be implemented
    //as "set" without weirding up the preprocessing for other "set" commands.

    char *charset = charactersetchars;
    int i;

    assertminimumargs (statement, "alphachars", 1);

    removeCR (statement[2]);
    removeCR (statement[3]);

    if ((statement[2][0] == '\'') && (statement[2][1] == '\''))
	prerror ("alphachars syntax error... empty set");
    else if (strncmp (statement[2], "ASCII", 5) == 0)
    {
	for (i = 0; i < 256; i++)
	    charactersetchars[i] = i;
	charactersetchars[0] = 1;
	charactersetchars[256] = 0;
    }
    else if (strncmp (statement[2], "default", 7) == 0)
    {
	memset (charactersetchars, 0, 257);
	strcpy (charactersetchars, " abcdefghijklmnopqrstuvwxyz.!?,\"$():");
    }
    else if (strncmp (statement[2], "HISCORE", 7) == 0)
    {
	memset (charactersetchars, 0, 257);
	strcpy (charactersetchars, "abcdefghijklmnopqrstuvwxyz:.- \"<#0123456789,C?");
    }
    else if (statement[2][0] == '\'')
    {
	i = 1;
	while (statement[2][i] != '\0' && statement[2][i] != '\'')
	{
	    *charset = statement[2][i];
	    charset = charset + 1;
	    i = i + 1;
	}
	if (statement[2][i] == '\0')	// assume there was a space in the list
	{
	    *charset = ' ';
	    charset = charset + 1;
	    i = 0;
	    while (statement[3][i] != '\0' && statement[3][i] != '\'')
	    {
		*charset = statement[3][i];
		charset = charset + 1;
		i = i + 1;
	    }
	}
	*charset = '\0';
    }
    else
    {
	prerror ("alphachars syntax error");
    }
}

void alphadata (char **statement)
{
    //      1          2             3            4
    //   alphadata uniquelabel graphicslabel [extrawide]
    //  'mytext strings here'
    //  'as many lines as I like'
    // end

    //default characters are ' abcdefghijklmnopqrstuvwxyz.!?,"$():'
    //this will be changeable via "alphachars 'characters'

    assertminimumargs (statement, "alphadata", 2);

    removeCR (statement[3]);
    removeCR (statement[4]);

    char data[SIZEOFSTATEMENT];
    char **data_length;
    char **deallocdata_length;
    char *alphachr, *alphaend;
    int charoffset;
    int i, j;
    int thisdatabankset;

    thisdatabankset = 0;

    removeCR (statement[3]);
    removeCR (statement[4]);

    if ((banksetrom) && (strncmp (statement[2], "bset_", 5) == 0))
	thisdatabankset = 1;

    if (!thisdatabankset)
	if (!(optimization & 4))
	    printf ("  jmp .skip%s\n", statement[0]);
    // if optimization level >=4 then data cannot be placed inline with code!

    if (!thisdatabankset)
	printf ("%s\n", statement[2]);
    else
	gfxprintf ("%s\n", statement[2]);

    while (1)
    {
	if (((!fgets (data, SIZEOFSTATEMENT, preprocessedfd))
	     || ((data[0] < (unsigned char) 0x3A) && (data[0] > (unsigned char) 0x2F))) && (data[0] != 'e'))
	{
	    prerror ("missing \"end\" keyword at end of alphadata");
	    exit (1);
	}
	line++;
	if (!strncmp (data, "end\0", 3))
	    break;

	alphachr = strchr (data, '\'');
	alphaend = strrchr (data, '\'');
	if ((alphaend == NULL) || (alphachr == alphaend) || (alphachr == (alphaend - 1)))
	{
	    int t, hasnum;
	    hasnum=0;
	    for(t=0;((t<SIZEOFSTATEMENT)&&(data[t]!=0));t++)
	    {
	        if ((data[t]>='0')&&(data[t]<='9'))
		{
	            hasnum=1; 
	            break;
		}
	    }
	    if (!hasnum)
            {
	        prerror ("malformed alphadata line");
	        exit (1);
            }
	    // We see a digit in there. We'll take it on faith that there's
	    // valid data listed. Validity checking here could be better.
	    if (!thisdatabankset)
		printf ("  .byte %s\n", data);
            else
		gfxprintf ("  .byte %s\n", data);
	    continue;
	}
	alphachr = alphachr + 1;
	*alphaend = '\0';
	for (; *alphachr != '\0'; alphachr++)
	{
	    charoffset = lookupcharacter (*alphachr);
	    if (charoffset < 0)
	    {
		prerror ("alphadata character '%c' is missing from alphachars", *alphachr);
	    }
	    if ((doublewide == 1) && (strncmp (statement[4], "singlewide", 10) != 0))
		charoffset = charoffset * 2;
	    if (strncmp (statement[4], "extrawide", 9) == 0)
		charoffset = charoffset * 2;

	    if (!thisdatabankset)
	    {
		printf ("  .byte (<%s + $%02x)", statement[3], charoffset);
		if (strncmp (statement[4], "extrawide", 9) == 0)
		    printf (", (<%s + $%02x)", statement[3], charoffset + 1);
		if (alphachr[1] != '\0')
		    printf ("\n");
	    }
	    else
	    {
		gfxprintf ("  .byte (<%s + $%02x)", statement[3], charoffset);
		if (strncmp (statement[4], "extrawide", 9) == 0)
		    gfxprintf (", (<%s + $%02x)", statement[3], charoffset + 1);
		if (alphachr[1] != '\0')
		    gfxprintf ("\n");
	    }
	}
	if (!thisdatabankset)
	    printf ("\n");
	else
	    gfxprintf ("\n");
    }
    if (!thisdatabankset)
    {
	printf (".skip%s\n", statement[0]);
	printf ("%s_length = [. - %s]\n", statement[2], statement[2]);
    }
    else
    {
	gfxprintf ("%s_length = [. - %s]\n", statement[2], statement[2]);
    }
    sprintf (constants[numconstants++], "%s_length", statement[2]);

}

int lookupcharacter (char mychar)
{
    int characterindex = 0;
    char *haystack = charactersetchars;
    for (;;)
    {
	if (*haystack == mychar)
	    return characterindex;
	haystack = haystack + 1;
	characterindex = characterindex + 1;
	if (*haystack == 0)
	    return (-1);
	if (characterindex > 255)
	{
	    //something very wrong has happened to our charactersetchars string!!!
	    prerror ("search through valid character string didn't end");
	    exit (1);
	}
    }
}

void shiftdata (char **statement, int num)
{
    int i, j;
    for (i = STATEMENTCOUNT-1; i > num; i--)
	for (j = 0; j < SIZEOFSTATEMENT; ++j)
	    statement[i][j] = statement[i - 1][j];
}

void compressdata (char **statement, int num1, int num2)
{
    int i, j;
    for (i = num1; i < STATEMENTCOUNT - num2; i++)
	for (j = 0; j < SIZEOFSTATEMENT; ++j)
	    statement[i][j] = statement[i + num2][j];
}

void ongoto (char **statement)
{
    int k, i = 4;


    if (!strncmp (statement[3], "gosub\0", 5))
    {
	assertminimumargs (statement, "on...gosub", 3);
	printf ("  lda #>(ongosub%d-1)\n", ongosub);
	printf ("  pha\n");
	printf ("  lda #<(ongosub%d-1)\n", ongosub);
	printf ("  pha\n");
    }
    else
	assertminimumargs (statement, "on...goto", 3);
    if (strcmp (statement[2], Areg))
	printf ("  ldx %s\n", statement[2]);
    printf ("  lda .%sjumptablehi,x\n", statement[0]);
    printf ("  pha\n");
    printf ("  lda .%sjumptablelo,x\n", statement[0]);
    printf ("  pha\n");
    printf ("  rts\n");
    printf (".%sjumptablehi\n", statement[0]);
    while ((statement[i][0] != ':') && (statement[i][0] != '\0'))
    {
	for (k = 0; k < SIZEOFSTATEMENT; ++k)
	    if ((statement[i][k] == (unsigned char) 0x0A) || (statement[i][k] == (unsigned char) 0x0D))
		statement[i][k] = '\0';
	printf ("  .byte >(.%s-1)\n", statement[i++]);
    }
    printf (".%sjumptablelo\n", statement[0]);
    i = 4;
    while ((statement[i][0] != ':') && (statement[i][0] != '\0'))
    {
	for (k = 0; k < SIZEOFSTATEMENT; ++k)
	    if ((statement[i][k] == (unsigned char) 0x0A) || (statement[i][k] == (unsigned char) 0x0D))
		statement[i][k] = '\0';
	printf ("  .byte <(.%s-1)\n", statement[i++]);
    }
    if (!strncmp (statement[3], "gosub\0", 5))
	printf ("ongosub%d\n", ongosub++);
}

void dofor (char **statement)
{
    assertminimumargs (statement, "for", 5);
    if (strcmp (statement[4], Areg))
    {
	printf ("  lda ");
	printimmed (statement[4]);
	printf ("%s\n", statement[4]);
    }

    printf ("  sta %s\n", statement[2]);

    forlabel[numfors][0] = '\0';
    sprintf (forlabel[numfors], "%sfor%s", statement[0], statement[2]);
    printf (".%s\n", forlabel[numfors]);

    removeCR (statement[6]);
    forend[numfors][0] = '\0';
    strcpy (forend[numfors], statement[6]);

    forvar[numfors][0] = '\0';
    strcpy (forvar[numfors], statement[2]);

    forstep[numfors][0] = '\0';

    if (!strncasecmp (statement[7], "step\0", 4))
    {
	removeCR (statement[8]);
	strcpy (forstep[numfors], statement[8]);
    }
    else
	strcpy (forstep[numfors], "1");

    numfors++;
}

void next (char **statement)
{
    int immed = 0;
    int immedend = 0;
    int failsafe = 0;
    char failsafelabel[60];

    invalidate_Areg ();

    if (!numfors)
    {
	prerror ("next without for");
    }
    numfors--;
    if (!strncmp (forstep[numfors], "1\0", 2))	// step 1
    {
	printf ("  lda %s\n", forvar[numfors]);
	printf ("  cmp ");
	printimmed (forend[numfors]);
	printf ("%s\n", forend[numfors]);
	printf ("  inc %s\n", forvar[numfors]);
	bcc (forlabel[numfors]);
    }
    else if ((!strncmp (forstep[numfors], "-1\0", 3)) || (!strncmp (forstep[numfors], "255\0", 4)))
    {				// step -1
	printf ("  dec %s\n", forvar[numfors]);
	if (strncmp (forend[numfors], "1\0", 2))
	{
	    printf ("  lda %s\n", forvar[numfors]);
	    printf ("  cmp ");
	    if (!strncmp (forend[numfors], "0\0", 2))
	    {
		// the special case of 0 as end, since we can't check to see if it was smaller than 0
		printf ("#255\n");
		bne (forlabel[numfors]);
	    }
	    else		// general case
	    {
		printimmed (forend[numfors]);
		printf ("%s\n", forend[numfors]);
		bcs (forlabel[numfors]);
	    }
	}
	else
	    bne (forlabel[numfors]);
    }
    else			// step other than 1 or -1
    {
	// normally, the generated code adds to or subtracts from the for variable, and checks
	// to see if it's less than the ending value.
	// however, if the step would make the variable less than zero or more than 255
	// then this check will not work.  The compiler will attempt to detect this situation
	// if the step and end are known.  If the step and end are not known (that is,
	// either is a variable) then much more complex code must be generated.

	printf ("  lda %s\n", forvar[numfors]);
	printf ("  clc\n");
	printf ("  adc ");
	immed = printimmed (forstep[numfors]);
	printf ("%s\n", forstep[numfors]);

	if (immed && isimmed (forend[numfors]))	// the step and end are immediate
	{
	    if (strictatoi (forstep[numfors]) & 128)	// step is negative
	    {
		if ((256 - (strictatoi (forstep[numfors]) & 255)) >= strictatoi (forend[numfors]))
		{		// if we are in danger of going < 0...we will have carry clear after ADC
		    failsafe = 1;
		    sprintf (failsafelabel, "%s_failsafe", forlabel[numfors]);
		    bcc (failsafelabel);
		}
	    }
	    else
	    {			// step is positive
		if ((strictatoi (forstep[numfors]) + strictatoi (forend[numfors])) > 255)
		{		// if we are in danger of going > 255...we will have carry set after ADC
		    failsafe = 1;
		    sprintf (failsafelabel, "%s_failsafe", forlabel[numfors]);
		    bcs (failsafelabel);
		}
	    }

	}
	printf ("  sta %s\n", forvar[numfors]);
	printf ("  cmp ");
	immedend = printimmed (forend[numfors]);
	// add 1 to immediate compare for increasing loops
	if (immedend && !(strictatoi (forstep[numfors]) & 128))
	    strcat (forend[numfors], "+1");
	printf ("%s\n", forend[numfors]);
	// if step number is 1 to 127 then add 1 and use bcc, otherwise bcs
	// if step is a variable, we'll need to check for every loop iteration
	//
	// Warning! no failsafe checks with variables as step or end - it's the
	// programmer's job to make sure the end value doesn't overflow
	if (!immed)
	{
	    printf ("  ldx %s\n", forstep[numfors]);
	    printf ("  bmi .%sbcs\n", statement[0]);
	    bcc (forlabel[numfors]);
	    printf ("  clc\n");
	    printf (".%sbcs\n", statement[0]);
	    bcs (forlabel[numfors]);
	}
	else if (strictatoi (forstep[numfors]) & 128)
	    bcs (forlabel[numfors]);
	else
	{
	    bcc (forlabel[numfors]);
	    if (!immedend)
		beq (forlabel[numfors]);
	}
    }
    if (failsafe)
	printf (".%s\n", failsafelabel);
}

void autodim (char **statement)
{
    #define AD_BYTE 0
    #define AD_44  1
    #define AD_88  2

    static int inititialized = 0;
    static char start_addr[80];
    static char end_addr[80];
    static int current_index;

    int variable_type;
    int memsize,t;
    int objsize, objcount;

    // when arg2=init...
    //         1           2         3         4
    //     autodim       init   start addr  end addr

    // when arg2=byte, 8.8, or 4.4...
    //         1           2         3         4
    //     autodim       type      name      count

    assertminimumargs (statement, "autodim", 2);
    removeCR (statement[3]);
    removeCR (statement[4]);

    if (strncmp(statement[2],"init",5)==0)
    {
        assertminimumargs (statement, "autodim (init)", 3);
        inititialized = 1;
        current_index=0;
	strncpy(start_addr,statement[3],79);
	strncpy(end_addr,statement[4],79);
        return;
    }

    if(!inititialized)
        prerror ("autodim used without initializing.");

    variable_type = -1;
    if (strncmp(statement[2],"byte",5)==0)
    {
        variable_type = AD_BYTE;
        objsize = 1;
    }
    else if (strncmp(statement[2],"4.4",5)==0)
    {
        variable_type = AD_44;
        objsize = 1;
    }
    else if (strncmp(statement[2],"8.8",5)==0)
    {
        variable_type = AD_88;
        objsize = 2;
    }

    if(variable_type == -1)
        prerror ("autodim type not recognized.");

    if ((statement[4][0] == 0) || (statement[4][0] == ':'))
        objcount=1;
    else
    {
        // retrieve and validate how many bytes/objects are needed
        objcount = strictatoi (statement[4]);
        if (objcount<1)
            prerror ("autodim invalid object count used.");
    }

    // register the base variable name
    snprintf (redefined_variables[numredefvars], 100, "%s = (%s + %d)",statement[3],start_addr,current_index);
    numredefvars++;

    if ( variable_type == AD_44 )
    {
        snprintf (redefined_variables[numredefvars], 100, "%sb44 = (%s + %d)",statement[3],start_addr,current_index);
        numredefvars++;
        snprintf (fixpoint44[0][numfixpoint44], 46, "%s",statement[3]);
        snprintf (fixpoint44[1][numfixpoint44], 46, "%sb44",statement[3]);
        numfixpoint44++;
    }

    if ( variable_type == AD_88 )
    {
        snprintf (redefined_variables[numredefvars], 100, "%s_hi = (%s + %d)",statement[3],start_addr,current_index);
        numredefvars++;
        snprintf (redefined_variables[numredefvars], 100, "%s_lo = (%s + %d)",statement[3],start_addr,current_index+objcount);
        numredefvars++;
        snprintf (fixpoint88[0][numfixpoint88], 46, "%s",statement[3]);
        snprintf (fixpoint88[1][numfixpoint88], 46, "%s_lo",statement[3]);
        numfixpoint88++;

    }

    // advance the autodim index past this recent allocation...
    current_index = current_index + (objcount * objsize);

    // Add an assembly check that the allocation didn't go past the end value
    // This needs to be at the asm level, because we allow the program to 
    // use symbols for the start and end address.
    printf(" if ((%s + %d) > %s)\n echo \"\"\n echo \"######## ERROR: autodim of variable '%s' exceeded range end. (%s)\"\n  ERR\n endif\n",start_addr,current_index-1, end_addr, statement[3],end_addr);
}

void dim (char **statement)
{
    // just take the statement and pass it right to a header file
    int i;
    char *fixpointvar1;
    char fixpointvar2[50];
    // check for fixedpoint variables
    i = findpoint (statement[4]);
    if (i < 500)
    {
	removeCR (statement[4]);
	strcpy (fixpointvar2, statement[4]);
	fixpointvar1 = fixpointvar2 + i + 1;
	fixpointvar2[i] = '\0';
	if (!strcmp (fixpointvar1, fixpointvar2))	// we have 4.4
	{
	    strcpy (fixpoint44[1][numfixpoint44], fixpointvar1);
	    strcpy (fixpoint44[0][numfixpoint44++], statement[2]);
	}
	else			// we have 8.8
	{
	    strcpy (fixpoint88[1][numfixpoint88], fixpointvar1);
	    strcpy (fixpoint88[0][numfixpoint88++], statement[2]);
	}
	statement[4][i] = '\0';	// terminate string at '.'
    }
    i = 2;
    redefined_variables[numredefvars][0] = '\0';
    while ((statement[i][0] != '\0') && (statement[i][0] != ':'))
    {
	strcat (redefined_variables[numredefvars], statement[i++]);
	strcat (redefined_variables[numredefvars], " ");
    }
    numredefvars++;
}

void doconst (char **statement)
{
    // basically the same as dim, except we keep a queue of variable names that are constant
    int i = 2;
    redefined_variables[numredefvars][0] = '\0';
    while ((statement[i][0] != '\0') && (statement[i][0] != ':'))
    {
	strcat (redefined_variables[numredefvars], statement[i++]);
	strcat (redefined_variables[numredefvars], " ");
    }
    numredefvars++;
    strcpy (constants[numconstants++], statement[2]);	// record to queue
}


void doreturn (char **statement)
{

    int index = 0;
    int indirectflag = 0;
    char getindex0[SIZEOFSTATEMENT];
    int bankedreturn = 0;
    // 0=no special action
    // 1=return thisbank
    // 2=return otherbank

    if (!strncmp (statement[2], "thisbank\0", 8) || !strncmp (statement[3], "thisbank\0", 8))
	bankedreturn = 1;
    else if (!strncmp (statement[2], "otherbank\0", 9) || !strncmp (statement[3], "otherbank\0", 9))
	bankedreturn = 2;

    // several types of returns:
    // return by itself (or with a value) can return to any calling bank
    // this one has the most overhead in terms of cycles and ROM space
    // use sparingly if cycles or space are an issue

    // return [value] thisbank will only return within the same bank
    // this one is the fastest

    // return [value] otherbank will slow down returns to the same bank
    // but speed up returns to other banks - use if you are primarily returning
    // across banks

    if (statement[2][0] && (statement[2][0] != ' ')
	&& (statement[2][0] != ':')
	&& (strncmp (statement[2], "thisbank\0", 8)) && (strncmp (statement[2], "otherbank\0", 9)))
    {
	index |= getindex (statement[2], &getindex0[0], &indirectflag);
	if (indirectflag != 0)
	    prerror ("indirect arrays not supported as return arguments");

	if (index & 1)
	    loadindex (&getindex0[0], indirectflag);

	if (!bankedreturn)
	    printf ("  ldy ");
	else
	    printf ("  lda ");
	printindex (statement[2], index & 1, indirectflag);
    }

    if (bankedreturn == 1)
    {
	printf ("  rts\n");
	return;
    }
    if (bankedreturn == 2)
    {
	printf ("  jmp BS_return\n");
	return;
    }

    if (bankcount > 0)		// check if we need a regular or bank switched return...
    {
	printf ("  tsx\n");
	// peek at the high address on the stack...
	printf ("  lda $102,x\n");

	// if it's 0 it must be a bank location rather than an actual address
	printf ("  beq bankswitchret%d\n", ++templabel);	//if it's 0, it's not an address
	if (statement[2][0] && (statement[2][0] != ' '))
	    printf ("  tya\n");
	printf ("  rts\n");

	// if it's non-0 it must be a plain address
	printf ("bankswitchret%d\n", templabel);
	if (statement[2][0] && (statement[2][0] != ' '))
	    printf ("  tya\n");
	printf ("  jmp BS_return\n");
	return;
    }

    if (statement[2][0] && (statement[2][0] != ' ') && (statement[2][0] != ':'))
	printf ("  tya\n");
    printf ("  rts\n");
}

void doasm ()
{
    char data[SIZEOFSTATEMENT];
    while (1)
    {
	if (((!fgets (data, SIZEOFSTATEMENT, preprocessedfd))
	     || ((data[0] < (unsigned char) 0x3A) && (data[0] > (unsigned char) 0x2F))) && (data[0] != 'e'))
	{
	    prerror ("missing \"end\" keyword at end of inline asm");
	    exit (1);
	}
	line++;
	if (!strncmp (data, "end\0", 3))
	    break;
	printf ("%s\n", data);

    }
}

void domacro (char **statement)
{
    int k, j = 1, i = 3;
    macroactive = 1;
    printf (" MAC %s\n", statement[2]);

    while ((statement[i][0] != ':') && (statement[i][0] != '\0'))
    {
	for (k = 0; k < SIZEOFSTATEMENT; ++k)
	    if ((statement[i][k] == (unsigned char) 0x0A) || (statement[i][k] == (unsigned char) 0x0D))
		statement[i][k] = '\0';
	if (!strncmp (statement[i], "const\0", 5))
	    strcpy (constants[numconstants++], statement[i + 1]);	// record to const queue
	else
	    printf ("%s SET {%d}\n", statement[i], j++);
	i++;
    }
}

void callmacro (char **statement)
{
    int k, i = 3;
    macroactive = 1;
    printf (" %s", statement[2]);

    while ((statement[i][0] != ':') && (statement[i][0] != '\0'))
    {
	for (k = 0; k < SIZEOFSTATEMENT; ++k)
	    if ((statement[i][k] == (unsigned char) 0x0A) || (statement[i][k] == (unsigned char) 0x0D))
		statement[i][k] = '\0';
	if (isimmed (statement[i]))
	    printf (" #%s,", statement[i]);	// we're assuming the assembler doesn't mind extra commas!
	else
	    printf (" %s,", statement[i]);	// we're assuming the assembler doesn't mind extra commas!
	i++;
    }
    printf ("\n");
}

void doextra (char *extrano)
{
    extraactive = 1;
    printf ("extra set %d\n", ++extra);
    printf (" MAC extra%c", extrano[5]);
    if (extrano[6] != ':')
	printf ("%c", extrano[6]);
    printf ("\n");
}

void doend ()
{
    if (extraactive)
    {
	printf (" ENDM\n");
	extraactive = 0;
    }
    else if (macroactive)
    {
	printf (" ENDM\n");
	macroactive = 0;
    }
    else
	prerror ("extraneous end statement found");
}

void dosizeof (char **statement)
{
    //         1           2
    //     sizeof    somelabel
    removeCR (statement[2]);
    if ((statement[2] == 0) || (statement[2][0] == 0))
	prerror ("missing argument in sizeof statement");
    printf ("  echo \" \",\"SIZEOF(%s):\",[* - %s]d,[* - .%s]d,\"bytes\"\n", statement[2], statement[2], statement[2]);
}

void ifconst (char **statement)
{
    //         1           2
    //     ifconst   somelabel
    removeCR (statement[2]);
    if ((statement[2] == 0) || (statement[2][0] == 0))
	prerror ("missing argument in ifconst statement");
    printf (" ifconst %s\n", statement[2]);
}

void doelse (void)
{
    printf (" else\n");
}

void endif (void)
{
    printf (" endif\n");
}

void incbin (char **statement)
{
    //         1           2
    //     incbin    somelabel
    removeCR (statement[2]);
    if ((statement[2] == 0) || (statement[2][0] == 0))
	prerror ("missing argument in incbin statement");
    printf ("  incbin \"%s\"\n", statement[2]);
}


void doif (char **statement)
{
    int index = 0;
    int situation = 0;
    char getindex0[SIZEOFSTATEMENT];
    char getindex1[SIZEOFSTATEMENT];
    int indirectflag0, indirectflag1;
    int not = 0;
    int complex_boolean = 0;
    int i, j, k, h;
    int push1 = 0;
    int push2 = 0;
    int bit = 0;
    int Aregmatch = 0;
    char Aregcopy[SIZEOFSTATEMENT];
    char **cstatement;		//conditional statement
    char **dealloccstatement;	//for deallocation

    strcpy (Aregcopy, "index-invalid");

    cstatement = (char **) malloc (sizeof (char *) * STATEMENTCOUNT);
    for (k = 0; k < STATEMENTCOUNT; ++k)
	cstatement[k] = (char *) malloc (sizeof (char) * SIZEOFSTATEMENT);
    dealloccstatement = cstatement;
    for (k = 0; k < SIZEOFSTATEMENT; ++k)
	for (j = 0; j < STATEMENTCOUNT; ++j)
	    cstatement[j][k] = '\0';
    if ((statement[2][0] == '!') && (statement[2][1] != '\0'))
    {
	not = 1;
	for (i = 0; i < (SIZEOFSTATEMENT-1); ++i)
	{
	    statement[2][i] = statement[2][i + 1];
	}
    }
    else if (!strncmp (statement[2], "!\0", 2))
    {
	not = 1;
	compressdata (statement, 2, 1);
    }

    if (statement[2][0] == '(')
    {
	j = 0;
	k = 0;
	for (i = 2; i < (SIZEOFSTATEMENT-1); ++i)
	{
	    if (statement[i][0] == '(')
		j++;
	    if (statement[i][0] == ')')
		j--;
	    if (statement[i][0] == '<')
		break;
	    if (statement[i][0] == '>')
		break;
	    if (statement[i][0] == '=')
		break;
	    if (statement[i][0] == '&' && statement[i][1] == '\0')
		k = j;
	    if (!strncmp (statement[i], "then\0", 4))
	    {
		complex_boolean = 1;
		break;
	    }			//prerror("complex boolean not yet supported");exit(1);}
	}
	if (i == (STATEMENTCOUNT-1) && k)
	    j = k;
	if (j)
	{
	    compressdata (statement, 2, 1);	//remove first parenthesis
	    for (i = 2; i < (STATEMENTCOUNT-1); ++i)
		if ((!strncmp (statement[i], "then\0", 4)) ||
		    (!strncmp (statement[i], "&&\0", 2)) || (!strncmp (statement[i], "||\0", 2)))
		    break;
	    if (i != (STATEMENTCOUNT-1))
	    {
		if (statement[i - 1][0] != ')')
		{
		    prerror ("unbalanced parentheses in if-then");
		    exit (1);
		}
		compressdata (statement, i - 1, 1);
	    }
	}
    }

    if ((!strncmp (statement[2], "joy0\0", 4))
	|| (!strncmp (statement[2], "joy1\0", 4))
	|| (!strncmp (statement[2], "keypad0key\0", 10))
	|| (!strncmp (statement[2], "keypad1key\0", 10))
	|| (!strncmp (statement[2], "switch\0", 6))
	|| (!strncmp (statement[2], "snes0\0", 5))
	|| (!strncmp (statement[2], "snes1\0", 5))
	|| (!strncmp (statement[2], "snes#\0", 5))
	|| (!strncmp (statement[2], "mega0\0", 5))
	|| (!strncmp (statement[2], "mega1\0", 5))
	|| (!strncmp (statement[2], "softswitches\0", 12))
	|| (!strncmp (statement[2], "softselect\0", 10)) || (!strncmp (statement[2], "softreset\0", 9)))
    {
	i = switchjoy (statement[2]);
	if (!islabel (statement))
	{
	    if (!i)
	    {
		if (not)
		    bne (statement[4]);
		else
		    beq (statement[4]);
	    }
	    else if (i == 1)	// bvc/bvs
	    {
		if (not)
		    bvs (statement[4]);
		else
		    bvc (statement[4]);
	    }
	    else if (i == 2)	// bpl/bmi
	    {
		if (not)
		    bmi (statement[4]);
		else
		    bpl (statement[4]);
	    }
	    else if (i == 3)	// bmi/bpl
	    {
		if (not)
		    bpl (statement[4]);
		else
		    bmi (statement[4]);
	    }
	    else if (i == 4)	// bne/beq
	    {
		if (not)
		    beq (statement[4]);
		else
		    bne (statement[4]);
	    }
	    else if (i == 5)	// bvs/bvc
	    {
		if (not)
		    bvc (statement[4]);
		else
		    bvs (statement[4]);
	    }
	    else if (i == 6)	// beq/bne
	    {
		if (not)
		    bne (statement[4]);
		else
		    beq (statement[4]);
	    }


	    freemem (dealloccstatement);
	    return;
	}
	else			// then statement
	{
	    if (!i)
	    {
		if (not)
		    printf ("  beq ");
		else
		    printf ("  bne ");
	    }
	    if (i == 1)
	    {
		if (not)
		    printf ("  bvc ");
		else
		    printf ("  bvs ");
	    }
	    if (i == 2)
	    {
		if (not)
		    printf ("  bpl ");
		else
		    printf ("  bmi ");
	    }
	    else if (i == 3)	// bmi/bpl
	    {
		if (not)
		    printf ("  bmi ");
		else
		    printf ("  bpl ");
	    }
	    else if (i == 4)	// bne/beq
	    {
		if (not)
		    printf ("  bne ");
		else
		    printf ("  beq ");
	    }
	    else if (i == 5)
	    {
		if (not)
		    printf ("  bvs ");
		else
		    printf ("  bvc ");
	    }
	    else if (i == 6)	// beq/bne
	    {
		if (not)
		    printf ("  beq ");
		else
		    printf ("  bne ");
	    }

	    printf (".skip%s\n", statement[0]);
	    // separate statement
	    for (i = 3; i < STATEMENTCOUNT; ++i)
	    {
		for (k = 0; k < SIZEOFSTATEMENT; ++k)
		{
		    cstatement[i - 3][k] = statement[i][k];
		}
	    }
	    printf (".condpart%d\n", condpart++);
	    keywords (cstatement);
	    printf (".skip%s\n", statement[0]);
	    freemem (dealloccstatement);
	    return;
	}


	if (!islabel (statement))
	{
	    if (!not)
	    {
		if (bit == 7)
		    bmi (statement[4]);
		else
		    bvs (statement[4]);
	    }
	    else
	    {
		if (bit == 7)
		    bpl (statement[4]);
		else
		    bvc (statement[4]);
	    }
	    freemem (dealloccstatement);
	    return;
	}
	else			// then statement
	{
	    if (not)
	    {
		if (bit == 7)
		    printf ("  bmi ");
		else
		    printf ("  bvs ");
	    }
	    else
	    {
		if (bit == 7)
		    printf ("  bpl ");
		else
		    printf ("  bvc ");
	    }

	    printf (".skip%s\n", statement[0]);
	    // separate statement
	    for (i = 3; i < STATEMENTCOUNT; ++i)
	    {
		for (k = 0; k < SIZEOFSTATEMENT; ++k)
		{
		    cstatement[i - 3][k] = statement[i][k];
		}
	    }
	    printf (".condpart%d\n", condpart++);
	    keywords (cstatement);
	    printf (".skip%s\n", statement[0]);

	    freemem (dealloccstatement);
	    return;
	}
    }

    if (!strncmp (statement[2], "boxcollision\0", 12))
    {

	//        2     3 4  5 6  7 8  9 10 11 12 13 14 15 16 17 18 19 20
	// boxcollision ( X1 , Y1 , W1 , H1 ,  X2  , Y2 ,  W2 ,  H2 )

	boxcollision (statement);
	if (!islabel (statement))
	{
	    if (not)
		bcs (statement[21]);
	    else
		bcc (statement[21]);
	    freemem (dealloccstatement);
	    return;
	}
	else			// then statement
	{
	    if (not)
		printf ("  bcs ");
	    else
		printf ("  bcc ");
	    printf (".skip%s\n", statement[0]);
	    // separate statement
	    for (i = 20; i < STATEMENTCOUNT; ++i)
	    {
		for (k = 0; k < SIZEOFSTATEMENT; ++k)
		{
		    cstatement[i - 20][k] = statement[i][k];
		}
	    }
	    printf (".condpart%d\n", condpart++);
	    keywords (cstatement);
	    printf (".skip%s\n", statement[0]);
	    freemem (dealloccstatement);
	    return;
	}
    }

    if (!strncmp (statement[2], "CARRY\0", 5))
    {

	if (!islabel (statement))
	{
	    if (not)
		bcs (statement[4]);
	    else
		bcc (statement[4]);
	    freemem (dealloccstatement);
	    return;
	}
	else			// then statement
	{
	    if (not)
		printf ("  bcs ");
	    else
		printf ("  bcc ");
	    printf (".skip%s\n", statement[0]);
	    // separate statement
	    for (i = 3; i < STATEMENTCOUNT; ++i)
	    {
		for (k = 0; k < SIZEOFSTATEMENT; ++k)
		{
		    cstatement[i - 3][k] = statement[i][k];
		}
	    }
	    printf (".condpart%d\n", condpart++);
	    keywords (cstatement);
	    printf (".skip%s\n", statement[0]);
	    freemem (dealloccstatement);
	    return;
	}
    }


    // check for array, e.g. x{1} to get bit 1 of x
    for (i = 3; i < SIZEOFSTATEMENT; ++i)
    {
	if (statement[2][i] == '\0')
	{
	    i = SIZEOFSTATEMENT;
	    break;
	}
	if (statement[2][i] == '}')
	    break;
    }
    if (i < SIZEOFSTATEMENT)		// found array
    {
	// extract expression in parentheses - for now just whole numbers allowed
	bit = (int) statement[2][i - 1] - '0';
	if ((bit > 9) || (bit < 0))
	{
	    prerror ("variables in bit access not supported");
	}
	if ((bit == 7) || (bit == 6))	// if bit 6 or 7, we can use BIT and save 2 bytes
	{
	    printf ("  bit ");
	    for (i = 0; i < SIZEOFSTATEMENT; ++i)
	    {
		if (statement[2][i] == '{')
		    break;
		printf ("%c", statement[2][i]);
	    }
	    printf ("\n");
	    if (!islabel (statement))
	    {
		if (!not)
		{
		    if (bit == 7)
			bmi (statement[4]);
		    else
			bvs (statement[4]);
		}
		else
		{
		    if (bit == 7)
			bpl (statement[4]);
		    else
			bvc (statement[4]);
		}
		freemem (dealloccstatement);
		return;
	    }
	    else		// then statement
	    {
		if (not)
		{
		    if (bit == 7)
			printf ("  bmi ");
		    else
			printf ("  bvs ");
		}
		else
		{
		    if (bit == 7)
			printf ("  bpl ");
		    else
			printf ("  bvc ");
		}

		printf (".skip%s\n", statement[0]);
		// separate statement
		for (i = 3; i < STATEMENTCOUNT; ++i)
		{
		    for (k = 0; k < SIZEOFSTATEMENT; ++k)
		    {
			cstatement[i - 3][k] = statement[i][k];
		    }
		}
		printf (".condpart%d\n", condpart++);
		keywords (cstatement);
		printf (".skip%s\n", statement[0]);

		freemem (dealloccstatement);
		return;
	    }
	}
	else
	{
	    Aregmatch = 0;
	    printf ("  lda ");
	    for (i = 0; i < SIZEOFSTATEMENT; ++i)
	    {
		if (statement[2][i] == '{')
		    break;
		printf ("%c", statement[2][i]);
	    }
	    printf ("\n");
	    if (!bit)		// if bit 0, we can use LSR and save a byte
		printf ("  lsr\n");
	    else
		printf ("  and #%d\n", 1 << bit);	//(int)pow(2,bit));
	    if (!islabel (statement))
	    {
		if (not)
		{
		    if (!bit)
			bcc (statement[4]);
		    else
			beq (statement[4]);
		}
		else
		{
		    if (!bit)
			bcs (statement[4]);
		    else
			bne (statement[4]);
		}
		freemem (dealloccstatement);
		return;
	    }
	    else		// then statement
	    {
		if (not)
		{
		    if (!bit)
			printf ("  bcs ");
		    else
			printf ("  bne ");
		}
		else
		{
		    if (!bit)
			printf ("  bcc ");
		    else
			printf ("  beq ");
		}

		printf (".skip%s\n", statement[0]);
		// separate statement
		for (i = 3; i < STATEMENTCOUNT; ++i)
		{
		    for (k = 0; k < SIZEOFSTATEMENT; ++k)
		    {
			cstatement[i - 3][k] = statement[i][k];
		    }
		}
		printf (".condpart%d\n", condpart++);
		keywords (cstatement);
		printf (".skip%s\n", statement[0]);

		freemem (dealloccstatement);
		return;
	    }

	}

    }

    // generic if-then (no special considerations)
    //check for [indexing]
    strcpy (Aregcopy, statement[2]);
    if (!strcmp (statement[2], Areg))
	Aregmatch = 1;		// do we already have the correct value in A?

    for (i = 3; i < STATEMENTCOUNT; ++i)
	if ((!strncmp (statement[i], "then\0", 4)) ||
	    (!strncmp (statement[i], "&&\0", 2)) || (!strncmp (statement[i], "||\0", 2)))
	    break;

    j = 0;
    for (k = 3; k < i; ++k)
    {
	if (statement[k][0] == '&' && statement[k][1] == '\0')
	    j = k;
	if ((statement[k][0] == '<') || (statement[k][0] == '>') || (statement[k][0] == '='))
	    break;
    }
    if ((k == i) && j)
	k = j;			// special case of & for efficient code

    if ((complex_boolean) || (k == i && i > 4))
    {
	// complex boolean found
	// assign value to contents, reissue statement as boolean
	strcpy (cstatement[2], "Areg\0");
	strcpy (cstatement[3], "=\0");
	for (j = 2; j < i; ++j)
	    strcpy (cstatement[j + 2], statement[j]);

	let (cstatement);

	if (!islabel (statement))	// then linenumber
	{
	    if (not)
		beq (statement[i + 1]);
	    else
		bne (statement[i + 1]);
	}
	else			// then statement
	{			// first, take negative of condition and branch around statement
	    j = i;
	    if (not)
		printf ("  bne ");
	    else
		printf ("  beq ");
	}
	printf (".skip%s\n", statement[0]);
	// separate statement
	for (i = j; i < STATEMENTCOUNT; ++i)
	{
	    for (k = 0; k < SIZEOFSTATEMENT; ++k)
	    {
		cstatement[i - j][k] = statement[i][k];
	    }
	}
	printf (".condpart%d\n", condpart++);

	keywords (cstatement);
	printf (".skip%s\n", statement[0]);

	Aregmatch = 0;
	freemem (dealloccstatement);
	return;
    }
    else if (((k < i) && (i - k != 2)) || ((k < i) && (k > 3)))
    {
	printf ("; complex condition detected\n");
	// complex statements will be changed to assignments and reissued as assignments followed by a simple compare
	// i=location of then
	// k=location of conditional operator
	// if is at 2
	if (not)
	{			// handles =, <, <=, >, >=, <>
	    // & handled later
	    if (!strncmp (statement[k], "=\0", 2))
	    {
		statement[3][0] = '<';	// force beq/bne below
		statement[3][1] = '>';
		statement[3][2] = '\0';
	    }
	    else if (!strncmp (statement[k], "<>", 2))
	    {
		statement[3][0] = '=';	// force beq/bne below
		statement[3][1] = '\0';
	    }
	    else if (!strncmp (statement[k], "<=", 2))
	    {
		statement[3][0] = '>';	// force beq/bne below
		statement[3][1] = '\0';
	    }
	    else if (!strncmp (statement[k], ">=", 2))
	    {
		statement[3][0] = '<';	// force beq/bne below
		statement[3][1] = '\0';
	    }
	    else if (!strncmp (statement[k], "<\0", 2))
	    {
		statement[3][0] = '>';	// force beq/bne below
		statement[3][1] = '=';
		statement[3][2] = '\0';
	    }
	    else if (!strncmp (statement[k], "<\0", 2))
	    {
		statement[3][0] = '>';	// force beq/bne below
		statement[3][1] = '=';
		statement[3][2] = '\0';
	    }
	}
	if (k > 4)
	    push1 = 1;		// first statement is complex
	if (i - k != 2)
	    push2 = 1;		// second statement is complex

	// <, >=, &, = do not swap
	// > or <= swap

	if (push1 == 1 && push2 == 1 && (strncmp (statement[k], ">\0", 2)) && (strncmp (statement[k], "<=\0", 2)))
	{
	    // Assign to Areg and push
	    strcpy (cstatement[2], "Areg\0");
	    strcpy (cstatement[3], "=\0");
	    for (j = 2; j < k; ++j)
	    {
		for (h = 0; h < SIZEOFSTATEMENT; ++h)
		{
		    cstatement[j + 2][h] = statement[j][h];
		}
	    }
	    let (cstatement);
	    printf ("  PHA\n");
	    // second statement:
	    strcpy (cstatement[2], "Areg\0");
	    strcpy (cstatement[3], "=\0");
	    for (j = k + 1; j < i; ++j)
	    {
		for (h = 0; h < SIZEOFSTATEMENT; ++h)
		{
		    cstatement[j - k + 3][h] = statement[j][h];
		}
	    }
	    let (cstatement);
	    printf ("  PHA\n");
	    situation = 1;
	}
	else if (push1 == 1 && push2 == 1)	// two pushes plus swaps
	{
	    // second statement first:
	    strcpy (cstatement[2], "Areg\0");
	    strcpy (cstatement[3], "=\0");
	    for (j = k + 1; j < i; ++j)
	    {
		for (h = 0; h < SIZEOFSTATEMENT; ++h)
		{
		    cstatement[j - k + 3][h] = statement[j][h];
		}
	    }
	    let (cstatement);
	    printf ("  pha\n");

	    // first statement second
	    strcpy (cstatement[2], "Areg\0");
	    strcpy (cstatement[3], "=\0");
	    for (j = 2; j < k; ++j)
	    {
		for (h = 0; h < SIZEOFSTATEMENT; ++h)
		{
		    cstatement[j + 2][h] = statement[j][h];
		}
	    }
	    let (cstatement);
	    printf ("  pha\n");

	    // now change operator
	    // > or <= swap
	    if (!strncmp (statement[k], ">\0", 2))
		strcpy (statement[k], "<\0");
	    if (!strncmp (statement[k], "<=\0", 2))
		strcpy (statement[k], ">=\0");
	    situation = 2;
	}
	else if (push1 == 1 && (strncmp (statement[k], ">\0", 2)) && (strncmp (statement[k], "<=\0", 2)))
	{
	    // first statement only, no swap
	    strcpy (cstatement[2], "Areg\0");
	    strcpy (cstatement[3], "=\0");
	    for (j = 2; j < k; ++j)
	    {
		for (h = 0; h < SIZEOFSTATEMENT; ++h)
		{
		    cstatement[j + 2][h] = statement[j][h];
		}
	    }
	    let (cstatement);
	    //printf("  PHA\n");
	    situation = 3;

	}
	else if (push1 == 1)
	{
	    // first statement only, swap
	    strcpy (cstatement[2], "Areg\0");
	    strcpy (cstatement[3], "=\0");
	    for (j = 2; j < k; ++j)
	    {
		for (h = 0; h < SIZEOFSTATEMENT; ++h)
		{
		    cstatement[j + 2][h] = statement[j][h];
		}
	    }
	    let (cstatement);
	    printf ("  pha\n");

	    // now change operator
	    // > or <= swap
	    if (!strncmp (statement[k], ">\0", 2))
		strcpy (statement[k], "<\0");
	    if (!strncmp (statement[k], "<=\0", 2))
		strcpy (statement[k], ">=\0");

	    // swap pushes and vars:
	    push1 = 0;
	    push2 = 1;
	    strcpy (statement[2], statement[k + 1]);
	    situation = 4;

	}
	else if (push2 == 1 && (strncmp (statement[k], ">\0", 2)) && (strncmp (statement[k], "<=\0", 2)))
	{
	    // second statement only, no swap:
	    strcpy (cstatement[2], "Areg\0");
	    strcpy (cstatement[3], "=\0");
	    for (j = k + 1; j < i; ++j)
	    {
		for (h = 0; h < SIZEOFSTATEMENT; ++h)
		{
		    cstatement[j - k + 3][h] = statement[j][h];
		}
	    }
	    let (cstatement);
	    printf ("  pha\n");
	    situation = 5;
	}
	else if (push2 == 1)
	{
	    // second statement only, swap:
	    strcpy (cstatement[2], "Areg\0");
	    strcpy (cstatement[3], "=\0");
	    for (j = k + 1; j < i; ++j)
	    {
		for (h = 0; h < SIZEOFSTATEMENT; ++h)
		{
		    cstatement[j - k + 3][h] = statement[j][h];
		}
	    }
	    let (cstatement);
	    //printf("  PHA\n");
	    // now change operator
	    // > or <= swap
	    if (!strncmp (statement[k], ">\0", 2))
		strcpy (statement[k], "<\0");
	    if (!strncmp (statement[k], "<=\0", 2))
		strcpy (statement[k], ">=\0");

	    // swap pushes and vars:
	    push1 = 1;
	    push2 = 0;
	    strcpy (statement[k + 1], statement[2]);
	    situation = 6;
	}
	else			// should never get here
	{
	    prerror ("parse error in complex if-then statement");
	}
	if (situation != 6 && situation != 3)
	{
	    printf ("  tsx\n");	//index to stack
	    if (push1)
		printf ("  pla\n");
	    if (push2)
		printf ("  pla\n");
	}
	if (push1 && push2)
	    strcpy (cstatement[2], " $102[TSX]\0");
	else if (push1)
	    strcpy (cstatement[2], " $101[TSX]\0");
	else
	    strcpy (cstatement[2], statement[2]);
	strcpy (cstatement[3], statement[k]);
	if (push2)
	    strcpy (cstatement[4], " $101[TSX]\0");
	else
	    strcpy (cstatement[4], statement[k + 1]);
	for (j = 5; j < 40; ++j)
	    strcpy (cstatement[j], statement[j - 5 + i]);
	strcpy (cstatement[0], statement[0]);	// copy label
	if (situation != 4 && situation != 5)
	    strcpy (Areg, cstatement[2]);	// attempt to suppress superfluous LDA

	if (not && statement[k][0] == '&')
	{
	    shiftdata (cstatement, 5);
	    cstatement[5][0] = ')';
	    cstatement[5][1] = '\0';
	    shiftdata (cstatement, 2);
	    shiftdata (cstatement, 2);
	    cstatement[2][0] = '!';
	    cstatement[2][1] = '\0';
	    cstatement[3][0] = '(';
	    cstatement[3][1] = '\0';
	}
	strcpy (cstatement[1], "if\0");
	if (statement[i][0] == 't')
	    doif (cstatement);	// okay to recurse
	else if (statement[i][0] == '&')
	{
	    //FIXME - kludge to fix spurious LDA...
	    if (situation != 4 && situation != 5)
		printf ("; todo: this LDA is spurious and should be prevented ->");
	    keywords (cstatement);	// statement still has booleans - attempt to reanalyze
	}
	else
	{
	    prerror ("if-then too complex for logical OR");
	}
	Aregmatch = 0;
	freemem (dealloccstatement);
	return;
    }
    index |= getindex (statement[2], &getindex0[0], &indirectflag0);
    if (strncmp (statement[3], "then\0", 4))
	index |= getindex (statement[4], &getindex1[0], &indirectflag1) << 1;

    /*
       bug alert! seems to be introducing spurious LDA 1,x when two or
       more complex statements are used.
     */

    if (!Aregmatch)		// do we already have the correct value in A?
    {
	if (index & 1)
	    loadindex (&getindex0[0], indirectflag0);
	printf ("  lda ");
	printindex (statement[2], index & 1, indirectflag0);
	strcpy (Areg, Aregcopy);
    }

    if (index & 2)
	loadindex (&getindex1[0], indirectflag1);
    //todo:check for cmp #0--useless except for <, > to clear carry
    if (strncmp (statement[3], "then\0", 4))
    {
	if (statement[3][0] == '&')
	{
	    printf ("	AND ");
	    if (not)
	    {
		statement[3][0] = '=';	// force beq/bne below
		statement[3][1] = '\0';
	    }
	    else
	    {
		statement[3][0] = '<';	// force beq/bne below
		statement[3][1] = '>';
		statement[3][2] = '\0';
	    }
	}
	else
	    printf ("  cmp ");
	printindex (statement[4], index & 2, indirectflag1);
    }

    if (!islabel (statement))
    {				// then linenumber
	if (statement[3][0] == '=')
	    beq (statement[6]);
	if (!strncmp (statement[3], "<>\0", 2))
	    bne (statement[6]);
	else if (statement[3][0] == '<')
	    bcc (statement[6]);
	if (statement[3][0] == '>')
	    bcs (statement[6]);
	if (!strncmp (statement[3], "then\0", 4))
	{
	    if (not)
		beq (statement[4]);
	    else
		bne (statement[4]);
	}


    }
    else			// then statement
    {				// first, take negative of condition and branch around statement
	if (statement[3][0] == '=')
	    printf ("  bne ");
	if (!strcmp (statement[3], "<>"))
	    printf ("  beq ");
	else if (statement[3][0] == '<')
	    printf ("  bcs ");
	if (statement[3][0] == '>')
	    printf ("  bcc ");
	j = 5;

	if (!strncmp (statement[3], "then\0", 4))
	{
	    j = 3;
	    if (not)
		printf ("  bne ");
	    else
		printf ("  beq ");
	}
	printf (".skip%s\n", statement[0]);
	// separate statement

	// separate statement
	for (i = j; i < STATEMENTCOUNT; ++i)
	{
	    for (k = 0; k < SIZEOFSTATEMENT; ++k)
	    {
		cstatement[i - j][k] = statement[i][k];
	    }
	}
	printf (".condpart%d\n", condpart++);
	keywords (cstatement);
	printf (".skip%s\n", statement[0]);

	freemem (dealloccstatement);
	return;
    }
    freemem (dealloccstatement);
}

void boxcollision (char **statement)
{
    //        2     3 4  5 6  7 8  9 10 11 12 13 14 15 16 17 18 19 20
    // boxcollision ( X1 , Y1 , W1 , H1 ,  X2  , Y2 ,  W2 ,  H2 )

    int t;
    invalidate_Areg ();

    for (t = 4; t < 19; t = t + 2)
    {
	if ((statement[t][0] == 0) || (statement[t][0] == ')')
	    || (statement[t][0] == ',') || (statement[t + 1][0] == 0))
	    prerror ("malformed boxcollision statement");
    }

    // check if widths and heights are all constants. if so, use quick box collision
    if (isimmed (statement[8]) && isimmed (statement[10]) && isimmed (statement[16]) && isimmed (statement[18]) && (!deprecatedboxcollision))
    {
	printf ("  QBOXCOLLISIONCHECK %s,%s,%s,%s,%s,%s,%s,%s\n",
		statement[4], statement[6], statement[8], statement[10],
		statement[12], statement[14], statement[16], statement[18]);
	return;
    }

    // we're going to use the boxcollision subroutine, so enable it
    if (boxcollisionused == 0)
    {
	strcpy (redefined_variables[numredefvars++], "BOXCOLLISION = 1");
	boxcollisionused = 1;
    }

    if (collisionwrap == 0)
    {
	//no collision wrapping is selected. we'll work directly with the variables, no prep required.
	printf ("\n");

	// ### boxh1
	if (isimmed (statement[8]))
	    printf ("  ldy #(%s-1)\n", statement[8]);
	else
	{
	    printf ("  ldy %s\n", statement[8]);
	    printf ("  dey\n");
	}
	printf ("  sty temp3\n");


	// ### boxw1
	if (isimmed (statement[10]))
	    printf ("  ldy #(%s-1)\n", statement[10]);
	else
	{
	    printf ("  ldy %s\n", statement[10]);
	    printf ("  dey\n");
	}
	printf ("  sty temp4\n");


	// ### boxx2
	printf ("  lda ");
	printimmed (statement[12]);
	printf ("%s\n", statement[12]);
	printf ("  sta temp5\n");

	// ### boxy2
	printf ("  lda ");
	printimmed (statement[14]);
	printf ("%s\n", statement[14]);
	printf ("  sta temp6\n");

	// ### boxh2
	if (isimmed (statement[16]))
	    printf ("  ldy #(%s-1)\n", statement[16]);
	else
	{
	    printf ("  ldy %s\n", statement[16]);
	    printf ("  dey\n");
	}
	printf ("  sty temp7\n");


	// ### boxw2
	if (isimmed (statement[18]))
	    printf ("  ldy #(%s-1)\n", statement[18]);
	else
	{
	    printf ("  ldy %s\n", statement[18]);
	    printf ("  dey\n");
	}
	printf ("  sty temp8\n");


	// ### boxy1
	printf ("  ldy ");
	printimmed (statement[6]);
	printf ("%s\n", statement[6]);

	// ### boxx1
	printf ("  lda ");
	printimmed (statement[4]);
	printf ("%s\n", statement[4]);

	printf ("  jsr boxcollision\n");
	printf ("\n");
    }
    else
    {

	//collision wrapping is selected. we need to adjust the x and y variables...
	printf ("\n");

	printf ("  clc ; one clc only. If we overflow we're screwed anyway.\n");

	// ### boxh1
	if (isimmed (statement[8]))
	    printf ("  ldy #(%s-1)\n", statement[8]);
	else
	{
	    printf ("  ldy %s)\n", statement[8]);
	    printf ("  dey\n");
	}
	printf ("  sty temp3\n");


	// ### boxw1
	if (isimmed (statement[10]))
	    printf ("  ldy #(%s-1)\n", statement[10]);
	else
	{
	    printf ("  ldy %s\n", statement[10]);
	    printf ("  dey\n");
	}
	printf ("  sty temp4\n");


	// ### boxx2
	printf ("  lda ");
	printimmed (statement[12]);
	printf ("%s\n", statement[12]);
	printf ("  adc #48\n");
	printf ("  sta temp5\n");

	// ### boxy2
	printf ("  lda ");
	printimmed (statement[14]);
	printf ("%s\n", statement[14]);
	printf ("  adc #((256-WSCREENHEIGHT)/2)\n");
	printf ("  sta temp6\n");

	// ### boxh2
	if (isimmed (statement[16]))
	    printf ("  ldy #(%s-1)\n", statement[16]);
	else
	{
	    printf ("  ldy %s\n", statement[16]);
	    printf ("  dey\n");
	}
	printf ("  sty temp7\n");

	// ### boxw2
	if (isimmed (statement[18]))
	    printf ("  ldy #(%s-1)\n", statement[18]);
	else
	{
	    printf ("  ldy %s\n", statement[18]);
	    printf ("  dey\n");
	}
	printf ("  sty temp8\n");

	printf ("  lda ");
	printimmed (statement[6]);
	printf ("%s\n", statement[6]);	//boxy1
	printf ("  adc #((256-WSCREENHEIGHT)/2)\n");
	printf ("  tay\n");

	printf ("  lda ");
	printimmed (statement[4]);
	printf ("%s\n", statement[4]);	//boxx1
	printf ("  adc #48\n");
	printf ("  jsr boxcollision\n");
	printf ("\n");

    }
}

int getcondpart ()
{
    return condpart;
}

int orderofoperations (char op1, char op2)
{
    // specify order of operations for complex equations
    // i.e.: parens, divmul (*/), +-, logical (^&|)
    if (op1 == '(')
	return 0;
    else if (op2 == '(')
	return 0;
    else if (op2 == ')')
	return 1;
    else if (((op1 == '^') || (op1 == '|') || (op1 == '&')) && ((op2 == '^') || (op2 == '|') || (op2 == '&')))
	return 0;
    else if ((op1 == '*') || (op1 == '/'))
	return 1;
    else if ((op2 == '*') || (op2 == '/'))
	return 0;
    else if ((op1 == '+') || (op1 == '-'))
	return 1;
    else if ((op2 == '+') || (op2 == '-'))
	return 0;
    else
	return 1;
}

int isoperator (char op)
{
    if (!((op == '+') || (op == '-') || (op == '/') ||
	  (op == '*') || (op == '&') || (op == '|') || (op == '^') || (op == ')') || (op == '(')))
	return 0;
    else
	return 1;
}

void displayoperation (char *opcode, char *operand, int index)
{
    int indirectflag = 0;
    if (!strncmp (operand, "stackpull\0", 9))
    {
	if (opcode[0] == '-')
	{
	    // operands swapped
	    printf ("  tay\n");
	    printf ("  pla\n");
	    printf ("  sty tempmath\n");
	    printf ("  sec\n");
	    printf ("  sbc tempmath\n");
	}
	else if (opcode[0] == '/')
	{
	    // operands swapped
	    printf ("  tay\n");
	    printf ("  pla\n");
	}
	else
	{
	    printf ("  tsx\n");
	    printf ("  inx\n");
	    printf ("  %s $100,x\n", opcode + 1);
	    printf ("  txs\n");
	}
    }
    else
    {
	printf ("	%s ", opcode + 1);
	printindex (operand, index, indirectflag);
    }
}

void dec (char **cstatement)	// "dec" is variation of "let" that uses decimal mode
{
    decimal = 1;
    printf ("  sed\n");
    let (cstatement);
    printf ("  cld\n");
    decimal = 0;
}

void let (char **cstatement)
{
    int i, j = 0, bit = 0, k;
    int index = 0;
    char getindex0[SIZEOFSTATEMENT];
    char getindex1[SIZEOFSTATEMENT];
    char getindex2[SIZEOFSTATEMENT];
    int indirectflag0, indirectflag1, indirectflag2;
    int score[6] = { 0, 0, 0, 0, 0, 0 };
    char **statement;
    char *getbitvar;
    int Aregmatch = 0;
    char Aregcopy[SIZEOFSTATEMENT];
    char operandcopy[SIZEOFSTATEMENT];
    int fixpoint1;
    int fixpoint2;
    int fixpoint3 = 0;
    char **deallocstatement;
    char **rpn_statement;	// expression in rpn
    char rpn_stack[STATEMENTCOUNT][SIZEOFSTATEMENT];	// prolly doesn't need to be this big
    int sp = 0;			// stack pointer for converting to rpn
    int numrpn = 0;
    char **atomic_statement;	// singular statements to recurse back to here
    char tempstatement1[SIZEOFSTATEMENT], tempstatement2[SIZEOFSTATEMENT];

    strcpy (Aregcopy, "index-invalid");

    statement = (char **) malloc (sizeof (char *) * STATEMENTCOUNT);
    deallocstatement = statement;
    if (!strncmp (cstatement[2], "=\0", 1))
    {
	for (i = STATEMENTCOUNT-2; i > 0; --i)
	{
	    statement[i + 1] = cstatement[i];
	}
    }
    else
	statement = cstatement;

    // check for unary minus (e.g. a=-a) and insert zero before it
    if ((statement[4][0] == '-') && (statement[5][0]) > (unsigned char) 0x3F)
    {
	shiftdata (statement, 4);
	statement[4][0] = '0';
    }


    fixpoint1 = isfixpoint (statement[2]);
    fixpoint2 = isfixpoint (statement[4]);
    removeCR (statement[4]);

    // check for complex statement
    if ((!((statement[4][0] == '-') && (statement[6][0] == ':'))) &&
	(statement[5][0] != ':') && (statement[7][0] != ':')
	&& (!((statement[5][0] == '(') && (statement[4][0] != '(')))
	&& ((unsigned char) statement[5][0] > (unsigned char) 0x20)
	&& ((unsigned char) statement[7][0] > (unsigned char) 0x20))
    {
	printf ("; complex statement detected\n");
	// complex statement here, hopefully.
	// convert equation to reverse-polish notation so we can express it in terms of
	// atomic equations and stack pushes/pulls
	rpn_statement = (char **) malloc (sizeof (char *) * STATEMENTCOUNT);
	for (i = 0; i < STATEMENTCOUNT; ++i)
	{
	    rpn_statement[i] = (char *) malloc (sizeof (char) * SIZEOFSTATEMENT);
	    for (k = 0; k < SIZEOFSTATEMENT; ++k)
		rpn_statement[i][k] = '\0';
	}

	atomic_statement = (char **) malloc (sizeof (char *) * 10);
	for (i = 0; i < 10; ++i)
	{
	    atomic_statement[i] = (char *) malloc (sizeof (char) * SIZEOFSTATEMENT);
	    for (k = 0; k < STATEMENTCOUNT; ++k)
		atomic_statement[i][k] = '\0';
	}

	// this converts expression to rpn
	for (k = 4; (statement[k][0] != '\0') && (statement[k][0] != ':'); k++)
	{
	    // ignore CR/LF
	    if (statement[k][0] == (unsigned char) 0x0A)
		continue;
	    if (statement[k][0] == (unsigned char) 0x0D)
		continue;

	    strcpy (tempstatement1, statement[k]);
	    if (!isoperator (tempstatement1[0]))
	    {
		strcpy (rpn_statement[numrpn++], tempstatement1);
	    }
	    else
	    {
		while ((sp) && (orderofoperations (rpn_stack[sp - 1][0], tempstatement1[0])))
		{
		    strcpy (tempstatement2, rpn_stack[sp - 1]);
		    sp--;
		    strcpy (rpn_statement[numrpn++], tempstatement2);
		}
		if ((sp) && (tempstatement1[0] == ')'))
		    sp--;
		else
		    strcpy (rpn_stack[sp++], tempstatement1);
	    }
	}



	// get stuff off of our rpn stack
	while (sp)
	{
	    strcpy (tempstatement2, rpn_stack[sp - 1]);
	    sp--;
	    strcpy (rpn_statement[numrpn++], tempstatement2);
	}

	// now parse rpn statement

	sp = 0;			// now use as pointer into rpn_statement
	while (sp < numrpn)
	{
	    // clear atomic statement cache
	    for (i = 0; i < 10; ++i)
		for (k = 0; k < SIZEOFSTATEMENT; ++k)
		    atomic_statement[i][k] = '\0';
	    if (isoperator (rpn_statement[sp][0]))
	    {
		// operator: need stack pull as 2nd arg
		// Areg=Areg (op) stackpull
		strcpy (atomic_statement[2], "Areg");
		strcpy (atomic_statement[3], "=");
		strcpy (atomic_statement[4], "Areg");
		strcpy (atomic_statement[5], rpn_statement[sp++]);
		strcpy (atomic_statement[6], "stackpull");
		let (atomic_statement);
	    }
	    else if (isoperator (rpn_statement[sp + 1][0]))
	    {
		// val,operator: Areg=Areg (op) val
		strcpy (atomic_statement[2], "Areg");
		strcpy (atomic_statement[3], "=");
		strcpy (atomic_statement[4], "Areg");
		strcpy (atomic_statement[6], rpn_statement[sp++]);
		strcpy (atomic_statement[5], rpn_statement[sp++]);
		let (atomic_statement);
	    }
	    else if (isoperator (rpn_statement[sp + 2][0]))
	    {
		// val,val,operator: stackpush, then Areg=val1 (op) val2
		if (sp)
		    printf ("  pha\n");
		strcpy (atomic_statement[2], "Areg");
		strcpy (atomic_statement[3], "=");
		strcpy (atomic_statement[4], rpn_statement[sp++]);
		strcpy (atomic_statement[6], rpn_statement[sp++]);
		strcpy (atomic_statement[5], rpn_statement[sp++]);
		let (atomic_statement);
	    }
	    else
	    {
		if ((rpn_statement[sp] == 0) || (rpn_statement[sp + 1] == 0) || (rpn_statement[sp + 2] == 0))
		{
		    // incomplete or unrecognized expression
		    prerror ("cannot evaluate expression");
		}
		// val,val,val: stackpush, then load of next value
		if (sp)
		    printf ("  pha\n");
		strcpy (atomic_statement[2], "Areg");
		strcpy (atomic_statement[3], "=");
		strcpy (atomic_statement[4], rpn_statement[sp++]);
		let (atomic_statement);
	    }
	}
	// done, now assign A-reg to original value
	for (i = 0; i < 10; ++i)
	    for (k = 0; k < SIZEOFSTATEMENT; ++k)
		atomic_statement[i][k] = '\0';
	strcpy (atomic_statement[2], statement[2]);
	strcpy (atomic_statement[3], "=");
	strcpy (atomic_statement[4], "Areg");
	let (atomic_statement);

	//free up our mallocs...
	for (i = 0; i < STATEMENTCOUNT; ++i)
	    free (rpn_statement[i]);
	for (i = 0; i < 10; ++i)
	    free (atomic_statement[i]);
	free (rpn_statement);
	free (atomic_statement);
	free (deallocstatement);

	return;			// bye-bye!
    }


    //check for [indexing]
    strcpy (Aregcopy, statement[4]);
    if (!strcmp (statement[4], Areg))
	Aregmatch = 1;		// do we already have the correct value in A?

    index |= getindex (statement[2], &getindex0[0], &indirectflag0);
    index |= getindex (statement[4], &getindex1[0], &indirectflag1) << 1;
    if (statement[5][0] != ':')
	index |= getindex (statement[6], &getindex2[0], &indirectflag2) << 2;


    // check for array, e.g. x(1) to access bit 1 of x
    for (i = 3; i < SIZEOFSTATEMENT; ++i)
    {
	if (statement[2][i] == '\0')
	{
	    i = SIZEOFSTATEMENT;
	    break;
	}
	if (statement[2][i] == '}')
	    break;
    }
    if (i < SIZEOFSTATEMENT)		// found bit
    {
	strcpy (Areg, "invalid");
	// extract expression in parentheses - for now just whole numbers allowed
	bit = (int) statement[2][i - 1] - '0';
	if ((bit > 9) || (bit < 0))
	{
	    prerror ("variables in bit access not supported");
	}
	if (bit > 7)
	{
	    prerror ("invalid bit access");
	}

	if (statement[4][0] == '0')
	{
	    printf ("  lda ");
	    for (i = 0; i < SIZEOFSTATEMENT; ++i)
	    {
		if (statement[2][i] == '{')
		    break;
		printf ("%c", statement[2][i]);
	    }
	    printf ("\n");
	    printf ("  and #%d\n", 255 ^ (1 << bit));	//(int)pow(2,bit));
	}
	else if (statement[4][0] == '1')
	{
	    printf ("  lda ");
	    for (i = 0; i < SIZEOFSTATEMENT; ++i)
	    {
		if (statement[2][i] == '{')
		    break;
		printf ("%c", statement[2][i]);
	    }
	    printf ("\n");
	    printf ("  ora #%d\n", 1 << bit);	//(int)pow(2,bit));
	}
	else if ((getbitvar = strtok (statement[4], "{")))
	{			// assign one bit to another
	    if (getbitvar[0] == '!')
		printf ("  lda %s\n", getbitvar + 1);
	    else
		printf ("  lda %s\n", getbitvar);
	    printf ("  and #%d\n", (1 << ((int) statement[4][strlen (getbitvar) + 1] - '0')));
	    printf ("  php\n");
	    printf ("  lda ");
	    for (i = 0; i < SIZEOFSTATEMENT; ++i)
	    {
		if (statement[2][i] == '{')
		    break;
		printf ("%c", statement[2][i]);
	    }
	    printf ("\n  and #%d\n", 255 ^ (1 << bit));	//(int)pow(2,bit));
	    printf ("  plp\n");
	    if (getbitvar[0] == '!')
		printf ("  .byte $D0, $02\n");	//bad, bad way to do BEQ addr+5
	    else
		printf ("  .byte $F0, $02\n");	//bad, bad way to do BNE addr+5

	    printf ("  ora #%d\n", 1 << bit);	//(int)pow(2,bit));

	}
	else
	{
	    prerror ("can only assign 0, 1 or another bit to a bit");
	}
	printf ("  sta ");
	for (i = 0; i < SIZEOFSTATEMENT; ++i)
	{
	    if (statement[2][i] == '{')
		break;
	    printf ("%c", statement[2][i]);
	}
	printf ("\n");
	free (deallocstatement);
	return;
    }

    if (statement[4][0] == '-')	// assignment to negative
    {
	strcpy (Areg, "invalid");
	if ((!fixpoint1) && (isfixpoint (statement[5]) != 12))
	{
	    if (statement[5][0] > (unsigned char) 0x39)	// perhaps include constants too?
	    {
		printf ("  lda #0\n");
		printf ("  sec\n");
		printf ("  sbc %s\n", statement[5]);
	    }
	    else
		printf ("  lda #%d\n", 256 - strictatoi (statement[5]));
	}
	else
	{
	    if (fixpoint1 == 4)
	    {
		if (statement[5][0] > (unsigned char) 0x39)	// perhaps include constants too?
		{
		    printf ("  lda #0\n");
		    printf ("  sec\n");
		    printf ("  sbc %s\n", statement[5]);
		}
		else
		    printf ("  lda #%d\n", (int) ((16 - atof (statement[5])) * 16));
		printf ("  sta %s\n", statement[2]);
		free (deallocstatement);
		return;
	    }
	    if (fixpoint1 == 8)
	    {
		printf ("  ldx ");
		sprintf (statement[4], "%f", 256.0 - atof (statement[5]));
		printfrac (statement[4]);
		printf ("  stx ");
		printfrac (statement[2]);
		printf ("  lda #%s\n", statement[4]);
		printf ("  sta %s\n", statement[2]);
		free (deallocstatement);
		return;
	    }
	}
    }
    else if (!strncmp (statement[4], "rand\0", 4))
    {
	strcpy (Areg, "invalid");
	if (optimization & 8)
	{
	    printf ("  lda rand\n");
	    printf ("  lsr\n");
	    printf (" ifconst rand16\n");
	    printf ("  rol rand16\n");
	    printf (" endif\n");
	    printf ("  bcc *+4\n");
	    printf ("  eor #$B4\n");
	    printf ("  sta rand\n");
	    printf (" ifconst rand16\n");
	    printf ("  eor rand16\n");
	    printf (" endif\n");
	}
	else
	    jsr ("randomize");
    }
    else if ((strncmp (statement[2], "score", 5) == 0)
	     && (strlen (statement[2]) == 6) && ((isalpha (statement[2][5])) || (isdigit (statement[2][5]))))
    {
	char scorei;
	scorei = statement[2][5];
	if (strlen (statement[2]) != 6)
	{
	    prerror ("malformed score manipulation");

	}
	// break up into three parts
	strcpy (Areg, "invalid");
	if (statement[5][0] == '+')
	{
	    removeCR (statement[6]);
	    printf ("  sed\n");
	    printf ("  clc\n");
	    for (i = 5; i >= 0; i--)
	    {
		if (statement[6][i] != '\0')
		    score[j] = number (statement[6][i]);
		score[j] = number (statement[6][i]);
		if ((score[j] < 10) && (score[j] >= 0))
		    j++;
	    }
	    if (score[0] | score[1])
	    {
		printf ("  lda score%c+2\n", scorei);
		if (statement[6][0] >= 'A')
		{
		    printf ("  adc ");
		    printimmed (statement[6]);
		    printf ("%s\n", statement[6]);
		}
		else
		    printf ("  adc #$%d%d\n", score[1], score[0]);
		printf ("  sta score%c+2\n", scorei);
	    }
	    if (score[0] | score[1] | score[2] | score[3])
	    {
		printf ("  lda score%c+1\n", scorei);
		if (score[0] > 9)
		    printf ("  adc #0\n");
		else
		    printf ("  adc #$%d%d\n", score[3], score[2]);
		printf ("  sta score%c+1\n", scorei);
	    }
	    printf ("  lda score%c\n", scorei);
	    if (score[0] > 9)
		printf ("  adc #0\n");
	    else
		printf ("  adc #$%d%d\n", score[5], score[4]);
	    printf ("  sta score%c\n", scorei);
	    printf ("  cld\n");
	}
	else if (statement[5][0] == '-')
	{
	    removeCR (statement[6]);
	    printf ("  sed\n");
	    printf ("  sec\n");
	    for (i = 5; i >= 0; i--)
	    {
		if (statement[6][i] != '\0')
		    score[j] = number (statement[6][i]);
		score[j] = number (statement[6][i]);
		if ((score[j] < 10) && (score[j] >= 0))
		    j++;
	    }
	    printf ("  lda score%c+2\n", scorei);
	    if (score[0] > 9)
	    {
		printf ("  sbc ");
		printimmed (statement[6]);
		printf ("%s\n", statement[6]);
	    }
	    else
		printf ("  sbc #$%d%d\n", score[1], score[0]);
	    printf ("  sta score%c+2\n", scorei);
	    printf ("  lda score%c+1\n", scorei);
	    if (score[0] > 9)
		printf ("  sbc #0\n");
	    else
		printf ("  sbc #$%d%d\n", score[3], score[2]);
	    printf ("  sta score%c+1\n", scorei);
	    printf ("  lda score%c\n", scorei);
	    if (score[0] > 9)
		printf ("  sbc #0\n");
	    else
		printf ("  sbc #$%d%d\n", score[5], score[4]);
	    printf ("  sta score%c\n", scorei);
	    printf ("  cld\n");
	}
	else
	{
	    for (i = 5; i >= 0; i--)
	    {
		if (statement[4][i] != '\0')
		    score[j] = number (statement[4][i]);
		score[j] = number (statement[4][i]);
		if ((score[j] < 10) && (score[j] >= 0))
		    j++;
	    }
	    printf ("  lda #$%d%d\n", score[1], score[0]);
	    printf ("  sta score%c+2\n", scorei);
	    printf ("  lda #$%d%d\n", score[3], score[2]);
	    printf ("  sta score%c+1\n", scorei);
	    printf ("  lda #$%d%d\n", score[5], score[4]);
	    printf ("  sta score%c\n", scorei);
	}
	free (deallocstatement);
	return;

    }
    else
    {				// This is generic x=num or var

	if (!Aregmatch)		// do we already have the correct value in A?
	{
	    if (index & 2)
		loadindex (&getindex1[0], indirectflag1);

	    // if 8.8=8.8+8.8: this LDA will be superfluous - fix this at some point

	    if (((!fixpoint1 && !fixpoint2) || (!fixpoint1 && fixpoint2 == 8)) && statement[5][0] != '(')
	    {
		if (strncmp (statement[4], "Areg\n", 4))
		{
		    printf ("  lda ");
		    printindex (statement[4], index & 2, indirectflag1);
		}
	    }
	    strcpy (Areg, Aregcopy);
	}
    }
    if ((statement[5][0] != '\0') && (statement[5][0] != ':'))
    {				// An operator was found
	fixpoint3 = isfixpoint (statement[6]);
	strcpy (Areg, "invalid");
	if (index & 4)
	    loadindex (&getindex2[0], indirectflag2);
	if (statement[5][0] == '+')
	{
	    if ((fixpoint1 == 8) && ((fixpoint2 & 8) == 8) && ((fixpoint3 & 8) == 8))
	    {			//8.8=8.8+8.8
		printf ("  lda ");
		printfrac (statement[4]);
		printf ("  clc \n");
		printf ("  adc ");
		printfrac (statement[6]);
		printf ("  sta ");
		printfrac (statement[2]);
		printf ("  lda ");
		printimmed (statement[4]);
		printf ("%s\n", statement[4]);
		printf ("  adc ");
		printimmed (statement[6]);
		printf ("%s\n", statement[6]);
	    }
	    else if ((fixpoint1 == 8) && ((fixpoint2 & 8) == 8) && (fixpoint3 == 4))
	    {
		printf ("  ldy %s\n", statement[6]);
		printf ("  ldx ");
		printfrac (statement[4]);
		printf ("  lda ");
		printimmed (statement[4]);
		printf ("%s\n", statement[4]);
		printf ("  jsr Add44to88\n");
		printf ("  stx ");
		printfrac (statement[2]);
	    }
	    else if ((fixpoint1 == 8) && ((fixpoint3 & 8) == 8) && (fixpoint2 == 4))
	    {
		printf ("  ldy %s\n", statement[4]);
		printf ("  ldx ");
		printfrac (statement[6]);
		printf ("  lda ");
		printimmed (statement[6]);
		printf ("%s\n", statement[6]);
		printf ("  jsr Add44to88\n");
		printf ("  stx ");
		printfrac (statement[2]);
	    }
	    else if ((fixpoint1 == 4) && (fixpoint2 == 8) && ((fixpoint3 & 4) == 4))
	    {
		if (fixpoint3 == 4)
		    printf ("  ldy %s\n", statement[6]);
		else
		    printf ("  ldy #%d\n", (int) (atof (statement[6]) * 16.0));
		printf ("  lda %s\n", statement[4]);
		printf ("  ldx ");
		printfrac (statement[4]);
		printf ("  jsr Add88to44\n");
	    }
	    else if ((fixpoint1 == 4) && (fixpoint2 == 4) && (fixpoint3 == 12))
	    {
		printf ("  clc\n");
		printf ("  lda %s\n", statement[4]);
		printf ("  adc #%d\n", (int) (atof (statement[6]) * 16.0));
	    }
	    else if ((fixpoint1 == 4) && (fixpoint2 == 12) && (fixpoint3 == 4))
	    {
		printf ("  clc\n");
		printf ("  lda %s\n", statement[6]);
		printf ("  adc #%d\n", (int) (atof (statement[4]) * 16.0));
	    }
	    else		// this needs work - 44+8+44 and probably others are screwy
	    {
		if (fixpoint2 == 4)
		    printf ("  lda %s\n", statement[4]);
		if ((fixpoint3 == 4) && (fixpoint2 == 0))
		{
		    printf ("  lda ");	// this LDA might be superfluous
		    printimmed (statement[4]);
		    printf ("%s\n", statement[4]);
		}
		displayoperation ("+CLC\n	ADC", statement[6], index & 4);
	    }
	}
	else if (statement[5][0] == '-')
	{
	    if ((fixpoint1 == 8) && ((fixpoint2 & 8) == 8) && ((fixpoint3 & 8) == 8))
	    {			//8.8=8.8-8.8
		printf ("  lda ");
		printfrac (statement[4]);
		printf ("  sec \n");
		printf ("  sbc ");
		printfrac (statement[6]);
		printf ("  sta ");
		printfrac (statement[2]);
		printf ("  lda ");
		printimmed (statement[4]);
		printf ("%s\n", statement[4]);
		printf ("  sbc ");
		printimmed (statement[6]);
		printf ("%s\n", statement[6]);
	    }
	    else if ((fixpoint1 == 8) && ((fixpoint2 & 8) == 8) && (fixpoint3 == 4))
	    {
		printf ("  ldy %s\n", statement[6]);
		printf ("  ldx ");
		printfrac (statement[4]);
		printf ("  lda ");
		printimmed (statement[4]);
		printf ("%s\n", statement[4]);
		printf ("  jsr Sub44from88\n");
		printf ("  stx ");
		printfrac (statement[2]);
	    }
	    else if ((fixpoint1 == 4) && (fixpoint2 == 8) && ((fixpoint3 & 4) == 4))
	    {
		if (fixpoint3 == 4)
		    printf ("  ldy %s\n", statement[6]);
		else
		    printf ("  ldy #%d\n", (int) (atof (statement[6]) * 16.0));
		printf ("  lda %s\n", statement[4]);
		printf ("  ldx ");
		printfrac (statement[4]);
		printf ("  jsr Sub88from44\n");
	    }
	    else if ((fixpoint1 == 4) && (fixpoint2 == 4) && (fixpoint3 == 12))
	    {
		printf ("  sec\n");
		printf ("  lda %s\n", statement[4]);
		printf ("  sbc #%d\n", (int) (atof (statement[6]) * 16.0));
	    }
	    else if ((fixpoint1 == 4) && (fixpoint2 == 12) && (fixpoint3 == 4))
	    {
		printf ("  sec\n");
		printf ("  lda #%d\n", (int) (atof (statement[4]) * 16.0));
		printf ("  sbc %s\n", statement[6]);
	    }
	    else
	    {
		if (fixpoint2 == 4)
		    printf ("  lda %s\n", statement[4]);
		if ((fixpoint3 == 4) && (fixpoint2 == 0))
		    printf ("  lda #%s\n", statement[4]);
		displayoperation ("-SEC\n	SBC", statement[6], index & 4);
	    }
	}
	else if (statement[5][0] == '&')
	{
	    if (fixpoint2 == 4)
		printf ("  lda %s\n", statement[4]);
	    displayoperation ("&AND", statement[6], index & 4);
	}
	else if (statement[5][0] == '^')
	{
	    if (fixpoint2 == 4)
		printf ("  lda %s\n", statement[4]);
	    displayoperation ("^EOR", statement[6], index & 4);
	}
	else if (statement[5][0] == '|')
	{
	    if (fixpoint2 == 4)
		printf ("  lda %s\n", statement[4]);
	    displayoperation ("|ORA", statement[6], index & 4);
	}
	else if (statement[5][0] == '*')
	{
	    if (isimmed (statement[4]) && !isimmed (statement[6]) && checkmul (strictatoi (statement[4])))
	    {
		// swap operands to avoid mul routine
		strcpy (operandcopy, statement[4]);	// place here temporarily
		strcpy (statement[4], statement[6]);
		strcpy (statement[6], operandcopy);
	    }
	    if (fixpoint2 == 4)
		printf ("  lda %s\n", statement[4]);
	    if ((!isimmed (statement[6])) || (!checkmul (strictatoi (statement[6]))))
	    {
		displayoperation ("*LDY", statement[6], index & 4);
		if (statement[5][1] == '*')
                {
		    printf ("  jsr mul16\n");	// general mul routine
		    printf (".calledfunction_mul16 = 1\n");	
                }
		else
                {
		    printf ("  jsr mul8\n");
		    printf (".calledfunction_mul8 = 1\n");	
                }
	    }
	    else if (statement[5][1] == '*')
		mul (statement, 16);
	    else
		mul (statement, 8);	// attempt to optimize - may need to call mul anyway

	}
	else if (statement[5][0] == '/')
	{
	    if (fixpoint2 == 4)
		printf ("  lda %s\n", statement[4]);
	    if ((!isimmed (statement[6])) || (!checkdiv (strictatoi (statement[6]))))
	    {
		displayoperation ("/LDY", statement[6], index & 4);
		if (statement[5][1] == '/')
                {
		    printf ("  jsr div16\n");	// general div routine
		    printf (".calledfunction_div16 = 1\n");	
                }
		else
                {
		    printf ("  jsr div8\n");
		    printf (".calledfunction_div8 = 1\n");	
                }
	    }
	    else if (statement[5][1] == '/')
		divd (statement, 16);
	    else
		divd (statement, 8);	// attempt to optimize - may need to call divd anyway

	}
	else if (statement[5][0] == ':')
	{
	    strcpy (Areg, Aregcopy);	// A reg is not invalid
	}
	else if (statement[5][0] == '(')
	{
	    // we've called a function, hopefully
	    strcpy (Areg, "invalid");
	    if (!strncmp (statement[4], "sread\0", 5))
		sread (statement);
	    else if (!strncmp (statement[4], "peekchar\0", 8))
		peekchar (statement);
	    else if (!strncmp (statement[4], "getfade\0", 10))
		getfade (statement);
	    else
		callfunction (statement);
	}
	else if (statement[4][0] != '-')	// if not unary -
	{
	    prerror ("invalid operator: %s", statement[5]);
	}

    }
    else			// simple assignment
    {
	// check for fixed point stuff here
	// bugfix: forgot the LDA (?) did I do this correctly???
	if ((fixpoint1 == 4) && (fixpoint2 == 0))
	{
	    printf ("  lda ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	    printf ("  asl\n");
	    printf ("  asl\n");
	    printf ("  asl\n");
	    printf ("  asl\n");
	}
	else if ((fixpoint1 == 0) && (fixpoint2 == 4))
	{
	    printf ("  lda ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	    printf ("  lsr\n");
	    printf ("  lsr\n");
	    printf ("  lsr\n");
	    printf ("  lsr\n");
	}
	else if ((fixpoint1 == 4) && (fixpoint2 == 8))
	{
	    printf ("  lda ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	    printf ("  ldx ");
	    printfrac (statement[4]);
	    // note: this shouldn't be changed to jsr(); (why???)
	    printf ("  jsr Assign88to44");
	    printf ("\n");
	}
	else if ((fixpoint1 == 8) && (fixpoint2 == 4))
	{
	    // note: this shouldn't be changed to jsr();
	    printf ("  lda ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	    printf ("  jsr Assign44to88");
	    printf ("\n");
	    printf ("  stx ");
	    printfrac (statement[2]);
	}
	else if ((fixpoint1 == 8) && ((fixpoint2 & 8) == 8))
	{
	    printf ("  ldx ");
	    printfrac (statement[4]);
	    printf ("  stx ");
	    printfrac (statement[2]);
	    printf ("  lda ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	}
	else if ((fixpoint1 == 4) && ((fixpoint2 & 4) == 4))
	{
	    if (fixpoint2 == 4)
		printf ("  lda %s\n", statement[4]);
	    else
		printf ("  lda #%d\n", (int) (atof (statement[4]) * 16));
	}
	else if ((fixpoint1 == 8) && (fixpoint2 == 0))
	{			// should handle 8.8=number w/o point or int var
	    printf ("  lda #0\n");
	    printf ("  sta ");
	    printfrac (statement[2]);
	    printf ("  lda ");
	    printimmed (statement[4]);
	    printf ("%s\n", statement[4]);
	}
    }
    if (index & 1)
	loadindex (&getindex0[0], indirectflag0);
    if (strncmp (statement[2], "Areg\0", 4))
    {
	printf ("  sta ");
	printindex (statement[2], index & 1, indirectflag0);
    }
    free (deallocstatement);
}

void loadrombank (char **statement)
{

    int anotherbank = 0;
    invalidate_Areg ();

    assertminimumargs (statement, "loadrombank", 1);

    if (!strncmp (statement[2], "bank", 4))
    {
	anotherbank = strictatoi (statement[2] + 4);
	anotherbank = anotherbank - 1;
    }
    else if ((statement[2][0] >= '0') && (statement[2][0] <= '9'))
    {
	anotherbank = strictatoi (statement[2]);
	anotherbank = anotherbank - 1;
    }
    else
	prerror ("loadrombank statement with malformed argument");

    if (currentbank != (bankcount - 1))
	prerror ("loadrombank called from outside of the last bank");
    if (anotherbank == (bankcount - 1))
	prerror ("loadrombank can not switch the last bank");

    printf ("  lda #%d\n", anotherbank);
    printf (" ifconst BANKRAM\n");
    printf ("  sta currentbank\n");
    printf ("  ora currentrambank\n");
    printf (" endif\n");
    printf (" ifconst MCPDEVCART\n");
    printf ("  ora #$18\n");
    printf ("  sta $3000\n");
    printf (" else\n");
    printf ("  sta $8000\n");
    printf (" endif\n");

}

void loadrambank (char **statement)
{
    int anotherbank = 0;
    invalidate_Areg ();

    assertminimumargs (statement, "loadrambank", 1);

    if (!strncmp (statement[2], "bank", 4))
    {
	anotherbank = strictatoi (statement[2] + 4);
	anotherbank = anotherbank - 1;
    }
    else if ((statement[2][0] >= '0') && (statement[2][0] <= '9'))
    {
	anotherbank = strictatoi (statement[2]);
	anotherbank = anotherbank - 1;
    }
    else
	prerror ("loadrambank statement with malformed argument");

    if ((anotherbank < 0) || (anotherbank > 1))
	prerror ("bad bank# used with loadrambank");

    printf ("  lda #%d\n", anotherbank << 5);
    printf ("  sta currentrambank\n");
    printf ("  ora currentbank\n");
    printf (" ifconst MCPDEVCART\n");
    printf ("  ora #$18\n");
    printf ("  sta $3000\n");
    printf (" else\n");
    printf ("  sta $8000\n");
    printf (" endif\n");
}

void dogoto (char **statement)
{
    int anotherbank = 0;
    invalidate_Areg ();

    assertminimumargs (statement, "goto", 1);

    if (!strncmp (statement[3], "bank", 4))
    {
	if ((statement[3][4] < '1') || (statement[3][4] > '9'))
	    prerror ("destination bank is malformed. should be in form 'bank#'");
	anotherbank = strictatoi (statement[3] + 4);
	anotherbank = anotherbank - 1;
    }
    else
    {
	printf ("  jmp .%s\n", statement[2]);
	return;
    }

    if (anotherbank == (bankcount - 1))
    {
	prerror ("bank switch not required for the last bank, since its always present in the rom format.");
    }
    if ((romat4k == 1) && (anotherbank == 0))
    {
	prerror ("bank switch not required for the first bank, since its always present in this rom format.");
    }

    if (romat4k == 1)
	anotherbank--;

    printf ("  sta temp9\n");	//save A register

    printf ("  lda #>(.%s-1)\n", statement[2]);
    printf ("  pha\n");
    printf ("  lda #<(.%s-1)\n", statement[2]);
    printf ("  pha\n");

    printf ("  lda temp9\n");
    printf ("  pha\n");
    printf ("  txa\n");
    printf ("  pha\n");

    printf (" ifconst BANKRAM\n");
    printf ("  lda #%d\n", anotherbank);
    printf ("  sta currentbank\n");
    printf ("  ora currentrambank\n");
    printf (" else\n");
    printf ("  lda #%d\n", anotherbank);
    printf (" endif\n");
    printf ("  jmp BS_jsr\n");

}

void strdelchr (char *str, char badchar)
{
    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++)
    {
	*dst = *src;
	if (*dst != badchar)
	    dst++;
    }
    *dst = '\0';
}

void adjustvisible (char **statement)
{
    //  1              2    3
    // adjustvisible first last
    assertminimumargs (statement, "adjustvisible", 2);
    removeCR (statement[3]);	//remove CR if present
    printf ("  lda #(%s*3)\n", statement[2]);
    printf ("  sta temp1\n");
    printf ("  lda #(%s*3)\n", statement[3]);
    printf ("  sta temp2\n");
    printf ("  jsr adjustvisible\n");
    printf ("USED_ADJUSTVISIBLE = 1\n");
}

void doublebuffer (char **statement)
{
    //  1              2                         3
    // doublebuffer [on/off/flip/quickflip] [minimumframe]
    assertminimumargs (statement, "adjustvisible", 1);

    // enable the compile-time optional code
    if (doublebufferused == 0)
    {
	strcpy (redefined_variables[numredefvars++], "DOUBLEBUFFER = 1");
	doublebufferused = 1;
    }

    removeCR (statement[2]);	//remove CR if present
    removeCR (statement[3]);	//remove CR if present

    if (strcmp (statement[2], "on") == 0)
    {
	printf ("  lda #1\n");
	printf ("  sta doublebufferstate\n");
    }
    else if (strcmp (statement[2], "off") == 0)
    {
	// we need to restore things back to normal.
	printf ("  jsr doublebufferoff\n");
    }
    else if (strcmp (statement[2], "flip") == 0)
    {
	printf ("  jsr flipdisplaybuffer\n");
    }
    else if (strcmp (statement[2], "quickflip") == 0)
    {
	printf ("  jsr quickbufferflip\n");
    }
    else
    {
	prerror ("doublebuffer argument needs to be 'on' or 'off'");
    }
    if ((statement[3] != 0) && (statement[3][0] != 0))
    {
	int framerate;
	framerate = strictatoi (statement[3]);
	printf ("  lda #%d\n", framerate);
	printf ("  sta doublebufferminimumframetarget\n");
    }
}


void gosub (char **statement)
{
    int anotherbank = 0;
    int permanentreturn = 0;
    invalidate_Areg ();
    assertminimumargs (statement, "gosub", 1);
    if (!strncmp (statement[3], "bank", 4))
    {
	if ((statement[3][4] < '1') || (statement[3][4] > '9'))
	    prerror ("destination bank is malformed. should be in form 'bank#'");
	anotherbank = strictatoi (statement[3] + 4);
	anotherbank = anotherbank - 1;
    }
    else
    {
	printf ("  jsr .%s\n", statement[2]);
	return;
    }

    if (anotherbank == (bankcount - 1))
    {
	prerror ("bank switch not required to the last bank, since its always present");
    }

    if ((romat4k == 1) && (anotherbank == 0))
    {
	prerror ("bank switch not required to the first bank, since its always present");
    }

    if (romat4k && (currentbank == 0))
	permanentreturn = 1;

    if (currentbank == (bankcount - 1))
	permanentreturn = 1;

    if (romat4k == 1)
	anotherbank--;

    printf ("  sta temp9\n");	//save A register

    //return address
    printf ("  lda #>(ret_point%d-1)\n", ++numjsrs);
    printf ("  pha\n");
    printf ("  lda #<(ret_point%d-1)\n", numjsrs);
    printf ("  pha\n");

    // the bank switch return info... don't push this if we're switching from a non-switched bank
    // we can detect the bank from a regular return address, because its less than $100
    if (permanentreturn == 0)
    {
	printf ("  lda #0\n");
	printf ("  pha\n");
	if (romat4k == 1)
	    printf ("  lda #%d\n", currentbank - 1);
	else
	    printf ("  lda #%d\n", currentbank);
	printf ("  pha\n");
    }

    printf ("  lda #>(.%s-1)\n", statement[2]);
    printf ("  pha\n");
    printf ("  lda #<(.%s-1)\n", statement[2]);
    printf ("  pha\n");

    printf ("  lda temp9\n");
    printf ("  pha\n");
    printf ("  txa\n");
    printf ("  pha\n");

    printf (" ifconst BANKRAM\n");
    printf ("  lda #%d\n", anotherbank);
    printf ("  sta currentbank\n");
    printf ("  ora currentrambank\n");
    printf (" else\n");
    printf ("  lda #%d\n", anotherbank);
    printf (" endif\n");
    printf ("  jmp BS_jsr\n");
    printf ("ret_point%d\n", numjsrs);

}


void set (char **statement)
{
    if (!strncasecmp (statement[2], "tv\0", 2))
    {
	assertminimumargs (statement, "set tv", 1);
	if (!strncasecmp (statement[3], "ntsc\0", 4))
	{
	    strcpy (redefined_variables[numredefvars++], "NTSC = 1");
	    append_a78info ("set tvntsc");
	}
	else if (!strncasecmp (statement[3], "pal\0", 3))
	{
	    strcpy (redefined_variables[numredefvars++], "PAL = 1");
	    append_a78info ("set tvpal");
	}
	else
	    prerror ("set TV using invalid TV type");
    }
    else if (!strncmp (statement[2], "smartbranching\0", 14))
    {
	assertminimumargs (statement, "set smartbranching", 1);
	if (!strncmp (statement[3], "on", 2))
	    smartbranching = 1;
	else
	    smartbranching = 0;
    }
    else if (!strncmp (statement[2], "collisionwrap\0", 13))
    {
	assertminimumargs (statement, "set collisionwrap", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "collisionwrap = 1");
	    collisionwrap = 1;
	}
	else
	    collisionwrap = 0;
    }
    else if (!strncmp (statement[2], "romsize\0", 7))
    {
	assertminimumargs (statement, "set romsize", 1);
	set_romsize (statement[3]);
    }
    else if (!strncmp (statement[2], "dumpgraphics\0", 12))
    {
	assertminimumargs (statement, "set dumpgraphics", 1);
	removeCR (statement[3]);	//remove CR if present
	dumpgraphics = 1;
	dumpgraphicsaddr = strictatoi (statement[3]);
    }
    else if (!strncmp (statement[2], "bankset\0", 7))
    {
	assertminimumargs (statement, "set bankset", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "BANKSETROM = 1");
	    banksetrom = 1;
	    append_a78info ("set bankset");
	}
    }
    else if (!strncmp (statement[2], "softresetpause\0", 15))
    {
	assertminimumargs (statement, "set softresetpause", 1);
	if (!strncmp (statement[3], "off", 3))
	    strcpy (redefined_variables[numredefvars++], "SOFTRESETASPAUSEOFF = 1");
    }
    else if (!strncmp (statement[2], "snes0pause\0", 10))
    {
	assertminimumargs (statement, "set snes0pause", 1);
	if (!strncmp (statement[3], "on", 2))
	    strcpy (redefined_variables[numredefvars++], "SNES0PAUSE = 1");
    }
    else if (!strncmp (statement[2], "snes1pause\0", 10))
    {
	assertminimumargs (statement, "set snes1pause", 1);
	if (!strncmp (statement[3], "on", 2))
	    strcpy (redefined_variables[numredefvars++], "SNES1PAUSE = 1");
    }
    else if (!strncmp (statement[2], "snes#pause\0", 10))
    {
	assertminimumargs (statement, "set snes#pause", 1);
	if (!strncmp (statement[3], "on", 2))
	    strcpy (redefined_variables[numredefvars++], "SNESNPAUSE = 1");
    }
    else if (!strncmp (statement[2], "7800GDmenuoff\0", 13))
    {
	assertminimumargs (statement, "set 7800GDmenuoff", 1);
	if (!strncmp (statement[3], "0", 1))
	    append_a78info ("set mega78001");
	if (!strncmp (statement[3], "1", 1))
	    append_a78info ("set mega78002");
	if (!strncmp (statement[3], "all", 3))
        {
	    append_a78info ("set mega78001");
	    append_a78info ("set mega78002");
        }
    }
    else if (!strncmp (statement[2], "7800header\0", 10))
    {
	assertminimumargs (statement, "set 7800header", 1);
        char *settingstr=statement[3];
        int t;
        if(settingstr[0]=='\'')
        {
            settingstr++; // skip the initial quote
            for(t=0;t<SIZEOFSTATEMENT;t++)
            {
                if (settingstr[t]==0)
                    break;
                if (settingstr[t]=='^')
                    settingstr[t]=' ';
                if (settingstr[t]=='\'')
                {
                    settingstr[t]=0;
                    break;
                }
            }
        }
	append_a78info (settingstr);
    }
    else if (!strncmp (statement[2], "multibuttonpause\0", 16))
    {
	assertminimumargs (statement, "set multibuttonpause", 1);
	if (!strncmp (statement[3], "on", 2))
        {
            if (!multibutton)
	        prerror ("\"set multibuttonpause\" requires multibutton support.");
	    strcpy (redefined_variables[numredefvars++], "MULTIBUTTONPAUSE = 1");
        }
    }
    else if (!strncmp (statement[2], "basepath\0", 7))
    {
	assertminimumargs (statement, "set basepath", 1);
	removeCR (statement[3]);	//remove CR from the filename, if present
	strcpy (incbasepath, statement[3]);
    }
    else if (!strncmp (statement[2], "avoxvoice\0", 9))
    {
	assertminimumargs (statement, "set avoxvoice", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "AVOXVOICE = 1");
	}
    }
    else if (!strncmp (statement[2], "pausesilence\0", 12))
    {
	assertminimumargs (statement, "set pausesilence", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "PAUSESILENT = 1");
	}
    }
    else if (!strncmp (statement[2], "plotvaluepage\0", 13))
    {
	assertminimumargs (statement + 1, "set plotvaluepage", 1);	//+1 to skip "dlmemory"
	removeCR (statement[3]);	//remove CR from the value, if present
	sprintf (redefined_variables[numredefvars++], "PLOTVALUEPAGE = %s", statement[3]);
    }
    else if (!strncmp (statement[2], "extradlmemory\0", 13))
    {
	assertminimumargs (statement, "set extradlmemory", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "EXTRADLMEMORY = 1");
	}
    }
    else if (!strncmp (statement[2], "multibutton\0", 11))
    {
	assertminimumargs (statement, "set multibutton", 1);
	if (!strncmp (statement[3], "on", 2))
	{
            multibutton = 1;
	    strcpy (redefined_variables[numredefvars++], "MULTIBUTTON = 1");
	    sprintf (constants[numconstants++], "MULTIBUTTON");
	    strcpy (redefined_variables[numredefvars++], "MEGA7800SUPPORT = 1");
	    sprintf (constants[numconstants++], "MEGA7800SUPPORT");
	    strcpy (redefined_variables[numredefvars++], "SNES2ATARISUPPORT = 1");
	    sprintf (constants[numconstants++], "SNES2ATARISUPPORT");
	}
    }
    else if (!strncmp (statement[2], "tallsprite\0", 10))
    {
	assertminimumargs (statement, "set tallsprite", 1);
	if (!strncmp (statement[3], "off", 3))
	    tallspritemode = 0;
	else if (!strncmp (statement[3], "on", 2))
	    tallspritemode = 1;
	else if (!strncmp (statement[3], "spritesheet", 11))
	    tallspritemode = 2;
    }
    else if (!strncmp (statement[2], "deprecated", 10))
    {
	if (!strncmp (statement[3], "frameheight", 11))
	    deprecatedframeheight = 1;
	if (!strncmp (statement[3], "160bindexes", 11))
	    deprecated160bindexes = 1;
	if (!strncmp (statement[3], "boxcollision", 12))
	    deprecatedboxcollision = 1;
	if (!strncmp (statement[3], "onepass", 7))
	    maxpasses = 1;
    }
    else if (!strncmp (statement[2], "dlmemory\0", 8))
    {
	assertminimumargs (statement + 1, "set dlmemory", 2);	//+1 to skip "dlmemory"
	removeCR (statement[4]);	//remove CR if present
	if (banksetrom)
	    prerror ("\"set dlmemory\" isn't compatible with banksets.");
	else
	{
	    printf ("DLMEMSTART = %s\n", statement[3]);
	    printf ("DLMEMEND   = %s\n", statement[4]);
	}
    }
    else if (!strncmp (statement[2], "tightpackborder", 15))
    {
	assertminimumargs (statement + 1, "set tightpackborder", 1);
	removeCR (statement[3]);	//remove CR if present
	if (!strncmp (statement[3], "top", 3))
            TIGHTPACKBORDER = 0xEFFF;
	else
            TIGHTPACKBORDER = strictatoi (statement[3]);
    }
    else if (!strncmp (statement[2], "hssupport\0", 10))
    {
	assertminimumargs (statement, "set hssupport", 1);
	removeCR (statement[3]);	//remove CR from the filename, if present
	if ((statement[3] == 0) || (statement[3][0] == 0))
	    prerror ("'set hssupport' requires an argument");
	else
	{
	    int gameidval;
	    gameidval = strictatoi (statement[3]);
	    if (gameidval == 0)
		prerror ("'set hssupport' requires a numeric argument");

	    sprintf (redefined_variables[numredefvars++], "HSIDHI = $%02x", (gameidval / 256) & 0xFF);
	    sprintf (redefined_variables[numredefvars++], "HSIDLO = $%02x", gameidval & 0xFF);
	    strcpy (redefined_variables[numredefvars++], "HSSUPPORT = 1");
	    append_a78info ("set savekey");
	    remove ("7800hsgamename.asm");
	    remove ("7800hsgameranks.asm");
	    remove ("7800hsgamediffnames.asm");
	}

    }
    else if (!strncmp (statement[2], "hsdifficultytext", 16))
    {
	FILE *outfile;
	int s = 0, t = 0, strindex = 3, c;

	assertminimumargs (statement, "hsdifficultytext off", 1);
	removeCR (statement[3]);	//remove CR from the name
	if (strcmp (statement[3], "off") == 0)
	{
	    strcpy (redefined_variables[numredefvars++], "HSNOLEVELNAMES = 1");
	    return;
	}

	assertminimumargs (statement, "hsdifficultytext", 4);

	outfile = fopen ("7800hsgamediffnames.asm", "w");
	if (outfile == NULL)
	    prerror ("couldn't create 7800hsgamediffnames.asm file.");
	sprintf (redefined_variables[numredefvars++], "HSCUSTOMLEVELNAMES = 1");
	fprintf (outfile, "\n ifnconst isBANKSETBANK\n");
	fprintf (outfile, "highscoredifficultytextlen\n");
	fprintf (outfile, "  .byte ");
	for (strindex = 3; strindex < 7; strindex++)
	{
	    if ((statement[strindex] != 0) && (statement[strindex][0] != 0))
	    {
		removeCR (statement[strindex]);
		char *EOS = strrchr (statement[strindex], '\'');
		if (EOS)
		    *EOS = 0;
		strdelchr (statement[strindex], '\'');
		if (strindex > 3)
		    fprintf (outfile, ", ");
		fprintf (outfile, "%d", (int) strlen (statement[strindex]));
	    }
	}
	fprintf (outfile, "\n endif ; isBANKSETBANK\n");
	fprintf (outfile, "\n ifconst HSCHARSHERE\n");

	for (strindex = 3; strindex < 7; strindex++)
	{
	    if (strindex == 3)
		fprintf (outfile, "\neasylevelname\n  .byte ");
	    if (strindex == 4)
		fprintf (outfile, "\nmediumlevelname\n  .byte ");
	    if (strindex == 5)
		fprintf (outfile, "\nhardlevelname\n  .byte ");
	    if (strindex == 6)
		fprintf (outfile, "\nexpertlevelname\n  .byte ");
	    s = 0;

	    if ((statement[strindex] == 0) || (statement[strindex][0] == 0))
		continue;

	    for (t = 0; statement[strindex][t] != 0; t++)
	    {
		c = toupper (statement[strindex][t]);
		if ((c >= 'A') && (c <= 'Z'))
		{
		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", c - 'A' + 0);
		    s++;
		}
		if (c == '^')
		{
		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", 29);
		    s++;
		}
		if (c == ':')
		{

		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", 26);
		    s++;
		}
		if (c == '.')
		{

		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", 27);
		    s++;
		}
		if (c == '"')
		{

		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", 30);
		    s++;
		}
		if ((c >= '0') && (c <= '9'))
		{
		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", c - '0' + 33);
		    s++;
		}

	    }
	}
	fprintf (outfile, "\n");
	fprintf (outfile, "\n endif ; HSCHARSHERE\n");
	fclose (outfile);

    }
    else if (!strncmp (statement[2], "hscolorbase", 12))
    {
	assertminimumargs (statement, "set hscolorbase", 1);
	int hscolorval;
	hscolorval = strictatoi (statement[3]);
	sprintf (redefined_variables[numredefvars++], "HSCOLORCHASESTART = %d", hscolorval);
    }
    else if (!strncmp (statement[2], "hsseconds", 10))
    {
	assertminimumargs (statement, "set hsseconds", 1);
	int hsseconds;
	hsseconds = strictatoi (statement[3]);
	sprintf (redefined_variables[numredefvars++], "HSSECONDS = %d", hsseconds);
    }
    else if (!strncmp (statement[2], "hsscoresize", 11))
    {
	assertminimumargs (statement, "set hsscoresize", 1);
	int hsscoresize;
	hsscoresize = strictatoi (statement[3]);
	if ((hsscoresize < 1) || (hsscoresize > 6))
	    prerror ("invalid high score size used");
	sprintf (redefined_variables[numredefvars++], "HSSCORESIZE = %d", hsscoresize);
    }

    else if (!strncmp (statement[2], "hsgamename", 10))
    {
	assertminimumargs (statement, "set hsgamename", 1);

	FILE *outfile;
	int s = 0, t = 0, strindex = 3, c;
	outfile = fopen ("7800hsgamename.asm", "w");
	if (outfile == NULL)
	    prerror ("couldn't create 7800hsgamename.asm file.");
	fprintf (outfile, "  .byte ");
	removeCR (statement[3]);	//remove CR from the name
	while ((statement[strindex] != 0) && (statement[strindex][0] != 0))
	{
	    if (strindex > 3)
	    {
		fprintf (outfile, ",$%02x", 29);
		s++;
	    }

	    for (t = 0; statement[strindex][t] != 0; t++)
	    {
		c = toupper (statement[strindex][t]);
		if (c == '^')
		    c = ' ';
		if ((c >= 'A') && (c <= 'Z'))
		{
		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", c - 'A' + 0);
		    s++;
		}
		if (c == ':')
		{

		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", 26);
		    s++;
		}
		if (c == '.')
		{

		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", 27);
		    s++;
		}
		if (c == '-')
		{

		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", 28);
		    s++;
		}
		if (c == '"')
		{

		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", 30);
		    s++;
		}

		if (c == ' ')
		{

		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", 29);
		    s++;
		}
		if ((c >= '0') && (c <= '9'))
		{
		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", c - '0' + 33);
		    s++;
		}

	    }
	    strindex = strindex + 1;
	    removeCR (statement[strindex]);	//remove CR from the name
	}
	sprintf (redefined_variables[numredefvars++], "HSGAMENAMELEN = %d", s);
	fclose (outfile);

    }
    else if (!strncmp (statement[2], "hsgameranks", 11))
    {
	FILE *outfile;
	int s = 0, t = 0, strindex = 3, c;
	int value, count = 0;
	int val[3][100];
	int vallen[100];
	char tmpval[3], valstr[20];
	outfile = fopen ("7800hsgameranks.asm", "w");
	if (outfile == NULL)
	    prerror ("couldn't create 7800hsgameranks.asm file.");
	sprintf (redefined_variables[numredefvars++], "HSGAMERANKS = 1");
	removeCR (statement[3]);	//remove CR from the name
	tmpval[2] = 0;
	fprintf (outfile, "\n ifconst HSCHARSHERE\n");
	while ((statement[strindex] != 0) && (statement[strindex][0] != 0))
	{
	    //save these for later. we need to output them in individual tables...
	    value = strictatoi (statement[strindex]);
	    sprintf (valstr, "%06d", value);
	    tmpval[0] = valstr[4];
	    tmpval[1] = valstr[5];
	    val[2][count] = strictatoi (tmpval);
	    tmpval[0] = valstr[2];
	    tmpval[1] = valstr[3];
	    val[1][count] = strictatoi (tmpval);
	    tmpval[0] = valstr[0];
	    tmpval[1] = valstr[1];
	    val[0][count] = strictatoi (tmpval);

	    strindex = strindex + 1;
	    removeCR (statement[strindex]);
	    if ((statement[strindex] == 0) || (statement[strindex][0] == 0))
		break;

	    fprintf (outfile, "\nranklabel_%d\n .byte ", count);
	    s = 0;

	    for (t = 0; statement[strindex][t] != 0; t++)
	    {
		c = toupper (statement[strindex][t]);
		if ((c >= 'A') && (c <= 'Z'))
		{
		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", c - 'A' + 0);
		    s++;
		}
		if (c == '^')
		{
		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", 29);
		    s++;
		}
		if (c == ':')
		{

		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", 26);
		    s++;
		}
		if (c == '.')
		{

		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", 27);
		    s++;
		}
		if ((c >= '0') && (c <= '9'))
		{
		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", c - '0' + 33);
		    s++;
		}
		if (c == '?')
		{

		    if (s > 0)
			fprintf (outfile, ",");
		    fprintf (outfile, "$%02x", 45);
		    s++;
		}

	    }
	    vallen[count] = s;
	    count = count + 1;
	    strindex = strindex + 1;
	    removeCR (statement[strindex]);	//remove CR from the name
	}

	fprintf (outfile, "\n endif ; HSCHARSHERE\n");
	fprintf (outfile, "\n ifnconst isBANKSETBANK\n");
	fprintf (outfile, "\nranklabellengths\n .byte ");
	for (s = 0; s < count; s++)
	{
	    if (s > 0)
		fprintf (outfile, ",");
	    fprintf (outfile, "$%02x", vallen[s]);
	}
	fprintf (outfile, "\nranklabello\n .byte ");
	for (s = 0; s < count; s++)
	{
	    if (s > 0)
		fprintf (outfile, ",");
	    fprintf (outfile, "<ranklabel_%d", s);
	}
	fprintf (outfile, "\nranklabelhi\n .byte ");
	for (s = 0; s < count; s++)
	{
	    if (s > 0)
		fprintf (outfile, ",");
	    fprintf (outfile, ">ranklabel_%d", s);
	}

	// we need to output the value data as individual lists
	for (t = 0; t < 3; t++)
	{
	    fprintf (outfile, "\nrankvalue_%d\n .byte ", t);
	    for (s = 0; s < count; s++)
	    {
		if (s > 0)
		    fprintf (outfile, ",");
		fprintf (outfile, "$%02x", val[t][s]);
	    }
	}
	fprintf (outfile, "\n endif ;  isBANKSETBANK\n");
	fprintf (outfile, "\n");
	fclose (outfile);

    }

    else if (!strncmp (statement[2], "tiasfx", 6))
    {
	assertminimumargs (statement, "set tiasfx", 1);
	if (!strncmp (statement[3], "mono", 4))
	{
	    strcpy (redefined_variables[numredefvars++], "TIASFXMONO = 1");
	}
    }
    else if (!strncmp (statement[2], "xm", 2))
    {
	assertminimumargs (statement, "set xm", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    append_a78info ("set xm");
	}
    }
    else if (!strncmp (statement[2], "trackersupport", 14))
    {
	assertminimumargs (statement, "set trackersupport", 1);
	if (!strncmp (statement[3], "basic", 5))
	{
	    strcpy (redefined_variables[numredefvars++], "MUSICTRACKER = 1");
	}
	else if (!strncmp (statement[3], "rmt", 3))
	{
	    strcpy (redefined_variables[numredefvars++], "RMT = 1");
	}
    }
    else if (!strncmp (statement[2], "rmtvolume", 9))
    {
	assertminimumargs (statement, "set rmtvolume", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "RMTVOLUME = 1");
	    strcpy (redefined_variables[numredefvars++], "FOURBITFADE = 1");
	}
    }
    else if (!strncmp (statement[2], "rmtspeed", 8))
    {
	assertminimumargs (statement, "set rmtspeed", 1);
	if (!strncmp (statement[3], "pal", 3))
	    strcpy (redefined_variables[numredefvars++], "RMTPALSPEED = 1");
	if (!strncmp (statement[3], "off", 3))
	    strcpy (redefined_variables[numredefvars++], "RMTOFFSPEED = 1");
    }
    else if (!strncmp (statement[2], "tiavolume", 9))
    {
	assertminimumargs (statement, "set tiavolume", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "TIAVOLUME = 1");
	    strcpy (redefined_variables[numredefvars++], "FOURBITFADE = 1");
	}
    }
    else if (!strncmp (statement[2], "fourbitfade", 11))
    {
	assertminimumargs (statement, "set fourbitfade", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "FOURBITFADE = 1");
	}
    }
    else if (!strncmp (statement[2], "pokeysupport", 12))
    {
	assertminimumargs (statement, "set pokeysupport", 1);
	if (strncmp (statement[3], "off", 3))
	{
	    strcpy (redefined_variables[numredefvars++], "pokeysupport = 1");
	    sprintf (constants[numconstants++], "PAUDF0");
	    sprintf (constants[numconstants++], "PAUDC0");
	    sprintf (constants[numconstants++], "PAUDF1");
	    sprintf (constants[numconstants++], "PAUDC1");
	    sprintf (constants[numconstants++], "PAUDF2");
	    sprintf (constants[numconstants++], "PAUDC2");
	    sprintf (constants[numconstants++], "PAUDF3");
	    sprintf (constants[numconstants++], "PAUDC3");
	    sprintf (constants[numconstants++], "PAUDCTL");
	    sprintf (constants[numconstants++], "PRANDOM");
	    sprintf (constants[numconstants++], "PSKCTL");

	    if ((!strncmp (statement[3], "on", 2)) || (!strncmp (statement[3], "auto", 4)))
	    {
		strcpy (redefined_variables[numredefvars++], "pokeysupport = 1");
		append_a78info ("set pokey@450");
		prwarn ("pokey autodetection is deprecated.");
	    }
	    else if (!strncmp (statement[3], "$450", 4))
	    {
		strcpy (redefined_variables[numredefvars++], "pokeysupport = 1");
		strcpy (redefined_variables[numredefvars++], "pokeyaddress = $450");
		append_a78info ("set pokey@450");
	    }
	    else if (!strncmp (statement[3], "$800", 4))
	    {
		strcpy (redefined_variables[numredefvars++], "pokeysupport = 1");
		strcpy (redefined_variables[numredefvars++], "pokeyaddress = $800");
		append_a78info ("set pokey@800");
	    }
	    else if (!strncmp (statement[3], "$4000", 5))
	    {
		strcpy (redefined_variables[numredefvars++], "pokeysupport = 1");
		strcpy (redefined_variables[numredefvars++], "pokeyaddress = $4000");
		append_a78info ("set pokey@4000");
	    }
	    else		// some other address
	    {
		strcpy (redefined_variables[numredefvars++], "pokeysupport = 1");
		snprintf (redefined_variables[numredefvars++], 30, "pokeyaddress = %s", statement[3]);
	    }
	}

    }
    else if (!strncmp (statement[2], "pokeysfxsupport", 14))
    {
	assertminimumargs (statement, "set pokeysfxsupport", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "pokeysfxsupport = 1");
	}
    }
    else if (!strncmp (statement[2], "hscsupport", 10))
    {
	assertminimumargs (statement, "set hscsupport", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    append_a78info ("set hsc");
	}
    }

    else if (!strncmp (statement[2], "doublewide", 10))
    {
	assertminimumargs (statement, "set doublewide", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "DOUBLEWIDE = 1");
	    doublewide = 1;
	}
    }
    else if (!strncmp (statement[2], "plotvalueonscreen", 17))
    {
	assertminimumargs (statement, "set plotvalueonscreen", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "plotvalueonscreen = 1");
	}
    }
    else if (!strncmp (statement[2], "frameskipfix", 12))
    {
	assertminimumargs (statement, "set frameskipfix", 1);
	if ((!strncmp (statement[3], "on", 2)) || (!strncmp (statement[3], "strong", 5)))
	{
	    strcpy (redefined_variables[numredefvars++], "FRAMESKIPGLITCHFIX = 1");
	}
	if (!strncmp (statement[3], "weak", 4))
	{
	    strcpy (redefined_variables[numredefvars++], "FRAMESKIPGLITCHFIXWEAK = 1");
	}
    }
    else if (!strncmp (statement[2], "zoneprotection", 14))
    {
	assertminimumargs (statement, "set zoneprotection", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "CHECKOVERWRITE = 1");
	}
    }

    else if (!strncmp (statement[2], "pauseroutine", 12))
    {
	assertminimumargs (statement, "set pauseroutine", 1);
	if (!strncmp (statement[3], "off", 3))
	{
	    strcpy (redefined_variables[numredefvars++], "pauseroutineoff = 1");
	}
    }
    else if (!strncmp (statement[2], "paddlerange", 11))
    {
	assertminimumargs (statement, "set paddlerange", 1);
	char outstr[256];
	int value = strictatoi (statement[3]);
	if ((value < 1) || (value > 240))
	    prerror ("'set paddlerange must have an argument >0 and <241");
	sprintf (outstr, "PADDLERANGE = %d", value);
	strcpy (redefined_variables[numredefvars++], outstr);
    }
    else if (!strncmp (statement[2], "paddlepair", 10))
    {
	assertminimumargs (statement, "set paddlepair", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "TWOPADDLESUPPORT = 1");
	}
    }
    else if (!strncmp (statement[2], "paddlescalex2", 13))
    {
	assertminimumargs (statement, "set paddlescalex2", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "PADDLESCALEX2 = 1");
	}
    }
    else if (!strncmp (statement[2], "mousetime", 9))
    {
	assertminimumargs (statement, "set mousetime", 1);
	char outstr[256];
	int value = strictatoi (statement[3]);
	if ((value < 1) || (value > 240))
	    prerror ("'set mousetime must have an argument >0 and <241");
	sprintf (outstr, "MOUSETIME = %d", value);
	strcpy (redefined_variables[numredefvars++], outstr);
    }
    else if (!strncmp (statement[2], "mousexonly", 10))
    {
	assertminimumargs (statement, "set mousexonly", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "MOUSEXONLY = 1");
	}
    }
    else if (!strncmp (statement[2], "traktime", 9))
    {
	assertminimumargs (statement, "set traktime", 1);
	char outstr[256];
	int value = strictatoi (statement[3]);
	if ((value < 1) || (value > 240))
	    prerror ("'set traktime must have an argument >0 and <241");
	sprintf (outstr, "TRAKTIME = %d", value);
	strcpy (redefined_variables[numredefvars++], outstr);
    }
    else if (!strncmp (statement[2], "trakxonly", 10))
    {
	assertminimumargs (statement, "set trakxonly", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "TRAKXONLY = 1");
	}
    }
    else if (!strncmp (statement[2], "drivingboost", 12))
    {
	assertminimumargs (statement, "set drivingboost", 1);
	if (!strncmp (statement[3], "on", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "DRIVINGBOOST = 1");
	}
    }
    else if (!strncmp (statement[2], "backupstyle", 11))
    {
	assertminimumargs (statement, "set backupstyle", 1);
	if (!strncmp (statement[3], "single", 6))
            SetBackupStyle(BACKUPSTYLE_SINGLE);
    }
    else if (!strncmp (statement[2], "backupfile", 10))
    {
        if (!backupflag)
        {
	    assertminimumargs (statement, "set backupfile", 1);
            // set backupname to statement[3]
            removeCR (statement[3]);
	    char *backupstr=statement[3];
            if (*backupstr == '\'')
                backupstr++;
            int t;
            for(t=0;t<SIZEOFSTATEMENT;t++)
            {
                if(backupstr[t]==0)
                    break;
                if(backupstr[t]=='^')
                    backupstr[t]=' ';
            }
            if(backupstr[t-1]=='\'')
                backupstr[t-1]=0;
            if(OpenArchive(backupstr)==FALSE)
	        prerror ("set backupfile failed - couldn't write to '%s'",backupstr);
            backupflag = TRUE;
            backupthisfile(backupname);
        }
    }
    else if (!strncmp (statement[2], "screenheight", 12))
    {
	assertminimumargs (statement, "set screenheight", 1);
	if (!strncmp (statement[3], "192", 3))
	{
	    strcpy (redefined_variables[numredefvars++], "SCREENHEIGHT = 192");
	}
	else if (!strncmp (statement[3], "208", 3))
	{
	    strcpy (redefined_variables[numredefvars++], "SCREENHEIGHT = 208");
	}
	else if (!strncmp (statement[3], "224", 3))
	{
	    strcpy (redefined_variables[numredefvars++], "SCREENHEIGHT = 224");
	}
	else
	    prerror ("set using illegal screen height... valid values are 192, 208, and 224");
    }

    else if (!strncmp (statement[2], "debug", 5))
    {
	if (!strncmp (statement[3], "color", 5))
	{
	    strcpy (redefined_variables[numredefvars++], "DEBUGCOLOR = 1");
	}
	if (!strncmp (statement[3], "frames", 6))
	{
	    strcpy (redefined_variables[numredefvars++], "DEBUGFRAMES = 1");
	}
	if (!strncmp (statement[3], "interrupt", 9))
	{
	    strcpy (redefined_variables[numredefvars++], "DEBUGINTERRUPT = 1");
	}
    }

    else if (!strncmp (statement[2], "zoneheight", 10))
    {
	assertminimumargs (statement, "set zoneheight", 1);
	if (!strncmp (statement[3], "8", 1))
	{
	    strcpy (redefined_variables[numredefvars++], "ZONEHEIGHT = 8");
	    zoneheight = 8;
	}
	else if (!strncmp (statement[3], "16", 2))
	{
	    strcpy (redefined_variables[numredefvars++], "ZONEHEIGHT = 16");
	    zoneheight = 16;
	}
	else
	    prerror ("set using illegal zone height... valid values are 8 and 16");
    }
    else if (!strncmp (statement[2], "mcpdevcart", 10))
    {
	assertminimumargs (statement, "set mcpdevcart", 1);
	if (strncmp (statement[3], "off", 3) != 0)
	    strcpy (redefined_variables[numredefvars++], "MCPDEVCART = 1");
    }
    else if (!strncmp (statement[2], "canary", 6))
    {
	assertminimumargs (statement, "set canary", 1);
	if (strncmp (statement[3], "off", 3) != 0)
	    strcpy (redefined_variables[numredefvars++], "CANARYOFF = 1");
    }
    else if (!strncmp (statement[2], "crashdump", 6))
    {
	assertminimumargs (statement, "set crashdump", 1);
	if (strncmp (statement[3], "off", 3) != 0)
	    strcpy (redefined_variables[numredefvars++], "CRASHDUMP = 1");
    }
    else if (!strncmp (statement[2], "breakprotect", 12))
    {
	assertminimumargs (statement, "set breakprotect", 1);
	if (strncmp (statement[3], "off", 3) != 0)
	    strcpy (redefined_variables[numredefvars++], "BREAKPROTECTOFF = 1");
    }
    else if (!strncmp (statement[2], "optimization", 12))
    {
	assertminimumargs (statement, "set optimization", 1);
	if (!strncmp (statement[3], "speed", 5))
	{
	    optimization |= 1;
	}
	if (!strncmp (statement[3], "size", 4))
	{
	    optimization |= 2;
	}
	if (!strncmp (statement[3], "noinlinedata", 12))
	{
	    optimization |= 4;
	}
	if (!strncmp (statement[3], "inlinerand", 10))
	{
	    optimization |= 8;
	}
	if (!strncmp (statement[3], "none\0", 4))
	{
	    optimization = 0;
	}
    }
    else
	prerror ("unknown set parameter");
}

void echo (char **statement)
{
    int t;
    for (t = 1; t < 100; t++)
    {
	if ((statement[t] != NULL) || (statement[t][0] != 0))
	{
	    removeCR (statement[t]);
	    printf (" %s ", statement[t]);
	}
	else
	    break;
    }
    printf ("\n");
}

void rem (char **statement)
{
    if (!strncmp (statement[2], "smartbranching\0", 14))
    {
	if (!strncmp (statement[3], "on\0", 2))
	    smartbranching = 1;
	else
	    smartbranching = 0;
    }
}

void dopop ()
{
    printf ("  pla\n");
    printf ("  pla\n");
}


void bmi (char *linenumber)
{
    removeCR (linenumber);
    if (smartbranching)
    {
	printf (" if ((* - .%s) < 127) && ((* - .%s) > -128)\n  bmi .%s\n", linenumber, linenumber, linenumber);
	// branches might be allowed as below - check carefully to make sure!
	// printf(" if ((* - .%s) < 127) && ((* - .%s) > -129)\n  bmi .%s\n",linenumber,linenumber,linenumber);
	printf (" else\n  bpl .%dskip%s\n  jmp .%s\n", branchtargetnumber, linenumber, linenumber);
	printf (".%dskip%s\n", branchtargetnumber++, linenumber);
	printf (" endif\n");
    }
    else
    {
	printf ("  bmi .%s\n", linenumber);
    }
}

void bpl (char *linenumber)
{
    removeCR (linenumber);
    if (smartbranching)
    {
	printf (" if ((* - .%s) < 127) && ((* - .%s) > -128)\n  bpl .%s\n", linenumber, linenumber, linenumber);
	printf (" else\n  bmi .%dskip%s\n  jmp .%s\n", branchtargetnumber, linenumber, linenumber);
	printf (".%dskip%s\n", branchtargetnumber++, linenumber);
	printf (" endif\n");
    }
    else
    {
	printf ("  bpl .%s\n", linenumber);
    }
}

void bne (char *linenumber)
{
    removeCR (linenumber);
    if (smartbranching)
    {
	printf (" if ((* - .%s) < 127) && ((* - .%s) > -128)\n  bne .%s\n", linenumber, linenumber, linenumber);
	printf (" else\n  beq .%dskip%s\n  jmp .%s\n", branchtargetnumber, linenumber, linenumber);
	printf (".%dskip%s\n", branchtargetnumber++, linenumber);
	printf (" endif\n");
    }
    else
    {
	printf ("  bne .%s\n", linenumber);
    }
}

void beq (char *linenumber)
{
    removeCR (linenumber);
    if (smartbranching)
    {
	printf (" if ((* - .%s) < 127) && ((* - .%s) > -128)\n  beq .%s\n", linenumber, linenumber, linenumber);
	printf (" else\n  bne .%dskip%s\n  jmp .%s\n", branchtargetnumber, linenumber, linenumber);
	printf (".%dskip%s\n", branchtargetnumber++, linenumber);
	printf (" endif\n");
    }
    else
    {
	printf ("  beq .%s\n", linenumber);
    }
}

void bcc (char *linenumber)
{
    removeCR (linenumber);
    if (smartbranching)
    {
	printf (" if ((* - .%s) < 127) && ((* - .%s) > -128)\n  bcc .%s\n", linenumber, linenumber, linenumber);
	printf (" else\n  bcs .%dskip%s\n  jmp .%s\n", branchtargetnumber, linenumber, linenumber);
	printf (".%dskip%s\n", branchtargetnumber++, linenumber);
	printf (" endif\n");
    }
    else
    {
	printf ("  bcc .%s\n", linenumber);
    }

}

void bcs (char *linenumber)
{
    removeCR (linenumber);
    if (smartbranching)
    {
	printf (" if ((* - .%s) < 127) && ((* - .%s) > -128)\n  bcs .%s\n", linenumber, linenumber, linenumber);
	printf (" else\n  bcc .%dskip%s\n  jmp .%s\n", branchtargetnumber, linenumber, linenumber);
	printf (".%dskip%s\n", branchtargetnumber++, linenumber);
	printf (" endif\n");
    }
    else
    {
	printf ("  bcs .%s\n", linenumber);
    }
}

void bvc (char *linenumber)
{
    removeCR (linenumber);
    if (smartbranching)
    {
	printf (" if ((* - .%s) < 127) && ((* - .%s) > -128)\n  bvc .%s\n", linenumber, linenumber, linenumber);
	printf (" else\n  bvs .%dskip%s\n  jmp .%s\n", branchtargetnumber, linenumber, linenumber);
	printf (".%dskip%s\n", branchtargetnumber++, linenumber);
	printf (" endif\n");
    }
    else
    {
	printf ("  bvc .%s\n", linenumber);
    }
}

void bvs (char *linenumber)
{
    removeCR (linenumber);
    if (smartbranching)
    {
	printf (" if ((* - .%s) < 127) && ((* - .%s) > -128)\n  bvs .%s\n", linenumber, linenumber, linenumber);
	printf (" else\n  bvc .%dskip%s\n  jmp .%s\n", branchtargetnumber, linenumber, linenumber);
	printf (".%dskip%s\n", branchtargetnumber++, linenumber);
	printf (" endif\n");
    }
    else
    {
	printf ("  bvs .%s\n", linenumber);
    }
}

void clearscreen ()
{
    invalidate_Areg ();
    jsr ("clearscreen");
}


void drawscreen (void)
{
    invalidate_Areg ();
    jsr ("drawscreen");
}

void drawwait (void)
{
    invalidate_Areg ();
    jsr ("drawwait");
}

void savescreen (void)
{
    invalidate_Areg ();
    jsr ("savescreen");
}

void changedmaholes (char **statement)
{

    //   1              2
    // changedmaholes value
    // accepted values are "enable" and "disable"

    assertminimumargs (statement, "changedmaholes", 1);

    if (!changedmaholescalled)
    {
	changedmaholescalled = 1;
	strcpy (redefined_variables[numredefvars++], "CHANGEDMAHOLES = 1");
    }

    if (!strncmp(statement[2], "disable",7))
        printf ("    jsr removedmaholes\n");
    else if (!strncmp(statement[2], "enable",6))
        printf ("    jsr createdmaholes\n");
    else
	prerror ("Unrecognized argument '%s' was provided to changedmaholes.\n", statement[2]);
}


void restorescreen (void)
{
    invalidate_Areg ();
    jsr ("restorescreen");
}

void orgprintf (char *format, ...)
{
    //    orgprintf()
    //    printf to stdout. If the bankset format is selected,
    //    then *also* printf to the bankset asm file.

    char buffer[4096];
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 4095, format, args);
    va_end (args);

    printf ("%s", buffer);

    if (banksetrom == 0)
	return;

    FILE *banksetout;
    banksetout = fopen (BANKSETASM, "ab");
    if (banksetout == NULL)
	prerror ("Couldn't open bankset assembly file %s for update\n", BANKSETASM);
    fprintf (banksetout, "%s", buffer);
    fclose (banksetout);
}

void gfxprintf (char *format, ...)
{
    // gfxprintf() 
    //    print the gfx assembly code to *either* stdout or the bankset assembly file,
    //    (depending if banksets are selected or not)

    char buffer[4096];
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 4095, format, args);
    va_end (args);

    if (banksetrom == 0)
    {
	printf ("%s", buffer);
	return;
    }

    FILE *banksetout;
    banksetout = fopen (BANKSETASM, "ab");
    if (banksetout == NULL)
	prerror ("Couldn't open bankset assembly file %s for update\n", BANKSETASM);
    fprintf (banksetout, "%s", buffer);
    fclose (banksetout);
}

void prinit()
{
    if(stderrfilepointer)
        return;
    stderrfilepointer = fopen ("message.log","wb");
    if (!stderrfilepointer)
        prerror("unable to open 'message.log'");
}

void prout (char *format, ...)
{
    char buffer[1024];
    prinit();
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 1023, format, args);
    fprintf (stderrfilepointer, "%s", buffer);
    va_end (args);
}

void prinfo (char *format, ...)
{
    char buffer[1024];
    prinit();
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 1023, format, args);
    fprintf (stderrfilepointer, "*** (): INFO, %s\n", buffer);
    va_end (args);
}

void prwarn (char *format, ...)
{
    char buffer[1024];
    prinit();
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 1023, format, args);
    if (savelevel)
        fprintf (stderrfilepointer, "*** (%s:%d): WARNING, %s\n", savelinesname[savelevel], line, buffer);
    else
        fprintf (stderrfilepointer, "*** (%d): WARNING, %s\n", line, buffer);
    va_end (args);
}

void prerror (char *format, ...)
{
    char buffer[1024];
    prinit();
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 1023, format, args);
    if (savelevel)
        fprintf (stderrfilepointer, "*** (%s:%d): ERROR, %s\n", savelinesname[savelevel], line, buffer);
    else
        fprintf (stderrfilepointer, "*** (%d): ERROR, %s\n", line, buffer);
    va_end (args);
    lastrites();
    exit (1);
}

int printimmed (char *value)
{
    int immed = isimmed (value);
    if (immed)
	printf ("#");
    return immed;
}

int isimmed (char *value)
{
    // search queue of constants defined in any pass
    int i;
    //removeCR(value);

    for (i = 0; i < MAXCONSTANTS; ++i)
    {
        if(constants[i][0]==0)
            break;
	if (!strcmp (value, constants[i]))
	{
	    // a constant should be treated as an immediate
	    return 1;
	}
    }
    if ((value[0] == '$') || (value[0] == '%') || (value[0] < (unsigned char) 0x3A))
    {
	return 1;
    }
    else
	return 0;
}

int isconstantdefined (char *value)
{
    // search queue of constants defined in this pass
    int i;
    //removeCR(value);

    for (i = 0; i < numconstants; ++i)
    {
	if (!strcmp (value, constants[i]))
	{
	    // a constant should be treated as an immediate
	    return 1;
	}
    }
    if ((value[0] == '$') || (value[0] == '%') || (value[0] < (unsigned char) 0x3A))
    {
	return 1;
    }
    else
	return 0;
}

int number (unsigned char value)
{
    return ((int) value) - '0';
}


void removeCR (char *linenumber)	// remove trailing CR from string
{
    char *CR;
    if (linenumber == 0)
	return;
    CR = strrchr (linenumber, (unsigned char) 0x0A);
    if (CR != NULL)
	*CR = 0;
    CR = strrchr (linenumber, (unsigned char) 0x0D);
    if (CR != NULL)
	*CR = 0;
}

void remove_trailing_commas (char *linenumber)	// remove trailing commas from string
{
    int i;
    for (i = strlen (linenumber) - 1; i > 0; i--)
    {
	if ((linenumber[i] != ',') &&
	    (linenumber[i] != ' ') &&
	    (linenumber[i] != (unsigned char) 0x0A) && (linenumber[i] != (unsigned char) 0x0D))
	    break;
	if (linenumber[i] == ',')
	{
	    linenumber[i] = ' ';
	    break;
	}
    }
}

void header_write (FILE * header, char *filename)
{
    int i;
    if ((header = fopen (filename, "w")) == NULL)	// open file
    {
	prerror ("can't open '%s' for writing", filename);
    }

    strcpy (redefined_variables[numredefvars],
	    "; This file contains variable mapping and other information for the current project.\n");

    for (i = numredefvars; i >= 0; i--)
    {
	fprintf (header, "%s\n", redefined_variables[i]);
    }
    fclose (header);

}

void lastrites()
{
    if(backupflag)
    {
        CloseArchive();
        prout ("Backed up %d project files.\n",backupcount);
    }
    prinit();
    fclose(stderrfilepointer);
    stderrfilepointer = fopen ("message.log","rb");
    int logchar;
    logchar = fgetc (stderrfilepointer);
    while (logchar != EOF)
    {
        fputc(logchar,stderr);
        logchar = fgetc (stderrfilepointer);
    }
    fclose(stderrfilepointer);
    remove("message.log");
    remove("7800hole.0.asm");
    remove("7800hole.1.asm");
    remove("7800hole.2.asm");
}
