# badge2022
RVASec Badge 2022 Firmware

# Initial Setup

There are two ways you can run the badge software:

1) You can build the badge software and run it on hardware. This is helpful when seeing how well something works on the
   badge itself.
2) You can build a simulator that can run the badge software on your computer. This is *usually* more helpful when
   developing an app, since debugging tools on your computer are more sophisticated and easier to set up.

They need a few different things in order to set yourself up to build and run the badge software.

In both cases, you will need **CMake**, which is a build system / build system generator. (Version 3.13 or higher)

## Building for Hardware

For more info, see the [Pico SDK README](https://github.com/raspberrypi/pico-sdk).

When building the software for badge, you will need a *cross-compiler*. This takes code written on your computer and
compiles it into machine code for the badge. The compiler used is the ARM Embedded GCC compiler. 

This may be available in your package manager, or [here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads).
If downloaded from the ARM site, you will need to take steps to add it to your `$PATH` or equivalent environment
variable. (If it works, when you open a new terminal window, the `arm-none-eabi-gcc` program can be run).

There are various editor plugins you can add to better support CMake ([example](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)),
but running just from CLI, you can run:

`cmake -S . -B build/ -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles"`

to configure the build. This generates a bunch of makefiles, and

`cd build && make` to build the firmware.

After this, the file to flash on the device is at `build/source/badge2022_c.uf2`.

You can clean the build by running `make clean`.

You can flash the Pico by holding down the button on the Pico board as you're plugging it in to USB. That will make it
show up as a USB mass storage device. Then, you can just put the `.uf2` on the mass storage device to flash it. The
Pico will boot your new firmware immediately.

You can use Ninja, if you like, as well. \(Specify `-G Ninja` instead of Makefiles in the `cmake` command.\)

## Building the Simulator

The simulator is intended to run on a Posix-y (that is, Linux or Mac) environment. Windows hasn't been tried yet, but
it's likely that Windows Subsystem for Linux and or MinGW either work or will work without too much effort. But to
build the simulator, you will need a C compiler for your computer, in addition to GTK2. The best way to install
GTK2 is probably through a package manager.

In a similar way to the hardware target, you can generate makefiles via CMake. Note that to make the simulator, there is
an extra flag that gets passed in:

`cmake -S . -B build_sim/ -DCMAKE_BUILD_TYPE=Debug -DTARGET=SIMULATOR -G "Unix Makefiles"`

After which, you can `cd` into the `build_sim/` directory and run `make` to build the simulator target. The output
program is called `build_sim/source/badge2022_c`, which you can run.

## Adding Your Own Apps

Apps are mostly contained within a single .c/.h file in the apps folder. Take a look at the comments inside the
`badge-app-template` files for help getting started.

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
| IR Tx/RX         | :heavy_check_mark: | :x:                |
| Rotary Encoder   | :heavy_check_mark: | :heavy_check_mark: |
| Audio Output     | :x:                | :x:                |
| Audio/Jack Input | :x:                | :x:                |

# To Do:

Basic Bringup:
* Add audio driver
* Enhance GPIO driver for rotary encoder

Other Extensions:
* Add a unit test framework
* Add a Rust build?
* MicroPython setup for badge hardware?
* Improve documentation for beginners
