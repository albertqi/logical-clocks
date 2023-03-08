# CS 262: Logical Clocks

## Build Information

This project uses CMake to generate build files. All you need is cmake, make (or your build system of choice), and a working c++ compiler.

Execute the following commands to compile the system:

```
mkdir build
cd build
cmake ../
make process # To compile a process
make test    # To compile the unit tests
```

Execute the following commands to run the files:

```
./process [Hz] # To run a process at [Hz] clock ticks per second

               # If [Hz] is not specified, then the number of clock ticks per
               # second will be a random integer between 1 and 6, inclusive

./test         # To run the unit tests
```