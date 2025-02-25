# Portable Executable Zork

The title says "Zork" but I'm using that classic game as a stand-in to mean "interactive fiction" in general.

The purpose of this repository is to gather my findings/research while using [Cosmopolitan Libc](https://github.com/jart/cosmopolitan) to build and run tools (modern and historical) for IF authoring. When necessary, I supply modified source code to enhance a project (make it more flexible, or improve output) or enable compilation via `cosmocc` (replace deprecated functions, etc.)

The final product of the build process for each tool is a [single executable in the APE format](https://justine.lol/ape.html). An APE file self-contains everything it needs to run natively on the [wide range of 64-bit machines supported by the Cosmopolitan project](https://github.com/jart/cosmopolitan?tab=readme-ov-file#support-vector). We don't need separate builds for each platform. In some cases we don't even really need a makefile any more. By swapping the compiler for `cosmocc` we can generate a platform-indepedent/agnostic tool which can be enjoyed by the widest audience possible.

An end-to-end interactive fiction development workflow boils down to three tasks: authoring, compiling, and playing. Projects that I've built and tested are noted here; I'll continue to expand the list as I gain experience with the build tools and various source code. Everything is built with `cosmocc` linked against the Cosmopolitan Libc libraries.
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

## [Inform6](https://github.com/DavidKinder/Inform6)
Inform7 is technically the more relevant language to use these days, but at my current experience level it is too large and machine-dependent (very graphics/UI-heavy; maybe there is a subset I can target?) to tackle for this project. I've been using the Inform6 compiler, which gives us a little more flexibility in the compilation to target the limitations of specific interpreters (see #Playing below)

### Building Inform6
The code can be cloned from: https://github.com/DavidKinder/Inform6<br><br>
[It's super simple to compile](https://github.com/DavidKinder/Inform6#using-inform-6) and we can trivially modify its compilation to point to `cosmocc`.<br>From inside the `inform6` repo folder:<br>`cosmocc -o inform6 *.c`

## [DialogC](https://linusakesson.net/dialog/index.php)
A newcomer to the scene, the interactive fiction community is working to keep the project alive (appears to have been abandoned?) [The Dialog language is quite different to Inform](https://linusakesson.net/dialog/index.php), taking a very Prolog "logic based" approach. This is conceptually similar to the changes made in Inform from v6 to v7.

Where Inform7 takes a "literate" and "English language-like" approach, Dialog looks and feels more like a "typical" programming language. However, at the end of the day it compiles itself down to the same z-machine opcodes as Inform.

Dialog can only target .z5 and higher for its builds, which will run on the `dfrotz` interpreter. I tested with compiling a .z8 file using this APE build and it ran perfectly in Status Line, so I feel confident an APE-based workflow for Dialog will be sound.

### Building DialogC
Sorce code can be downloaded from: https://hd0.linusakesson.net/files/dialog-0m03_0_46.zip

The Dialog source includes a makefile, but it isn't strictly necessary.<br>Run either of these from inside `/dialog-0m03_0_46/src`

- Using the makefile: `/cosmocc/bin/make -j dialogc`<br>
- Invoking `cosmocc` directly: `cosmocc -DVERSION=\"0m/03\" -o dialogc frontend.c backend_z.c runtime_z.c blorb.c dumb_output.c dumb_report.c arena.c ast.c parse.c compile.c eval.c accesspred.c unicode.c backend.c aavm.c backend_aa.c crc32.c`


# Playing
Once we have our code written and compiled into a z-machine ready file, we need to be able to play and test it. That's where a z-machine interpreter comes in. I've written a full-featured one called Status Line for the Pico-8 in Lua, but of course we need a C-based one for this project.

## [Mojozork](https://github.com/icculus/mojozork)
The first z-machine I tried, it gave me a quick way to test a z-machine interpreter with its single-file, no-external-dependencies simplicity. It only supports `.z3` games, which excludes a huge number of games, but does enable Infocom classics and many recent PunyInform games.
(link here)

I modified this to add VT100 terminal codes for a more polished presentation. This repo adds the inverted status bar and provides simple window scrolling in an 80 char x 24 line window. Classic presentation for classic games.

### Building Mojozork
`cosmocc -o mojozork mojozork.c`

## [Frotz](https://davidgriffith.gitlab.io/frotz/)
A Unix/DOS z-machine interpreter, this long-running project has been ported over [to a huge number of vintage and modern machines](http://www.ifarchive.org/if-archive/infocom/interpreters/frotz/). Its "dumb" version plays a wider variety of games than #Mojozork (above), but provides only raw output; no status line or anything resembling "layout."

There are two other variations of Frotz which provide a more robust visual experience: SDL and Curses
I felt intimidated by the SDL version; I'm unclear about Cosmopolitan Libc's boundaries when it comes to GUI and and graphics stuff. The Curses version (which relies on `ncurses`) stumped me for now. See [Adding additional libraries](#adding-additional-libraries), below.

### Building DFrotz
Clone the source from here: https://gitlab.com/DavidGriffith/frotz
From the `frotz/src/dumb` directory of Frotz: `cosmocc/bin/make -j`

## [Infocom's ZIP](https://github.com/erkyrath/infocom-zcode-terps/blob/master/unix/phg_zip.c)
This is it, the big one, the OG written by the company that invented the z-machine itself. Until a couple of years ago we never had the source code to Infocom's own z-machine interpreters. [Now we have them all.](https://github.com/erkyrath/infocom-zcode-terps) 

Most of the interpreters are written in assembly, targeting each platform's hardware directly. They had to really squeeze as much as they could out of those older machines, especially since Infocom was asking each machine to do natural language processing in a virtual machine in realtime with as little as 16K.

Their first work was on the PDP-10 in FORTRAN, but to bring Zork to a wider audience they realized they would need to abstract things more to support the burgeoning home computer market. One operating system they did choose to target was Unix which they wrote in plain-ole C. Lucky for us! 

I focused on Paul Gross's modified Unix code, `phg_zip.c` because I somehow find it easier to follow. 🤷‍♂️ With a little elbow grease to modernize older coding conventions and deprecations, it worked.

I can't express what a fantastic feeling it was to see that *original* code spring back to life on a modern computer and to know it was instantly available to almost anyone who wanted to try it out. No platform-specific shenanigans or workaround or makefiles. Just build and go. This, to my mind, is how preservation should be; so simple anyone can do it.

This version of their interpreter only plays z3 games, but *does* include VT100-style character/terminal handling code to give a proper, real-deal, Infocom presentation. I saw code for split screen handling so even *Seastalker*'s radar should work I think. 🤔

### Building ZIP
`cosmocc -o zip dgh_zip.c`

One thing to note is that this interpreter is made to run as a commercial product. So game files cannot be executed via command-line parameters, like the others can; it is expecting data to be ready to load at launch. Rename your .z3 file to 
`<exact_name_of_the_executable_you_are_running>.dat`

The interpreter looks for that specific, exact file name and auto-loads it. Note on Windows, you will need to add `.com` or `.exe` to the executable to make Windows happy. This means you also need to add that to your .dat filename
`<exact_name_of_the_executable_you_are_running>.<file_extension_you're_using>.dat`

# Testing the Workflow
Once I have built the APE files (actually portable executables), I copy them as-is to each of my systems and run them natively. Windows wants file extensions, so I do have to append `.exe` to each executable, but otherwise the executables are unchanged from system to system.

## My hardware
- MacBook Pro w/M3 Pro, macOS 15.3
- MSI Katana w/13th Gen Core i7, Windows 11/WSL2
- Ubuntu Desktop (via VMware on the Windows box)

## A sample end-to-End test
In a terminal or PowerShell from within the `starter_kit` folder
1. Start a new .inf file in vim 
`./vim test.inf`
2. Write a simple "Hello World" style single-room adventure
3. Compile with inform6 against the PunyInform libraries 
`./inform6 v3 test +./punyinform`
4. Play in each interpreter
`./dfrotz ./test.z3`

# Next

## Adding additional libraries
`cosmocc` comes with the Cosmopolitan Libc libraries (think "standard C libraries") ready to go (notice how the compilation commands so far haven't needed to point to any libraries or includes). What I can't figure out just yet is how to add additional libraries to the process.

There is a project called [superconfigure](https://github.com/ahgamut/superconfigure/) which seems to address this, but I found it kind of hard to understand. I was only able to get it to build properly (`ncurses` is one of its bundled projects) using Ubuntu desktop in a VMware instance. Even then, I saw it had created x86_64 and ARM64... somethings... and a supposed APE file, which was only 0 bytes. I don't know what I'm supposed to do with those just yet. It definitely seems possible; I just need more experience.

Getting `ncurses` working should enable us to do a universal build of Curses Frotz. That will give us a basically "perfect" interpreter you can run on any 64-bit machine you like (🤞), without the [fractured distributables](https://pkgs.org/download/frotz) as exists currently.

## ZIL with [ZAP](https://eblong.com/infocom/#zap)
Getting a universal toolset for running these would be a great addition to having ZIP running. ZIL was the programming language Infocom used internally for their game development, where we tend to use Inform today. This was then compiled into z-machine assembly using a tool called ZILCH. That assembly was then processed into z-machine code through a program called ZAP. The output of ZIP was a data file that ZIP interpreters could play. 

ZILF and ZAPF are from-scratch replacements to the ZILCH and ZAP toolchain above. [I use these tools](https://github.com/ChristopherDrum/status-line-classics/blob/main/build.sh) to provide bespoke customizations of classic Infocom source code which are formatted to look good in Status Line. However, these tools were written in Visual Studio in C# and would need to be completely rewritten in portable C to preserve via Cosmopolitan.

[ZILCH is quite complex](https://github.com/ZoBoRf/ZILCH-How-to?tab=readme-ov-file#to-run-zilch-muddle-is-needed), and likely won't be possible to target for this project. Running original ZILCH requires ITS MUDDLE and... that's about as far as I can understand right now. Getting ZILCH alone to run at all sounds like it was a pretty monumental task; an easy compilation simply looks out of reach.

## More authoring options
Need to check on emacs and the state of interactive fiction language support built-in. I doubt Dialog is included, so offering a build of `vim` of `emacs` ready for both of those languages would be nice. Maybe as I get more comfortable with more complex projects we can suggest more visual editors (Zed?). The current cosmos builds are pretty comprehensive, so maybe a stripped-down build could be provided that is focused exclusively on interactive fiction authoring?

## [Scott Adams games](https://www.mobygames.com/group/8028/scott-adams-classic-adventures/)
I did a quick-and-dirty build of [ScottFree](https://www.ifarchive.org/if-archive/scott-adams/interpreters/scottfree) and played Pirate Adventure in a universal build. I had to remove some deprecated string handling to get it working, which I could put more effort into repairing properly, but the two word verb-noun system still maintains a loyal following. I'm not so clear on what "language" those games are written in these days, or how to compile for ScottFree. This is a large gap in my IF knowledge base.
