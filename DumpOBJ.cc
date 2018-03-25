/**************************************************************
*
*  DumpOBJ 2.0
*
*  By Phil Montoya
*  Translation to ORCA/C & 2.0 enhancements by Mike Westerfield.
*
*  DumpOBJ 1.1 Copyright November 1986-88
*  By the Byte Works, Inc.
*
*  DumpOBJ 2.0 Copyright 1991
*  Byte Works, Inc.
*
****************************************************************
*
*  Version 2.0.1, 7 July 94
*  Mike Westerfield
*
*  1. Added resource fork.
*
****************************************************************
*
*  Version 2.0.2, June 96
*  Mike Westerfield
*
*  1. Fixed hexadecimal dump bug.
*
****************************************************************/

#pragma keep "DumpOBJ"
#pragma optimize 9

/* GS specific declarations */
/*--------------------------*/

extern pascal void PDosInt(); 
int toolerror(void);

#define WRITE_CONSOLE(parm)     (PDosInt(0x015A,parm))
#define GetFileInfoGS(pBlockPtr)  PDosInt(0x2006,pBlockPtr)

#pragma lint -1

/* Standard C Libraries */
/*----------------------*/

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Constants */
/*-----------*/

#define BOOLEAN	int			/* boolean variables */
#define TRUE 1
#define FALSE 0
#define FILEMAX 1024			/* max length of file name */
#define NAMEMAX 512			/* maximum size of variable len name */
#define NAMESIZE 11			/* size of label name */
#define OBJFL 177			/* minimum object file type */
#define MAXFL 190			/* maximum object file type */
#define LIBFL 178			/* library file type */
#define BLKSIZE 512			/* size of block */
#define COP 2   			/* COP opcode */
#define REP 194				/* REP opcode */
#define SEP 226				/* SEP opcode */

/* Object File Opcodes */
/*---------------------*/

#define OPALIGN 224			/* align to a boundry */
#define OPORG 225			/* set code origin */
#define OPRELOC 226			/* relocation entry */
#define OPINTSEG 227			/* inserseg record */
#define OPUSING 228			/* use data area */
#define OPSTRONG 229			/* force segment inclusion */
#define OPGLOBAL 230			/* define global label */
#define OPGEQU 231			/* define global constant */
#define OPMEM 232			/* reserve absolute memory */
#define OPEXPR 235			/* computed value */
#define OPZEXPR 236			/* computed zero page value */
#define OPBEXPR 237			/* computed local bank value */
#define OPRELEXPR 238			/* computed relative ofset */
#define OPLOCAL 239			/* define local label */
#define OPEQU 240			/* define local constant */
#define OPDS 241			/* define storage */
#define OPLONG 242			/* long constant > $DF bytes */
#define OPLEXPR 243			/* outer segment expression */
#define OPENTRY 244			/* run time lib entry dictionary */
#define OPSRELOC 245			/* short relocation entry */
#define OPSINTSEG 246			/* short interseg entry */
#define OPSUPER 247			/* super compressed records */

/* Program variables */
/*-------------------*/

char fname[FILEMAX];			/* input file name buffer */
char nlist[NAMEMAX];			/* names list */
char symbol[NAMEMAX];			/* symbol name buffer */

BOOLEAN names;				/* names list flag */
BOOLEAN sTable;				/* found a COP symbol table? */

int act_lablen;				/* actual length of label read */
int status;				/* shell return status */
int namc;				/* name argument counter */
int ftype;				/* current file type */

long blocks;				/* used to get to next segment */
long constant;				/* constant byte counter and flag */
long opcode;				/* opcode storage */
long segmark;				/* start of segment marker */
long count;				/* file counter */
long number;				/* number read variable */
long pc;				/* segment program counter */

long tkind;				/* type of node for tree */
long *root;				/* root pointer for expression tree */

FILE *input;				/* input file */

/* Output Control Settings */
/*-------------------------*/

int format;				/* format of output
						0 opcode dump
						1 dissassembly dump
						2 hex dump
					*/
BOOLEAN shorth;				/* print short headers? */
BOOLEAN ireg;				/* long index registers? */
BOOLEAN mreg;				/* long accumulator? */
BOOLEAN longc;				/* print long constants? */
BOOLEAN header;				/* print segment headers? */
BOOLEAN body;				/* print segment bodies? */
BOOLEAN checkf;				/* check file types? */
BOOLEAN colf;				/* print displacement numbers? */

/* Object Module Header */
/*----------------------*/

int kind;				/* kind of segment */

long blkcnt;				/* number of block in segment */
long resspc;				/* reserved space at end of segment */
long length;				/* length of finished segment */

int kind1;				/* this is kind for version 1 */
int lablen;				/* label length */
int numlen;				/* number length */
int version;				/* version number */

long banksize;				/* banksize */

int kind2;				/* this is kind for version 2 */
int unused;				/* undefined field */

long pgorg;				/* program org */
long align;				/* alignment factor */

int numsex;				/* significance of numbers */
int lang_card;				/* language card */

int segnum;				/* segment number */

long ent_disp;				/* disp to beg of segment */

int nam_disp;				/* displacment to names */
int bod_disp;				/* displacment to body */

char loadname[NAMESIZE];		/* load file name */
char segname[NAMEMAX];			/* segment name */

/* Opcode arrays */
/*---------------*/

char opKind[] = {5,8,5,14,2,2,2,13,5,1,4,5,2,2,2,3,
                 17,6,12,15,2,9,9,7,5,10,4,5,2,9,9,11,
                 2,8,2,14,2,2,2,13,5,1,4,5,2,2,2,3,
                 17,6,12,15,9,9,9,7,5,10,4,5,9,9,9,11,
                 5,8,5,14,16,2,2,13,5,1,4,5,2,2,2,3,
                 17,6,12,15,16,9,9,7,5,10,5,5,3,9,9,11,
                 5,8,5,14,2,2,2,13,5,1,4,5,12,2,2,3,
                 17,6,12,15,9,9,9,7,5,10,5,5,8,9,9,11,
                 17,8,17,14,2,2,2,13,5,1,5,5,2,2,2,3,
                 17,6,12,15,9,9,10,7,5,10,5,5,2,9,9,11,
                 1,8,1,14,2,2,2,13,5,1,5,5,2,2,2,3,
                 17,6,12,15,9,9,10,7,5,10,5,5,9,9,10,11,
                 1,8,1,14,2,2,2,13,5,1,5,5,2,2,2,3,
                 17,6,12,15,5,9,9,7,5,10,5,5,12,9,9,11,
                 1,8,1,14,2,2,2,13,5,1,5,5,2,2,2,3,
                 17,6,12,15,5,9,9,7,5,10,5,5,8,9,9,11};

char opLen[] = {1,1,1,1,1,1,1,1,0,4,0,0,2,2,2,3,
                1,1,1,1,1,1,1,1,0,2,0,0,2,2,2,3,
                2,1,3,1,1,1,1,1,0,4,0,0,2,2,2,3,
                1,1,1,1,1,1,1,1,0,2,0,0,2,2,2,3,
                0,1,0,1,1,1,1,1,0,4,0,0,2,2,2,3,
                1,1,1,1,1,1,1,1,0,2,0,0,3,2,2,3,
                0,1,2,1,1,1,1,1,0,4,0,0,2,2,2,3,
                1,1,1,1,1,1,1,1,0,2,0,0,2,2,2,3,
                1,1,2,1,1,1,1,1,0,4,0,0,2,2,2,3,
                1,1,1,1,1,1,1,1,0,2,0,0,2,2,2,3,
                5,1,5,1,1,1,1,1,0,4,0,0,2,2,2,3,
                1,1,1,1,1,1,1,1,0,2,0,0,2,2,2,3,
                5,1,1,1,1,1,1,1,0,4,0,0,2,2,2,3,
                1,1,1,1,1,1,1,1,0,2,0,0,2,2,2,3,
                5,1,1,1,1,1,1,1,0,4,0,0,2,2,2,3,
                1,1,1,1,2,1,1,1,0,2,0,0,2,2,2,3};

char opName[] = "BRKORACOPORATSBORAASLORAPHPORAASLPHDTSBORAASLORA"
                "BPLORAORAORATRBORAASLORACLCORAINCTCSTRBORAASLORA"
                "JSRANDJSLANDBITANDROLANDPLPANDROLPLDBITANDROLAND"
                "BMIANDANDANDBITANDROLANDSECANDDECTSCBITANDROLAND"
                "RTIEORWDMEORMVPEORLSREORPHAEORLSRPHKJMPEORLSREOR"
                "BVCEOREOREORMVNEORLSREORCLIEORPHYTCDJMPEORLSREOR"
                "RTSADCPERADCSTZADCRORADCPLAADCRORRTLJMPADCRORADC"
                "BVSADCADCADCSTZADCRORADCSEIADCPLYTDCJMPADCRORADC"
                "BRASTABRLSTASTYSTASTXSTADEYBITTXAPHBSTYSTASTXSTA"
                "BCCSTASTASTASTYSTASTXSTATYASTATXSTXYSTZSTASTZSTA"
                "LDYLDALDXLDALDYLDALDXLDATAYLDATAXPLBLDYLDALDXLDA"
                "BCSLDALDALDALDYLDALDXLDACLVLDATSXTYXLDYLDALDXLDA"
                "CPYCMPREPCMPCPYCMPDECCMPINYCMPDEXWAICPYCMPDECCMP"
                "BNECMPCMPCMPPEICMPDECCMPCLDCMPPHXSTPJMLCMPDECCMP"
                "CPXSBCSEPSBCCPXSBCINCSBCINXSBCNOPXBACPXSBCINCSBC"
                "BEQSBCSBCSBCPEASBCINCSBCSEDSBCPLXXCEJSRSBCINCSBC";

/**** Utility Subroutines *****************************************/

/***************************************************************
*
*  fgeti - read an integer from the file
*
*  Inputs:
*       f - file to read from
*
*  Outputs:
*	Returns integer read
*
***************************************************************/

int fgeti (FILE *f)

{
return fgetc(f) | (fgetc(f) << 8);
}

/***************************************************************
*
*  Error - Writes out an error
*
***************************************************************/

void Error (int errnum, char *str)

{
status = -1;
switch (errnum) {
   case 1: fputs("Format type not supported", stderr);
           break;
   case 2: fputs("Could not read segment header", stderr);
           break;
   case 3: fputs("Segment version not supported", stderr);
           break;
   case 4: fputs("Bad input ", stderr);
           break;
   case 5: fputs("Could not open file ", stderr);
           break;
   case 6: fputs("Not an object file ", stderr);
           break;
   case 7: fputs("Unknown Opcode", stderr);
           break;
   case 8: fputs("Illegal header entry", stderr);
           break;
   case 9: fputs("End of file encountered", stderr);
   default: break;
   }
if (str[0] != ' ')
   fputs(str, stderr);
fputc('\n', stderr);
}

/***************************************************************
*
*  CheckESC - Checks to see if 'escape' is pressed
*
*  Notes: Returns to shell if open-apple period has been pressed.
*
***************************************************************/

void CheckESC (void)

{
char *keyboard;
char *strobe;
char *flags;
struct {
   int pcount;
   char ch;
   } parm;

keyboard = (void *) 0x00C000;
strobe = (void *) 0x00C010;
flags = (void *) 0x00C025;

if (*keyboard & 0x80) {
   if ((*keyboard & 0x7F) == '.')
      if (*flags & 0x80) {
         *strobe = (char) 0;
         exit(-1);
         }
   *strobe = (char) 0;
   parm.pcount = 1;
   parm.ch = (char)   27; WRITE_CONSOLE(&parm);
   parm.ch = (char)   15; WRITE_CONSOLE(&parm);
   parm.ch = (char) 0x43; WRITE_CONSOLE(&parm);
   parm.ch = (char)   24; WRITE_CONSOLE(&parm);
   parm.ch = (char)   14; WRITE_CONSOLE(&parm);
   parm.ch = (char)    8; WRITE_CONSOLE(&parm);
   while (! (*keyboard & 0x80)) ;
   parm.ch = (char)   32; WRITE_CONSOLE(&parm);
   parm.ch = (char)    8; WRITE_CONSOLE(&parm);
   if ((*keyboard & 0x7F) == '.')
      if (*flags & 0x80) {
         *strobe = (char) 0;
         exit(-1);
         }        
   *strobe = (char) 0;
   }
}

/**** Segment I/O Subroutines *************************************/

/***************************************************************
*
*  Checks and updates output header information.
*
*  Returns:
*	TRUE - if header OK.
*	FALSE - if header not OK and flags error.
*
****************************************************************/

BOOLEAN CheckHead (void)

{
long talign;

if ((version != 1) && (version != 2)) {
   Error(3," ");
   return FALSE;
   }
if ((lablen > 256) || (numlen > 4) || (numlen < 1)) {
   Error(8," ");
   return FALSE;
   }
if (align) {				/* insure only 1 bit set in alignment */
   talign = align;
   while (! (talign & 1))
      talign = talign >> 1;
   if (talign != 1) {
      Error(8," ");
      return FALSE;
      }
   }
return TRUE;
}

/***************************************************************
*
*  GetEOF - Returns The End of File Marker
*
*  Returns: mark or -1 if error
*
***************************************************************/

long GetEOF (FILE *f)

{
long mark, pos;

pos = ftell(f);
if (pos == -1)
   return -1;
if (fseek(f, -1, SEEK_END))
   return -1;
mark = ftell(f)+1;
if (pos == -1)
   return -1;
if (fseek(f, pos, SEEK_SET))
   return -1;
return mark;
}               

/***************************************************************
*
*  GetHead - Reads In The Segment Header
*
*  Inputs:
*	f - file to read the header from
*
***************************************************************/

int GetHead (FILE *f)

{
struct {
   long blkcnt, resspc, length;
   char kind1, lablen, numlen, version;
   long banksize;
   int kind2, unused;
   long pgorg, align;
   char numsex, lang_card;
   int segnum;
   long ent_disp;
   int nam_disp, bod_disp;
   } header;
int i, disp, len;

fseek(f, 0, SEEK_CUR);			/* read the common parts of the header */
if (fread(&header, sizeof(header), 1, f) != 1) 
   return -1;
blkcnt = header.blkcnt;
resspc = header.resspc;
length = header.length;
kind1 = header.kind1;
lablen = header.lablen;
numlen = header.numlen;
version = header.version;
banksize = header.banksize;
kind2 = header.kind2;
unused = header.unused;
pgorg = header.pgorg;
align = header.align;
numsex = header.numsex;
lang_card = header.lang_card;
segnum = header.segnum;
ent_disp = header.ent_disp;
nam_disp = header.nam_disp;
bod_disp = header.bod_disp;
disp = nam_disp-sizeof(header);		/* skip any extra stuff */
if (disp)
   for (i = 0; i < disp; ++i) 
      fgetc(f);
for (i = 0; i < 10; ++i)	        /* read the load segment name */
   loadname[i] = fgetc(f);
loadname[10] = (char) 0;
len = fgetc(f);				/* read the code segment name */
for (i = 0; i < len; ++i)
   segname[i] = fgetc(f);
segname[len] = (char) 0;
if (version == 2)			/* set the kind field */
   kind = kind2;
else
   kind = kind1;

if (ftype == 0 && version == 1 && (kind & 0x001F) == 0x08)
  ftype = LIBFL;

return 0;
}

/***************************************************************
*
*  IsInList - Checks to see if segment is in names list
*
***************************************************************/

BOOLEAN IsInList (void)

{
int len, chpos;

if (! names)
   return TRUE;
len =
chpos = 0;

					/* get length of segment name */
while (isalnum(segname[len]) || (segname[len] == '~') ||
   (segname[len] == '_')) len++;
while (nlist[chpos]) {			/* see if name matches a list name */
   while (nlist[chpos] == ' ') chpos++;
   if ((! strncmp(segname,nlist+chpos,len)) &&
      ((nlist[chpos+len] == ' ') || (! nlist[chpos+len])))
      return TRUE;
   while ((nlist[chpos]) && (nlist[chpos] != ' ')) chpos++;
   }
return FALSE;
}

/***************************************************************
*
*  NextSeg - Returns the next segment
*
*  Output:
*      positions file marker at next segment 0 if at end of file
*
***************************************************************/

BOOLEAN NextSeg(void)

{

if ((format == 2 || format == 1) && !header && !checkf) {
   numsex = 0;
   return TRUE;
   }

do
   {
   if ((ftype == LIBFL) || (version == 2)) {
      if (fseek(input, segmark + blocks, SEEK_SET))
          return FALSE;
      }
   else {
      if (fseek(input,segmark + (blocks * BLKSIZE), SEEK_SET))
          return FALSE;
      }
   segmark = ftell(input);		/* set next segment marker */
   if (segmark >= (GetEOF(input)))
      return FALSE;
   GetHead(input);
   blocks = blkcnt;
   if (! blocks)
      return FALSE;
   fseek(input, segmark, SEEK_SET);
   }
while (! IsInList());			/* check to see if in names list */
return TRUE;
}

/***************************************************************
*
*  PutComment - Writes a comment line if colf is not active
*
***************************************************************/

void PutComment (void)

{
if (! colf)
   printf("! ");
}

/***************************************************************
*
*  PutKind - Writes out the kind in name format
*
***************************************************************/

void PutKind (int kind)

{
if (version == 1) {
   if (kind & 128)
      printf("dynamic ");
   else
      printf("static ");
   if (kind & 64)
      printf("private ");
   if (kind & 32)
      printf("position independent ");
   }
else {
   if (kind & 32768)
      printf("dynamic ");
   else
      printf("static ");
   if (kind & 16384)
      printf("private ");
   if (kind & 8192)
      printf("position independent ");
   if (kind & 4096)
      printf("no special memory ");
   if (kind & 2048)
      printf("absolute bank ");
   if (kind & 1024)
      printf("reload ");
   }
switch(kind & 31) {
   case 0:
      printf("code segment");
      break;
   case 1:
      printf("data segment");
      break;
   case 2:
      printf("jump table segment");
      break;
   case 4:
      printf("pathname segment");
      break;
   case 8:
      printf("library dictionary segment");
      break;
   case 16:
      printf("initialization segment");
      break;
   case 17:
      printf("absolute bank segment");
      break;
   case 18:
      printf("direct page/stack segment");
      break;
   default:
      printf("unknown segment: $04X", kind);
   }
}

/***************************************************************
*
*  PutHeader - Writes Out The Header
*
*  Returns: FALSE if an error was found, else TRUE.
*
***************************************************************/

/* debug */

BOOLEAN PutHeader (void)

{
long mark;
int kind2;

mark = ftell(input);			/* save the file marker */
if (GetHead(input)) {                                
   Error(2," ");
   return FALSE;
   }
if ((checkf) && ((version != 1) && (version != 2))) {
   Error(3," ");
   return FALSE;
   }
blocks = blkcnt;			/* set the new block counter */
if (! header)				/* split on write/don't write header */
   fseek(input,mark,SEEK_SET);
else {
   count = ftell(input) - mark;		/* set the file counter */
   if (shorth) {			/* split on short or long header */
      PutComment();
      printf("%s  (", segname);
      PutKind(kind);
      printf(")\n");
      return CheckHead();
      }
   else {				/* print the full header */
      PutComment();			/* write block count */
      if (version == 2)
         printf("Byte count    : $%08lX%14ld\n", blkcnt, blkcnt);
      else
         printf("Block count   : $%08lX%14ld\n", blkcnt, blkcnt);
      CheckESC();
      PutComment();			/* write reserved space */
      printf("Reserved space: $%08lX%14ld\n", resspc, resspc);
      CheckESC();
      PutComment();			/* write segment length */
      printf("Length        : $%08lX%14ld\n", length, length);
      CheckESC();
      PutComment();			/* write label length */
      printf("Label length  : $%02X%20d\n", lablen, lablen);
      CheckESC();
      PutComment();			/* write number length */
      printf("Number length : $%02X%20d\n", numlen, numlen);
      CheckESC();
      PutComment();			/* write segment version */
      printf("Version       : $%02X%20d\n", version, version);
      CheckESC();
      PutComment();			/* write bank size */
      printf("Bank size     : $%08lX%14ld\n", banksize, banksize);
      CheckESC();
      PutComment();			/* write segment kind */
      if (version == 2) {
         printf("Kind          : $%04X%18d (", kind, kind);
         kind2 = kind;
         }
      else {
         printf("Kind          : $%02X%20d (", kind, kind);
         kind2 = ((kind & 0xE0) << 8) | (kind & 0x1F);
         }
      switch (kind & 0x001F) {
         case 0x00: printf("code");
                    break;
         case 0x01: printf("data");
                    break;
         case 0x02: printf("jump table");
                    break;
         case 0x04: printf("pathname");
                    break;
         case 0x08: printf("library");
                    break;
         case 0x10: printf("initialization");
                    break;
         case 0x11: printf("absolute bank");
                    break;
         case 0x12: printf("direct-page/stack");
                    break;
         default:   printf("unknown");
                    break;
         }     
      if (kind & 0x8000)
         printf(",dynamic");
      else
         printf(",static");
      if (kind & 0x4000) printf(",private");
      if (kind & 0x2000) printf(",position independent");
      if (kind & 0x1000) printf(",no special memory");
      if (kind & 0x0800) printf(",absolute bank");
      if (kind & 0x0400) printf(",reload");
      if (kind & 0x0200) printf(",skip");
      if (kind & 0x0100) printf(",bank relative");
      printf(")\n");
      CheckESC();
      PutComment();			/* write segment org */
      printf("Org           : $%08lX%14ld\n", pgorg, pgorg);
      CheckESC();
      PutComment();			/* write alignment factor */
      printf("Alignment     : $%08lX%14ld\n", align, align);
      CheckESC();
      PutComment();			/* write number sex */
      printf("Number sex    : $%02X%20d\n", numsex, numsex);
      CheckESC();
      if (version == 1) {		/* write language card */
         PutComment();
         printf("Language card : $%02X%20d\n", lang_card, lang_card);
         CheckESC();
         }
      PutComment();			/* write segment number */
      printf("Segment number: $%04X%18d\n", segnum, segnum);
      CheckESC();
      PutComment();			/* write segment entry */
      printf("Segment entry : $%08lX%14ld\n", ent_disp, ent_disp);
      CheckESC();
      PutComment();			/* write segment names disp */
      printf("Disp to names : $%04X%18d\n", nam_disp, nam_disp);
      CheckESC();
      PutComment();			/* write segment body disp */
      printf("Disp to body  : $%04X%18d\n", bod_disp, bod_disp);
      CheckESC();
      if (! CheckHead())		/* don't print labels if header error */
         return FALSE;
      PutComment();			/* write load name */
      printf("Load name     : %s\n", loadname);
      CheckESC();
      PutComment();			/* write segment name */
      printf("Segment name  : %s\n", segname);
      CheckESC();
      }
   }
return TRUE;
}

/**** Opcode Dump Subroutines *************************************/

void DoExpr (void);
    
/***************************************************************
*
*  Reads the opcode into "opcode"
*
***************************************************************/

int GetOpCode (void)

{
++count;				/* advance byte counter */
opcode = fgetc(input);
if (opcode == -1) {			/* check for end of file */
   opcode = 0;
   Error(9," ");			/* end of file encountered */
   exit(status);
   }
return opcode;
}

/***************************************************************
*
*  NewLine - Issues a Formatted New Line
*
***************************************************************/

void NewLine (void)

{
int count;

putchar('\n');
if (colf)
   count = 33;
else
   count = 17;
while (count--)
   putchar(' ');
}

/***************************************************************
*
*  ReadInt - reads an integer
*
*  Inputs:
*	numsex - number sex
*	input - reference number of file
*	numlen - length of number
*	nPtr - pointer to place result
*
***************************************************************/

void ReadInt (long *nPtr, int numlen, FILE *f, int numsex)

{
int i;
long k,n;
union {
   char c[4];
   long l;
   } u;

if (numlen <= 4) {
   u.l = 0;
   for (i = 0; i < numlen; ++i)
      u.c[i] = fgetc(f);
   *nPtr = u.l;
   if (numsex) {
      n = *nPtr;
      for (i = 0; i < 4; ++i) {
         k = (k << 8) | (n && 0xFF);
         n = n >> 8;
         }
      *nPtr = k;
      }
   }
}

/***************************************************************
*
*  ReadName - reads in a symbol name
*
*  Inputs:
*	name - pointer to place the name
*	lablen - length of name
*	f - open file ref number
*
***************************************************************/

void ReadName (char *name, int lablen, FILE *f)

{
int i;

if (lablen)			        /* get the label length */
   act_lablen = lablen;
else
   act_lablen = fgetc(f);
for (i = 0; i < act_lablen; ++i)        /* read the name */
   name[i] = fgetc(f);
name[act_lablen] = (char) 0;		/* set the null terminator */
}

/***************************************************************
*
*  PrintW - Writes a word in a string to the console
*
*  Inputs:
*	str - pointer to the string to write
*
***************************************************************/

void PrintW (char *str)

{
int i = 0;

while ((str[i]) && (! isspace(str[i])))
   putchar(str[i++]);
}

/***************************************************************
*
*  PutConst - Writes the constant bytes for opcode format
*
*  Inputs:
*	len - number of bytes to write
*	input - file to get bytes from
*
***************************************************************/

void PutConst (long len, int flag)

{
long k;
int i;

if (flag) {				/* see if we print out record */
   i = 0;
   for (k = 0; k < len; ++k) {
      ++i;				/* allow 23 numbers per line */
      if (i == 24) {			/* see if we need to issue a new line */
         i = 1;
         NewLine();
         }
      printf("%02X", (int) fgetc(input)); /* write the hex */
      CheckESC();
      }
   }
else
   fseek(input, ftell(input)+len, SEEK_SET);
}

/***************************************************************
*
*  PutCTPC - Writes the count and program counter
*
***************************************************************/

void PutCTPC(void)

{
putchar('\n');
if (colf)				/* check to make sure you write col */
   printf("%06lX %06lX | ", count, pc);
}

/***************************************************************
*
*  PutNumber - Writes a number
*
*  Inputs:
*      input - input file
*
*  Outputs:
*      number - contains number read
*
***************************************************************/

void PutNumber (void)

{
ReadInt(&number,numlen,input,numsex);
switch (numlen) {
   case 1:	printf("%02X", (int) number);
   case 2:	printf("%04X", (int) number);
   case 3:	printf("%06lX", (long) number);
   case 4:	printf("%08lX", (long) number);
   }
count = count + numlen;
}

/***************************************************************
*
*  Putsymbol - Writes the symbol
*
*  Inputs:
*	input - input file
*
*  Outputs:
*	symbol - symbol read
*
***************************************************************/

void PutSymbol (void)

{
ReadName(symbol,lablen,input);
PrintW(symbol);
if (lablen)
   count = count + lablen;
else
   count = count + act_lablen + 1;
}

/***************************************************************
*
*  PutOpCode - Writes the opcode
*
*  Inputs:
*      opcode - opcode to write
*      input - input file
*
*  Outputs:
*      write opcode
*      pc, count = updates accordingly
*
***************************************************************/

void PutOpCode (void)

{
long k;
int p1, p2, p3;				/* temp storage for calculated parms */

if (opcode < OPALIGN) {		        /* handle short constant opcode */
   printf("CONST    ($%02X) | ", (int) opcode);
   pc = pc + opcode;
   count = count + opcode;
   PutConst(opcode,longc);		/* write the constant opcodes */
   }
else switch (opcode) {
   case OPALIGN:			/* handle align opcode */
      printf("ALIGN    ($E0) | ");
      PutNumber();
      pc = pc + (pc % number);
      break;
   case OPORG:				/* handle org opcode */
      printf("ORG      ($E1) | ");
      PutNumber();
      pc = pc + number;
      break;
   case OPRELOC:			/* handle reloc opcode */
      p1 = fgetc(input);
      p2 = fgetc(input);
      printf("RELOC    ($E2) | %02X : %02X : ", p1, p2);
      PutNumber();
      printf(" : ");
      PutNumber();
      count = count + 2;
      break;
   case OPINTSEG:			/* handle interseg opcode */
      p1 = fgetc(input);
      p2 = fgetc(input);
      printf("INTERSEG ($E3) | %02X : %02X : ", p1, p2);
      PutNumber();
      printf(" : ");
      ReadInt(&number,2,input,numsex);
      printf("%04X : ", (int) number);
      ReadInt(&number,2,input,numsex);
      printf("%04X : ", (int) number);
      PutNumber();
      count = count + 6;
      break;
   case OPUSING:			/* handle using opcode */
      printf("USING    ($E4) | ");
      PutSymbol();
      break;
   case OPSTRONG:			/* handle strong opcode */
      printf("STRONG   ($E5) | ");
      PutSymbol();
      break;
   case OPGLOBAL:			/* handle global opcode */
      printf("GLOBAL   ($E6) | ");
      PutSymbol();
      if (version == 1) {
         p1 = fgetc(input);
         p2 = fgetc(input);
         p3 = fgetc(input);
         printf(" : %02X : %02X : %02X", p1, p2, p3);
         count = count + 3;
         }
      else {
         p1 = fgeti(input);
         p2 = fgetc(input);
         p3 = fgetc(input);
         printf(" : %04X : %02X : %02X", p1, p2, p3);
         count = count + 4;
         }
      break;
   case OPGEQU:				/* handle gequ opcode */
      printf("GEQU     ($E7) | ");
      PutSymbol();
      if (version == 1) {
         p1 = fgetc(input);
         p2 = fgetc(input);
         p3 = fgetc(input);
         printf(" : %02X : %02X : %02X :", p1, p2, p3);
         count = count + 3;
         }
      else {
         p1 = fgeti(input);
         p2 = fgetc(input);
         p3 = fgetc(input);
         printf(" : %04X : %02X : %02X :", p1, p2, p3);
         count = count + 4;
         }
      DoExpr();
      break;
   case OPMEM:				/* handle mem opcode */
      printf("MEM      ($E8) | ");
      PutNumber();
      printf(" : ");
      PutNumber();
      break;
   case OPEXPR:				/* handle expr opcode */
      k = fgetc(input);
      count++;
      printf("EXPR     ($EB) | %02X :", (int) k);
      pc = pc + k;
      DoExpr();
      break;
   case OPZEXPR:			/* handle zexpr opcode */
      k = fgetc(input);
      count++;
      printf("ZEXPR    ($EC) | %02X :", (int) k);
      pc = pc + k;
      DoExpr();
      break;
   case OPBEXPR:			/* handle bexpr opcode */
      k = fgetc(input);
      count++;
      printf("BEXPR    ($ED) | %02X :", (int) k);
      pc = pc + k;
      DoExpr();
      break;
   case OPRELEXPR:			/* handle relexpr opcode */
      k = fgetc(input);
      count++;
      printf("RELEXPR  ($EE) | %02X : ", (int) k);
      PutNumber();
      pc = pc + k;
      printf(" :");
      DoExpr();
      break;
   case OPLOCAL:			/* handle local opcode */
      printf("LOCAL    ($EF) | ");
      PutSymbol();
      if (version == 1) {
         p1 = fgetc(input);
         p2 = fgetc(input);
         p3 = fgetc(input);
         printf(" : %02X : %02X : %02X", p1, p2, p3);
         count = count + 3;
         }
      else {
         p1 = fgeti(input);
         p2 = fgetc(input);
         p3 = fgetc(input);
         printf(" : %04X : %02X : %02X", p1, p2, p3);
         count = count + 4;
         }
      break;
   case OPEQU:				/* handle equ opcode */
      printf("EQU      ($F0) | ");
      PutSymbol();
      if (version == 1) {
         p1 = fgetc(input);
         p2 = fgetc(input);
         p3 = fgetc(input);
         printf(" : %02X : %02X : %02X :", p1, p2, p3);
         count = count + 3;
         }
      else {
         p1 = fgeti(input);
         p2 = fgetc(input);
         p3 = fgetc(input);
         printf(" : %04X : %02X : %02X :", p1, p2, p3);
         count = count + 4;
         }
      DoExpr();
      break;
   case OPDS:				/* handle ds opcode */
      printf("DS       ($F1) | ");
      PutNumber();
      pc = pc + number;
      break;
   case OPLONG:				/* handle long constant opcode */
      printf("LCONST   ($F2) | ");
      PutNumber();
      printf(" : ");
      NewLine();
      pc = pc + number;
      count = count + number;
      PutConst(number,longc);		/* write the constant opcodes */
      break;
   case OPLEXPR:			/* handle long expression opcode */
      k = fgetc(input);
      count++;
      printf("LEXPR    ($F3) | %02X :", (int) k);
      pc = pc + k;
      DoExpr();
      break;
   case OPENTRY:			/* handle entry opcode */
      printf("ENTRY    ($F4) | ");
      PutNumber();
      printf(" : ");
      PutSymbol();
      break;
   case OPSRELOC:			/* handle short reloc opcode */
      p1 = fgetc(input);
      p2 = fgetc(input);
      printf("cRELOC   ($F5) | %02X : %02X : ", p1, p2);
      ReadInt(&number,2,input,numsex);
      printf("%04X : ", (int) number);
      ReadInt(&number,2,input,numsex);
      printf("%04X", (int) number);
      count = count + 6;
      break;
   case OPSINTSEG:			/* handle short interseg opcode */
      p1 = fgetc(input);
      p2 = fgetc(input);
      printf("cINTRSEG ($F6) | %02X : %02X : ", p1, p2);
      ReadInt(&number,2,input,numsex);
      printf("%04X : %02X : ", (int) number, (int) fgetc(input));
      ReadInt(&number,2,input,numsex);
      printf("%04X", (int) number);
      count = count + 7;
      break;
   case OPSUPER:			/* handle short interseg opcode */
      printf("SUPER    ($F7) | ");
      ReadInt(&number,4,input,numsex);
      printf("%08lX : %02X", (long) number, (int) fgetc(input));
      NewLine();
      count = count + number;
      PutConst(number-1,1);
      break;
   default:
      Error(7," ");
      break;
   }
}

/***************************************************************
*
*  PutOpCodeBody - dump the body of one segment in opcode format
*
***************************************************************/

void PutOpCodeBody (void)

{
while (GetOpCode()) {
   --count;				/* adjust the counter */
   PutCTPC();				/* write the count and pc */
   count++;
   PutOpCode();				/* write out the opcode */
   CheckESC();
   }
--count;				/* write out the end opcode */
PutCTPC();
puts("END      ($00)");			/* print end directive */
}

/***************************************************************
*
*  DpOpCode - Dump file in opcode format
*
***************************************************************/

void DpOpCode(void)

{
while (NextSeg()) {
   count =				/* initialize counter and pc */
   pc = 0;
   CheckESC();
   if (! PutHeader())			/* write out the header */
      return;
   if (body) {				/* write out the body */
      if ((! header) && checkf) {
         count = bod_disp;
         fseek(input, ftell(input) + bod_disp, SEEK_SET);
         }
      PutOpCodeBody();
      }
   if ((! shorth) || body)
      puts("\n");
   }
}

/**** Expression Output Subroutines *******************************/

/***************************************************************
*
*  DoOper - Writes an operation
*
*  Inputs:
*       space - print a leading space?
*
***************************************************************/

void DoOper (BOOLEAN space)

{
if (space)
   putchar(' ');
switch(opcode) {
   case 1:				/* handle addition */
      putchar('+');
      break;
   case 2:				/* handle subtraction */
      putchar('-');
      break;
   case 3:				/* handle multiplication */
      putchar('*');
      break;
   case 4:				/* handle division */
      putchar('/');
      break;
   case 5:				/* handle modulo */
      putchar('%');
      break;
   case 6:				/* handle negation */
      if (format != 1)
         printf("--");
      else
         printf("unary -");
      break;
   case 7:				/* handle bit shift */
      putchar('|');
      break;
   case 8:				/* handle logical and */
      printf(".AND.");
      break;
   case 9:				/* handle logical or */
      printf(".OR.");
      break;
   case 10:				/* handle logical xor */
      printf(".EOR.");
      break;
   case 11:				/* handle logical not */
      printf(".NOT.");
      break;
   case 12:				/* handle less or equal */
      printf("<=");
      break;
   case 13:				/* handle greater or equal */
      printf(">=");
      break;
   case 14:				/* handle not equal */
      printf("<>");
      break;
   case 15:				/* handle less */
      putchar('<');
      break;
   case 16:				/* handle greater */
      putchar('>');
      break;
   case 17:				/* handle equal */
      putchar('=');
      break;
   case 18:				/* handle bit and */
      printf(".BAND.");
      break;
   case 19:				/* handle bit or */
      printf(".BOR.");
      break;
   case 20:				/* handle bit xor */
      printf(".BEOR.");
      break;
   }
}

/***************************************************************
*
*  DoOpnd - Writes an operand
*
***************************************************************/

void DoOpnd (void)

{
switch(opcode) {
   case 128:				/* handle location counter */
      printf(" (*)");
      break;
   case 129:				/* handle constant */
      printf(" $");
      PutNumber();
      break;
   case 130:				/* handle weak reference */
      printf(" W:");
      PutSymbol();
      break;
   case 131:				/* handle value of label */
      putchar(' ');
      PutSymbol();
      break;
   case 132:				/* handle length attribute */
      printf(" L:");
      PutSymbol();
      break;
   case 133:				/* handle type attribute */
      printf(" T:");
      PutSymbol();
      break;
   case 134:				/* handle count attribute */
      printf(" C:");
      PutSymbol();
      break;
   case 135:				/* handle relative ofset */
      printf(" (");
      PrintW(segname);
      printf("+$");
      PutNumber();
      putchar(')');
      break;
   }
}

/***************************************************************
*
*  DoExpr - Writes an expression
*
***************************************************************/

void DoExpr (void)

{
while (GetOpCode()) {
   if (opcode < 128)
      DoOper(TRUE);
   else
      DoOpnd();
   }
}

/**** Disassembler Subroutines ************************************/

/***************************************************************
*
*  ReadSym - Reads in a symbol and appends it to symbol
*
***************************************************************/

void ReadSym (void)

{
int i,k;

k = strlen(symbol);
if (lablen)				/* handle variable length labels */
   act_lablen = lablen;
else {
   act_lablen = fgetc(input);
   ++count;
   }
for (i=0; i<act_lablen; i++) {		/* read in the name and append it */
   symbol[i+k] = fgetc(input);
   ++count;
   }
symbol[k+act_lablen] = 0;
i = 0;
while (symbol[i] && (symbol[i] != ' '))	/* remove excess blanks */
   ++i;
symbol[i] = 0;
}

/***************************************************************
*
*  NextToken - return the next token in an expression
*
***************************************************************/

BOOLEAN NextToken (void)

{
symbol[0] = 0;				/* clear the symbol */
if (! GetOpCode())			/* get the opcode */
   return FALSE;
if (opcode < 128)
   switch(opcode)
      {
      case 6: case 11: case 21:		/* check for unary operator */
         tkind = 1;
         break;
      default:
         tkind = 0;
         break;
      }
else
   switch(opcode)
      {
      case 128:				/* handle location counter */
         strcpy(symbol,"(*)");
         tkind = 3;
         break;
      case 129: case 135:		/* handle constant */
         ReadInt(&number,numlen,input,numsex);
         count += numlen;
         tkind = 2;
         break;
      case 130: case 131:		/* handle weak reference */
         ReadSym();
         tkind = 3;
         break;
      case 132:				/* handle length attribute */
         strcpy(symbol,"L:");
         ReadSym();
         tkind = 3;
         break;
      case 133:				/* handle type attribute */
         strcpy(symbol,"T:");
         ReadSym();
         tkind = 3;
         break;
      case 134:				/* handle count attribute */
         strcpy(symbol,"C:");
         ReadSym();
         tkind = 3;
         break;
      }
return TRUE;
}

/***************************************************************
*
*  BuildT - Builds the expression tree
*
***************************************************************/

void BuildT (void)

{
long *lptr, *ptr;

root = NULL;
while (NextToken())
   switch(tkind) {
      case 0:				/* handle binary operators */
         /* check for a NULL head pointer */
         lptr = malloc(20);
         lptr[0] = opcode;		/* assign values */
         lptr[1] = tkind;
         lptr[2] = root[4];		/* left */
         lptr[3] = (long) root;		/* right */
         ptr = (long *) root[4];
         lptr[4] = ptr[4];
         root = lptr;
         break;
      case 1:				/* handle unary operator */
         /* check for a NULL head pointer */
         lptr = malloc(20);
         lptr[0] = opcode;		/* assign values */
         lptr[1] = tkind;
         lptr[2] = (long) root;		/* left */
         lptr[4] = root[4];		/* next */
         root = lptr;
         break;
      case 2:				/* handle number value */
         lptr = malloc(24);
         lptr[0] = opcode;		/* assign values */
         lptr[1] = tkind;
         lptr[2] = (long) NULL;		/* left */
         lptr[3] = (long) NULL;		/* right */
         lptr[4] = (long) root;		/* next */
         lptr[5] = number;		/* set the number value */
         root = lptr;
         break;
      case 3:				/* handle string value */
         lptr = malloc(21+strlen(symbol));
         lptr[0] = opcode;		/* assign values */
         lptr[1] = tkind;
         lptr[2] = (long) NULL;		/* left */
         lptr[3] = (long) NULL;		/* right */
         lptr[4] = (long) root;		/* next */
         strcpy((char *) (lptr+5),symbol); /* set the string value */
         root = lptr;
         break;
      }
}

/***************************************************************
*
*  DisposeT - Recursively frees all nodes from the expression tree
*
***************************************************************/

void DisposeT (long *root)

{
if (root) {
   switch(root[1]) {
      case 3: case 2: break;		/* do nothing with value */
      case 1:				/* handle unary operator */
         DisposeT((long *) root[2]);
         break;
      case 0:				/* handle binary operator */
         DisposeT((long *) root[2]);
         DisposeT((long *) root[3]);
         break;
      }
   free(root);
   }
}

/***************************************************************
*
*  WriteT - Writes the expression tree in infix form
*
***************************************************************/

void WriteT (long *root)

{
if (root)           
   switch(root[1]) {
      case 3:				/* handle label value */
         printf((char *) (root+5));
         break;
      case 2:				/* handle number value */
         if (root[0] == 135)  		/* split on case of relative ofset */
            printf("(%s+$%0*lX)", segname, (int) numlen*2, (long) root[5]);
         else
            printf("$%0*lX", (int) numlen*2, (long) root[5]);
         break;
      case 1:				/* handle unary operator */
         putchar('(');
         opcode = root[0];
         DoOper(FALSE);
         WriteT((long *) root[2]);
         putchar(')');
         break;
      case 0:				/* handle binary operator */
         putchar('(');
         WriteT((long *) root[2]);
         opcode = root[0];
         DoOper(FALSE);
         WriteT((long *) root[3]);
         putchar(')');
         break;
      }
}

/***************************************************************
*
*  DoInexp - Writes an expression in infix form
*
***************************************************************/

void DoInexp (void)

{
BuildT();				/* build the expression tree */
WriteT(root);				/* write the expression tree */
DisposeT(root);				/* dispose of the tree */
}

/***************************************************************
*
*  GtOpKind - Returns the addressing mode of opcode
*
*  Inputs:
*	opcode - operation code
*
*  Outputs:
*	addressing mode
*
***************************************************************/

int GtOpKind (int opcode)

{
return opKind[opcode];
}

/***************************************************************
*
*  GtOpLen - Return the operand length for the given opcode
*
*  Inputs:
*	operand - operation code
*	imode - index register size
*	amode - accumulator size
*
*  Outputs:
*	addressing mode
*
***************************************************************/

int GtOpLen (int operand, int amode, int imode)

{
int len;

len = opLen[operand];			/* get the operand length */
if (len == 4) {				/* handle a variable A instruction */
   if (amode)
      return 2;
   return 1;
   }
if (len > 4) {				/* handle avariable I instruction */
   if (imode)
      return 2;
   return 1;
   }
return len;				/* return a fixed length */
}

/***************************************************************
*
*  GtOpName - Returns Pointer to Opcode Name
*
*  Inputs:
*	opcode - operation code
*
*  Outputs:
*	pointer to numonic string
*
*  Notes: There is no terminating null on the string.  The
*	caller should use exactly 3 characters.
*
***************************************************************/

char *GtOpName (int opcode)

{
return &opName[opcode*3];
}

/***************************************************************
*
*  IsExpr - Check to see if OPCODE is from an expression
*
*  Inputs:
*	op - object module opcode
*
*  Outputs:
*	true if from an expression; else false
*	Note:	if constant != 0 then --constant, return false
*		if constant == 0 then
*		if (constant_opcode) set constant, getopcode, return false
*		else return true
*
***************************************************************/

BOOLEAN IsExpr (int op)

{
if ((! checkf) && (! header))
   return FALSE;
if (constant)				/* handle in a constant */
   return FALSE;
if (op < OPALIGN) {			/* handle short constant */
   constant = op;
   GetOpCode();
   return FALSE;
   }
if (op == OPLONG) {			/* handle long constant */
   ReadInt(&constant,numlen,input,numsex);
   count = count + numlen;
   GetOpCode();
   return FALSE;
   }
return TRUE;
}

/***************************************************************
*
*  PutLSymbol - Writes the symbol
*
*  Inputs:
*	input - input file
*
*  Outputs:
*	symbol - symbol read
*
***************************************************************/

void PutLSymbol (void)

{
int i;

ReadName(symbol,lablen,input);
printf(symbol);
for (i=act_lablen; i<8; i++)		/* fill out with spaces */
   putchar(' ');
putchar(' ');
if (lablen)				/* handle variable length labels */
   count = count + lablen;
else
   count = count + act_lablen + 1;
}

/***************************************************************
*
*  PutStreg - Writes out the registar status
*
*  Inputs:
*      mreg - accumulator flag
*      ireg - index registar flag
*
***************************************************************/

void PutStreg (void)

{
PutCTPC();
printf("         LONGA ");
if (mreg)
   printf("ON");
else
   printf("OFF");
PutCTPC();
printf("         LONGI ");
if (ireg)
   printf("ON");
else
   printf("OFF");
}

/***************************************************************
*
*  ReadOpVal - reads an integer from a constant field
*
*  Inputs:
*	numsex - number sex
*	input - reference number of file
*	numlen - length of number
*	nPtr - pointer to place result
*
*  Outputs:
*       Returns true if successful, else false.
*
***************************************************************/

BOOLEAN ReadOpVal (long *nPtr, int numlen, FILE *f, int numsex)

{
int i;
long k,n;
union {
   char c[4];
   long l;
   } u;

if (numlen <= 4) {
   u.l = 0;
   for (i = 0; i < numlen; ++i) {
      if (!constant) {
         GetOpCode();
         if (opcode && (opcode < OPALIGN))
            constant = opcode;
         else
            return FALSE;
         }
      else
         --constant;
      u.c[i] = fgetc(f);
      }
   *nPtr = u.l;
   if (numsex) {
      n = *nPtr;
      for (i = 0; i < 4; ++i) {
         k = (k << 8) | (n && 0xFF);
         n = n >> 8;
         }
      *nPtr = k;
      }
   }
return TRUE;
}

/***************************************************************
*
*  PutOpnd - Writes the operand for an instruction
*
*  Outputs:
*       Returns true if successful, false if an expression
*       was encountered in the middle of operand.
*
***************************************************************/

BOOLEAN PutOpnd (int op)

{
BOOLEAN success;
int i;
long addr;

i = GtOpLen(op,mreg,ireg);
pc = pc + i;
if (constant) {				/* split on constant or expression */
   count = count + i;			/* update the file counter */
   success = ReadOpVal(&number,i,input,numsex);	/* get the value and print it */
   printf("$%0*lX", (int) i*2, (long) number);
   if (op == REP) {			/* check for REP */
      if (number & 32) mreg = 1;
      if (number & 16) ireg = 1;
      PutStreg();
      }
   if (op == SEP) {			/* check for SEP */
      if (number & 32) mreg = 0;
      if (number & 16) ireg = 0;
      PutStreg();
      }
   if (op == COP) {			/* special handling for debug COPs */
      switch (number) {
         case 0:
         case 1:
         case 2:
            success = ReadOpVal(&number,2,input,numsex);
            printf(",%d", (int) number);
            count += 2;
            pc += 2;
            break;
         case 5:
            sTable = TRUE;
            break;
         }
      }
   }          
else {					/* handle case where operand is an exp */
   switch(GetOpCode()) {
      case OPEXPR: case OPZEXPR: case OPBEXPR: case OPLEXPR:
         fgetc(input);
         ++count;
         DoInexp();			/* write expression in infix form */
         break;
      case OPRELEXPR:
         fgetc(input);
         ReadInt(&number,numlen,input,numsex);
         count = count + numlen + 1;
         DoInexp();			/* write expression in infix form */
         break;
      default:				/* case with unexpected input */
         printf("???");
         break;
      }
   }
return success;
}

/***************************************************************
*
*  Dis_DC - Disassembles a DC format
*
*  Inputs:
*	op - object module opcode
*
***************************************************************/

void Dis_DC (int op)

{
int i;
int p1, p2;				/* temps for computed parameters */

switch(op) {
   case OPGEQU: case OPEQU:		/* handle equates */
      PutLSymbol();
      if (op == OPEQU)
         printf("EQU   ");
      else
         printf("GEQU  ");
      fgetc(input); fgetc(input); fgetc(input);
      count = count+3;
      if (version == 2) {
         fgetc(input);
         ++count;
         }
      DoInexp();
      break;
   case OPUSING:			/* handle using directives */
      printf("         USING ");
      PutSymbol();
      break;
   case OPLOCAL: case OPGLOBAL:		/* handle local and global labels */
      PutLSymbol();
      if (op == OPLOCAL)
         printf("ANOP");
      else
         printf("ENTRY");
      fgetc(input); fgetc(input); fgetc(input);
      count = count + 3;
      if (version == 2) {
         fgetc(input);
         ++count;
         }
      break;
   case OPDS: case OPALIGN:		/* handle ds and align directive */
      if (op == OPALIGN)
         printf("         ALIGN $");
      else
         printf("         DS    $");
      PutNumber();
      if (op == OPALIGN)
         pc = pc + (pc % number);
      else
         pc = pc + number;
      break;
   case OPSTRONG:			/* handle strong reference */
      printf("         DC    R'");
      PutSymbol();
      putchar('\'');
      break;
   case OPORG:				/* handle inter-segment org */
      printf("         ORG   *+$");
      PutNumber();
      pc = pc + number;
      break;
   case OPMEM:				/* handle inter-segment mem */
      printf("         MEM   $");
      PutNumber();
      printf(",$");
      PutNumber();
      break;
   case OPEXPR: case OPZEXPR:		/* handle expressions */
   case OPBEXPR: case OPLEXPR: case OPRELEXPR:
      printf("         DC    ");
      i = fgetc(input);
      if (i == 1)
         printf("I1'");
      else if (i == 2)
         printf("I2'");
      else if (i == 3)
         printf("I3'");
      else
         printf("I4'");
      ++count;
      pc = pc + i;
      if (op == OPRELEXPR) {
         PutNumber();
         putchar('+');
         }
      DoInexp();			/* write expression in infix form */
      putchar('\'');
      break;
   case OPRELOC:			/* handle reloc opcode */
      p1 = fgetc(input);
      p2 = fgetc(input);
      printf("! RELOC    ($E2) | %02X : %02X : ", p1, p2);
      PutNumber();
      printf(" : ");
      PutNumber();
      count = count + 2;
      break;
   case OPINTSEG:			/* handle interseg opcode */
      p1 = fgetc(input);
      p2 = fgetc(input);
      printf("! INTERSEG ($E3) | %02X : %02X : ", p1, p2);
      PutNumber();
      printf(" : ");
      ReadInt(&number,2,input,numsex);
      printf("%04X : ", (int) number);
      ReadInt(&number,2,input,numsex);
      printf("%04X : ", (int) number);
      PutNumber();
      count = count + 6;
      break;
   case OPENTRY:			/* handle entry opcode */
      printf("! ENTRY    ($F4) | ");
      PutNumber();
      printf(" : ");
      PutSymbol();
      break;
   default:
      Error(7," ");
      break;
   }
}

/***************************************************************
*
*  Disasm - Disassembles OPCODE
*
*  Inputs:
*      op - assembly language opcode
*
*  Outputs:
*       Returns true if successful, false if an expression
*       was hit trying to process an operand.
*
***************************************************************/

BOOLEAN Disasm (int op)

{
BOOLEAN success;
int i;
char opn[4];

sTable = FALSE;				/* no COP symbol table found (yet) */
success = TRUE;			        /* assume everything will work */
--constant;				/* now this is a constant */
++pc;					/* update program counter */
strncpy(opn, GtOpName(op), 3);
opn[3] = (char) 0;
printf("         %s   ", opn);
switch(GtOpKind(op)) {			/* split on type of output format */
   case 1:				/* handle '# operand' */
      putchar('#');
      success = PutOpnd(op);
      break;
   case 2:				/* handle 'operand' */
      success = PutOpnd(op);
      break;    
   case 3:				/* handle '> operand' */
      putchar('>');
      success = PutOpnd(op);
      break;    
   case 4:				/* handle 'A' */
      putchar('A');
      break;
   case 5:				/* handle ' ' or 'operand' */
      if (GtOpLen(op,mreg,ireg))
         success = PutOpnd(op);
      break;
   case 6:				/* handle '(operand),Y' */
      putchar('(');
      success = PutOpnd(op);
      printf("),Y");
      break;
   case 7:				/* handle '[operand],Y' */
      putchar('[');
      success = PutOpnd(op);
      printf("],Y");
      break;
   case 8:				/* handle '(operand,X)' */
      putchar('(');
      success = PutOpnd(op);
      printf(",X)");
      break;
   case 9:				/* handle 'operand,X' */
      success = PutOpnd(op);
      printf(",X");
      break;
   case 10:				/* handle 'operand,Y' */
      success = PutOpnd(op);
      printf(",Y");
      break;
   case 11:				/* handle '>operand,X' */
      putchar('>');
      success = PutOpnd(op);
      printf(",X");
      break;
   case 12:				/* handle '(operand)' */
      putchar('(');
      success = PutOpnd(op);
      putchar(')');
      break;
   case 13:				/* handle '[operand]' */
      putchar('[');
      success = PutOpnd(op);
      putchar(']');
      break;
   case 14:				/* handle 'operand,S' */
      success = PutOpnd(op);
      printf(",S");
      break;
   case 15:				/* handle '(operand,S),Y */
      putchar('(');
      success = PutOpnd(op);
      printf(",S),Y");
      break;
   case 16:				/* handle 'operand,operand' */
      success = PutOpnd(op);
      putchar(',');
      success = PutOpnd(op);
      break;
   case 17:				/* handle relative branches */
      if (constant) {
         i = GtOpLen(op,mreg,ireg);
         pc = pc + i;
         count = count + i;		/* update the file counter */
         constant = constant - i;	/* set new constant number */
         number = 0;
         ReadInt(&number,i,input,numsex); /* get the value and print it */
         if (i == 1) {
            if (number > 127)
               number = number | -256;
            }
         else {
            if (number > 32767)
               number = number | -32768;
            }
         number = number + i + 1;
         if (number < 0) {
            number = -number;
            printf("*-$");
            }
         else
            printf("*+$");
         printf("%0*lX", (int) i*2, (long) number);
         }
      else
         success = PutOpnd(op);
      break;
   default:
      printf("Operand Error");
      status = -1;
      break;
   }
return success;
}

/***************************************************************
*	
*  DisSTable - disassemble a COP 5 symbol table
*
***************************************************************/

void DisSTable (void)

{
long tlen;
int p1, p2, p3, p4;			/* temps for computed parms */

if (!constant) {
   GetOpCode();
   if (opcode && (opcode < OPALIGN))
      constant = opcode;
   else {
      PutCTPC();
      Dis_DC(opcode);
      }
   }
if (constant >= 2) {
   ReadInt(&tlen,2,input,numsex);	/* get and print the table length */
   PutCTPC();
   printf("         DC    I'%d'", (int) tlen);
   pc += 2;
   count += 2;
   constant -= 2;
   CheckESC();
   while (tlen > 0) {
      if (!constant) {
         GetOpCode();
         if (opcode && (opcode < OPALIGN))
            constant = opcode;
         }
      if (constant) {
         ReadOpVal(&number,4,input,numsex);
         PutCTPC();
         printf("         DC    I4'%d'", (long) number);
         pc += 4;
         count += 4;
         }
      else {
         PutCTPC();
         Dis_DC(opcode);
         }
      if (!constant) {
         GetOpCode();
         if (opcode && (opcode < OPALIGN))
            constant = opcode;
         }
      if (constant) {   
         ReadOpVal(&number,4,input,numsex);
         PutCTPC();
         printf("         DC    I4'%d'", (long) number);
         pc += 4;
         count += 4;
         }
      else {
         PutCTPC();
         Dis_DC(opcode);
         }
      if (!constant) {
         GetOpCode();
         if (opcode && (opcode < OPALIGN))
            constant = opcode;
         else {
            PutCTPC();
            Dis_DC(opcode);
            }
         }
      if (constant >= 4) {
         PutCTPC();
         p1 = fgetc(input);
         p2 = fgetc(input);
         p3 = fgetc(input);
         p4 = fgetc(input);
         printf("         DC    H'%02X %02X %02X%02X'", p1, p2, p3, p4);
         pc += 4;
         count += 4;
         constant -= 4;
         }
      tlen -= 12;
      }
   }
}

/***************************************************************
*
*  Dis_Cns - disassemble a constant as a DC statement(s)
*
***************************************************************/

void Dis_Cns (int opcode)

{
int i, delta;
int b1,b2;

while (opcode != 0) {
   printf("         DC    H'");
   i = 16;
   while ((i != 0) && (opcode != 0)) {
      if (opcode == 1) {
         printf("%02X", (int) fgetc(input));
         delta = 1;
         }
      else {
         b1 = fgetc(input);
         b2 = fgetc(input);
         if (opcode == 2)
            printf("%02X%02X", (int) b1, (int) b2);
         else
            printf("%02X%02X ", (int) b1, (int) b2);
         delta = 2;
         }                   
      opcode -= delta;
      i -= delta;
      pc += delta;
      count += delta;
      }
   putchar('\'');
   if (opcode != 0) {
      CheckESC();
      PutCTPC();
      }
   }
}

/***************************************************************
*
*  DpDisasm - Dump file in disassembled format
*
***************************************************************/

void DpDisasm (void)

{
int i;
long mark;
long codelen;

while (NextSeg()) {
   count =				/* initialize counter and pc */
   pc =
   constant = 0;			/* we're not in a constant */
   CheckESC();
   if (checkf || header)		/* write out the header */
      if (! PutHeader())
         return;
   if ((! checkf) && (! header)) {	/* get segment length */
      mark = GetEOF(input);
      blocks = 30000;
      }
   else if ((ftype == LIBFL) || (version == 2)) 
      mark = blkcnt;
   else
      mark = blkcnt*512;
   if (body) {				/* write out the body */
      if ((! header) && checkf) {
         count = bod_disp;
         fseek(input, ftell(input) + bod_disp, SEEK_SET);
         }
      PutStreg();			/* write the registar status */
      PutCTPC();
      printf(segname);			/* print the segment name */
      for (i=strlen(segname); i<8; i++) /* fill out with spaces */
         putchar(' ');
      if (kind & 1)			/* print out segment type */
         printf(" DATA");
      else
         printf(" START");
      codelen = length-resspc;
      while (count <= mark) {
         PutCTPC();
         GetOpCode();
         if (checkf && ((pc > codelen) || ((! opcode) && (pc == codelen))))
            break;
         if (((kind & 0x001F) == 1) && checkf
            && (opcode < OPALIGN) && (opcode != 0))
            Dis_Cns(opcode);
         else if (IsExpr(opcode))
            Dis_DC(opcode);
         else
            if (!Disasm(opcode)) {
               PutCTPC();
               Dis_DC(opcode);
               }
            else if (sTable)
               DisSTable();
         CheckESC();
         }
      if (resspc) {
         printf("         DS    $%08lX", (long) resspc);
         count += resspc;
         }
      if (count > mark)
         PutCTPC();
      puts("         END");		/* print out the end directive */
      }
   if ((! shorth) || body)
      puts("\n");

   if (!checkf && !header) break;
   }
}

/**** Hex Dump Subroutines ****************************************/

/***************************************************************
*
*  DpHex - Dump file in hex format
*
***************************************************************/

void DpHex (void)                                                             
																			
{
long mark;				/* next segment mark */
long ch = 0;
char chline[20];
int k, i, space;

while (NextSeg()) {
   count = 0L;
   CheckESC();
   if (checkf || header)                /* write out the header */
      if (! PutHeader())
         return;

   /* if we don't print header and we're not doing file checking then
      dump the entire file and forget about segment scanning */
   if ((! checkf) && (! header)) {	/* get the segment length */
      mark = GetEOF(input);
      blocks = 30000;
      }
   else if ((ftype == LIBFL) || (version == 2)) 
      mark = blkcnt;            
   else 
      mark = blkcnt*512;
   if (body) {                          /* write out the body */
      if ((! header) && checkf) {
         count = bod_disp;
         fseek(input, ftell(input) + bod_disp, SEEK_SET);
         }

      while (count <= mark) {
         putchar('\n');
         if (colf) 
            printf("%06lX  |  ", count);
         space = 0;
         for (k=0; k<16; k++) {
            ch = fgetc(input);
            count++;
            if (count > mark) {
               printf("  ");
               ch = 0;
               }
            else
               printf("%02X", ch);
            if ((ch & 127) > 31)
               chline[k] = ch & 127;
            else
               chline[k] = ' ';
            space++;
            if (space == 4) {
               putchar(' ');
               space = 0;
               }
            CheckESC();
            }
         chline[16] = 0;
         printf(" |  %s", chline);
         CheckESC();
         }
      }
   if ((! shorth) || body) printf("\n\n");

   if (!checkf && !header) break;
   }
}

/**** Command Line Input Subroutines ******************************/

/***************************************************************    
*
*  GetFType - Return Type Of File
*
*  Inputs:
*	name - file name to get the type of
*
*  Returns: file type; 0 if not found
*
***************************************************************/

int GetFType (char name[])

{
struct FileInfoRecGS {
   int pCount;
   struct {
      int length;
      char name[FILEMAX];
      } *fname;
   int access;
   int fileType;
   } parm;

parm.pCount = 3;
parm.fname = (void *) malloc (sizeof(int) + FILEMAX);
if (parm.fname == NULL)
   return 0;
parm.fname->length = strlen(name);
strcpy(parm.fname->name, name);
GetFileInfoGS(&parm);
free(parm.fname);
if (toolerror()) 
   return 0;
return parm.fileType;
}

/***************************************************************
*
*  CheckFile - Checks For Valid File
*
***************************************************************/

BOOLEAN CheckFile (char file[])

{
if (! checkf) {				/* see if we have to check file type */
   ftype = FALSE;			/* unspecified file type */
   return TRUE;
   }
/* insure file exists and is an OMF type of file */
if (! (ftype = GetFType(file))) {
   Error(5,file);
   return FALSE;
   }
if ((ftype < OBJFL) || (ftype > MAXFL)) {
   Error(6,file);
   return FALSE;
   }
return TRUE; 
}

/***************************************************************
*
*  ExpandDev - Expand devices and prefixes
*
***************************************************************/

#define INIT_WILDCARD(parm)    (PDosInt(0x0149,parm))
#define NEXT_WILDCARD(parm)    (PDosInt(0x014A,parm))

void ExpandDev (char name[])

{
struct {				/* parameter block for Init_Wildcard */
   int pcount;
   struct {
      int length;
      char name[FILEMAX];
      } *inName;
   int flags;
   } iwRec;

struct {				/* parameter block for Next_Wildcard */
   int pcount;
   struct {
      int buffLen;
      int length;
      char name[FILEMAX];
      } *outName;
   } nwRec;

iwRec.inName = (void *) malloc (sizeof(int) + FILEMAX);
nwRec.outName = (void *) malloc (sizeof(int)*2 + FILEMAX);
if ((iwRec.inName == NULL) || (nwRec.outName == NULL)) {
   if (iwRec.inName != NULL)
      free(iwRec.inName);
   if (nwRec.outName != NULL)
      free(nwRec.outName);
   return;
   }
iwRec.pcount = 2;
iwRec.inName->length = strlen(name);
strcpy(iwRec.inName->name, name);
iwRec.flags = 0x4000;
nwRec.pcount = 1;
nwRec.outName->buffLen = FILEMAX+4;
INIT_WILDCARD(&iwRec);
if (!toolerror()) {
   NEXT_WILDCARD(&nwRec);
   if (!toolerror()) {
      memcpy(name, nwRec.outName->name, nwRec.outName->length);
      name[nwRec.outName->length] = (char) 0;
      }
   }
free(iwRec.inName);
free(nwRec.outName);
}

/***************************************************************
*
*  SetFlag - Sets the input parameters flags
*
***************************************************************/

BOOLEAN SetFlag (char *str)

{
char ch;

if (strlen(str) != 2) {
   Error(4,str);
   return FALSE;
   }
ch = toupper(str[1]);
if ((str[0]) == '+') {			/* handle + flags */
   if (ch == 'X') {format = 2;}
   else if (ch == 'D') {format = 1;}
   else if (ch == 'S') {shorth = FALSE;}
   else if (ch == 'I') {ireg = TRUE;}
   else if (ch == 'L') {longc = TRUE;}
   else if (ch == 'M') {mreg = TRUE;}
   else if (ch == 'H') {header = TRUE;}
   else if (ch == 'O') {body = TRUE;}
   else if (ch == 'F') {checkf = TRUE;}
   else if (ch == 'A') {colf = TRUE;}
   else {Error(4, str); return FALSE;}
   }
else {					/* handle - flags */
   if (ch == 'X') {format = 0;}
   else if (ch == 'D') {format = 0;}
   else if (ch == 'S') {shorth = TRUE;}
   else if (ch == 'I') {ireg = FALSE;}
   else if (ch == 'L') {longc = FALSE;}
   else if (ch == 'M') {mreg = FALSE;}
   else if (ch == 'H') {header = FALSE;}
   else if (ch == 'O') {body = FALSE;}
   else if (ch == 'F') {checkf = FALSE;}
   else if (ch == 'A') {colf = FALSE;}
   else {Error(4, str); return FALSE;}
   }
return TRUE;
}

/***************************************************************
*
*  Reads the names list and sets up namelist pointers
*
***************************************************************/

BOOLEAN GetList (int num, int argc, char *argv[])

{
char local[NAMEMAX];
int i, k;

if (num == argc) {
   names = FALSE;
   return TRUE;
   }
for (i=num; i<=argc; i++) {
   strncat(nlist, argv[i], strlen(argv[i]));
   strncat(nlist, " ", 1);
   }
strcpy(local, nlist);
if (strlen(nlist) < 8) {
   Error(4,local);
   names = FALSE;
   return FALSE;
   }
for (i=0; i<5; i++)
   nlist[i] = toupper(nlist[i]);
if (strncmp(nlist,"NAMES=(",7) != 0) {
   Error(4,local);
   names = FALSE;
   return FALSE;
   }
for (i=0; i<7; i++)
   nlist[i] = ' ';
i = 0;
while (nlist[i] && (nlist[i] != ')')) {
   if (nlist[i] == ',') nlist[i] = ' ';
   i++;
   }
if (! nlist[i]) {
   Error(4,local);
   names = FALSE;
   return FALSE;
   }
nlist[i] = 0;
names = TRUE;
return TRUE;
}

/***************************************************************
*
*  GetName - This is the name input routine
*
***************************************************************/

void GetName (char *name)

{
int i, x;
char ch;

fputs("File name: ", stdout);		/* prompt for file name */
name[0] = (char) 0;			/* read the file name */
fgets(name, NAMEMAX, stdin);
x = strlen(name);			/* strip whitespace */
while ((x != 0) && isspace(name[x-1])) 
   name[--x] = (char) 0;
while (((x = strlen(name)) != 0) && isspace(name[0])) 
   for (i = 0; i < x; ++i)
      name[i] = name[i+1];
}

/***************************************************************
*
*  GetParms - Gets the input parameters
*
***************************************************************/

BOOLEAN GetParms (int argc, char *argv[])

{
char local[NAMEMAX];
int i;

i = 1;
while (i <= argc) {			/* first set the flags */
   strcpy(local,argv[i]);
   ++i;
   if ((local[0] == '+') || (local[0] == '-')) {
      if (! SetFlag(local))
      return FALSE;
      }
   else break;
   }
if (i <= argc)				/* get file name */
   strcpy(fname,argv[i-1]);
else {
   GetName(fname);			/* ask for and get file name */
   i = argc;				/* no names list */
   }
ExpandDev(fname);			/* expand devices */
if (! CheckFile(fname))
   return FALSE;
if ((input = fopen(fname,"rb")) == NULL) {
   Error(5,fname);
   return FALSE;
   }
return GetList(i, argc, argv);
}

/****************************************************************
*
*  Initialize - Initialize variables
*
****************************************************************/

void Initialize (void)

{
puts("DumpOBJ 2.0.2 B1\n");	        /* write the header */

format =				/* dump in opcode format */
namc =					/* no names in namelist */
ftype =					/* no specified file type yet */
status = 0;				/* assume no errors */

segmark =				/* start with begining segment */
blocks = 0L;				/* no block yet */

shorth = FALSE;				/* don't do "scan" type output */

ireg =					/* use long index registars */
mreg =					/* use long accumulator */
header =				/* print the header */
longc =					/* print long constants */
body =					/* print the body */
checkf =				/* check input file type */
colf = TRUE;				/* write out the numbers */
}

/**** Main Programs ***********************************************/

/****************************************************************
*
*  Main - Begin Program
*
****************************************************************/

int main (int argc, char * argv[])

{
Initialize();				/* initialize variables */
if (GetParms(argc, argv)) {		/* get input parameters */
   switch (format) {
      case  0: DpOpCode(); break;	/* dump file in opcode format */
      case  1: DpDisasm(); break;	/* dump file in disassembled format */
      case  2: DpHex(); break;		/* dump file in hex format */
      default: Error(1," ");		/* flag error if not one of these */
      }
   }
putchar('\n');
return status;
}
