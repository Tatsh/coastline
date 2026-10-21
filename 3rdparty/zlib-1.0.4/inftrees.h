/* inftrees.h -- header to use inftrees.c
 * Copyright (C) 1995-1996 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* Huffman code lookup table entry--this entry is four bytes for machines
   that have 16-bit pointers (e.g. PC's in the small or medium model). */

typedef struct inflate_huft_s FAR inflate_huft;

struct inflate_huft_s {
  union {
    struct {
      Byte Exop;        /* number of extra bits or operation */
      Byte Bits;        /* number of bits in this code or subcode */
    } what;
    Bytef *pad;         /* pad structure to a power of 2 (4 bytes for */
  } word;               /*  16-bit, 8 bytes for 32-bit machines) */
  union {
    uInt Base;          /* literal, length base, or distance base */
    inflate_huft *Next; /* pointer to next level of table */
  } more;
};

#ifdef DEBUG
  extern uInt inflate_hufts;
#endif

/* Game change: the data of inftrees.c is the game's, at the USA addresses given. */
extern char inflate_copyright[];        /* 0x8028c8a0 */
extern uInt cplens[31];                 /* 0x8028c8d0 */
extern uInt cplext[31];                 /* 0x8028c94c */
extern uInt cpdist[30];                 /* 0x8028c9c8 */
extern uInt cpdext[30];                 /* 0x8028ca40 */
extern int fixed_built;                 /* 0x8028cab8 */
#define FIXEDH 530      /* number of hufts used by fixed tables */
extern inflate_huft fixed_mem[FIXEDH];  /* 0x80368ad0 */
extern uInt fixed_bl;                   /* 0x80369b60 */
extern uInt fixed_bd;                   /* 0x80369b64 */
extern inflate_huft *fixed_tl;          /* 0x80369b68 */
extern inflate_huft *fixed_td;          /* 0x80369b6c */

extern int inflate_trees_bits OF((
    uIntf *,                    /* 19 code lengths */
    uIntf *,                    /* bits tree desired/actual depth */
    inflate_huft * FAR *,       /* bits tree result */
    z_streamp ));               /* for zalloc, zfree functions */

extern int inflate_trees_dynamic OF((
    uInt,                       /* number of literal/length codes */
    uInt,                       /* number of distance codes */
    uIntf *,                    /* that many (total) code lengths */
    uIntf *,                    /* literal desired/actual bit depth */
    uIntf *,                    /* distance desired/actual bit depth */
    inflate_huft * FAR *,       /* literal/length tree result */
    inflate_huft * FAR *,       /* distance tree result */
    z_streamp ));               /* for zalloc, zfree functions */

extern int inflate_trees_fixed OF((
    uIntf *,                    /* literal desired/actual bit depth */
    uIntf *,                    /* distance desired/actual bit depth */
    inflate_huft * FAR *,       /* literal/length tree result */
    inflate_huft * FAR *));     /* distance tree result */

extern int inflate_trees_free OF((
    inflate_huft *,             /* tables to free */
    z_streamp ));               /* for zfree function */

