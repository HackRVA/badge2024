# requirements: Pillow, PyYAML, bitstring
# Install using `pip install -r requirements.txt`
#
# Also, python3 is required. If you see this:
#
#   File "asset_converter.py", line 30
#   asset_header_name = f"{image_yaml['name']}.h"
#
# It means python is python2, and you want python3
# We can't check this in the script, because python2 fails to parse
# the script and doesn't execute any of it.
#
# Try running the script by "python3 asset_converter.py ..."
#

# generates:
# A .c file for each generated sprite
# An .h file with external declarations
# A CMakeLists.txt file

import argparse
from pathlib import Path

import bitstring
from PIL import Image
import yaml


def bytes_for_color_image(rgb_image: Image.Image, num_bits: int):
    """
    Returns an int representing the total number of bytes in `rgb_image`, and
    a string containing all the bytes laid out as a C array.
    """
    width, height = rgb_image.size
    image_bytes = b""
    for y in range(0, height):
        for x in range(0, width):
            pix = rgb_image.getpixel((x, y))
            assert isinstance(pix, list)
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


def bytes_for_palette_image(palette_image: Image.Image, num_bits: int):
    image_bitstr = bitstring.BitArray()
    assert isinstance(image_bitstr, bitstring.BitArray)
    width, height = palette_image.size
    # loop through every pixel, add a value to `image_bitstr` of length
    # `num_bits` bits.
    for y in range(0, height):
        for x in range(0, width):
            pix: int = palette_image.getpixel((x, y))
            image_bitstr.append(bitstring.Bits(uint=pix, length=num_bits))

        # pad bit arry to be a multiple of 8
        padding = 8 - (len(image_bitstr) % 8)
        if padding == 8:
            padding = 0
        image_bitstr += bitstring.Bits([0] * padding)

    # only 1 bit images have bit order reversed in byte
    if num_bits == 1:
        corrected_image_bitstr = bitstring.BitArray()
        for bitstr_byte in [
            image_bitstr[i : i + 8] for i in range(0, len(image_bitstr), 8)  # noqa
        ]:
            bitstr_byte.reverse()
            corrected_image_bitstr += bitstr_byte
        image_bitstr = corrected_image_bitstr

    # convert bitstring back to bytes, and create the code for a C array
    # containing those bytes
    image_bitstr_bytes = image_bitstr.bytes
    c_array = ", ".join([hex(byte) for byte in image_bitstr_bytes])
    return len(image_bitstr_bytes), c_array


def colormap_for_palette_image(colormap):
    """
    Returns the number of colors in the colormap (+1), and a string with the
    C representation of an array of those colors (plus black)
    """

    c_array = ""
    inverted_map = {v: k for k, v in colormap.items()}
    for i in range(0, len(inverted_map) - 1):
        color_tuple = inverted_map[i]
        c_array += f"    {{{color_tuple[0]}, {color_tuple[1]}, {color_tuple[2]}}},"
        c_array += "\n"
    # add background color (always black, for now)
    c_array += "    {0, 0, 0}\n"
    return len(inverted_map), c_array


def get_struct_name(project_name: str, asset_name: str) -> str:
    """
    Returns a suitable name for the struct to represent the asset, replacing any invalid
    characters
    """
    return f"{project_name}_{asset_name}".replace(" ", "_").replace("-", "_")


def get_asset_header_name(config: dict) -> str:
    """
    Given a configuration as loaded from the file images.yaml, returns the file
    name for the assets header (.h) file.
    """
    return f"{config['name']}.h".replace(" ", "_")


def get_c_file_name(asset: dict[str, str | int]) -> str:
    """
    Given an asset entry, returns the name of the C file to be generated for that asset.
    """
    return f"{asset['name']}.c".replace(" ", "_").replace("-", "_")


def write_asset_header(config: dict[str, str | list[dict]], input_path: Path) -> str:
    """
    Uses the configuration from images.yaml to create an asset header file listing
    all the asset structures in the asset source files.

    returns:
        the name of the header file
    """
    asset_header_name = get_asset_header_name(config)
    print(f'Writing to asset header file "{asset_header_name}"')

    with open(input_path.joinpath(asset_header_name), "w") as asset_header:
        asset_header.write(
            f"""// ASSET HEADER AUTOMATICALLY GENERATED

        // Included for struct asset
        #include "assetList.h"

        #ifndef _{config["name"]}_h_
        #define _{config["name"]}_h_

        """.replace(
                "\n        ", "\n"
            )  # remove leading spaces, probably there's a better way
        )

        assert isinstance(config["images"], list)
        # TODO: sort lines alphabetically
        for asset in config["images"]:
            assert isinstance(config["name"], str)  # make static type checker happy
            struct_name = get_struct_name(config["name"], asset["name"])
            asset_header.write(f"extern const struct asset {struct_name};\n")

        asset_header.write(
            f"""
        #endif /* _{config["name"]}_h_ */
        """.replace(
                "\n        ", "\n"
            )  # remove leading spaces, probably there's a better way
        )
    return asset_header_name


def write_cmakelists(config: dict[str, str | list[dict]], input_path: Path) -> None:
    """
    Uses configuration from images.yaml file to build a CMakeLists.txt file for compiling
    assets into the application executable.
    """
    cmake_path = input_path.joinpath("CMakeLists.txt")
    print(f"Writing cmake directives to {cmake_path}")
    with open(cmake_path, "w") as asset_cmakelists:
        asset_cmakelists.write(
            """# THIS CMakeLists.txt AUTOMATICALLY GENERATED

        target_sources(
            badge2023_c PUBLIC

        """.replace(
                "\n        ", "\n"
            )  # remove leading spaces, probably there's a better way
        )

        assert isinstance(config["images"], list)  # assist static type checker
        for asset in config["images"]:
            image_source_name = get_c_file_name(asset)
            asset_cmakelists.write(
                f"    ${{CMAKE_CURRENT_LIST_DIR}}/{image_source_name}\n"
            )

        asset_cmakelists.write(
            """)

        target_include_directories(badge2023_c PUBLIC .)
        """.replace(
                "\n        ", "\n"
            )  # remove leading spaces, probably there's a better way
        )


def write_asset_c_file(
    config: dict[str, str | list[dict]],
    asset: dict,
    input_path: Path,
    asset_header_name: str,
):
    """
    Writes a C file containing the structures necessary to compile the asset
    into the program executable.

    Params:
        config: the configuration as read from the images.yaml file
        asset: A dictionary representing a single image in the config["images"] list
            asset["name"]: The full name of the asset
            asset["shortname"]: A shorter version of the name used in menus. defaults to the
                value of asset["name"]
            asset["y_sprite_count"]: number of separate sprites in the image, default 1
            asset["filename"]: the name of the image .png file
            asset["bits"]: The number of bits to use per pixel
    """
    color_count = 0
    color_map_array = ""
    if "y_sprite_count" not in asset:
        asset["y_sprite_count"] = 1

    image_path = input_path.joinpath(asset["filename"])

    with Image.open(image_path) as im:
        if len(im.split()) > 3:
            # we don't have alpha on the image renderer, so eliminate that
            # from the image
            rgb_image = Image.new("RGB", im.size, (255, 255, 255))
            rgb_image.paste(im, mask=im.split()[3])  # 3 is the alpha channel
        else:
            rgb_image = im

        palette_image = None
        if asset["bits"] in (1, 2, 4, 8):
            # 8, 4, 2, and 1 bit images: Create a palette.
            palette_image = rgb_image.convert(
                "P", palette=Image.Palette.ADAPTIVE, colors=2 ** asset["bits"]
            )

        if palette_image:
            array_len, image_bytes = bytes_for_palette_image(
                palette_image, asset["bits"]
            )
            color_count, color_map_array = colormap_for_palette_image(
                palette_image.palette.colors
            )
        else:
            array_len, image_bytes = bytes_for_color_image(rgb_image, asset["bits"])

    image_source_name = get_c_file_name(asset)
    print(f'Writing image C source to "{image_source_name}"')

    with open(input_path.joinpath(image_source_name), "w") as image_f:
        image_f.write("#include <stddef.h>\n")  # for NULL
        image_f.write('#include "assetList.h"\n')
        assert isinstance(config["name"], str)
        struct_name = get_struct_name(config["name"], asset["name"])
        image_f.write(f'#include "{asset_header_name}"\n\n')

        image_f.write(
            f"static const unsigned char {struct_name}_data[{array_len}] = {{\n\n"
        )
        image_f.write(image_bytes)
        image_f.write("\n\n};\n\n")

        if palette_image:
            image_f.write(
                f"static const unsigned char {struct_name}_cmap[{color_count}][3] = {{\n"
            )
            image_f.write(color_map_array)
            image_f.write("};\n\n")

        image_f.write(f"const struct asset {struct_name} = {{\n")
        # assetId gets set later by user of const struct
        image_f.write("    .assetId = 0,\n")
        image_f.write(f"    .type = PICTURE{asset['bits']}BIT,\n")
        image_f.write(f"    .seqNum = {asset['y_sprite_count']},\n")
        image_f.write(f"    .x = {rgb_image.size[0]},\n")
        # '//' performs integer division, otherwise .y might be a floating-point #
        image_f.write(
            f"    .y = {int(rgb_image.size[1] // asset['y_sprite_count'])},\n"
        )
        if palette_image:
            image_f.write(f"    .data_cmap = (const char*) {struct_name}_cmap,\n")
        else:
            image_f.write("    .data_cmap = NULL,\n")
        image_f.write(f"    .pixdata = (const char*) {struct_name}_data,\n")
        # cbdata isn't valid member of struct asset? never used anyway.
        # image_f.write("    .cbdata = NULL\n")
        image_f.write("};\n\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("folder", help="folder containing images and images.yaml")

    args = parser.parse_args()
    input_path = Path(args.folder)

    with open(input_path.joinpath("images.yaml"), "r") as f:
        image_yaml: dict[str, str | list[dict]] = yaml.safe_load(f)

    asset_header_name = write_asset_header(image_yaml, input_path)
    write_cmakelists(image_yaml, input_path)

    assert isinstance(image_yaml["images"], list)
    for asset in image_yaml["images"]:
        write_asset_c_file(image_yaml, asset, input_path, asset_header_name)


if __name__ == "__main__":
    main()
