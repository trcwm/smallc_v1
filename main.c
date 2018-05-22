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

#include<stdio.h>
#define eol 10
/*      Define the symbol table parameters      */

#define symsiz  14
#define symtbsz 5040
#define numglbs 300
#define startglb symtab
#define endglb startglb+numglbs*symsiz
#define startloc endglb+symsiz
#define endloc symtab+symtbsz-symsiz

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

/*      Define the "while" statement queue      */

#define wqtabsz 100
#define wqsiz   4
#define wqmax   wq+wqtabsz-wqsiz

/*      Define entry offsets in while queue     */

#define wqsym   0
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
#define stif    1
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
        if(glbflag==0)return;   /* don't if user said no */
        cptr=startglb;
        while(cptr<glbptr)
                {if(cptr[ident]!=function)
                        /* do if anything but function */
                        {outstr(cptr);col();
                                /* output name as label... */
                        defstorage(); /* define storage */
                        j=((cptr[offset]&255)+
                                ((cptr[offset+1]&255)<<8));
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
                ctext=1; /* user said yes   */
        /* see if user wants us to allocate static * /
        /* variables by name In this module */
        /* (pseudo external capability)     */
        pl("Do you wish the globals to be defined?");
        gets(line);
        glbflag=0;
        if((ch()=='Y')|(ch()=='y'))
                glbflag=1; /* user said yes */
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
        if(ch()==0)return;      /*none given... */
        if((output=fopen(line,"wt"))==NULL) /* it given, open */
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
        input=0;                /* none to start with */
        while (input==0)        /* any above 1 allowed */
                {kill();        /* clear line */
                if(eof)break;   /* it user said none */
                pl("Input filename? ");
                gets(line);     /* get a name */
                if(ch()==0)
                        {eof=1;break;}  /* none given... */
                if((input=fopen(line,"rt"))==NULL)
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
                        if (match("["))         /* array? */
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
                        if (match("["))
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
                                        k=1;else k=2;
                        /* change machine stack */
                        sp=modstk(sp-k);
                        addloc(sname,j,typ,sp);
                        break;
                        }
                if (match(",")==0) return;
                }
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
        needbrack("]");         /* force single dimension */
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
                else{error("illegal argument name");junk();}
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

        while(argstk)
                /* now let user declare what types of things */
                /*      those arguments were */
                {if (amatch("char",4)) {getarg(cchar);ns();}
                else if (amatch("int",3)) {getarg(cint);ns();}
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
        }
/*                                      */
/*      Declare argument types          */
/*                                      */
/* called from "newfunc" this routine adds an entry in the */
/*      local symbol table for each named argument */
getarg(t)               /* t = cchar or cint */
        int t;
        {
        char n[namesize],c;int j;
        while(1)
                {if(argstk==0) return;  /* no more args */
                if(match("*"))j=pointer;
                        else j=variable;
                if (symname(n)==0) illname();
                if(findloc(n))multidef(n);
                if(match("[")) /* pointer ? */
                /* it is a pointer, so skip all */
                /* stuff between "[]" */
                        {while(inbyte()!="]")
                                if(endst())break;
                        j=pointer;
                        /* add entry as pointer */
                        }
                addloc(n,j,t,argstk);
                argstk=argstk-2;        /* cnt down */
                if(endst())return;
                if(match(",")==0)error("expected comma");
                }
        }
/*                                      */
/*      Statement parser                */
/*                                      */        
/* called whenever syntax requires      */
/*      a stat ement.                   */
/*  this routine performs that statement */
/*  and returns a number telling which one */
statement()
 {      if((ch()==0) & (eof)) return;
        else if(amatch("char",4))
                {declloc(cchar);ns();}
        else if(amatch("int",3))
                {declloc(cint);ns();}
        else if(amatch("{")) compound();
        else if(amatch("if",2))
                {doif(); lastst=stif;}
        else if(amatch("while",5))
                {dowhile(); lastst=stwhile;}
        else if(amatch("return",6))
                {doreturn(); ns(); lastst=streturn;}
        else if(amatch("break",5))
                {dobreak(); ns(); lastst=stbreak;}
        else if(amatch("continue",8))
                {docont(); ns() ; lastst=stcont;}
        else if(match(";"));
        else if(match("#asm"))
                {doasm(); lastst=stasm;}
        /* if nothing else, assume it's an expr expression */
        else{expression(); ns(); lastst=stexp; }
        return lastst;
 }

/*                                      */
/*      Semicolon enforcer              */ 
/*                                      */
/* called whenever syntax requires a semicolon */
ns() {if(match(";")==0)error("missing semicolon");}
/*                                      */
/*      Compound statement              */
/*                                      */
/* allow any number of statements to fall between "{}" */
compound()
        {
        ++ncmp;         /* new level open */
        while(match("}")==0)
                if(eof) return;
                else statement();
        --ncmp;         /* close current level */
        }
/*                                      */
/*      "if" statement                  */
/*                                      */
doif()
        {
        int flev,fsp,flab1,flab2;
        flev=locptr;    /* record current local level */
        fsp=sp;         /* record current stk ptr */
        flab1=getlabel(); /* get label for false branch */
        test(flab1);    /* get expression, and branch false */
        statement();    /* if true, do a statement */
        sp=modstk(fsp); /* then clean up the stack */
        locptr=flev;    /* and deallocate any locals */
        if (amatch("else",4)==0)        /* if...else ? */
                /* simple "if"...print false label */
                {printlabel(flab1);col();nl();
                return;         /* and exit */
                }
        /* an "if...else" statement. */
        jump(flab2=getlabel()); /* jump around false code */
        printlabel(flab1);col();nl();   /* print false label */
        statement();            /* and do "else" clause */
        sp=modstk(fsp);         /*then clean up stk ptr */
        locptr=flev;            /* and deallocate locals */
        printlabel(flab2);col();nl(); /* print true label */
        }
/*                                      */
/* "while" statement                    */
/*                                      */
dowhile()
        {
        int wq[4];              /* allocate local queue */
        wq[wqsym]=locptr;       /* record local level */
        wq[wqsp]=sp;            /* and stk ptr */
        wq[wqloop]=getlabel();  /* and looping label */
        wq[wqlab]=getlabel();   /* and exit label */
        addwhile(wq);           /* add entry to queue */
                                /* (for "break" statement) */
        printlabel(wq[wqloop]);col();nl(); /* loop label */
        test(wq[wqlab]);        /* see if true */
        statement();            /* if so, do a statement */
        jump(wq[wqloop]);       /* loop to label */
        printlabel(wq[wqlab]);col();nl(); /* exit label */
        locptr=wq[wqsym];       /* deallocate locals */
        sp=modstk(wq[wqsp]);    /* clean up stk ptr */
        delwhile();             /* delete queue entry */
        }
/*                                      */
/*      "return statement"              */
/*                                      */
doreturn()
        {
        /* if not end of statement/ get an expression */
        if(endst()==0)expression();
        modstk(0);      /* clean up stk */
        ret();          /* and exit function */
        }
/*                                      */
/*      "break" statement               */
/*                                      */
dobreak()
        {
        int *ptr;
        /* see if any "whiles" are open */
        if ((ptr=readwhile())==0) return;       /* no */
        modstk((ptr[wqsp]));    /* else clean up stk ptr */
        jump(ptr[wqlab]);       /* jump to exit label */
        }
/*                                      */
/*      "continue" statement            */
/*                                      */
docont()
        {
        int *ptr;
        /* see if any "whiles" are open */
        if ((ptr=readwhile())==0) return;       /* no */
        modstk((ptr[wqsp]));    /* else clean up stk ptr */
        jump(ptr[wqloop]);      /* jump to loop label */
        }

/*                                      */
/*      "asm" pseudo-statement          */
/*                                      */
/* enters mode where assembly language statement are */
/*      passed Intact through parser    */
doasm()
        {
        cmode=0;                /* mark mode as "asm" */
        while (1)
                {inline();      /* get and print lines */
                if (match("#endasm")) break;    /* until... */
                if(eof)break;
                outstr(line);
                nl();
                }
        kill();         /* invalidate line */
        cmode=1;        /* then back to parse level */
        }
/*      >>>>>> start of cc3 <<<<<<      */

/*                                      */
/*      Perform a function call         */
/*                                      */
/* called from heir11, this routine will either call */
/*      the named function, or if the supplied ptr is */
/*      zero, will call the contents of HL              */
callfunction(ptr)
        char *ptr;      /* symbol table entry (or 0) */
{       int nargs;
        nargs=0;
        blanks();       /* already saw open paren */
        if(ptr==0)push();       /* calling HL */
        while(streq(line+lptr,")")==0)
                {if(endst())break;
                expression();   /* get an arguirent */
                if(ptr==0)swapstk(); /* don't push addr */
                push(); /* push argument */
                nargs=nargs+2;  /* count args*2 */
                if (match(",")==0) break;
                }
        needbrack(")");
        if(ptr)call(ptr);
        else callstk();
        sp=modstk(sp+nargs);    /* clean up arguments */
}
junk()
{       if(an(inbyte()))
                while(an(ch()))gch();
        else while(an(ch())==0)
                {if(ch()==0)break;
                gch();
                }
        blanks();
}
endst()
{       blanks();
        return ((streq(line+lptr,";")|(ch()==0)));
}
illname()
{       error("illegal symbol name");junk();}

multidef(sname)
        char *sname;
{       error("already defined");
        comment();
        outstr(sname);nl();
}
needbrack(str)
        char *str;
{       if (match(str)==0)
                {error("misslng bracket");
                comment();outstr(str);nl();
                }
}
needlval()
{       error("must be lvalue");
}
findglb(sname)
        char *sname;
{       char *ptr;
        ptr=startglb;
        while(ptr!=glbptr)
                {if(astreq(sname,ptr,namemax))return ptr;
                ptr=ptr+symsiz;
                }
        return 0;
}
findloc(sname)
        char *sname;
{       char *ptr;
        ptr=startloc;
        while(ptr!=locptr)
                {if(astreq(sname,ptr,namemax))return ptr;
                ptr=ptr+symsiz;
                }
        return 0;
}
addglb(sname,id,typ,value)
        char *sname,id,typ;
        int value;
{       char *ptr;
        if(cptr=findglb(sname))return cptr;
        if(glbptr>=endglb)
                {error("global symbol table overflow");
                return 0;
                }
        cptr=ptr=glbptr;
        while(an(*ptr++ = *sname++));   /* copy name */
        cptr[ident]=id;
        cptr[type]=typ;
        cptr[storage]=statik;
        cptr[offset]=value;
        cptr[offset+1]=value>>8;
        glbptr=glbptr+symsiz;
        return cptr;
}
addloc(sname,id,typ,value)
        char *sname,id,typ;
        int value;
{       char *ptr;
        if(cptr=findloc(sname))return cptr;
        if(locptr>=endloc)
                {error("local symbol table overflow");
                return 0;
                }
        cptr=ptr=locptr;
        while(an(*ptr++ = *sname++));   /* copy nane */
        cptr[ident]=id;
        cptr[type]=typ;
        cptr[storage]=stkloc;
        cptr[offset]=value;
        cptr[offset+1]=value>>8;
        locptr=locptr+symsiz;
        return cptr;
}
/* Test if next input string is legal symbol name */
symname(sname)
        char *sname;
{       int k;char c;
        blanks();
        if(alpha(ch())==0)return 0;
        k=0;
        while(an(ch()))sname[k++]=gch();
        sname[k]=0;
        return 1;
}
/* Return next avail Internal label number */
getlabel()
{       return(++nxtlab);
}
/* print specified number as label */
printlabel(label)
        int label;
{       outstr("cc");
        outdec(label);
}
/* Test if given character is alpha */
alpha(c)
        char c;
{       c=c&127;
        return(((c>='a')&(c<='z'))|
                ((c>='A')&(c<='Z'))|
                (c=='_'));
}
/* Test if given character is numeric */
numeric(c)
        char c;
{       c=c&127;
        return((c>='0')&(c<='9'));
}
/* Test if given character is alphanumeric */
an(c)
        char c;
{       return((alpha(c))|(numeric(c)));
}
/* Print a carriage return and a string only to console */
pl(str)
        char *str;
{       int k;
        k=0;
        putchar(eol);
        while(str[k])putchar(str[k++]);
}
addwhile(ptr)
        int ptr[];
{
        int k;
        if (wqptr==wqmax)
                {error("too many active whiles");return;};
        k=0;
        while (k<wqsiz)
                {*wqptr++ = ptr[k++];}
}
delwhile()
        {if(readwhile()) wqptr=wqptr-wqsiz;
        }
readwhile()
{
        if (wqptr==wq){error("no active whiles");return 0;}
        else return (wqptr-wqsiz);
}
ch()
{       return(line[lptr]&127);
}
nch()
{       if(ch()==0)return 0;
                else return(line[lptr+1]&127);
}
gch()
{       if(ch()==0)return 0;
                else return(line[lptr++]&127);
}
kill()
{       lptr=0;
        line[lptr]=0;
}
inbyte()
{
        while(ch()==0)
                {if (eof) return 0;
                inline();
                preprocess();
                }
        return gch();
}
inchar()
{
        if(ch()==0)inline();
        if(eof)return 0;
        return(gch());
}
inline()
{
        int k,unit;
        while(1)
                {if (input==0)openin();
                if(eof)return;
                if((unit=input2)==0)unit=input;
                kill();
                while((k=getc(unit))>0)
                        {if((k==eol)|(lptr>=linemax))break;
                        line[lptr++]=k;
                        }
                line[lptr]=0;   /* append null */
                if (k<=0)
                        {fclose(unit);
                        if(input2)input2=0;
                                else input=0;
                        }
                if(lptr)
                        {if((ctext)&(cmode))
                                {comment();
                                outstr(line);
                                nl();
                                }
                        lptr=0;
                        return;
                        }
                }
}
/*      >>>>>> start of cc4 <<<<<<      */

keepch(c)
        char c;
{       mline[mptr]=c;
        if(mptr<mpmax)mptr++;
        return c;
}
preprocess()
{       int k;
        char c,sname[namesize];
        if(cmode==0) return;
        mptr=lptr=0;
        while(ch())
                {if((ch()==' ')|(ch()==9))
                        {keepch(' ');
                        while((ch()==' ')|
                                (ch()==9))
                                gch();
                        }
                else if(ch()=='"')
                        {keepch(ch());
                        gch();
                        while(ch()!='"')
                                {if(ch()==0)
                                  {error("missing quote");
                                  break;
                                  }
                                keepch(gch());
                                }
                        gch();
                        keepch('"');
                        }
                else if(ch()==39)
                        {keepch(39);
                        gch();
                        while(ch()!=39)
                                {if(ch() ==0)
                                  {error("missing apostrophe");
                                  break;
                                  }
                                keepch(gch());
                                }
                        gch();
                        keepch(39);
                        }
                else if((ch()=='/')&(nch()=='*'))
                        {inchar();inchar();
                        while(((ch()=='*')&
                                (nch()=='/'))==0)
                                {if(ch()==0)inline();
                                        else inchar();
                                if(eof)break;
                                }
                        inchar();inchar();
                        }
                else if(an(ch()))
                        {k=0;
                        while(an(ch()))
                                {if(k<namemax)sname[k++]=ch();
                                gch();
                                }
                        sname[k]=0;
                        if(k=findmac(sname))
                                while(c=macq[k++])
                                        keepch(c);
                        else
                                {k=0;
                                while(c=sname[k++])
                                        keepch(c);
                                }
                        }
                else keepch(gch());
        }
        keepch(0);
        if(mptr>=mpmax)error( "line too long");
        lptr=mptr=0;
        while(line[lptr++]=mline[mptr++]);
        lptr=0;
}
addmac()
{       char sname[namesize];
        int k;
        if(symname(sname)==0)
                {illname();
                kill();
                return;
                }
        k=0;
        while(putmac(sname[k++]));
        while(ch()==' ' | ch()==9) gch();
        while(putmac(gch()));
        if(macptr>=macmax)error("macro table full");
}
putmac(c)
        char c;
{       macq[macptr]=c;
        if(macptr<macmax)macptr++;
        return c;
}
findmac(sname)
        char *sname;
{       int k;
        k=0;
        while(k<macptr)
                {if(astreq(sname,macq+k,namemax))
                        {while(macq[k++]);
                        return k;
                        }
                while(macq[k++]);
                while(macq[k++]);
                }
        return 0;
}
outbyte(c)
        char c;
{
        if(c==0)return 0;
        if(output)
                {if((putc(c,output))<=0)
                        {closeout();
                        error("Output file error");
                        }
                }
        else putchar(c);
        return c;
}
outstr(ptr)
        char ptr[];
{
        int k;
        k=0;
        while(outbyte(ptr[k++]));
}
nl()
        {outbyte(eol);}
tab()
        {outbyte(9);}
col()
        {outbyte(58);}
error(ptr)
        char ptr[];
{       
        int k;
        comment();outstr(line);nl();comment();
        k=0;
        while(k<lptr)
                {if(line[k]==9) tab();
                        else outbyte(' ');
                ++k;
                }
        outbyte('^');
        nl();comment();outstr("*****  ");
        outstr(ptr);
        outstr("  *****");
        nl();
        ++errcnt;
}
ol(ptr)
        char ptr[];
{
        ot(ptr);
        nl();
}
ot(ptr)
        char ptr[];
{
        tab();
        outstr(ptr);
}
streq(str1,str2)
        char str1[],str2[];
{
        int k;
        k=0;
        while (str2[k])
                {if ((str1[k]) != (str2[k])) return 0;
                k++;
                }
        return k;
}

astreq(str1,str2,len)
        char str1[],str2[];int len;
{
        int k;
        k=0;
        while (k<len)
                {if ((str1[k])!=(str2[k]))break;
                if(str1[k]==0)break;
                if(str2[k]==0)break;
                k++;
                }
        if (an(str1[k]))return 0;
        if (an(str2[k]))return 0;
        return k;
}

match(lit)
        char *lit;
{
        int k;
        blanks();
        if (k=streq(line+lptr,lit))
                {lptr=lptr+k;
                return 1;
                }
        return 0;
}
amatch(lit,len)
        char *lit;int len;
{
        int k;
        blanks();
        if (k=astreq(line+lptr,lit,len))
                {lptr=lptr+k;
                while(an(ch())) inbyte();
                return 1;
                }
        return 0;
}
blanks()
        {while(1)
                {while(ch()==0)
                        {inline();
                        preprocess();
                        if(eof)break;
                        }
                if(ch()==' ') gch();
                else if(ch()==9)gch();
                else return;
                }
        }
outdec(number)
        int number;
{
        int k,zs;
        char c;
        zs = 0;
        k=10000;
        if (number<0)
                {number=(-number);
                outbyte('-');
                }
        while (k>=1)
                {
                c=number/k + '0';
                if ((c!='0')|(k==1)|(zs))
                        {zs=1;outbyte(c);}
                number=number%k;
                k=k/10;
                }
}
/*      >>>>>> start of cc5 <<<<<<      */

expression()
{
        int lval[2];
        if(heir1(lval))rvalue(lval);
}
heir1(lval)
        int lval[];
{
        int k,lval2[2];
        k=heir2(lval);
        if (match("="))
                {if(k==0){needlval();return 0;}
                if (lval[1])push();
                if(heir1(lval2))rvalue(lval2);
                store(lval);
                return 0;
                }
        else return k;
}
heir2(lval)
        int lval[];
{
        int k,lval2[2];
        k=heir3(lval);
        blanks();
        if(ch()!='|')return k;
        if (k) rvalue(lval);
        while(1)
                {if (match("|"))
                        {push();
                        if(heir3(lval2)) rvalue(lval2);
                        pop();
                        or();
                        }
                else return 0;
                }
}
heir3(lval)
        int lval[];
{
        int k,lval2[2];
        k=heir4(lval);
        blanks();
        if(ch()!='^')return k;
        if(k)rvalue(lval);
        while(1)
                {if (match("^"))
                        {push();
                        if(heir4(lval2))rvalue(lval2);
                        pop();
                        xor();
                        }
                else return 0;
                }
}
heir4(lval)
        int lval[];
{
        int k,lval2[2];
        k=heir5(lval);
        blanks();
        if(ch()!='&')return k;
        if(k)rvalue(lval);
        while(1)
                {if (match("&"))
                        {push();
                        if(heir5(lval2))rvalue(lval2);
                        pop();
                        and();
                        }
                else return 0;
                }
}
heir5(lval)
        int lval[];
{
        int k,lval2[2];
        k=heir6(lval);
        blanks();
        if((streq(line+lptr,"==")==0)&
                (streq(line+lptr,"!=")==0))return k;
        if(k)rvalue(lval);
        while(1)
                {if (match("=="))
                        {push();
                        if(heir6(lval2))rvalue(lval2);
                        pop();
                        eq();
                        }
                else if (match("!="))
                        {push();
                        if(heir6(lval2))rvalue(lval2);
                        pop();
                        ne();
                        }
                else return 0;
                }
}
heir6(lval)
        int lval[];
{
        int k,lval2[2];
        k=heir7(lval);
        blanks();
        if((streq(line+lptr,"<")==0)&
                (streq(line+lptr,">")==0)&
                (streq(line+lptr,"<=")==0)&
                (streq(line+lptr,">=")==0))return k;
                if(streq(line+lptr,">>"))return k;
                if(streq(line+lptr,"<<"))return k;
        if(k)rvalue(lval);
        while(1)
                {if (match("<="))
                        {push();
                        if(heir7(lval2))rvalue(lval2);
                        pop();
                        if (cptr=lval[0])
                                if(cptr[ident]==pointer)
                                {ule();
                                continue;
                                }
                        if(cptr=lval2[0])
                                if(cptr[ident]==pointer)
                                {ule();
                                continue;
                                }
                        le();
                        }
                else if (match(">="))
                        {push();
                        if(heir7(lval2))rvalue(lval2);
                        pop();
                        if(cptr=lval[0])
                                if(cptr[ident]==pointer)
                                {uge();
                                continue;
                                }
                        if(cptr=lval2[0])
                                if(cptr[ident]==pointer)
                                {uge();
                                continue;
                                }
                        ge();
                        }
                else if((streq(line+lptr,"<"))&
                        (streq(line+lptr,"<<")==0))
                        {inbyte();
                        push();
                        if(heir7(lval2))rvalue(lval2);
                        pop();
                        if(cptr=lval[0])
                                if(cptr[ident]==pointer)
                                {ult();
                                continue;
                                }
                        if(cptr=lval2[0])
                                if(cptr[ident]==pointer)
                                {ult();
                                continue;
                                }
                        lt();
                        }
                else if ((streq(line+lptr,">"))&
                        (streq(line+lptr,">>")==0))
                        {inbyte();
                        push();
                        if(heir7(lval2))rvalue(lval2);
                        pop();
                        if(cptr=lval[0])
                                if(cptr[ident]==pointer)
                                {ugt(); 
                                continue;
                                }
                        if(cptr=lval2[0])
                                if(cptr[ident]==pointer)
                                {ugt();
                                continue;
                                }
                        gt();
                        }
                else return 0;
                }
}

/*      >>>>>>> start of cc6 <<<<<<     */
heir7(lval)
        int lval[];
{
        int k,lval2[2];
        k=heir8(lval);
        blanks();
        if((streq(line+lptr,">>")==0)&
                (streq(line+lptr,"<<")==0))return k;
        if(k)rvalue(lval);
        while(1)
                {if (match(">>"))
                        {push();
                        if(heir8(lval2))rvalue(lval2);
                        pop();
                        asr();
                        }
                else if (match("<<"))
                        {push();
                        if(heir8(lval2))rvalue(lval2);
                        pop();
                        asl();
                        }
                else return 0;
                }
}
heir8(lval)
        int lval[];
{
        int k, lval2[2];
        k=heir9(lval);
        blanks();
        if((ch()!='+')&(ch()!='-'))return k;
        if(k)rvalue(lval);
        while(1)
                {if (match("+"))
                        {push();
                        if(heir9(lval2))rvalue(lval2);
                        if (cptr=lval[0])
                                if((cptr[ident]==pointer)&
                                (cptr[type]==cint))
                                doublereg();
                        pop();
                        add();
                        }
                else if (match("-"))
                        {push();
                        if(heir9(lval2))rvalue(lval2);
                        if (cptr=lval[0])
                                if((cptr[ident]==pointer)&
                                (cptr[type]==cint))
                                doublereg();
                        pop();
                        sub();
                        }
                else return 0;
                }
}
heir9(lval)
        int lval[];
{
        int k, lval2[2];
        k=heir10(lval);
        blanks();
        if((ch()!='*')&(ch()!='/')&
                (ch()!='%'))return k;
        if(k)rvalue(lval);
        while(1)
                {if (match("*"))
                        {push();
                        if(heir9(lval2))rvalue(lval2);
                        pop();
                        mult();
                        }
                else if (match("/"))
                        {push();
                        if(heir10(lval2))rvalue(lval2);
                        pop();
                        div();
                        }
                else if (match("%"))
                        {push();
                        if(heir10(lval2))rvalue(lval2);
                        pop();
                        mod();
                        }
                else return 0;
                }
}

heir10(lval)
        int lval[];
{
        int k;
        char *ptr;
        if (match("++"))
                {if((k=heir10(lval))==0)
                        {needlval();
                        return 0;
                        }
                if(lval[1])push();
                rvalue(lval);
                inc();
                ptr=lval[0];
                if((ptr[ident]==pointer)&
                        (ptr[type]==cint))
                                inc();
                store(lval);
                return 0;
                }
        else if(match("--"))
                {if((k=heir10(lval))==0)
                        {needlval();
                        return 0;
                        }
                if(lval[1])push();
                rvalue(lval);
                dec();
                ptr=lval[0];
                if((ptr[ident]==pointer)&
                        (ptr[type]==cint))
                                dec();
                store(lval);
                return 0;
                }
        else if (match("-"))
                {k=heir10(lval);
                if (k) rvalue(lval);
                neg();
                return 0;
                }
        else if(match("*"))
                {k=heir10(lval);
                if(k)rvalue(lval);
                lval[1]=cint;
                if(ptr=lval[0])lval[1]=ptr[type];
                lval[0]=0;
                return 1;
                }
        else if(match("&"))
                {k=heir10(lval);
                if(k==0)
                        {error("illegal address");
                        return 0;
                        }
                else if(lval[1])return 0;
                else
                        {immed();
                        outstr(ptr=lval[0]);
                        nl();
                        lval[1]=ptr[type];
                        return 0;
                        }
                }
        else
                {k=heir11(lval);
                if(match("++"))
                        {if(k==0)
                                {needlval();
                                return 0;
                                }
                        if(lval[1])push();
                        rvalue(lval);
                        inc();
                        ptr=lval[0];
                        if((ptr[ident]==pointer)&
                                (ptr[type]==cint))
                                        inc();
                        store(lval);
                        dec();
                        if((ptr[ident]==pointer)&
                                (ptr[type]==cint))
                                        dec();
                        return 0;
                        }
                else if(match("--"))
                        {if(k==0)
                                {needlval();
                                return 0;
                                }
                        if(lval[1])push();
                        rvalue(lval);
                        dec();
                        ptr=lval[0];
                        if((ptr[ident]==pointer)&
                                (ptr[type]==cint))
                                        dec();
                        store(lval);
                        inc();
                        if((ptr[ident]==pointer)&
                                (ptr[type]==cint))
                                        inc();
                        return 0;
                        }
                else return k;
                }
}

/*      >>>>>> start of cc7 <<<<<<<          */
heir11(lval)
        int *lval;
{
        int k;char *ptr;
        k=primary(lval);
        ptr=lval[0];
        blanks();
        if((ch()=='[')|(ch()=='('))
        while(1)
                {if(match("["))
                        {if(ptr==0)
                                {error("can't subscript");
                                junk();
                                needbrack("]");
                                return 0;
                                }
                        else if(ptr[ident]==pointer)rvalue(lval);
                        else if(ptr[ident]==array)
                                {error("can't subscript");
                                k=0;
                                }
                        push();
                        expression();
                        needbrack("]");
                        if (ptr[type]==cint)doublereg();
                        pop();
                        add();
                        lval[0]=0;
                        lval[1]=ptr[type];
                        k=1;
                        }
                else if(match("("))
                        {if(ptr==0)
                                {callfunction(0);
                                }
                        else if(ptr[ident]!=function)
                                {rvalue(lval);
                                callfunction(0);
                                }
                        else callfunction(ptr);
                        k=lval[0]=0;
                        }
                else return k;
                }
        if(ptr==0)return k;
        if(ptr[ident]==function)
                {immed();
                outstr(ptr);
                nl();
                return 0;
                }
        return k;
}

primary(lval)
        int *lval;
{       char *ptr,sname[namesize]; int num[1];
        int k;
        if(match("("))
                {k=heir1(lval);
                needbrack(")");
                return k;
                }
        if(symname(sname))
                {if(ptr=findloc(sname))
                        {getloc(ptr);
                        lval[0]=ptr;
                        lval[1]=ptr[type];
                        if(ptr[ident]==pointer)lval[1]=cint;
                        if(ptr[ident]==array)return 0;
                                else return 1;
                        }
                if(ptr=findglb(sname))
                        if(ptr[ident]!=function)
                                {lval[0]=ptr;
                                lval[1]=0;
                                if(ptr[ident]!=array)return 1;
                                immed();
                                outstr(ptr);nl();
                                lval[1]=ptr[type];
                                return 0;
                                }
                ptr=addglb(sname,function,cint,0);
                lval[0]=ptr;
                lval[1]=0;
                return 0;
                }
        if(constant(num))
                return(lval[0]=lval[1]=0);
        else
                {error("invalid expression");
                immed();outdec(0);nl();
                junk();
                return 0;
                }
}

store(lval)
        int *lval;
{
        if (lval[1]==0)putmem(lval[0]);
        else putstk(lval[1]);
}
rvalue(lval)
        int *lval;
{
        if((lval[0] != 0) & (lval[1] == 0))
                getmem(lval[0]);
                else indirect(lval[1]);
}
test(label)
        int label;
{
        needbrack("(");
        expression();
        needbrack(")");
        testjump(label);
}

constant(val)
        int val[];
{
        if (number(val))
                immed();
        else if (pstr(val))
                immed();
        else if (qstr(val))
                {immed();printlabel(litlab);outbyte('+');}
        else return 0;
        outdec(val[0]);
        nl();
        return 1;
}
number(val)
        int val[];
{
        int k,minus;char c;
        k=minus=1;
        while(k)
                {k=0;
                if (match("+")) k=1;
                if (match("-")) {minus=(-minus);k=1;}
                }
        if(numeric(ch())==0)return 0;
        while (numeric(ch()))
                {c=inbyte();
                k=k*10+(c-'0');
                }
        if (minus<0) k=(-k);
        val[0]=k;
        return 1;
}
pstr(val)
        int val[];
{       int k;char c;
        k=0;
        if (match("'")==0) return 0;
        while((c=gch())!=39)
                k=(k&255)*256 + (c&127);
        val[0]=k;
        return 1;
}
qstr(val)
        int val[];
{       char c;
        if (match(quote)==0) return 0;
        val[0]=litptr;
        while (ch()!='"')
                {if(ch()==0)break;
                if(litptr>=litmax)
                        {error("strlng space exhausted");
                        while(match(quote)==0)
                                if(gch()==0)break;
                        return 1;
                        }
                litq[litptr++]=gch();
                }
        gch();
        litq[litptr++]=0;
        return 1;
}

/*      >>>>>> start of cc8 <<<<<<      */

/* Begin a comment line for the assembler */
comment()
{
        outbyte(';');
}
/* Print all assembler info before any code is generated */
header()
{
        comment();
        outstr("small-c compiler rev 1.1");
        nl();
}
/* Print any assembler stuff needed after all code */
trailer()
{ /* ol("END"); */
}
/* Fetch a static memory cell into the primary register */
getmem(sym)
        char *sym;
{
        if((sym[ident]!=pointer)&(sym[type]==cchar))
                {ot("LDA ");
                outstr(symname);
                nl();
                call("ccsxt");
                }
        else
                {ot("LHLD ");
                outstr(sym+name);
                nl();
                }
}
/* Fetch the address of the specified symbol    */
/*      into the primary register               */
getloc(sym)
        char *sym;
{
        immed();
        outdec(((sym[offset]&255)+
                ((sym[offset+1]&255)<<8))-
                sp);
        nl();
        ol("DAD SP");
        }
/* store the primacy register into the specified */
/*      static memory cell */
putmem(sym)
        char *sym;
{
        if((sym[ident]!=pointer)&(sym[type]==cchar))
                {ol("MOV A,L");
                ot("STA ");
                }
        else ot("SHLD ");
        outstr(sym+name);
        nl();
}
/* Store the specified object type in the primary register */
/*      at the address on the top of the stack          */
putstk(typeobj)
        char typeobj;
{       pop();
        if(typeobj==cchar)call("ccpchar");
                else call("ccpint");
}
/* Fetch the specified object type indirect through the */
/*      primary register into the prirary register      */
indirect(typeobj)
        char typeobj;
{       if(typeobj==cchar)call("ccgchar");
                else call("ccgint");
}
/* Swap the primary and secondary registers */
swap()
{
        ol("XCHG");
}
/* Print partial instruction to get an immediate value  */
/*      into the primary register                       */
immed()
{       ot("LXI H," );
}
/* Push the primary register onto the stack */
push()
{       ol("PUSH H");
        sp=sp-2;
}
/* Pop the top of the stack into the secondary register */
pop()
{       ol("POP D");
        sp=sp+2;
}
/* Swap the primary register and the top of the stack */
swapstk()
{       ol("XTHL");
}
/* Call the specified subroutine name */
call(sname)
        char *sname;
{       ot("CALL ");
        outstr(sname);
        nl();
}
/* Return from subroutine */
ret()
{       ol("RET");
}
/* Perform subroutine call to value on top of stack */
callstk()
{       immed();
        outstr("$+5");
        nl();
        swapstk();
        ol("PCHL");
        sp=sp+2;
}
/* Jump to specified internal label number */
jump(label)
        int label;
{       ot("JMP ");
        printlabel(label);
        nl();
}
/* Test the primary register and jump if false to label */
testjump(label)
        int label;
{       ol("MOV A, H");
        ol("ORA L");
        ot("JZ ");
        printlabel(label);
        nl();
}
/* Print pseudo-op to define a byte */
defbyte()
{       ot("DB ");
}
/* Print pseudo-op to define storage */
defstorage()
{       ot("DS ");
}
/* Print pseudo-op to define a word */
defword()
{       ot("DW ");
}
/* Modify the stack pointer to the new value indicated */
modstk(newsp)
        int newsp;
{       int k;
        k=newsp-sp;
        if(k==0)return newsp;
        if(k>=0)
                {if(k<7)
                        {if (k&1)
                                {ol("INX SP");
                                k--;
                                }
                        while(k)
                                {ol("POP B");
                                k=k-2;
                                }
                        return newsp;
                        }
                }
        if(k<0)
                {if(k>-7)
                        {if(k&1)
                                {ol("DCX SP");
                                k++;
                                }
                        while(k)
                                {ol("PUSH B");
                                k=k+2;
                                }
                        return newsp;
                        }
                }
        swap();
        immed();outdec(k);nl();
        ol("DAD SP");
        ol("SPHL");
        swap();
        return newsp;
}
/* Double the primary register */
doublereg()
        {ol("DAD H");}
/* Add the primary and secondary registers */
/*      .(results in primary) */
add()
        {ol("DAD D");}
/* Subtract the primary register from the secondary * /
/* (results in primary) */
sub()
        {call("ccsub");}
/* Multiply the primary and secondary registers */
/* (results in primary */
mult()
        {call("ccmult");}
/* Divide the secondary register by the primary * /
/* (quotient in primary, remainder in secondary) */
div()
        {call("ccdiv");}
/* Compute remainder (mod) of secondary register divided */
/*      by the primary */
/*      (remainder in primary, quotient in secondary) */
mod()
{
        div();
        swap();
}
/* Inclusive 'or' the primary and the secondary registers */
/*      (results in primary) */
or()
        {call("ccor");}
/* Exclusive 'or* the primary anc seconday registers */
/*      (results In primary) */
xor()
        {call("ccxor");}
/* 'And' the primary and secondary registers */
/*      (results in primary) */
and()
        {call("ccand");}
/* Arithmetic shift right the secondary register number of * /
/* times in primary (results in primary) */
asr()
        {call("ccasr");}
/* Arithmetic left shift the secondary register number ot * /
/*      times in primary (results in primary) */
asl()
        {call("ccasl");}
/* Form tuo's complement of primary register */
neg()
        {call("ccneg");}
/* Form one's complement of primary register */
com()
        {call("cccom");}
/* Increment the primary register by one */
inc()
        {ol("INX H");}
/* Decrement the primary register by one */
dec()
        {ol("DCX H");}
/* Following are the conditional operators */
/* They compare the secondary register against the primary */
/* and put a literal 1 in the primary if the condition is */
/* true, otherwise they clear the primary register */

/* Test for equal */
eq()
        {call("cceq");}

/* Test for not equal */
ne()
        {call("ccne");}
/* Test for less than (signed) */
lt()
        {call("cclt");}
/* Test for less than or equal to (signed) */
le()
        {call("ccle");}
/* Test for greater than (signed) */
gt()
        {call("ccgt");}
/* Test for greater than or equal to (signed) */
ge()
        {call("ccge");}
/* Test for less than (unsigned) */
ult()
        {call("ccult");}
/* Test for less than or equal to (unsigned) */
ule()
        {call("ccule");}
/* Test for greater than (unsigned) */
ugt()
        {call("ccugt");}
/* Test for greater then or equal to (unsigned) */
uge()
        {call("ccuge");}

/*      <<<<< End of compiler >>>>> */

