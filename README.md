tfrdump
=======

Small Utility to dump Lucas Arts TFR files (TIE-Fighter savegames).

This program opens savegamefiles (*.TFR) from Lucas Arts TIE FIGHTER, reads and interprets known byte offsets, then prints
everything known so far to the standard output.

The code was tested with the german TIE Fighter CD-ROM Edition and works on Linux AMD64 systems, on Big Endian systems the betoW/betoDW 
functions should not be used because they change the endianness of larger datatypes, but as I don't use Windows I did not write some
OS-aware routines regarding the endianness. More details are in the code and documentation.

It is not complicated to extend the code to also write values into TFR files but it is not yet possible to use this code to
completely create such a file because there are some offset ranges with unknown purpose, e.g. the first two bytes of each file.

I'd like to have a class or struct which maps the TFR content exactly, thus can be used to read and write a TFR file directly, this should
not be complicated but rather tedious like always in reverse engineering. Patches welcome :-)

I only use C++ STL so no external libraries are needed.
I use some C++11 language features though (like the array datatype) so you should use a relatively modern compiler.

Compiling, Documentation, Usage
===============================

The code can be compiled with
g++ -std=c++0x tfrdump.cpp -O3 -s -o tfrdump
or
clang++ -std=c++11 tfrdump.cpp -O3 -s -o tfrdump

To create the documentation, use and simply invoke doxygen. All known hex offsets are documented in comments within the sourcecode,
I found it tedious to create an extra documentation about all offsets, the sourcecode should suffice to extend the work.

Usage: tfrdump <TFR-File>

History
=======

I wrote this code just for the fun of it. I wanted to know how to reverse-engineer TIE Fighter savegames with a hexeditor and as I
gained more knowledge about the TFR-file structure, I wrote this program to dump the contents.
First, I searched the web for some information but the results were rather sparse. There is a document about X-Wing and lists some
interesting details but they are not directly applyable to TIE Fighter.

Then I launched the game via dosbox, opened the stats menu and tried to find some of my pilot's numbers like kill counts and so on
in his TFR file.

As every offset is just one character of 8 bit and can represent a char, bool, short, I had to know how to combine these characters to 
longer datatypes with 16 or 32 bit like integers and long integers and as I was using a little endian system but opened a file created 
on a big endian system, I knew I had to reverse the bitorder from my offsets to get the values from the stats menu.

From then it was pretty straightforward: compare my savegame with the default savegames, change some bytes and find out what these changes 
made in the stats menu or create new pilots, made backups and then tried some things out in the game and see what changed.
This way I found block ranges for missions etc. and saw that they all have the same size even though not all battles have the same amount 
of missions, which means there are some unused ranges in the file.

There are some things I don't know, e.g. the purpose of the first two bytes in the files and the difference between killed and captured 
pilots (because the CD version of the game immediately restores the pilot in this case).
Having files of a fixed size makes it very easy for beginners, it was a funny little project.
If you are more advanced you may try out Privateer's savegames which are dynamic in size and content.

TIE-Fighter Remakes and tools
=============================

You could easily create an xml output or whatever with my codebase if you like to write some remake of the game and need an import module for savegames.
But beware: I needed about a (sparetime) week or so for this little experiment and found an old mission and savegame editor saved from geocities (R.I.P.) where 
the author said that he coded fulltime for about half a year to achieve his work (just an editor, no graphics no game mechanics, menus etc).
Sadly, all of these programs are closed source and the readme files contain contactinformation which are exactly as dead as geocities so you need to reinvent the 
wheel all over again.

License
=======

If you write some remake of the game, send me a note because I find the idea of a remake very exciting.
In other cases: Well, I don't care. Use the code or don't use it.
In any case: Don't complain if you set something on fire.