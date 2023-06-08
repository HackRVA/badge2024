# badge2023
RVASec Badge 2023 Firmware

# Initial Setup

There are two ways you can run the badge software:

1) You can build the badge software and run it on hardware. This is helpful when seeing how well something works on the
   badge itself.
2) You can build a simulator that can run the badge software on your computer. This is *usually* more helpful when
   developing an app, since debugging tools on your computer are more sophisticated and easier to set up.

They need a few different things in order to set yourself up to build and run the badge software.

In both cases, you will need **CMake**, which is a build system / build system generator. (Version 3.13 or higher)

## Git Setup

We're using `git` for version control, and there's one submodule being used.
This means that after cloning the repository, you'll need to run the command

```
	git submodule update --init --recursive
```
to get the submodule.

## Building for Hardware

For more info, see the [Pico SDK README](https://github.com/raspberrypi/pico-sdk).

When building the software for badge, you will need a *cross-compiler*. This takes code written on your computer and
compiles it into machine code for the badge. The compiler used is the ARM Embedded GCC compiler.

This may be available in your package manager (maybe called
"gcc-arm-none-eabi"), or
[here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads).
If downloaded from the ARM site, you will need to take steps to add it to your
`$PATH` or equivalent environment
variable. (If it works, when you open a new terminal window, the
`arm-none-eabi-gcc` program can be run).

### Visual Studio Code Setup

There are various editors and plugins you can add to better support CMake. A
common setup is using Visual Studio Code with its CMake Tools plugin. (When you
open the project folder with VS Code, it will suggest installing this plugin
automatically, which you should do.)

CMake Tools has a couple of additional concepts: Kits that define compilers it
can find, and Variants that define build parameters. If you've installed the
ARM Embedded GCC compiler, the plugin should be able to scan for kits and
automatically find your cross-compiler (you'll see one named `arm-none-eabi`).

If using VS Code with CMake Tools, you'll be able to use the Pico Variant (alongside the Kit for
the embedded compiler). Make sure to pick both the Kit and Variant that correspond with each other!
To switch Variants, you can click on the information icon in the blue bar the bottom of the VS code window.
To switch Kits, you can click on the wrench icon in the same bar.
Find more information on CMake Tools [here](https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/README.md)

You will want to change the target from `[all]` in the same bar at the bottom to `[badge2023_c]`.

The build folder will be named `build-pico` if using the VS Code variants.


### Command Line Build

If not using an IDE or VS code, you can use the following command to set up the build:

`	./run_cmake.sh`

which just does this:

`	cmake -S . -B build/ -G "Unix Makefiles"`

to configure the build. This generates a bunch of makefiles. To build the firmware:

```
	cd build
	make
```

(You can clean the build by running `make clean`.)

If the build is successful, a firmware blob will be produced at `build/source/badge2023_c.uf2`.

### A note for Windows users
[This link](https://community.element14.com/products/raspberry-pi/b/blog/posts/working-with-the-raspberry-pi-pico-with-windows-and-c-c)
has a bunch of useful information for getting started and installing prerequisites. Note that you don't need to do the
`PICO_SDK_PATH` setting portion, and when running and building this repository, you will want to use "NMake Makefiles"
instead of "Unix Makefiles" (unless you want to install and use `make` as well).

### Alternatively for Windows users
 Installing the development environment on Ubuntu, or another distro of your choice in WSL2 has been successfully accomplished, and was a fairly straightforward process. The best docs, surprisingly, came from [Microsoft](https://learn.microsoft.com/en-us/windows/wsl/). Don't skip the [VS Code integration with WSL](https://learn.microsoft.com/en-us/windows/wsl/tutorials/wsl-vscode).

You can use Ninja, if you like, as well. \(Specify `-G Ninja` instead of Makefiles in the `cmake` command.\)

## Flashing the Badge

To flash your firmware to the badge, press and hold the small white button just to
the right of the screen on the badge, and connect the badge via micro usb cable to
your computer and release the button.  This will cause the badge to act as a USB
storage device, and you should see a filesystem mounted on your computer.  On linux this
will typically appear at `/media/*username*/RPI-RP2`. Copy the firmware to this
location:

```
	cp source/badge2023_c.uf2 /media/*username*/RPI-RP2/
```

## Building the Simulator

The simulator is intended to run on a Posix-y (that is, Linux or Mac) environment. Windows can build and run it,
although by using Windows Subsystem for Linux if your Linux subsystem has a desktop environment set up.

To build the simulator, you will need a C compiler for your computer ("apt-get install build-essential"
on Debian based distros).  The simulator relies on SDL2
for graphics and keyboard/mouse/game controller support, so you will need to install SDL2.  For images,
libpng is needed.  ("apt-get install libsdl2-dev" package on Debian based distros, libpng-dev is usually already
present, on Mac, "brew install sdl2" and "brew install libpng").  In addition, portaudio is needed, the
debian package names might be libportaudio2 and portaudio19-dev. If you want to compile the simulator
without audio support, in the top level CMakelists.txt, make the following change:

```
diff --git a/CMakeLists.txt b/CMakeLists.txt
index 4b58925e..8c7eab01 100644
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -5,7 +5,7 @@ set(PICO_SDK_FETCH_FROM_GIT ON CACHE BOOL "Download Pico SDK from Git. Default o
 set(PICO_EXTRAS_FETCH_FROM_GIT ON CACHE BOOL "Download Pico SDK Extras from Git. Default on.")
 set(TARGET "PICO" CACHE STRING "Target hardware. For now, only Pico, in the future, badge/simulator")
 set(PRODUCT "badge2023_c")
-set(SIMULATOR_AUDIO "yes") # change to "no" to avoid compiling audio code in simulator
+set(SIMULATOR_AUDIO "no") # change to "no" to avoid compiling audio code in simulator
 
 if (${TARGET} STREQUAL "PICO")
     # Pull in SDK (must be before project).
```

### Visual Studio Code Setup

If you're using VS Code with CMake Tools, you should be able to pick the Simulator Variant (alongside the Kit for
your local computer's compiler to build the simluator). Make sure to pick both the Kit and Variant that correspond
 with each other!

The build folder will be named `build-simulator` if using the VS Code variants.

### Command Line Build

In a similar way to the hardware target, you can generate makefiles via CMake.

`	./run_cmake_sdl_sim.sh`

which just does this:

`	cmake -S . -B build_sdl_sim/ -DTARGET=SDL_SIMULATOR -G "Unix Makefiles"`

Note that to make the simulator, there is an extra flag that gets passed in: `-DTARGET=SDL_SIMULATOR`.

After which, you can `cd` into the `build_sdl_sim/` directory and run `make` to build the simulator target. The output
program is called `build_sdl_sim/source/badge2023_c`, which you can run.  It is compiled with debug information and
with address sanitizer and undefined behavior sanitizer to help catch bugs early and so you can easily debug things
with gdb.

## Off-Target Unit Tests

Off-target unit tests are run using CTest (part of CMake). The `test_key_value_storage` executable provides a template
that can be used.

## Generating Documentation

The code has doxygen-style comments that can be pulled out to an HTML site (or the other formats doxygen supports).
To generate it locally, you'll need `doxygen` and `graphviz` to be installed in your local environment. Once you do,
running `doxygen` in the `source` folder will create a folder called `docs` with the documentation output.

## Adding Your Own Apps

Apps are mostly contained within a single .c/.h file in the apps folder. Take a look at the comments inside the
`badge-app-template` files for help getting started.  See also
[BADGE-APP-HOWTO.md](https://github.com/HackRVA/badge2023/blob/main/BADGE-APP-HOWTO.md)

# Current Status

The overall structure of the repository is:
* `CMakeLists.txt` in the root directory is the main project definition. It includes subdirectories to add files/
  modules to the build. `pico_sdk_import.cmake` is provided by the Pico SDK.
* Code for apps and games is in the `source/apps` folder.
* Code for an interactive terminal (which may or may not be useful after the main application is running) is the
  `source/cli` folder. To run the CLI, hold the D-Pad left button down as the badge is starting. The display will
  show noise, and if you connect to the serial terminal that shows up on your computer, you can enter commands.
* Code that depends on Pico interfaces is within `source/hal/*_rp2040.c` files, with a platform agnostic header in the
  corresponding `source/hal/*.h` file. Code built for the simulator is in `source/hal/*_sim.c`.
* Generally helpful system code (main menus, screensavers, and the like) is in the `source/core` folder.
* Code for display buffers and drawing is in the `source/display` folder.

## What's working

Here's a list of major functional blocks and their current availability in software.
* :heavy_check_mark: indicates the functionality is there!
* :x: indicates the functionality still needs to be implemented.

| Component        | Badge Hardware     | Simulator          |
|------------------|--------------------|--------------------|
| LCD Display      | :heavy_check_mark: | :heavy_check_mark: |
| 3-Color LED      | :heavy_check_mark: | :heavy_check_mark: |
| D-Pad            | :heavy_check_mark: | :heavy_check_mark: |
| IR Tx/RX         | :heavy_check_mark: | :heavy_check_mark: |
| Rotary Encoder   | :heavy_check_mark: | :heavy_check_mark: |
| Audio Output     | :heavy_check_mark: | :heavy_check_mark: |
| Audio/Jack Input | :x:                | :x:                |

# To Do:

Basic Bringup:
* Add audio driver

Other Extensions:
* Add a unit test framework (perhaps for mocks?)
* Add a Rust build?
* MicroPython setup for badge hardware?
* Improve documentation for beginners
* GitHub Actions integration (build firmware/run tests/build docs)
* Add audio to simulator

[//]: # (MetaCTF{stop_asking_about_badge_problems})
