# Modern-Zork
A modern rewrite of Zork from the original MDL source code into C++ 23. 

The original Zork was written in 1971 for a timeshare mainframe called the PDP-10. It was written in a dialetc of Lisp called MDL. 

1971 was pretty very early for computing, with most people's epoch starting in 1977 with the Apple II. Even though it arrived six years earlier, the PDP-10 was very powerful; where the Apple 2 had 48 kb of ram in a 8-bit word space, the PDP-10 had 256 kilowords (as it was a 36-bit word space computer), which, depending on what instruction set you used, amounted to about a megabyte of RAM! Of course, it also cost 5 million dollars adjusted for inflation.

When Zork and other famous Infocom games were ported to the Apple II, it was done in BASIC. Later, it was done in another intermediary languages called ZIL (Zork Implementation Language), but ZIL and MDL share a commonality in both being high-level bytecode languages. Yes, even in 1971 the PDP-10 was running an interpreter! 

This is an implementation of the original MDL source code in modern C++ using ncurses. I tried to write it as optimized as possible, as if it were still running on a PDP-10. This endeavor was somewhat pointless as the PDP-10 is more than capable of handling any C++ one could write (if it could run x86 opcode), and I ended up using ncurses instead of <stdio> to really capture that terminal feeling. 

In the original MDL, the entire program is one garbage collected heap. Using static allocation removes the heap overhead, guarantees no fragmentation, and lets the compiler place data in BSS. The Object IDs are simple array indices (int16_t), making lookups O(1).

Futhermore, In MDL, each flag is a full heap-allocated ATOM (8–16 bytes on a 36-bit word machine).  Replacing with bitfields reduces per-object flag storage from ~200 bytes to 4 bytes. 

Potential issues: Not all of the files included in the include directory from ncurses are strictly nessecary. I may be missing some rooms and/or items...

For the original Zork MDL source code this project was based on, see here: https://github.com/MITDDC/zork
