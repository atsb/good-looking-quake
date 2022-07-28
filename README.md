SDLquake 1.0.9 (approximately)
==============================

This is SDLquake, a port of id Software's Quake engine to the Simple Direct-media Layer 1.2 for Linux.  The Autotools build system has been updated to 2015 standards more or less.

Installation
------------

Run `autogen.sh` to generate the `configure` file. Then execute `./configure && make` to build the `sdlquake` executable. Run the `sdlquake` executable from within a directory containing the original Quake data files.

The Quake data files are expected to be named in lower case! This is true if you installed from Linux Quake installation media, but not necessarily true if you copied the data files over from Windows or performed the installation inside of DOSBox. If `sdlquake` fails to start and complains about missing files, be sure the `id1` directory and the `.pak` files within it are renamed to lower case.

See `INSTALL` for detailed build instructions. Support for building on 64-bit multiarch systems has been added and may need explanation.

See `README.SDL` for the original porter's comments.

Notes
-----

I have pulled in some 64bit fixes from older Fitzquake builds from 2010.  Seems to be ok, just when you start a new game it has "illegible server..." error.  This is being looked into.

License
-------

SDLquake is licensed under the GPLv2.  You should have received a copy of the GPLv2 in a file called COPYING in the same directory as this README.  If you did not, contact the distributor from whom you recieved this software for a copy.
