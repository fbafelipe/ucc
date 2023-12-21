# UCC

<p>Ucc is a very simple (and incomplete) C ansi compiler. Ucc stands for "Unnamed C Compiler".</p>
<p>This project was developed in 2009 when for my Compiler class at college. To run it you will need all three repositories: ucc, ucc-vm, and libparser.</p>

BUILDING UCC
===============

To build ucc you will need CMake (http://www.cmake.org) building system,
libparser (https://github.com/fbafelipe/libparser) and
ucc-vm (https://github.com/fbafelipe/ucc-vm) (ucc, ucc-vm and libparser
must be in the same folder).

ucc binaries will be placed in folder build.

To build ucc run these commands:

	$ cd build
	$ cmake ..
	$ make
