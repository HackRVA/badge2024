
This README describes tools in this folder.

# Asset Generator / Image Converter

This utility is a Python program that takes images in a folder and generates C structures that can be used
with the framebuffer functions to draw that image on the badge.

## Dependencies

You'll need Python. Additional dependencies can be installed by running `pip install -r requirements.txt` 
in this directory.

## What you'll need

You should have a folder that contains:

1) The images you want to convert in png, jpg, or bmp format (Others may work too).
2) an `images.yaml` with a list of the images to be converted. More details on that down the page.

It generates several things inside that folder:

1) a C file for each image. This C file contains the data structure for your converted image.
2) a single header file, containing extern declarations of the structures in your C files. You can include this
   inside your app, in order to reference your images.
3) a CMakeLists.txt file that adds your files to the build.

It is best to have a setup something like the following...

```
   source/apps/
               ...
               CMakeLists.txt
               ...
               myapp.h
               myapp.c
               myapp_images/
                            cool_picture.png
                            cool_picture_2.png
                            images.yaml
```

When you run the script:

```python tools/asset_converter.py source/apps/myapp_images/```

If everything succeeds, you should have some new files, in addition to the old ones:
```
   source/apps/myapp_images/
                            CMakeLists.txt
                            cool_pic.c
                            cool_pic_2.c
                            myapp_assets.h
```

You can modify the already-existing `source/apps/CMakeLists.txt` to add a line: `add_subdirectory(myapp_images)`. Now,
your images are added to the build! you can `#include "myapp_assets.h"` and start drawing them!

### `images.yaml`

(See feed-asset-converter, below)

Your `images.yaml` should have the following structure:

```yaml
name: myapp_assets
images:
  - name: "cool_pic"
    filename: "myapp_cool_picture.png"
    bits: 4
  - name: "cool_pic_2"
    filename: "myapp_cool_picture_2.png"
    bits: 8
```

The `name` at the top of the file will be name of the generated header (.h) file. Each image needs three fields:
* Another `name`, which will be the name of the (.c) file.
* The `filename` of the image to be converted
* The bit level to use. Valid values are 1, 2, 4, 8, and 16. The smaller the bit level, the smaller the images, so use
  the smallest one you can.
  * 1-bit images are "monochrome" images. You can specify the exact colors to use in the framebuffer code.
  * 2, 4, and 8 bit images are "palette" images. The converter will generate a palette of colors to use, and encode
    the images to use only those colors. The maximum number of colors used at the bit levels 4, 16, and 256 colors, 
    respectively. If your images have more than the maximum number of colors in them, the library used to the script 
    will figure out a good set of colors to use automatically.
  * 16-bit images are pure color images. (it stores the image in a raw, display-native format.)

### feed-asset-converter

The above YAML file is a pain in the ass to create.  feed-asset-converter can help you
create it from a directory of .png files.

```
    ./feed-asset-converter mypngdir myapp_assets [bits-per-pixel] > myfile.yaml
```

bits-per-pixel is by default, 8.

