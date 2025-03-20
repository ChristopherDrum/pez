# Getting `cosmocc` to run on Windows

This is kind of diary of my investigations (struggles) with getting Windows to play nicely with Cosmopolitan build tools. The final executable does not require any of these shenanigans. On Windows, just add a `.exe` extension to the executable file name and enjoy the program.

I didn't have any troubles running cosmocc from macOS terminal, but I've had difficulties getting it to work properly on Windows. This process seems to be working right now (but I'm starting to think that WSL2 installed is giving me a helping hand; paths for `bash` and `dash` report being at `usr/bin` rather than the cosmos folder)

  1. Get the `cosmos` files and point $PATH to the `\bin` folder therein. This includes a number of useful UNIX utilites which have been compiled into APE files and enables Windows to utilize them. In particular, we want `dash` but `bash` is also there and useful.
  1. Get `cosmoscc` and point $PATH to the `\bin` folder therein. The main utility we're interested in is `cosmoscc`, a `sh` script which launches the build toolchain. That toolchain consists entirely of APE builds. So, this is all single-executable/multi-platform stuff. `cosmos` batch of utilities and `dash` and such just help get Windows the tools it needs to keep up with the UNIX crowd.
  1. In the directory with the `.c` file you're wanting to compile
     `dash DRIVE:/full/path/to/cosmocc -o hello.com hello.c`
     On Windows, the `/` is important here; don't use `\`. `dash` doesn't seem to recognize our $PATH All we're trying to do here is just get Windows to run the sh script and resolve the directory/file paths properly while doing so. That seems to be the primary blocker in getting this process to work.<br><br>The `.com` extension is only there to help Windows understand it is a valid Windows executable. It does not change HOW the file is built! The same executable will still run on macOS and Ubuntu just fine (minus extension?). Windows doesn't seem to sense APE files properly; perhaps there is a way to teach it?
  1. This worked without `dash`<br>
`bash -c "D:/_Development/_Tools/cosmocc/bin/cosmocc -o hello.com hello.c"`<br>
Even simpler: `bash -c "cosmocc -o hello.com hello.c"`<br><br>
  The `bash -c "..."` pattern works great for cosmos utilities, including `vim`. But why doesn't `bash` itself need that pattern? Neither does `dash`. What is the difference between those tools and the other tools?
  1. Even stranger, if I start up a terminal directly into `bash` I can't get it to resolve the path to `cosmocc` even though it is listed in `echo $PATH`
  1. OK, the Cosmopolitan instructions for registering APE loader with Linux worked for WSL2, it seems. I don't know if this persists between launches, but now from PowerShell or Ubuntu shell I can run `wsl cosmocc -o hello hello.c` and it works just fine.
    - update: no, it requires running a bunch of commands in WSL2 first, which don't seem to persist across sessions; maybe we can do a little script?
    - another update:
      ```bash
      sudo sh -c 'echo -1 > /proc/sys/fs/binfmt_misc/cli'     # remove Ubuntu's MZ interpreter
      sudo sh -c 'echo -1 > /proc/sys/fs/binfmt_misc/status'  # remove ALL binfmt_misc entries
      ```
      seems to be the trick. I put an alias to this in my .zshrc which I can trigger with `prepcosmo` before I start doing builds.
1. (2025/03/20) Running zip/cosmo_source/makefile in PowerShell directly works perfectly; no `wsl` prefix/setup/prep necessary.
