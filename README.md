# Small-C version 1.1

This is the source code to the original Small-C as written by Ron Cain and published in Dr. Dobbs journal. It was reconstructed by correcting badly OCR-ed scans and compiling/debugging the result.

<b>Beware: this compiler has known bugs and limitations. YMMV</b>

### References:
* Dr. Dobbs Journal of Computer Calisthenics and Orthodontia, Box E, Menlo Park, CA 94025 No. 45 (May 1980) "A Small C Compiler for the 8080's" by Ron Cain A listing in C of the 8080 compiler + notes on implementation. This issue also contains other articles about C.

* Dr. Dobbs Journal of Computer Calisthenics and Orthodontia, Box E, Menlo Park, CA 94025 No. 48 (Sept. 1980) "A Runtime Library for the Small C Compiler" by Ron Cain - Run time library + useful notes. (See also Nos. 52, 57 and 62 for bug notes by P.L.Woods, K.Bailey, M.Gore & B.Roehl, J.E.Hendrix) Nos. 81 and 82 (July, Aug. 1983) 

* C Users Group Vol 9. This contains Mike Bernson's Small C. This is an 8080 version and uses a special assembler which is provided. The original was got going via BDS C.SIG/M Vol 149 This includes C86 adapted by Glen FisherSIG/M Vol 224 This contains a floating point, Z80 version of Small C. There is also a Z80 assembler and linker. There are some formatted I/O facilities. The programming constructs are those in the original compiler. Modifications by J.R.Van Zandt.

* "A book on C" by R.E.Berry and B.A.E.Meekings. (Macmillan 1984) This includes a listing of 'Rat.C' which is very similar to Ron Cain's version but has useful cross-referrence listings etc. This reference is also worth examining if you want to adapt the compiler to other CPU's. It's also quite a good introductory low priced text on C in general.

### Example output

C code:
```
print(ptr)
    char *ptr;
{
}

main()
{
    int a,b,c;
    print("Hello, world");
    c = a*b+5;
}```

Resulting 8080 code:

```
;small-c compiler rev 1.1
print:
        RET
main:
        PUSH B
        PUSH B
        PUSH B
        LXI H,cc1+0
        PUSH H
        CALL print
        POP B
        LXI H,65536
        DAD SP
        PUSH H
        LXI H,65542
        DAD SP
        CALL ccgint
        PUSH H
        LXI H,65542
        DAD SP
        CALL ccgint
        POP D
        CALL ccmult
        PUSH H
        LXI H,5
        POP D
        DAD D
        POP D
        CALL ccpint
        POP B
        POP B
        POP B
        RET
cc1:    DB 72L101L108L108L111L44L32L119L111L114
        DB 108L100L0

;0 errors in compilation.```
