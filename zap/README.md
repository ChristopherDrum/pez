I don't know how to use zap, but I did get a Cosmopolitan build to run in Ubuntu and Windows 11.

Had to `cosmocc -g -c -o <filename>.o <filename>.c`
for every file we need

cosmocc -g   -c -o zap.o zap.c
cosmocc -g   -c -o zasm.o zasm.c
cosmocc -g   -c -o zdir.o zdir.c
cosmocc -g   -c -o zop.o zop.c
cosmocc -g   -c -o zlabel.o zlabel.c
cosmocc -g   -c -o zeval.o zeval.c
cosmocc -g   -c -o zfixup.o zfixup.c
cosmocc -g   -c -o zerror.o zerror.c
cosmocc -g   -c -o zlex.o zlex.c
cosmocc -g   -c -o zsymtab.o zsymtab.c
cosmocc -g   -c -o zdata.o zdata.c
cosmocc -g   -c -o zobj.o zobj.c
cosmocc -g   -c -o zstring.o zstring.c
cosmocc -g   -c -o zsymout.o zsymout.c
cosmocc -g   -c -o ztable.o ztable.c
cosmocc -g   -c -o zsegment.o zsegment.c
cosmocc -g   -c -o ALLOC.o ALLOC.c
cosmocc -g   -c -o resfile.o resfile.c
cosmocc -g   -c -o zapres.o zapres.c

Also had to modify `mkcomptime.c` to output `cosmocc` instead of `cc`
Then build `cosmocc -o mkcomptime mkcomptime.c -mtiny`
The build needs the file this generates, via the `./getdate` script
Then we link all of the .o files together
`cosmocc -o zap *.o`

I'll put all of the above into a simple bash script or something.

Running .zap executable against `zork2.zap` in Ubuntu (WSL2) gives us
```
 % ./zap ./zork2.zap
ZAP is running!
zap of Feb 28 2025 14:25:25.
Release: 996632432
     0  Inserting ./zork2.zap.
     0  Inserting zork2dat.zap.
zsh: segmentation fault  ./zap ./zork2.zap
```

Which, I mean, hey it is at least *attempting* to do something useful! Not sure how far I can debug this. I had to typecast a lot of parameters to a lot of functions; probably screwed something up along the way.

AND it works exactly the same in Windows 11 PowerShell. So we do have our APE build working. That was the main goal up to this point.

I'll at least make some effort to debug what's happening. Maybe there's something simple and obvious that will solve this problem. Might not be a deep problem? (might also be super deep)


----- 

Trying to run the original zap in Sun Sparc VM in qemu.
Need a baseline to run against; does this version of zap even work?

Not sure how to get files onto the vm. Tried making a simple data .iso to mount, but the sparc won't see the virtual cd-rom. Which is strange, because the vm saw the OS iso, booted from it, and installed the OS without issue. Not sure what the trouble is with just mounting a simple data disk now.

Data .iso was created in WSL2:
`mkisofs -o transfer.iso -r -R -V "SUN_TRANSFER" -graft-points <data_folder_path>`

Running qemu like this:
```
qemu-system-sparc -machine SS-5 -m 64 -hda sunos-hdd.img -device scsi-cd,channel=0,scsi-id=6,id=cdrom,drive=cdrom,physical_block_size=512 -drive if=none,file=transfer.iso,media=cdrom,id=cdrom
```


export CC="cosmocc -I/opt/cosmos/include -L/opt/cosmos/lib"
export CXX="cosmoc++ -I/opt/cosmos/include -L/opt/cosmos/lib"
export INSTALL=cosmoinstall
export AR=cosmoar
./configure --prefix=/opt/cosmos
make -j
make install