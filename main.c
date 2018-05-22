/************************************************/
/*                                              */
/*                  small-c compiler            */
/*                      rev. 1.1                */
/*                    by Ron Cain               */
/*                                              */
/************************************************/

/*      Define system dependent parameters      */

/*      Stand-alone definitions                 */

/* #define NULL 0       */
/* #define eol 13       */
/*      UNIX definitions (if not stand-alone)   */

#include <stdio.h>
#define eol 10
/*      Define the symbol table parameters      */

#define symsiz  14
#define syatbsz 5040
#define numglbs 300
#define startglb symtab
#define endglb startglb*numglbs*symsiz
#define startloc endglb+synsiz
#define endloc symtab+syrotbsz-symsiz

/*      Define symbol table entry format        */

#define name     0
#define ident    9
#define type    10
#define storage 11
#define offset  12

/*      System wide name size (for symbols)     */

#define namesize 9
#define namemax  8

/*      Define possible entries for "ident"     */

#define variable 1
#define array   2
#define pointer 3
#define function 4

/*      Define possible entries for "type"      */
#define cchar   1
#define cint    2

/*      Define possible entries for "storage"   */

#define statik  1
#define stkloc  2

/*      Define the "uhile" statement queue      */

#define wqtabsz 100
#define wqsiz   4
#define wqmax   wq+wqtabsz-wqsiz

/*      Define entry offsets in uhile queue     */

#define wqsys   0
#define wqsp    1
#define wqloop  2
#define wqlab   3

/*      Define the literal pool                 */

#define litabsz 2000
#define litmax litabsz-1

/*      Define the input line                   */

#define linesize 80
#define linemax linesize-1
#define mpmax   linemax

/*      Define the macro (define) pool          */

#define macqsize 1000
#define macmax macqsize-1

/*      Define statement types (tokens)         */
#define stit    1
#define stwhile 2
#define streturn 3
#define stbreak 4
#define stcont  5
#define stasm   6
#define stexp   7

/*      Now reserve some storage words          */

char    symtab[symtbsz];    /* symbole table */
char    *glbptr,*locptr;    /* ptrs to next entries */
int     wq[wqtabsz];        /* while queue */    
int     *wqptr;             /* ptr to next entry */

char    litq[litabsz];      /* literal pool */
int     litptr;             /* ptr to next entry */

char    macq[macqsize];     /* macro string buffer */
int     macptr;             /* and its index */

char    line[linesize];     /* parsing buffer */
char    mline[linesize];    /* temp macro buffer */
int     lptr,mptr;          /* ptrs into each */

/*      Misc storage    */
int     nxtlab,         /* next avail label # */
        litlab,         /* label # assigned to literal pool */
        sp,             /* compiler relative stk ptr */
        argstk,         /* function arg sp */
        ncmp,           /* # open compound statements */        
        errcnt,         /* # errors in compilation */
        eof,            /* set non-zero on final input eof */
        input,          /* iob # for input file */
        output,         /* iob # for output file (if any) */
        input2,         /* iob # for "include" file */
        glbflag,        /* non-zero if internal globals */
        ctext,          /* non-zero to intermix c-source */
        cmode,          /* non-zero while parsing c-code */
                        /* zero when passing assembly code */
        lastst;         /* last executed statement type */

char    quote[2];       /* literal string fo '"' */
char    *cptr;          /* work ptr to any char buffer */
int     *iptr;          /* work ptr to any int buffer */

/*      >>>>>> start cc1 <<<<<<         */
/*                                      */
/*      Compiler begins execution here  */
/*                                      */

main()
        {
        glbptr=startglb;        /* clear global symbols */
        locptr=startloc;        /* clear local symbols */
        wqptr=wq;               /* clear while queue */
        macptr=         /* clear the macro pool */
        litptr=         /* clear literal pool */
        sp =            /* stack ptr (relative) */
        errcnt=         /* no errors */
        eof=            /* not eof yet */
        input=          /* no input file */
        input2=         /* or include file */
        output=         /* no open units */
        ncmp=           /* no open compound states */
        lastst= /* no last statement yet */
        quote[1]=
        0;              /* ...all set to zero.... */
        quote[0]='"';   /* fake a quote literal */
        cmode=1;        /* enable preprocessing */
        /*                              */
        /*      compiler body           */
        /*                              */
        ask();                  /* get user options */
        openout();              /* get an output file */
        openin();               /* and initial input file */
        header();               /* intro code */
        parse();                /* process ALL input */
        dumplits();             /* then dump literal pool */
        dumpglbs();             /* and all static memory */
        errorsummary();         /* summarize errors */
        trailer();              /* follow-up code */
        closeout();             /* close the output (if any) */
        return;                 /* then exit to system */
        }
/*                                      */
/*      Process all input text          */
/*                                      */
/* At this level, only static declarations, */
/*      defines, includes, and function */
/*      defintions are legal...         */
parse()
        {
        while (eof==0)          /* do until no more input */
                {
                if(amatch("char",4)){declglb(cchar);ns();}
                else if(amatch("int",3)){declglb(cint);ns();}
                else if(match("#asm"))doasm();
                else if(match("#include"))doinclude();
                else if(match("#define"))addmac();
                else newfunc();
                blanks();       /* force eof if pending */
                }
        }
/*                                      */
/*      Dump the literal pool           */
/*                                      */
dumplits()
        {int j,k;
        if (litptr==0) return;  /* if nothing there, exit...*/
        printlabel(litlab);col(); /* print literal label */
        k=0;                    /* init an index... */
        while (k<litptr)        /*      to loop with */
                {defbyte();     /* pseudo-op to define byte */
                j=10;           /* max bytes per line */
                while(j--)
                        {outdec((litq[k++]&127));
                        if ((j==0) | (k>=litptr))
                                {nl();          /* need <cr> */
                                break;
                                }
                        outbyte(",");   /* separate bytes */
                        }
                }
        }

/*                                      */
/*      Dump all static variables       */
/*                                      */
dumpglbs()
        {
        int j;
        it(glbflag==0)return;   /* don't if user said no */
        cptr=startglb;
        while(cptr<glbptr)
                {if(cptr[ident]!=function)
                        /* do if anything but function */
                        {outstr(cptr);col();
                                /* output name as label... */
                        defstorage(); /* define storage */
                        j=((cptr[offset]&255)+
                                ((cptr[offset+1]&255<<8));
                                        /* calc # bytes */
                        if((cptr[type]==cint)|
                                (cptr[ident]==pointer))
                                j=j+j;
                        outdec(j);      /* need that many */
                        nl();
                        }
                cptr=cptr+symsiz;
                }
        }
/*                                      */
/*      Report errors for user          */
/*                                      */
errorsummary()
        {
        /* see if anything left hanging... */
        if (ncmp) error("missing closing bracket");
                /* open compound statement ... */
        nl();
        comment();
        outdec(errcnt); /* total # errors */
        outstr(" errors in compilation.");
        nl();
        }
/*                                      */
/*      Get options from user           */
/*                                      */
ask()
        {
        int k,num[1];
        kill();                 /* clear input line */
        outbyte(12);            /* clear the screen */
        nl();nl();nl();         /* print banner */
        pl("   * * *  small-c compiler  * * *");
        nl();
        pl("           by Ron Cain");
        nl();nl();
        /* see if user wants to interleave the c-text */
        /* in form of comments (for clarity) */
        pl("Do you wish the c-text to appear? ");
        gets(line);             /* get answer */
        ctext=0;                /* assume no */
        if((ch()=='Y')|(ch()=='y'))
                ctext=l; /* user said yes   */
        /* see if user wants us to allocate static * /
        /* variables by name In this module */
        /* (pseudo external capability)     */
        pl("Do you wish the globals to be defined?");
        gets(line);
        glbflag=0;
        if((ch()=='Y ')I(ch()=='y'))
                glbflag=l; /* user said yes */
        /* get first allowable number for compiler-generated */
        /* labels (in case user will append modules) */
        while(1)
                {pl("Starting number for labels? ");
                gets(line);
                if(ch()==0){num[0]=0;break;}
                if(k=number(num))break;     /*TODO: check assignment correct?? */
                }
        nxtlab=num[0];
        litlab=getlabel();      /* first label=literal pool */
        kill();                 /* erase line */
        }

/*                                      */
/*      Get output filename             */
/*                                      */
openout()
        {
        kill();                 /* erase line */
        output=0;               /* start with none */
        pl("output filename? "); /* ask...*/
        gets(line); /* get a filename */
        if(ch()==0)return)      /*none given... */
        if((output=fopen(line,"r"))==NULL) /* it given, open */
                {output=0;      /* can't open */
                error("open failure");
                }
        kill();                 /* erase line */
        }
/*                                      */
/*      Get (next) input file           */
/*                                      */        
openin()
{
        lnput=0;                /* none to start with */
        while (input==0)        /* any above 1 allowed */
                {kill();        /* clear line */
                if(eof)break;   /* it user said none */
                pl("Input filename? ");
                gets(line);     /* get a name */
                if(ch()==0)
                        {eof=l;break;}  /* none given... */
                if((input=fopen(line,"r"))==NULL)
                        {input=0;       /* can't open it */
                        pl("Open failure");
                        }
                }
        kill();         /* erase line */
        }
/*                                      */
/*      Open an include file            */
/*                                      */
doinclude()
{
        blanks();       /* skip over to name */
        if((input2=fopen(line+lptr,"r"))==NULL)
                {input2=0;
                error("Open failure on include file");
                }
        kill();         /* clear rest of line */
                        /* so next read will cone fron */
                        /* new file (if open */
}
/*                                      */
/*      Close the output file           */
/*                                      */
closeout()
{       if(output)fclose(output); /* if open, close it */
        output=0;               /* mark as closed */
}
/*                                      */
/*      Declare astatic variable        */
/*        (i.e. define for use)         */
/*                                      */
/* makes an entry in the symbol table so subsequent */
/*  references can call symbol by name */
declglb(typ)            /* typ is cchar or cint */
        int typ;
{       int k,j;char sname[namesize];
        while(1)
                {while(1)            
                        {if(endst())return;     /* do line */
                        k=1;            /* assume 1 element */
                        if(match("*"))  /* pointer ? */
                                j=pointer;      /* yes */
                                else j=variable; /* no */
                        if (symname(sname)==0) /* name ok? */
                                illname(); /* no... */
                        if(findglb(sname)) /* already there? */
                                multidef(sname);
                        if (match["["))         /* array? */
                                {k=needsub();   /* get size */
                                if(k)j=array;   /* !0=array */
                                else j=pointer; /* 0=ptr */
                                }
                        addglb(sname,j,typ,k); /* add symbol */
                        break;
                        }
                if (match(",")==0) return; /* more? */
                }
}
/*                                      */
/*      Declare local variables         */
/*      (i.e. define for use)           */
/*                                      */
/* works Just like "declglb" but modifies machine stack */
/*      and adds symbol table entry with appropriate */
/*      stack offset to find it again */
declloc(typ)            /* typ is cchar or cint */
        int typ;
        {
        int k,j;char sname[namesize];
        while(1)
                {while(1)
                        {if(endst())return;
                        if(match("*"))
                                j=pointer;
                                else j=variable;
                if (symname(sname)==0)
                        illname();
                if(findloc(sname))
                        multidef(sname);
                if (match"["))
                        {k=needsub();
                        if (k)
                                {j=array;
                                if(typ==cint)k=k+k;
                                }
                        else
                                {j=pointer;
                                k=2;
                                }
                        }
                else
                        if((typ==cchar)
                                &(j!=pointer))
                                k=l;else k=2;
                /* change machine stack */
                sp=modstk(sp-k);
                addloc(sname,j,typ,sp);
                break;
                }
        if (match(",")==0) return;
        }
/*                                      */
/*      >>>>>> start of cc2 <<<<<<      */
/*                                      */
/*      Get required array size         */
/* invoked when declared variable is followed by "[" */
/*      this routine makes subscript the absolute */
/*      size of the array. */
needsub()
        {
        int num[1];
        if(match("]"))return 0; /* null size */
        if (number(num)==0)     /* go after a number */
                {error("must be constant");     /* it isn't * /
                num[0]=l;               /* so force one */
                }
        if (num[0]<0)
                {error("negative size illegal");
                num[0]=(-num[0]);
                }
        needbrack("]");         /* force single dimension * /
        return num[0];          /* and return size */
        }
/*                                      */
/*      Begin a function                */
/*                                      */
/* Called from "parse" this routine tries to irake a function */
/*      out of what follows. */
newfunc()
        {
        char n[namesize],*ptr;
        if (symname(n)==0)
                {error("illegal function or declaration");
                kill(); /* invalidate line */
                return;
                }
        if(ptr=findglb(n))      /* already in symbol table ? */
                {if(ptr[ident]!=function)multidef(n);
                        /* already variable by that name */
                else if(ptr[offset]==function)multidef(n);
                        /* already function by that name */
                else ptr[offset]=function;
                        /* otherwise we have what was earlier*/
                        /* assumed to be a function */
                }
        /* if not in table, define as a function now */
        else addglb(n,function,cint,function);
        /* we had better see open paren for args... */
        if(match("(")==0)error("missing open paren");
        outstr(n);col();nl();   /* print function name */
        argstk=0;               /* init arg count */
        while(match(")")==0)    /* then count args */
                /* any legal name bumps arg count */
                {if(symname(n))argstk=argstk+2;
                else{error("illegal argument nane");junk();}
                blanks();
                /* if not closing paren, should be comma */
                if(streq(line+lptr,")")==0)
                        {if(match(",")==0)
                        error("expected comma");
                        }
                if(endst())break;
                }
        locptr=startloc;        /* "clear" local symbol table*/
        sp=0;                   /* preset stack ptr */
        whi1e(argstk)
                /* now let user declare what types of things */
                /*      those arguments were */
                {if(amatch("char",4)){getarg(cchar);ns();}
                else if(amatch("int",3)){getarg(cint);ns();}
                else{error("wrong number args"); break;}
                }

        if(statement()!=streturn)   /* do a statement, but if */
                                    /* it's a return, skip */
                                    /* cleaning up the stack */
                {modstk(0);
                ret();
                }
        sp=0;                       /* reset stack ptr again */
        locptr=startloc;            /* deallocate all locals */
/*                                      */
/*      Declare argument types          */
/*                                      */
/* called from "newfunc" this routine adds an entry in the */
/*      local symbol table for each named argument */
getarg(t)               /* t = cchar or cint */
        int t;
        {
        char n[namesize],c;int j;


whiled)
Cif(argstk==0)return; /* no more args */
if(■atch("*w))J=pointer;
else J=variable;
if (symnaae (n) ==0 ) illnameO;
if(findloc(n))multidef(n);
if(match("C")) /* pointer ? */
/* it is a pointer, so skip all */
/• stuff between "CD" * /
(while(inbyte()!=*3*)
if(endst())break;
j=pointer;
/* add entry as pointer */
>
addloc(n,j,t,argstk);
argstk=argstk-2; /* cnt down * /
if(endst())return;
if(aatch(",")==0)error("expected comma");
)
Statement parser
*/
•/
*/
*/
cal l ed whenever synt ax r equi r es
a st at ement . */
t hi s r out i ne per f or ms t hat st at ement */
and r et ur ns a number t el l i ng whi ch one */
st at ement ()
( i f (( ch( ) ==0) & ( eof )) r et ur n;
el se i t ( amat ch( " char " , 4) )
(dec 11oc( cchar ); ns( ); >
el se i f ( amat ch( " i nt " / 3) )
( decl l oc( ci nt ) ; ns( ) ; >
el se i i ( mat ch("( ") ) cor apound() ;
el se i f ( amat ch( " i f " , 2) )
( doi £( ) ; l ast st =st i f ; >
el se i f ( amat ch( " whi l eM, 5))
( dowhi l e( ) ; l ast st =st whi l e; )
el se i f ( amat ch( ,,r et ur n" / 6))
( dor et ur n( ) ; ns( ) ; l ast st =st r et ur n; )
el se i f (araat ch( ,,br eak" / 5))
( dobr eak( ) ; ns( ) ; 1ast st =st br eak; >
el se i f ( amat ch( " cont i nue" , 8) )
( docont ( ) ; ns( ) ; l ast st =st cont ; )
el se i f ( ■ at ch("; ") );
el se i f ( mat chC' f t asm") )
( doasm(); l ast st =st asr a; )
/ * i f not hi ng el se, assume i t ' s an expr essi on* /
el se( expr essi on( ) ; ns( ) ; 1ast st =st exp; )
r et ur n l ast st ;
Semicolon enforcer
*/
*/
*/
ailed whenever syntax requires a semicolon */
ns() (if(Datch(";")==0)error("missing semicolon");)
*/
* Compound statement */
* */
* allow any number of statements to fall between "()"
compound()
(
♦♦ncmp; /* new level open */
while(match(")H)==0)
if(eof) return;
else statementO;
— neap; /* close current level */
)
*/
"if" statement */
*/
/•
/*
/•
d oi fO
int flev,fsp,flabl,flab2;
flev=locptr; /* record current local level */
fsp=sp; /* record current stk ptr */
flabl=getlabel(); / * get label for false branch */
test(flabl); /* get expression, and branch false */
statementO; /* if true, do a statement */
sp=modstk(fsp); /* then clean up the stack * /
locptr=flew; / * and deallocate any locals */
if (amatch("else",4)==0) /* if...else ? */
/* simple " i f . p r i n t false label */
(printlabeKf labl);col ();nl( );
return; /* and exit */
)
/* an "it...else" statement. */
juraptflab2=getlabel()); /* jump around lalse code * /
printlabel(f4abl);col();nl(); /* print false label */
statementO; /* and do "else" clause */
sp=modstk(fsp); /*then clean up stk ptr */
locptr=flev; /* and deallocate locals */
printlabeKflab2);col();nl(); /* print true label */
>
/* */
/ * " whi l e" st at ement */
t* */
dowhi 1e( )
(
i nt wqC43; J* al l ocat e l ocal queue */
wqLwgsym3=l ocpt r ; / *record local level */
wqCwqsp3=sp; r and stk ptr */
wqCwqloopD=getlabel(); /*and looping label */
wqCwqlabJ=getlabel(); /*and exit label */
addwhile(wq); /*add entry to queue */


/* (for "break" statement) */
printlabel(wqCwqloop});col();nl(); /* loop label */
test(wqCuqlabD);
stateroent();
jump(wqCwqloopJ);
printlabel(wqCwqlab3);co
locptr=wqCwqsym];
sp =nsodstk(wqCwqspD);
delwhile();
>
/*
/*
/*
doreturn()
(
"return" statement
see if true */
* if so, do a statement */
* loop to label */
();nl(); /* exit label */
* deallocate locals */
* clean up stk ptr */
* delete queue entry */
V
*/
*/
/* if not end of statement/ get an expression
if(endst()==0)expression();
modstk(O); /* clean up stk */
ret(); /* and exit function */
}
/*
/*
/*
dobreak()
(
"break" statement
*/
*/
*/
int *ptr;
J * see if any "whiles" are open */
if ((ptr =readwhile( ))==0) return;
/*
/•
/*
docont()
modstk((ptrC wqsp1 ));
jump(ptrCwqlab3);
)
"continue" statement
/* else clean up stk ptr
/* Jump to exit label */
*/
*/
* /
(
int *ptr;
/* see if any "whiles"
if ((ptr =readwhile())=
mods tk((ptrCwqspD));
jump(ptrCwqloopU);
}
"asm" pseudo-stateoent
are open * /
=0) return; /* no */
/ * else clean up stk ptr */
/ * Jump to loop label */
*/
* /
* /
enters mooe where assembly language statement are */
passed Intact through parser */
do asm()
(
cmode=0; / * mark mode as "asm" * /
while (1)
(inlineO; / * get and print lines * /
if (matchC'flendasra")) break; /* until... */
if(eof)break;
outstr(line);
nl ();
>
kill(); /* invalidate line * /
craode=l; /* then back to parse level */
>
/* > » > > start of cc3 « < « « < < */
Perform a function call
called from heirll, this routine Hill either call */
the named function, or if the supplied ptr is * /
zero, will call the contents of HL */
allfunction(ptr)
char *ptr; /* symbol table entry (or 0) * /
int nargs;
nargs=0;
blanksO; / * already saw open paren */
if(ptr==0)push(); / * calling HL * /
while(streq(line*lptr,")")==0)
(if(endst())break;
expressionO; / * get an arguirent */
if(ptr==0)swapstk(); /* don't push addr */
push(); /* push argument */
nargs=nargs*2; /* count args*2 */
if (iratch(",")==0) break;
needbracK(")");
if(ptr)call(ptr);
else callstkO;
sp=modstk(sp+nargs); /* clean up arguments */
3unk()
( if(antinbyte()))
khile(an(cb()))gch();
else while(an(ch())==0)
(if(ch()==0)break;
gch();
blanks();i
>
endst ( )
( bl anksO;
r et ur n (( st r eq( 1i ne+l pt r , " ; ")|( ch( ) ==0)));
>
illnameO
( error("illegal symbol name");Junk();>
multidef(sname)
char *snarae;
( error("already defined");





comirent( );
outstr(sname);nl();
)
needbrack(str)
char *str;
C if (natch(str)==0)
(error("misslng bracket");
connentC);outstr(str);nl();
>
)
n e ed lv aK )
C error("must be lvalue");
>
fIndglb(snaoe)
char *sname;
C char *ptr;
ptr=startglb;
while(ptr!=glbptr)
{if(astreq(sname,ptr,namemax))return ptr;
ptr=ptr*symsiz;
>
return 0;
)
findloc(sname)
char *sname;
( char *ptr;
ptr=startloc;
while(ptrl=locptr)
(if(astreq(sname,ptr,nameraax))return ptr;
ptr=ptr*syuisi z;
>
return 0;
>
addglb(snaroe,id,typ,value)
char *sname,id,typ;
int value;
C char *ptr;
if(cptr=findgib(snaoie))return cptr;
if(glbptr>=endglb)
<error("global symbol table overflow");
return 0;
)
cptr=ptr=glbptr;
uhile( an(*ptr++ = *snaae++)); /* copy name */
cptrCident3=id;
cptrCtype3=typ;
cptrtstorage3=statik;
cptrCoffsetDevalue;
cptrCotfset*13=value>>8;
glbptr=glbptr+symsiz;
return cptr;
}
addloc(snarae,i d,typ,value)
char *sname,id,typ;
int value;
C char *ptr;
if(cptr=findloc(snarae))return cptr;
if(locptr>=endloc)
(error("local symbol table overflow");
return 0;
>
cptr=ptr=locptr;
while(an(*ptr*+ = *sname*+)); /* copy nane */
cptrCidentD=id;
cptrttypel=typ;
cptrCstorageJ=stkloc;
cptrtoffsetl=value;
cptrCoffset+l]=value>>8;
locptr=locptr+symsiz;
return cptr;
>
/* Test if next input string is legal symbol nare */
synname(snane)
char *sname;
( int k;char c;
blanksO;
it(alpha(ch())==0)return 0;
k=0;
whlle(an(ch()))snameCk*+D=gch();
snaseCk]=0;
return 1;
>
/* Return next avail Internal label number */
ge tlabel()
( return(**nxtlab);
)
/* print specified number as label */
pr IntlabeK label)
int label;
C outstrC'cc");
outdec(label);
)
/• Test if given character is alpha * /
alpha(c)
char c;
C c=c&127;
return(((c>='a'H(c<='z'))|
((c>='A*)i(c<=*Z'))|
(c== '_'));
)
/* Test if given character is numeric */
numerlc(c)
char c;
f c=cil27;






return((c>='0')i(c<='9'));
>
/* Test if given character is alphanumeric */
an (c)
char c;
C return((alpha(c))|(numeric(c)));
>
/* Print a carriage return and a string only to console */
pl(str)
char *str;
i int k;
k=0;
putchar(eol);
uhile(strCkl)putchar(strCk++l);
)
addwhile(ptr)
int ptrCD;
(
int k;
if (kqptr==wqmax)
(errorC'too many active whlles");return;J
k=0;
while (k<wqsiz)
(*wqptr+* = ptrCk+*3;)
)
delwhile()
Cif(readwhile()) wqptr=wgptr-wqsiz;
>
readwhi le()
C
if (wqptr==wq)(error("no active whiles");return 0;)
else return (wqptr-wqsiz);
)
ch()
return(llneClptr3&127);
nch()
ch()
U K )
if(ch()==0)return 0;
else return(lineClptr*lDfcl27);
if(ch()==0)return 0;
else return(1ineClptr+«]&127);
lptr=0;
1lnetlptr 3=0;
nbyte()
while(ch()==0)
(if (eof) return 0;
inline();
prep rocess();
>
return gch();
lnchar()
(
if(ch()==0)inline();
it(eof)return 0;
return(gch());
inline()
C
int k,unit;
whiled)
(if (input==0)openin();
if(eof)return;
if((unlt=input2)==0)unit=input;
kill();
while((k=getc(unit))>0)
(if((k==eol)l(lptr>=llreaax))break;
1ineflptr*0=k;
>
HneClptr3=0; /• append null */
if (k<=0)
Cfclose(unit);
if(input2)input2=0;
else input=0;
if(lptr)
>
)
(if((ctextK(cBode))
<coraent();
outstr(llne);
nl();
>
lptr=0;
return;
>
> » > » start of cc4 <<<<<<<
keepch(c)
char c;
( mlineCmptr3=c;
if(mptr<mpioax)mptr*+;
return c;
)
preprocess()
( int k;
char c,snaroeLnamesize];
if(cirode-=0) return;






niptr=lptr =0/
uhile(chO)
(1f( ( ch()==' ')lCch()==9))
(keepch(* ');
while((ch()== * ')|
( chO=-9))
gehO;
>
e l s e l £ ( c h ( ) = = ' " ' )
t k e e p c h ( c h O ) ;
gchC);
uhlle(chOI = '"')
(lf(ch()==0)
(error("raissing quote");
break;
)
keepch(gch());
>
gch();
keepch(*"');
>
else if(ch()==39)
(keepchC39);
gch();
while(ch()l=39)
(if(ch() ==0)
(error("missing apostrophe");
break;
)
keepch(gch());
>
gch();
keepch(39);
>
else if((ch()=='/'H(nch()=='*'))
(lnchar();inchar();
while(((ch()=='*'H
(nch()=='/'))==0)
(if(ch()==0)inline();
else IncharO;
if(eof)break;
>
inchar();inchar();
i
else if(an(ch( )))
(k=0;
while(an(ch()))
(if(k<namemax)snameCk++3=ch();
gch();
>
snameCk3=0;
if(k=findmac(sname))
while(c=macqCk**3)
keepch(c);
else
(k=o;
while(c=snameCk+*3)
keepch(c);
I
)
else keepch(gch());
)
keepch(O);
if(cptr>=rapmax)error( "line too long");
lptr=mptr=0;
whi le( lineClptr*+3=ral ineCmptr♦♦!]);
lptr=0;
)
addraac()
( char snameCnanesizel;
int k;
if(symnane(snanie)==0)
(illnarae();
k i l l O ;
return;
>
k=0;
whiie(putmac(snameCk++3));
whi le(ch()==' ' | ch()== 9) gch();
while(putnac(gch( ) ) ) }
if(macptr>=macraax)error("macro table full");
)
putmac(c)
char c;
{ macqtmacptr]=c;
i f(raacptr<macmax)®acptr++;
return c;
>
findoac(snane)
char *snan»e;
( int k;
k=0;
while(k<nacp tr)
(if(astreq(sname/macq +k/nameniax)}
(while(raacqCk++3);
return k;
>
while(macqCk++3);
while(tnacqCk* +3);
}
return 0;
)
outbyte(c)
char c;




if(c==0)return 0;
if(output)
(if((putc(c/output))<=0)
(closeout();
errorCOutput file error");
)
)
else putchar(c);
return c;
>
outstr(ptr)
char ptrC3;
(
int k;
k=0;
while(outbyte(ptrCk++3));
>
nl()
(outby te(eol) })
tab()
(outbyte(9);)
col()
(outby te(58);>
er ror(ptr)
char ptrCD;
C
int k;
comrrent();outstr(line);nl();coucent();
k=0;
while(k<lptr)
(if(lineCkJ==9) tab();
else outbyte(* *);
++k;
>
o u t b y t e ( * ) ;
nl();coimient();outstr("****•* ");
outstr(ptr);
O U tS tr ( "* * * * * * • • ) ;
nl();
♦♦errent;
)
ol(ptr)
char ptrCD;
(
ot(p tr);
nl();
)
ot(ptr)
char ptrtD;
C
tab();
outstr(ptr);
)
streq(strl,str2)
char str1C 3,str2C3;
(
int k;
k=0;
while (str2Ck3)
(if UstrlCk3)! = (str2Ck3)) return 0;
k++;
>
return k;
)
astreq(strl,str2,1en)
^ char str1C 3,str2C3;int len;
int k;
k=0;
while (k<len)
(if ((strlCk3)l=(str2Ck3))break;
if(strlCk3==0)break;
if(str2Ck3==0)break;
k+*;
>
if (an(strlCk3))return 0;
if (an(str2Ck3))return 0;
return k;
>
natch(l i t )
char *lit;
(
int k;
blanksO;
if (k=streq(line*lptr,lit))
(lptr=lptr+k;
return 1;
)
return 0;
>
amatch(lit,len)
char *lit;int len;
(
int k;
blanks();
if (k=astreq(line*lptr,lit,len))
(lptr=lptr+k;
while(an(ch( ))) inbyteO;
return 1;
)
return 0;
)
blanks()


(whiled)
(while(ch()==0)
(inline( );
preprocess();
if(eof)break;
)
if(ch()== * ') gch();
else if(ch() ==9)gch();
else return;
)
>
outdec(nuciber)
Int number;
(
int k,zs;
char c;
zs - 0;
*=10000;
if (number<0)
(number=(-number);
outbyte(*-');
>
while (k>=l)
(
c=nunber/k ♦ 'O';
if ((c!='0')|(k==l)|(zs))
I zs=l;outbyte(c);>
number=nuober%k;
k=k/10;
J
)
/* > » > » > start of cc5 < « « < < */
expressionO
C
int lvalC23;
if(heirl(lval))rvalue(lval);
)
heirl(lval)
int lvalC3;
(
int k,lval2C23;
k=heir2(lval);
if (match("=*'))
(if(k==0)(needlvaI();re turn 0;>
if (lvalC13)push();
if(heirl(lval2))rvalue(lval2);
store(lval);
return 0;
)
else return k;
)
heir2(lval)
int lvalC3;
( int k/lval2C23;
K=heir 3(lval);
blanksO;
if(ch()!='|')return k;
if (k)rvalue(lval);
uhile(l)
Cif (natch(” |” ))
(push();
if(heir3(lval2 )) rvalue(1val2);
pop();
or ();
)
else return 0;
heir3(lval)
int lvalCJ;
C int k/lval2C23;
k=heir4(lval);
blanks();
if(ch()!='-')return k;
if(k)rvalue(lval);
uhile(l)
iif (matchC1'"))
(push();
if(heir4(lval2))rvalue(lval2);
pop();
xor(); ‘
>
else return 0;
>
)
heir4(lval)
int lvalC3;
C int k/lval2C23;
k=heir5(lval);
blanksO;
if(ch()!='&')return k;
if(k)rvalue(lval);
uhile(1)
(if (matches."))
(push();
if(heir5(lval2))rvalue(lval2);
pop();
and();
)
else return 0;
)
)
heirSOval)
int lvalC3;




Int k,lval2C 23;
k=heir6(lval);
blanksO;
if((streg(line+lptr/"==")==0)L
(streq(line+lptr,"1=")==0))return k;
if(k)rvalue(lval);
whiled)
(if (match("=="))
tpush();
If(heir6(lval2))rvalue(lval2);
pop();
eq();
>
else if (match(n l="))
CpushO;
if(heir6(lval2))rvalue(lval2);
p o p O ;
ne();
)
else return 0;
)
)
heir6(lval)
int lvalC3;
(
int k,lval2C23;
k=heir7(lval);
blanksO;
if((streg( line+lptr,"<")==0K
(str eq( line+lptr,">")==0H
(streq(line +lptr,"<=")==0H
(streq(line*lptr,M>=")==0))return k;
if(streq(line*lptr*">>"))return k;
if (s tr eq( 1ine-flp tr, "<<")) return k;
if(k)rvalue(lval);
whiled)
(if (match("<="))
(push();
If(heir7(lval2))rvalue(lval2);
p opO ;
if (cptr=lvali:03)
if(cptrCident3==pointer)
CuleO;
continue;
)
if(cptr=lval2C03)
if(cptrCldent3==pointer)
tule();
continue;
)
le();
)
else if (match(">="))
(push();
If(heir7(lval2))rvalue(lval2);
p op O;
if(cptr=lvalC03)
if(cptrCident3==pointer)
(uge();
continue;
>
if(cptr=lval2C03)
if(cptrCident3==pointer)
Cuge();
continue;
)
ge();
)
else if((streq(line+lptr/M<"))i
(streq(line+lptr,"<<")==0))
(inbyte();
push();
if(heir7(lval2))rvalue(lval2);
po pO;
if(cptr=lvalC03)
lf(cptrCident3==pointer)
(ult();
continue;
)
if(cptr=lva12(0 3)
if(cptrCident3==pointer)
(ult();
continue;
>
lt() ;
)
else if (( streq( 1 ine+lptr,">") )(,
(streq(llne+lptr,"»")==0 ))
(inbyte();
push();
if(heir7(lval2))rvalue(lval2);
po pO;
if(cptr=lvalC03)
if(cptrCident3==pointer)
(ugt();
continue;
)
if(cptr=lval2C03)
if(cptrtident3==pointer)
tugt();
continue;
)
gt();




)
else return 0;
)
)
/* > > » » start of cc6 < < « « */
he li:7(lval)
int lvalC3;
t
int V , lval2C23;
k=heir8(lval);
blanks();
if((streq( line-flptr/">>n )==0)&
(streq(line+lptr/"<<")==0))return k;
if(k)rvalue(lval);
uhlle(l)
Cif (*atrti(N» " ) >
{push();
i f ( h e i r B ( l v a l 2 ) ) r v a l u e ( l v a l 2 ) ;
pop();
asr();
>
else if (Batch("«"))
(push();
if(heir8(lval2))rvalue(lval2);
pop();
asl();
)
else return 0;
)
)
helr8(lval)
int lvalCl;
C
int k, lva12C23;
k=heir9(lval);
blanksO;
lf((ch()!='♦')&(ch()I='-'))return k;
if(k)rvalue(lval);
uhile(l)
Cif (*atch("+n ))
CpushO;
if(heir9(lval2))rvalue(lval2);
if (cptr=lvalC03)
if((cptrCident3==pointer)i
(cptrCtype3==cint))
doublereqO;
pop();
addO;
)
else if (match("-"))
CpushO;
if(heir9(lval2))rvalue(lval2);
if (cptr=lvalC03)
if((cptrCident3==pointer)t
(cptrC type3==cint))
doublereqO;
pop();
sub();
>
else return 0;
>
)
heir 9(lval)
int lvaici;
C
int Y , 1val2C 23;
k=heir10(1val);
blanksO;
if(<ch()!=•**)&(ch<)!= '/')&
(ch()l='%'))return k;
if(k)rvalue(lval);
uhile(l)
(if (iratch("*"))
(push();
it(helr9(lval2))rvalue(lval2);
pop();
rault( );
)
else if (natch("/N))
(pushO;
ifCheirlO(lval2))rvalue(lval2);
p o pO;
di vO;
>
else if (raatch("%"))
( p u s h ( ) ;
if(heirl0(lval2))rvalue(lval2);
po pO ;
mod();
>
else return 0;
)
)
he irl0(lval)
int lvalC3;
(
int k;
char *ptr;
if (n1atch(,,♦♦,,))
(i f((k=he irl0(lval))==0)
(needlval();
return 0;
>
if(Iva1C 13)push();
rvalue(lval);




inc();
ptr=lvalC03;
if((ptrC ident 3==pointer) k
(ptrCtype3==cint))
inc();
stored va l);
return 0;
)
else if(isatch("— "))
(if((k=heirl0(lval))==0)
Cneedlval();
return 0;
)
if(lvalC13)push();
rvaluetlval);
dec();
ptr=lvalC03;
if((ptrCident3==pointer)4
(ptrCtype3==cint))
dec();
store(1va1);
return 0;
)
else if (match("-"))
(k=heirl0(lval);
if (k) rvalue(lval);
neg();
return 0;
>
else if(natchC"*"))
(k=heirl0(lval);
if(k)rvalue(lval);
lvalC13=cint;
if(ptr=lvalC03)lvalC13=ptrCtype3;
lvalC03=0;
return 1;
>
else if(natch("&"))
(k=heirl0(lval);
if(k==0)
(errorC'illegal address");
return 0;
>
else if(lvalC13)return 0;
e 1se
(lmmed();
outstr(ptr=lvalC03);
nl();
lvalC13=ptrCtype3;
return 0;
)
)
else
(k=heirl1(lval);
if(match("*^"))
(lf(k==0)
CneedlvalO;
return 0;
)
if(lvalC13)push();
rvalue(1val);
inc();
P tr=lvalC03;
if((ptrC ident3==pointer )&
(ptrCtype3==cint))
inc();
store(lval);
dec();
if((ptrC ident3==pointer)&
(ptrCtype3==cint))
dec();
return 0;
)
else it(match("— "))
Cif(k==0)
(needlval();
return 0;
)
if(lvalC13)push();
rvalue(lval);
dec();
ptr=lvalC03;
if((ptrCident3 ==pointer)lk
(ptrCtype3==cint))
dec();
store(lval);
incO;
if((ptrCident3==pointer)i
(ptrCtype3==cint))
inc();
return 0;
>
else return k;
>
>
/* >>>>>> start of cc7 < < « « * /
heirllClval)
int *lval;
C int k;char *ptr;
k=pr imary(lval);
ptr=lvalC03;
blanks();
if((ch()=='C')l(ch()=='('))
uhile(1)




(if(match("C"))
(if(ptr==0)
terror("can't subscript");
Junk();
needbrack("3");
return 0;
)
else if(ptrCident3==pointer)rvalue(lval);
else it(ptrCldent]I=array)
Cerror("can't subscript");
k=0;
)
pushf);
expression();
needbrack("3");
if (ptretype3==cInt)doub leregO;
P o p ( ) ;
add();
lvalC03=0;
lvalC13=ptrCtype3;
k=l;
}
else if(natch("("))
tif(ptr= =0)
(califunction(0);
)
else If(ptrCident]I=functlon)
( r v a l u e ( l v a l ) ;
califunction(O);
)
else calIfunctlon(ptr);
k=lvalC03=0;
>
else return k;
i
if(ptr==0)return k;
if(ptrCident3==function)
(inmed();
outstr(ptr);
nl();
return 0;
>
return k;
)
prlnary(lval)
int Mv al;
C char *ptr/snaraeCnaDesize3;int numC13;
int k;
if(*atch("("))
Ck=heirl(lval);
needbrack(")");
return k;
>
if(symname(sname))
Cif(ptr=findloc(sname))
(getloc(ptr );
lvalC03=ptr;
lvalC13=ptrCtype3;
if(ptrCident3==pointer)lvalC13=cint;
if(ptrCident3==array)return 0;
else return 1;
>
if(ptr=findglb(sname))
if(ptrCident3!=function)
(lvalC03=ptr;
lvalC13=0;
if(ptrCident3!=array)return l;
iromed();
outstr(ptr);nl();
lvalC13=ptrCtype3;
return 0;
>
ptr=addglbCsnaraeyfunction,cint/0);
lvalC03=ptr;
lvalC13=0;
return 0;
>
if(constant(nun))
return(lvalC03=lvalC13=0);
else
(errorC'invalid expression");
imned();outdec(0);nl();
JunkO;
return 0;
>
)
store(lval)
int M val;
( if (lvalC13==0)putmen;(lvalC03);
else putstk(lvalCl3);
>
rvalue(lval)
int *lval;
C if((lvalC03 != 0) & (lvalC13 == 0))
getnemd valC03);
else indirect(lvalC13);
)
test(labeI)
int label;
(
needbr ack("(");
expression();
needbt acl i ( ,,) " ) J
t e s t J u m p ( l a b e l ) ;



>
constant(val)
int valC3;
( if (number(val))
iraaed();
else if (pstr(val))
iraraed();
else If (qstr(val))
(iaaed();printlabel(lltlab);outbyte('-» );)
else return 0;
outdec(valCO 3);
n 1();
return 1;
>
number(va1)
int valC3;
( int k,minus;char c;
k=minus=l;
while(k)
(k=0;
if (watch("♦")) k=l;
if (iratch( "-")) (minus=(-ninus);k=l;)
)
if(numeric(ch())==0)return 0;
while (numeric(ch( )))
(c=i nbyte();
k=k*10+(c-'0');
>
if (irlnus<0) k=(-k);
valC03=k;
return 1;
)
pstr(val)
Int va1C3;
{ int k;char c;
k=0;
if (match("'")==0) return 0;
while((c=gch())!=39)
k=(k&255)*256 ♦ (cU27);
valC03=k;
return 1;
>*
qstr(val)
int va1C3;
( char c;
if (match(quote)==0) return 0;
valC03=litptr;
while (ch()t ='"*)
(if(ch()==0)break;
if(1itptr>=litmax)
(error("strlng space exhausted");
whlle(match(guote)==0)
if(gch() ==0)break;
return 1;
)
litqClltptr*0=gch();
>
gch();
litqClitptr*+3=0;
return 1;
>
/* >>>>>> start of cc8 < « < « < */
/* Begin a comment line for the assembler */
comment()
( outbyte( ';');
)
/* Print all assembler info before any code is generated *j
header()
( commentO;
outstrC'snal1-c compiler rev 1.1");
nl ();
>
/* Print any assembler stuff needed after all code */
trailer()
( /* olC'EKD"); */
)
/* Fetch a static memory cell into the primary register */
ge tmem(sym)
char *sym;
C if((symC ident3I=pointer)&(symCtype3==cchar))
(otC'LOA ");
outstr(sya+name);
nl() ;
call("ccsxt");
)
else
(ot("LHLD ");
outstr(syn+name);
nl();
)
)
/* Fetch the address of the specified symbol */
/* into the primary register */
getloc(sym)
char *sy«;
( immed();
outdec(((syroCoffset3&255)+
((symCof fSet+13S.255 ^ S n
ap)*
nl();
ol("DAD SP");
1
/* stor e the primacy re giste c into the specifi ed */

























