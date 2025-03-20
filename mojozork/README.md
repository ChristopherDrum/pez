# About This Project

Before I thought of tackling the original Infocom ZIP code (/zip/cosmo_source in this repo), my early work in using Cosmopolitan needed a simple project. `mojozork` is a single-file implementation of a basic z-machine. I modified it to include basic VT100 terminal character codes and more `fflush(stdout)` calls for realtime updates.

`/tests/vt100test.c` is a sample project I did to make sure the character codes output as expected before putting them in place into the mojozork code.

The original project pointed to a zork.dat file, but I switched the internal reference in `main()` to a standard z3 file `zork1-r88.z3` and it works just fine. That was as far as I took this test project.

## Compiling with cosmocc

Compile this .c file with `cosmocc` as
`cosmocc -o mojozork mojozork.c -mtiny`

On Windows, append the `.exe` file extension to the built executable to make Windows happy. From a terminal or PowerShell run `./mojozork` with `zork1-r88.z3` in the same directory.
