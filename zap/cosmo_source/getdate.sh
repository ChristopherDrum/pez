#
# Copyright (c) 1989, Infocom, Inc.  All rights reserved.
# Get the current date and time into a preprocessor definition,
# so we can tell lusers which version they have.

./mkcomptime > ./mkctscr
chmod 775 ./mkctscr
./mkctscr
