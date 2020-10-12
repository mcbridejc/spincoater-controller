"""Convert .ppm format images to header files for inclusion
"""

import glob
import os

cpp_template = """
#include <modm/architecture/interface/accessor.hpp>

namespace images
{{
	FLASH_STORAGE(uint16_t {}[]) =
	{{
{}
	}};
}}
"""

hpp_template = """
#include <modm/architecture/interface/accessor.hpp>

namespace images
{{
	/**
	 * Generated from file "{}".
	 *
	 * width  : {}
	 * height : {}
	 */
	EXTERN_FLASH_STORAGE(uint16_t {}[]);
}}
"""


def read_ppm(fp):

    def whitespace_split_generator(fd):
        """Generator to yield all whitespace separated "words"
        """
        for line in fd:
            for token in line.split():
                if len(token) == 0:
                    continue
                else:
                    yield token

    with open(fp) as f:
        magic_number = f.readline().strip()
        if magic_number != "P3":
            raise RuntimeError(f"Unexpected file format. Expected a P3 magic number for ASCII PPM, got {magic_number}")
        f.readline() # Skip comment line
        numbers = whitespace_split_generator(f)
        width = int(next(numbers))
        height = int(next(numbers))
        _maxval = int(next(numbers))
        data = [int(x) for x in numbers]

        return width, height, data

def make_rgb(red, green, blue):
    return ((red >> 3) << 11) + ((green >> 2) << 5) + (blue >> 3)
 
def main():
    basedir = os.path.dirname(__file__)
    outdir = os.path.join(basedir, '../src/ui/images')

    ppm_files = glob.glob(os.path.join(basedir, "*.ppm"))

    for ppm in ppm_files:
        width, height, data = read_ppm(ppm)
        if len(data) % 3 != 0:
            raise RuntimeError(f"Expected multiple of 3 pixel values from {ppm}, found {len(data)}")
    
        image_name = os.path.splitext(os.path.basename(ppm))[0]

        dataiter = iter(data)
        byte_data = []
        for _ in range(height*width):
            r = next(dataiter)
            g = next(dataiter)
            b = next(dataiter)
            value = make_rgb(r, g, b)
            byte_data.append(format(value, "#06x"))

        with open(os.path.join(outdir, image_name+'.cpp'), 'w') as f:
            carray = " " * 8 + f"{width}, {height},\n"
            carray += " " * 8 + ",".join(byte_data)
            f.write(cpp_template.format(image_name, carray))
        
        with open(os.path.join(outdir, image_name+'.hpp'), 'w') as f:
            f.write(hpp_template.format(ppm, width, height, image_name))

if __name__ == '__main__':
    main()