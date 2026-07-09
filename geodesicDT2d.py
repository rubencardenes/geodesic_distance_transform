import ctypes
import platform
import os

_LIB_EXT = {"Darwin": "dylib", "Windows": "dll"}.get(platform.system(), "so")
_LIB_PATH = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), f"libgeodesicDT2d.{_LIB_EXT}"
)

# Built by `make` (see Makefile target libgeodesicDT2d.$(LIBEXT)).
my_lib = ctypes.CDLL(_LIB_PATH)

# Function prototype in Python
# Assuming your C function returns void and takes the specified parameters
run_geodesic2dDT = my_lib.run_geodesic2dDT
run_geodesic2dDT.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int]
run_geodesic2dDT.restype = None

# Python function to call your C function.
# Parameter order matches the C signature: source points file, domain image,
# then output image (previously the first two parameters were swapped here,
# which silently mismapped inputs if this function was called with keyword
# arguments instead of positionally).
def run_geodesic2dDT_wrapper(sourcefile, domainfile, outputfile, color_mode, debug):
    run_geodesic2dDT(sourcefile.encode(), domainfile.encode(), outputfile.encode(), color_mode, debug)
