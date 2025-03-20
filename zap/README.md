# Resurrecting ZAP

This is one of the ZIL compiler tools from Infocom. These days, we have ZILF and ZAPF as modern day replacements. The original pipeline for a game was:

ZIL -> ZILCH -> ZAP -> ZIP

- ZIL: the game is written in this language
- ZILCH: a pre-processor that ran in the Muddle environment
- ZAP: the zap files produced by ZILCH are further processed into a proper game file
- ZIP: the virtual machine that runs the game file

`zap/infocom_source` (nee `zap-sun`) is the original code, grabbed from
https://eblong.com/infocom/#zap

`zap/cosmo_source` builds with `cosmocc` against the `-std=c89` flag in a (so far, failed) attempt to get zap built for Windows, etc. The primary problem I'm facing on this project is that I don't know how the original works, nor what the expected behaviour/output is.

I have tried to get a Sun Microsystem machine running in qemu (which works!) but is only the first step. I can't for the life of me get files transferred into that thing; I'm not sure why it's so hard. I've seen complex instructions for running some server on the Windows side that interfaces with a custom application on the Sun side to let it accept raw data streams and dump those bytes to a file. For a weekend project, the scope of this has turned out to be bigger than expected.

The basic plan is:

- Get the Sun system working and run the `zap` executable that is part of the zap-sun archive.
- Maybe add some printf() debugging lines that trace through the execution, just so I have some sanity checkpoints to compare my build against.
- Build zork2 from its .zap file and show that it works.

The truth of the matter is that I don't know the state of the original source code. Can it even genuinely compile and run?

In my first compilation/build tests, I think `malloc()` and pointer references/dereferences are going wrong. The source code defines its own malloc(), and then defines another MALLOC() on top of that one. I suspect that if I gut all of those (malloc, alloc, calloc, free) to just use more modern, standard library calls I'll be able to push through many of the troubles.

## What I've seen so far

- empty values for strings that should have resolved to SOMETHING
- `time(NULL)` returned `1`? (in `/test/timer.c`, it worked fine)