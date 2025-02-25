# Portable Executable Zork

The title says "Zork" but I'm using that classic game as a stand-in to mean "interactive fiction" in general.

The purpose of this repository is to collect my findings while using [Cosmopolitan Libc](https://github.com/jart/cosmopolitan) to build and run the tools (both modern and historical) for interactive fiction authoring. In a nutshell, I wish to preserve an end-to-end development workflow which runs natively on the [wide range of 64-bit machines supported by the Cosmopolitan project](https://github.com/jart/cosmopolitan?tab=readme-ov-file#support-vector). This is a software preservation project; we can almost trivially preserve interactive fiction for the 64-bit era of computing systems.

An end-to-end interactive fiction workflow boils down to three tasks: authoring, compiling, and playing. Projects that I've built and tested are noted here; I'll continue to expand the list as I gain experience with the build tools and various source code. Everything is built with `cosmocc` linked against the Cosmopolitan Libc libraries.
|||||
|-|-|-|-|
|**Authoring**|[vim](#vim)|||
|**Compiling**|[Inform6](#inform6)|[DialogC](#dialogc)||
|**Playing**|[Mojozork](#mojozork)|[Frotz](#frotz)|[Infocom's ZIP](#infocoms-zip)|

# Authoring

## vim
For authoring I have only been testing the `vim` build provided by Justine Tunney in her `cosmos` project.
It worked perfectly on my test systems, and *comes with Inform syntax highlighting built-in*. That made it an easy choice for this project, for now. `emacs` should also be viable, but I haven't given it a proper shakedown yet. There are other languages that I need to look into for their syntax support, namely ZIL and Dialog.

Download it from here: https://cosmo.zip/pub/cosmos/bin/vim

# Compiling
Not to be confused with the `cosmocc` C compiler which builds the executables for this project, interactive fiction programming languages have their own special compilers. These compilers translate the code we write in the interactive fiction domain-specific langauges (Inform6, Dialog, ZIL) into intermediate z-machine opcodes. The z-machine itself (see #Playing below) is a virtual machine with its own opcodes. I recommend this page to understand those: https://zspec.jaredreisinger.com/zz03-opcodes

## Inform6
Inform7 is technically the more relevant language to use these days, but at my current experience level it is too large and machine-dependent (very graphics/UI-heavy; maybe there is a subset I can target?) to tackle for this project. I've been using the Inform6 compiler, which gives us a little more flexibility in the compilation to target the limitations of specific interpreters (see #Playing below)

### Building Inform6
It couldn't be simpler. Assuming `cosmocc` is in your `$PATH` and you're in the `inform6` folder:
`cosmocc -o inform6 inform.c`

## DialogC
A newcomer to the scene, the interactive fiction is working to keep the project alive (appears to have been abandoned?) The language is quite different to Inform, taking a very Prolog "logic based" approach. This is similar to the change made to Inform from 6 to 7, however the languages couldn't be more different.

Where Inform7 takes a "literate" and "English language-like" approach, Dialog looks and feels more like a "typical" programming language. However, at the end of the day it can compile itself down to the same z-machine opcodes as Inform. If I write another text adventure, I will most likely write it in Dialog.

Dialog can only target .z5 and higher for its builds, so they won't run on the interpreters in this list (yet). But a .z8 file compiled using this APE build ran perfectly in Status Line, so I feel confident the APE workflow will be sound.

### Building DialogC
Again, it was simplicity, just copying some text from the makefile. I think now it could probably be have been built using the `cosmos` buid of `make` and running the makefile directly.
`cosmocc -DVERSION=\"0m/03\" -o dialogc frontend.c backend_z.c runtime_z.c blorb.c dumb_output.c dumb_report.c arena.c ast.c parse.c compile.c eval.c accesspred.c unicode.c backend.c aavm.c backend_aa.c crc32.c`

# Playing
Once we have our code written and compiled into a z-machine ready file, we need to be able to play and test it. That's where a z-machine interpreter comes in. I've written a full-featured one called Status Line for the Pico-8 in Lua, but of course we need a C-based one for this project.

## Mojozork
The first z-machine I tried, due to its single-file, no external dependencies simplicity. It only supports `.z3` games, which excludes a huge number of games, but does enable Infocom classics and many recent PunyInform games.
(link here)

I modified this to add VT100 terminal codes for a more polished presentation. This repo adds the inverted status bar and provides simple window scrolling in an 80 char x 24 line window. Classic presentation for classic games.

### Building Mojozork
`cosmocc -o mojozork mojozork.c`

## Frotz ("dumb" version)
A Unix/DOS z-machine interpreter, this long-running project has been ported over [to a huge number of vintage and modern machines](http://www.ifarchive.org/if-archive/infocom/interpreters/frotz/). It's "dumb" version plays a wider variety of games than #Mojozork (above), but provides only raw output; no status line or anything resembling "layout."

There are two other variations of Frotz which provide a more robust, visual experience: SDL and Curses
I felt intimidated by the SDL version; I'm unclear about Cosmopolitan Libc's boundaries when it comes to GUI and and graphics stuff. The Curses version (based on ncurses) stumped me for now. See #Unsolved Mysteries, below.

### Building DFrotz
From the `dumb` directory of Frotz: `cosmocc/bin/make -j`

## Infocom's ZIP
This is it, the big one, the OG written by the company that invented the z-machine itself. Until a couple of years ago we never had the source code to Infocom's own z-machine interpreters. [Now we have them all.](https://github.com/erkyrath/infocom-zcode-terps)

Most are written in assembly, targetting each platform's hardware directly. They had to really squeeze as much as they could out of those older machines, especially since Infocom was asking each machine to do natural language processing in a virutal machine in realtime with as little as 16K.

Their first work was on the PDP-10 in FORTRAN, but to bring Zork to wider audience they realized they would need to abstract things more to support the burgeoning home computer market. One operating system they did choose to target was Unix, and they wrote that in plain-ole C. Lucky for me! With a little elbow grease to modernize some of the older C coding conventions they used, it worked.

I can't express what a fantastic feeling it was to see that *original* code spring back to life on a modern computer.

This version of their interpreter only plays z3 games, but *does* include nice VT100-style character/terminal handling code to give a proper, real-deal, Infocom presentation. I saw code for split screen handling so even *Seastalker*'s radar should work I think. 🤔

### Building ZIP
`cosmocc -o cosmo_zip dgh_zip.c`

One thing to note is that this interpreter is made to run as a commercial product. So game files cannot be executed via command-line parameters, like the others can; it is expecting data to be ready to load at launch. Rename your .z3 file to 
`<exact_name_of_the_executable_you_are_running>.dat`

The interpreter looks for that specific, exact file name and auto-loads it. Note on Windows, you will need to add `.com` or `.exe` to the executable to make Windows happy. This means you also need to add that to your .dat filename
`<exact_name_of_the_executable_you_are_running>.<file_extension_you're_using>.dat`

# Unsolved Mysteries

## Adding additional libraries
`cosmocc` comes with the Cosmopolitan Libc libraries ready to go (notice how the compilation commands so far haven't needed to point to any libraries or includes). What I can't figure out just yet is how to add new libraries to the process.

There is a project called `superconfigure` which seems to address this, but I found it kind of hard to understand. I was only able to get it to build anything properly (`ncurses` is one of its bundled projects) using Ubuntu desktop in a VMware instance. Even then, I saw it had created x86_64 and ARM64... somethings... and a supposed APE file, which was only 0 bytes. I don't know what I'm supposed to do with those just yet, but it definitely seems possible; I just need more experience.

Getting Curses Frotz built will give us a basically "perfect" interpreter that will run anything you can throw at it on any* (within reason) 64-bit machine you like, (🤞) without the [makefile madness](https://gitlab.com/DavidGriffith/frotz/-/blob/master/Makefile?ref_type=heads) and [fractured distributables](https://pkgs.org/download/frotz) as exists currently.

# Testing the Workflow
Once I have built the APE files (actually portable executables), I copy them as-is to each of my systems and run them natively. Windows wants file extensions, so I do have to append `.exe` to each executable, but otherwise the executables are unchanged from system to system.

## My hardware
- MacBook Pro w/M3 Pro, macOS 15.3
- MSI Katana w/13th Gen Core i7, Windows 11/WSL2
- Ubuntu Desktop (via VMware on the Windows box)

## End-to-End test steps
In a terminal or PowerShell from within the `starter_kit` folder
1. Start a new .inf file in vim 
`./vim test.inf`
2. Write a simple "Hello World" style single-room adventure
3. Compile with inform6 against the PunyInform libraries 
`./inform6 v3 test +./punyinform`
4. Play in each interpreter
`./dfrotz ./test.z3`
