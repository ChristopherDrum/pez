## About This Project
This build of Infocom's Unix ZIP (z-machine interpreter) was created using the cosmocc compiler and the Cosmopolitan Libc libraries. The -mtiny was used to strip out debugging stuff. Read about Cosmopolitan here: https://github.com/jart/cosmopolitan

You almost don't even need to ask, "Will this run on my computer?"

The magic of using Cosmopolitan is that this one file should run natively on pretty much any relatively recent 64-bit machine and OS: macOS (both Intel and M-series), Windows 8 through 11, Linux (lots of options), BSD (lots of options).

The minimum system support list is here: https://github.com/jart/cosmopolitan#support-vector

### Changes From the Original Source
To get this to build with a modern compiler, a few simple changes were required
1. `NULL` definition conflict with modern C libraries; replaced those with `0` which matches the old definition
1. Backspace (ASCII 8) recognized but my keyboard backspace (ASCII 128) needed to handling. Added check for this when inputting game commands.
1. `<sys/termio.h>` deprecated, replaced with `<termios.h>`
    - replaced attribute set/get calls with termios equivalents
1. Simplified the `srand()` seeding
1. Old C coding style used "locally-scoped forward declarations" for external functions that would be called in a routine. A modern C compiler doesn't seem to understand this pattern and complained about functions not being defined before use, or being "redefined." A full set of forward declarations for all functions made compilation much easier.
1. Return types added to all functions.
1. Added support for CTRL-D to exit the game.
1. Function definitions declared in K&R style, with parameter types in a separate line after the initial declaration. I matched those declarations with the new forward declarations (though K&R style seems to work OK)
1. I'm not entirely clear what was happening but stdout buffer needed more frequent `fflush()` calls than was being done. I wasn't seeing realtime updates of the status bar; screen refreshes were lagging by one full user action.
1. Inclusion of a couple of Cosmopolitan best practices
    - `__static_yoink("zipos");` at top of .c file
    - Call `cosmo_args()` to leverage APE ability to embed launch arguments and embedded data
<br><br>
## How to Use `zm` (the basic z-machine)

(Windows and Linux users, see below for a little additional information)

The executable called `zm` is just the raw Infocom UNIX z-machine. It can play games in the .z3 format, of which there are many (including modern games written in PunyInform). At the command-line of your choice, launch a game like this

`path/to/zm -g path/to/gamefile.z3`

So, if they're in the same folder, and your command-line context is currently within that folder

`./zm -g gamefile.z3`
<br><br><br>
## The Standalone Executables

Included in this archive is the Zork trilogy, as standalone executables. These take advantage of a fascinating feature of the APE file format. In addition to holding the cross-platform z-machine executable, these *also* embed the game data itself and the command-line arguments which can auto-launch directly into the embedded data. Just double-click the game to run; it couldn't be easier.
<br><br><br>

## Building Your Own

### Getting cosmocc
You will need the `cosmocc` compiler (a modified `gcc`) which self-embeds the Cosmopolitan Libc libraries you need. Grab it from https://cosmo.zip/pub/cosmocc/ and make sure to add the install location for `cosmocc` to your $PATH.

Alternately, modify the makefile references to `cosmocc` to point to wherever you have `cosmocc` copied to.

Alternately, alternately, just follow the commands in the makefile yourself manually, substituting in whatever is necessary for your current setup/installation of tools.


### Using the Makefile

`make zmachine`<br>
This builds a fresh copy of the `zm` executable from source. Feel free to add your own modifications and generate a custom zmachine for yourself.

`make embed EXE=<desired_exe_name> GAME=<path_to_gamefile.z3>`<br>
This will let you make a standlone zmachine with embedded game data, like the Zork distributions in this github release archive.

- `EXE=` needs to know what to name the final executable.<br>`EXE=zork1` will generate an executable named `zork1`
- `GAME=` needs to know the path to the .z3 game file you want to embed.<br>`GAME=/wherever/you/have/it/saved/zork1-r88-s840726.z3` will leave the original file alone, and put a copy of it directly into your executable file.

Taken all together (assuming everything is in the same folder)<br>
`make embed EXE=zork1 GAME=zork1-r88-s840726.z3`

<br>

## Troubleshooting

- [Robust troubleshooting steps for all platforms can be found here.](https://github.com/Mozilla-Ocho/llamafile/?tab=readme-ov-file#gotchas-and-troubleshooting)
- Building on macOS requires Xcode Command Line Tools be installed.
- Windows users need to add `.exe` extension to the executable name.
- Some Linux distibutions need a helping hand to run APE files.

    ```
    sudo wget -O /usr/bin/ape https://cosmo.zip/pub/cosmos/bin/ape-$(uname -m).elf
    sudo chmod +x /usr/bin/ape
    sudo sh -c "echo ':APE:M::MZqFpD::/usr/bin/ape:' >/proc/sys/fs/binfmt_misc/register"
    sudo sh -c "echo ':APE-jart:M::jartsr::/usr/bin/ape:' >/proc/sys/fs/binfmt_misc/register"
    ``` 
<br>

## Finding More z3 Games to Play
Your easiest source for classic Infocom games is at https://eblong.com/infocom/

[A nice list on IFDB (Interactive Fiction Database)](https://ifdb.org/search?searchfor=tag%3Apunyinform&sortby=&pg=all) has some 70+ .z3 games written in [PunyInform](https://github.com/johanberntsson/PunyInform?tab=readme-ov-file) to enjoy.

I've been testing the Zork Trilogy but other .z3 files (and ONLY .z3 files) should work just fine. I make no claims to the stability or compatibility of this legacy code! All I did was give it a little kick to bring it to life, so it exists in "whatever state Infocom left it in."

