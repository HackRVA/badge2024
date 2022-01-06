# badge2022
RVASec Badge 2022 Firmware

# Initial Setup

For more info, see the [Pico SDK README](https://github.com/raspberrypi/pico-sdk).

You will need two prerequisites:
* the `arm-none-eabi-gcc` toolchain, installed locatable in your `$PATH`.
* CMake, which is used as the build system and downloads the Pico SDK.

There are various editor plugins you can add to better support CMake, but running just from CLI, you can run:

`cmake -S . -B build/ -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles"`

to configure the build, and

`cd build && make`

After this, the file to flash on the device is at `build/source/badge2022_c.uf2`.

You can flash the Pico by holding down the button on the Pico board as you're plugging it in to USB. That will make it
show up as a USB mass storage device. Then, you can just put the `.uf2` on the mass storage device to flash it. The
Pico will boot your new firmware immediately.

You can use Ninja, if you like, as well. \(Specify `-G Ninja` instead of Makefiles in the `cmake` command.\)

## Current Status
This repository is in a prototype phase. Major changes may be introduced.

The overall structure of the repository is:
* `CMakeLists.txt` in the root directory is the main project definition. It includes subdirectories to add files/
  modules to the build. `pico_sdk_import.cmake` is provided by the Pico SDK.
* Code that depends on Pico interfaces is within `source/hal/*_rp2040.c` files, with a platform agnostic header in the 
  corresponding `source/hal/*.h` file. For the simulator, the hope is that implementation code can be put in a 
  source file for the other platform (linux/posix). Hopefully also, this provides an interface we can use to 
  run a more complete simulator.
* Code for an interactive terminal (which may or may not be useful after the main application is running) is the
  `source/hal` folder.
* Code for display buffers and drawing is in the `source/display` folder.

## Hardware
Currently, this is hacked on to a 2021 badge by removing the Microchip processor nad flying wires out from a 
Pico board out to the hardware. See `pinout_rp2040.h` for the Pico pinout I (Sam) have wired up.

## What's working
There are a few functional components:

* The display driver works over the SPI peripheral, and enough of the old code is copied with it to display 
  assets and demonstrate this.
* Basic flash storage is demonstrated.
* The 3-color and white LEDs can be turned on and PWMed with Pico peripherals.

Right now, there is one way you can use the device interactively. After flashing, the Pico board will enumerate to
the computer using an emulated serial port over USB. You can use `screen` or a similar utility to act as a serial
terminal. There are commands for LED brightness, writing/reading nonvolatile storage, and some basic
other commands. The `cli` files have the code that makes this work.

# To Do:

Basic Bringup:
* Bring up IR TX / RX
* Bring up Audio Driver
* Bring up Button GPIO interfaces
* Bring up microphone interface?
* Bring up 3 color light sensors?

Simulator:
* Add a simulator target for the main badge app
* Port HALs that are currently in the old repo 
* Add HAL files for currently not-simulated data.

Driver and application enhancements, as we're porting:
* Display: double buffering, so software can update a display buffer while the other is being updated via DMA
* Rework main loop to support double buffering.
* Get current main app running
* Audio: are there fancier things we can do with the driver we have currently?

Other Extensions:
* Add a unit test framework
* Add a Rust build?
* MicroPython setup for badge hardware?
* Improve documentation for beginners
