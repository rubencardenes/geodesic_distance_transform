from geodesicDT2d import run_geodesic2dDT_wrapper

debug = 0
color_mode = 0
run_geodesic2dDT_wrapper('example_data/source_2D_02.txt','example_data/domain256_1.png','output01_.png', color_mode, debug)
color_mode = 1
run_geodesic2dDT_wrapper('example_data/source_2D_03.txt','example_data/domain256_2.png','output02_.png', color_mode, debug)
