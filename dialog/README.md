## About Dialog and Cosmopolitan

Compiling the dialog compiler with `cosmocc` is simplicity.
Download the Dialog compiler source code from: https://hd0.linusakesson.net/files/dialog-0m03_0_46.zip

A makefile is located inside `/dialog-0m03_0_46/src`<br>
Use the cosmocc version of make `/cosmocc/bin/make -j dialogc`

Windows users need to append `.exe` to the executable.

`dialog_example.dg` file is just something simple to test compilation against. Dialog only targets .z5 and higher, so ZIP and mojozork cannot run the resultant game file.

Any other modern z-machine interpreter will play it without trouble.
Might I suggest [Status Line?](https://christopherdrum.itch.io/statusline)