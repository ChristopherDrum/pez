#!/bin/zsh

#This is only for building ZAP, not any of its sibling tools

#cleanup
echo "Cleaning up previous build files..."
rm -f ./*.o
rm -f ./mkcomptime
rm -f ./mkcomptime.com.dbg
rm -f ./mkcomptime.aarch64.elf
rm -f ./zap
rm -f ./zap.exe
rm -f ./zap.com
rm -f ./zap.com.dbg
rm -f ./zap.aarch64.elf
rm -Rf ./.aarch64
echo "...done"

#first, build the mkcomptime helper executable
#we can probably just rework things to eliminate the need for this
echo "Building mkcomptime and running getdate..."
cosmocc -std=c89 -o mkcomptime mkcomptime.c
chmod 775 ./mkcomptime


#getdate script uses the mkcomptime executable made above
#overly convoluted way to create a timestamp :/
./getdate
echo "...done"

function make_o_files {
	local arr=("$@")
	for element in "${arr[@]}"; do
		echo "Building $element.o"
		cosmocc -std=c89 -w -g -c -o "$element.o" "$element.c"
	done
}

zap_files=("zap" "zasm" "zdir" "zop" "zlabel" "zeval" "zfixup" "zerror" "zlex" "zsymtab" "zobj" "zstring" "zsymout" "ztable" "zsegment" "ALLOC" "resfile" "zapres" "zdata" "comptime")

make_o_files "${zap_files[@]}"

#combine the .o files into an APE
echo "Linking into an APE file..."
cosmocc -std=c89 -g -o zap *.o
chmod 775 ./zap
echo "Copying files..."
cp ./zap ./zork2/zap; cp ./zap.com.dbg ./zork2/zap.dbg 
echo "Done with the build!"
