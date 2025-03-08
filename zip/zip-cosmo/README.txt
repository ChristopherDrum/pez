This build of Infocom's Unix ZIP (z-machine interpreter) was created using the cosmocc compiler and the Cosmopolitan Libc libraries. The -mtiny was used to strip out debugging stuff. Read about Cosmopolitan here: https://github.com/jart/cosmopolitan

You almost don't even need to ask, "Will this run on my computer?"

The magic of using Cosmopolitan is that this one file should run natively on pretty much any relatively recent 64-bit machine and OS: macOS (both Intel and M-series), Windows 8 through 11, Linux (lots of options), BSD (lots of options).

The minimum system support list is here: https://github.com/jart/cosmopolitan#support-vector

---------------

Infocom's ZIP was intended for use as a commercial product. As such, it auto-loads game data at launch, rather than being passed a game file as a launch parameter (though it could be modified to handle that case).

The game data file needs to be named the same as the executable, plus a ".dat" extension.
So, if the executable here is "zip" it will auto-load "zip.dat"

It plays .z3 games, so just rename any .z3 game to be "zip.dat" and put it in the same directory as the "zip" executable. Launch "zip" from a terminal or PowerShell window (double-clicking might even work; at least it did for me on Windows 11, see below)

---------------

Windows users need to add ".exe" extension to the "zip."
(remember, this was originally for Unix!)

This also means your data file name has to match the executable name exactly, so call it "zip.exe.dat." You can launch it directly from PowerShell, and even just double-clicking the .exe like any other application should (might?) work.

---------------

Finding a game to play:
Your easiest source for classic Infocom games is at https://eblong.com/infocom/

I've been testing with "zork1-r88-s840726.z3" but other .z3 files (and ONLY .z3 files) should work just fine. I make no claims to the stability or compatibility of this legacy code! All I did was give it a little kick to bring it to life, so it exists in "whatever state Infocom left it in."

Which is part of the fun of it all, I think.