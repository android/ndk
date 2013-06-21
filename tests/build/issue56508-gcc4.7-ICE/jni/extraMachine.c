
typedef int Boolean;

typedef unsigned char univ;
typedef unsigned short int Uint16;
typedef unsigned char Uint8;
typedef int Sint32;
typedef void *hdlno;
typedef Sint32 fontno;
typedef short int Sint16;

struct save_restore_link {
  struct hdl *next;  /* next modified handle */
  struct hdl *prev;  /* previous modified handle */
};

/*
** The first five entries entries must match "bakhdl" below.
*/
typedef struct hdl {
    univ *raw;		/* composite object value */
    Uint16 dim;		/* # items in composite object (formerly "cnt") */
    char cat;		/* object category (a.k.a. type) + non-gc status bits */
    char acc_lev;	/* object attributes, save level modified or created */
    struct save_restore_link svlink;
    Uint8 coded_depth;	/*   0 ==> at some depth > 253;
			     1 ==> not on stack;
			     2 ==> at dictstak[0];
			     3 ==> at dictstak[1];
			   255 ==> at dictstack[253]
			   (see macros below)
			*/
    char stat_rootlev;	/* status bits + object creation savelevel */
    Uint8 handlev;      /* VM level of this handle; used to maintain freelist */
    Uint8 fastlev;	/* coded save level for faster OBsave() logic:
			 *   0 ==> (acc_lev == 0x10)  PreScript,    ROM
			 *   1 ==> (acc_lev == 0x00)  savelevel  0, RAM
			 *   2 ==> (acc_lev == 0x01)  savelevel  1
			 *  16 ==> (acc_lev == 0x0F)  savelevel 15 (last)
			 *  17 ==>		      global level 1
			 */
    union {
      /*
      ** In a string, array, or packedarray subinterval handle,
      ** "superset" points to the parent string handle.
      **/
      struct hdl *superset;  /* see above */
      struct hdl *gclink;    /* next composite to be scanned */
      struct hdl *nextfree;  /* next handle on freelist */
    } u;
} HDL;

#define INI_HDLCNT	742

struct hdl localhand[INI_HDLCNT];

#define QUADINIT(tp,at) \
    ((((unsigned long)(at)<<8)&0x0000ff00) | \
     ((unsigned long)(tp)&0x000000ff))

#define 	COMPOSGROUP		0x10
#define		CT_PAKARRY	0x0D
#define		PAKARRYTYPE		(COMPOSGROUP| CT_PAKARRY)

#define 	EXECATTR	01
#define 	READONLY	04

#define LCL_i(hnum) ((unsigned long)&localhand[hnum])
#define JOBXHDL 117

struct compositeOB {
	unsigned long   type_attr_pos;    /* force .ALIGN 4 */
	unsigned long   hdl;
};

static const struct compositeOB
  do_jobx_i = {QUADINIT (PAKARRYTYPE, EXECATTR | READONLY), LCL_i(JOBXHDL)};

#define 	OPERGROUP		0x20
#define		ABORTTYPE		(OPERGROUP | 0x0C)
#define		MACHCALLTYPE	(OPERGROUP | 0x0D)

#define STOP_CALL	1	/* --stop-- operator */


struct filerep {
    Sint16 des;
    Sint16 assoc;
} ;

struct stoprep {
    Uint16 userjob;	/* True if in user job at "stopped" operator */
    Uint16 saverr;	/* If true, any error is saved for machcall return */
} ;

struct dhdr1rep {
    Uint16 entmax;	/* Maximum entry count    */
    Uint16 entcnt;	/* Current entry count */
} ;

struct dhdr2rep {
    Uint16 sorted;	/* Total sorted entries   */
    Uint16 sortable;	/* Total sortable entries */
} ;

typedef struct ob {
    Uint8 type;
    Uint8 attr;
    Uint16 pos;
    union {
	Sint32        boolrepr;
	Sint32        intrepr;
	float         realrepr;
	hdlno         hdl;
	struct stoprep stoprepr;
	struct filerep filrepr;
	struct tok   *namrepr;
	const struct par *oprrepr;
	fontno        fontrepr;
	Sint16        savrepr;
	void          (*machcallhandler)(int, int);
	void          (*aborthandler)(int);
	void         *voidprepr;
	struct dhdr1rep dhdr1repr;
	struct dhdr2rep dhdr2repr;
    } un;
} OB;

/* get type field from object */
#define OBtype(obp) ((obp)->type)

/* get hdl entry from object */
#define OBhdl(obp) ((obp)->un.hdl)

/* get pos field from object */
#define OBpos(obp) ((obp)->pos)

OB *execptr;

/*
* Clean the execution stack back to the outermost user job level
* or until the stack is completely empty.
*/
int clean_exec_stack (Boolean early_end)
{
    OB  *pobr;
    hdlno jobhdl;

    jobhdl  = OBhdl((OB *)&do_jobx_i);

    pobr = execptr - 1;

    if (OBhdl(pobr) == jobhdl)
	{
	    return 1;

	}

    return (0);

}

int main()
{
}
