XGS
===

XGS is a Linux-based Apple IIGS emulator.

# About This Version

This beta is a major rewrite of previous (pre-0.60) versions. The code has
been converted to C++ and refactored to better match (when possible) how the
actual hardware is implemented. In addition, the multiple input/output drivers
have been replaced with a single SDL2 driver.

The CPU emulation (M65816) is a heavy rewrite from previous versions as well,
and may contain subtle emulation bugs.

More information can be found in the XGS section of [my blog](https://www.thompson.us.org/joshua/xgs/).

# Compiling

To compile this code you will need

- CMake
- g++ 4.8 or higher. For versions prior to 6.0 you need to compile with --std=c++11
- The Boost libraries
- SDL2

To build XGS, from the source directory run:

```
mkdir build
cd build
cmake ..
make 
```

The binary will be compiled to build/src/xgs.

# Usage

Before starting XGS for the first time you'll need to create the XGS home directory
and copy some files into it.

First, from the source directory run:

```
mkdir ~/.xgs
cp xgs40.fnt xgs80.fnt ~/.xgs/
```

You'll need an Apple IIGS ROM file (either v01 or v03 will work):

```
cp PATH_TO_YOUR_ROM_FILE ~/.xgs/xgs.rom
```

Now you can start XGS as follows (-3 is only needed if your ROM image is v03):

```
./build/src/xgs -3
```

You can also display the help text if you need help with the command-line options:

```
./build/src/xgs --help
```

# Status

At the moment, this version mostly boots. That is to say, it will start up but
dump to the Monitor prompt almost immediately. This is likely due to CPU bugs.
I've been finding and fixing these by comparing the debug output against the
output from older versions of XGS.

This code will be very much in flux for the next few months as I continue to
debug, improve, and streamline the code base.
