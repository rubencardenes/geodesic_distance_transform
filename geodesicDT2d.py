import ctypes

# Load your shared library (update the library name to your compiled library)
my_lib = ctypes.CDLL('geodesicDT2d')

# Function prototype in Python
# Assuming your C function returns void and takes the specified parameters
run_geodesic2dDT = my_lib.run_geodesic2dDT
run_geodesic2dDT.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int]
run_geodesic2dDT.restype = None

# Python function to call your C function
def run_geodesic2dDT_wrapper(domainfile, sourcefile, outputfile, color_mode, debug):
    run_geodesic2dDT(domainfile.encode(), sourcefile.encode(), outputfile.encode(), color_mode, debug)
