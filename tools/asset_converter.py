# requirements: Pillow, PyYAML, bitstring
# Install using `pip install Pillow PyYAML bitstring`

# generates:
# A C file for each generated sprite
# A h file with external declarations
# A CMakeLists.txt file

import bitstring
from PIL import Image
import yaml
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("folder", help="folder containing images and images.yaml")

args = parser.parse_args()
input_path = Path(args.folder)

with open(input_path.joinpath("images.yaml"), 'r') as f:
    image_yaml = yaml.safe_load(f)

asset_header_name = f"{image_yaml['name']}.h"
asset_header = open(input_path.joinpath(asset_header_name), 'w')

asset_header.write(f"""

// ASSET HEADER AUTOMATICALLY GENERATED

// Included for struct asset
#include "assetList.h"

#ifndef _{image_yaml["name"]}_h_
#define _{image_yaml["name"]}_h_

""")

asset_cmakelists = open(input_path.joinpath("CMakeLists.txt"), 'w')
asset_cmakelists.write("""

# THIS CMakeLists.txt AUTOMATICALLY GENERATED

target_sources(badge2022_c PUBLIC

""")


def bytes_for_color_image(rgb_image, num_bits):
    width, height = rgb_image.size
    image_bytes = b""
    for y in range(0, height):
        for x in range(0, width):
            pix = rgb_image.getpixel((x, y))
            if num_bits == 8:
                pix_bitstr = bitstring.BitString(uint=int(pix[0] / 32), length=3)
                pix_bitstr.append(bitstring.Bits(uint=int(pix[1] / 32), length=3))
                pix_bitstr.append(bitstring.Bits(uint=int(pix[2] / 64), length=2))
                image_bytes += pix_bitstr.bytes
            else:
                pix_bitstr = bitstring.BitString(uint=int(pix[0] / 8), length=5)
                pix_bitstr.append(bitstring.Bits(uint=int(pix[1] / 4), length=6))
                pix_bitstr.append(bitstring.Bits(uint=int(pix[2] / 8), length=5))
                # little-endian
                image_bytes += bytes(reversed(pix_bitstr.bytes))

    c_array = ", ".join([hex(byte) for byte in image_bytes])

    return len(image_bytes), c_array


def bytes_for_palette_image(palette_image, num_bits):
    image_bitstr = bitstring.BitArray()
    width, height = palette_image.size
    for y in range(0, height):
        for x in range(0, width):
            pix = palette_image.getpixel((x, y))
            image_bitstr.append(bitstring.Bits(uint=pix, length=num_bits))

        padding = 8 - (len(image_bitstr) % 8)
        if padding == 8:
            padding = 0
        image_bitstr += bitstring.Bits([0] * padding)

    # only 1 bit images have bit order reversed in byte
    if num_bits == 1:
        corrected_image_bitstr = bitstring.BitArray()
        for bitstr_byte in [image_bitstr[i:i+8] for i in range(0, len(image_bitstr), 8)]:
            bitstr_byte.reverse()
            corrected_image_bitstr += bitstr_byte
        image_bitstr = corrected_image_bitstr

    image_bitstr_bytes = image_bitstr.bytes
    c_array = ", ".join([hex(byte) for byte in image_bitstr_bytes])
    return len(image_bitstr_bytes), c_array


def colormap_for_palette_image(colormap):
    c_array = ""
    inverted_map = {v: k for k, v in colormap.items()}
    for i in range(0, len(inverted_map) - 1):  # one too many colors, last one is always black
        color_tuple = inverted_map[i]
        c_array += f"{{{color_tuple[0]}, {color_tuple[1]}, {color_tuple[2]}}},\n"
    return len(inverted_map) - 1, c_array


for asset in image_yaml["images"]:

    if "y_sprite_count" not in asset:
        asset["y_sprite_count"] = 1

    image_path = input_path.joinpath(asset["filename"])

    with Image.open(image_path) as im:

        if len(im.split()) > 3:
            # we don't have alpha on the image renderer, so eliminate that from the image
            rgb_image = Image.new("RGB", im.size, (255, 255, 255))
            rgb_image.paste(im, mask=im.split()[3])  # 3 is the alpha channel
        else:
            rgb_image = im

        palette_image = None
        if asset["bits"] in (1, 2, 4, 8):
            # 8, 4, 2, and 1 bit images: Create a palette.
            palette_image = rgb_image.convert("P", palette=Image.Palette.ADAPTIVE, colors=2 ** asset["bits"])

        if palette_image:
            array_len, image_bytes = bytes_for_palette_image(palette_image, asset["bits"])
            color_count, color_map_array = colormap_for_palette_image(palette_image.palette.colors)
        else:
            array_len, image_bytes = bytes_for_color_image(rgb_image, asset["bits"])

    image_source_name = f"{asset['name']}.c"
    struct_name = f"{image_yaml['name']}_{asset['name']}"

    with open(input_path.joinpath(image_source_name), 'w') as image_f:
        image_f.write(f"#include \"{asset_header_name}\"\n\n")

        image_f.write(f"static const unsigned char {struct_name}_data[{array_len}] = {{\n\n")
        image_f.write(image_bytes)
        image_f.write(f"\n\n}};\n\n")

        if palette_image:
            image_f.write(f"static const unsigned char {struct_name}_cmap[{color_count}][3] = {{\n")
            image_f.write(color_map_array)
            image_f.write(f"}};\n\n")

        image_f.write(f"const struct asset {struct_name} = {{\n")
        # asset ID not necessary
        image_f.write(f"    .type = PICTURE{asset['bits']}BIT,\n")
        image_f.write(f"    .seqNum = {asset['y_sprite_count']},\n")
        image_f.write(f"    .x = {rgb_image.size[0]},\n")
        image_f.write(f"    .y = {int(rgb_image.size[1] / asset['y_sprite_count'])},\n")
        if palette_image:
            image_f.write(f"    .data_cmap = (const char*) {struct_name}_cmap,\n")
        image_f.write(f"    .pixdata = (const char*) {struct_name}_data,\n")
        # datacb not necessary
        image_f.write(f"}};\n\n")

    asset_header.write(f"extern const struct asset {struct_name};\n")
    asset_cmakelists.write(f"${{CMAKE_CURRENT_LIST_DIR}}/{image_source_name}\n")

asset_header.write(f"""

#endif /* _{image_yaml["name"]}_h_ */

""")
asset_header.close()

asset_cmakelists.write("""
)

target_include_directories(badge2022_c PUBLIC .)

""")

asset_cmakelists.close()
