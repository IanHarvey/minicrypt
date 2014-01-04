minicrypt
=========

This is a library containing a cryptographic primitives intended for embedded systems.

Its key features are:
- optimised for the smallest code and data size.
- portable C, with minimal dependence on runtime environment. 
- includes test vectors.
- all code is placed into the public domain.

I am also including the Python models I've used when developing the code.


Testing
=======

All algorithm implementations can be turned into self-test programs by #defining
macro TEST_HARNESS. There is a Makefile in the host/ directory which does just
this. I'm testing these on Mac OS 10.8 with XCode 5 command-line tools, but it
should work as-is on pretty much any Linux too.

