The primary changes between cosmo/phg_zip.c and original/phg_zip.c are
1. `NULL` definition conflict with modern C libraries; replaced those with `0` which matches the old definition
1. Backspace (ASCII 8) recognized but my keyboard backspace (ASCII 128) needed to handling. Added check for this when inputting game commands.
1. `<sys/termio.h>` deprecated, replaced with `<termios.h>`
    - replaced attribute set/get calls with termios equivalents
1. Simple `srand()` seeding
1. Old C coding style used a kind of "locally scoped forward declaration" for external functions that would be called later in a routine. Modern C compiler didn't seem to understand this pattern and complained about functions not being defined before use, or being "redefined." I was also adding proper return types as well, so it was just easier to add a full, proper set of forward declarations for all functions and fix that problem cleanly.
1. Return types added to all functions
1. Function definitions declared the parameter types in a separate line after the initial declaration. I switched those to all be more modern style to match the forward declarations (though that old style seems to still compile and work OK)
1. I'm not entirely clear what was happening (did default stdout behavior change over the years?) but stdout buffer needed more frequent `fflush()` calls than was being done. I wasn't seeing realtime updates of the status bar; screen refreshes were lagging by one full user action.