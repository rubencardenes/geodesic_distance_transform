
# 2D geodesic DT with occlusion points propagation
- Algorithm: Ruben Cardenes
- Implementation: Ruben Cardenes
- Paper: Occlusion Points Propagation Geodesict Distance Transformation, ICIP 2003

## 2D version:
```
geodesicDT2d dimX dimY source_points input_domain_image output_image
```
- dimX: integer 
- dimY: integer
- source_points: txt file with coordinates of source points
- input_domain_image: input image in raw data format: float 
- ouput_image: output image in raw data format: float 

Example for one point:
```
geodesicDT2d 256 256 mapfile.domain_sint domain_sint.flt outfileOP.flt
```

Example for several points (4 points)
```
geodesicDT2d 256 256 mapfile.domain_sint_4 domain_sint.flt outfileOP.flt
```

## 3D version 

Usage:
```
geodesicDT3d dimX dimY dimZ source_points input_domain_image output_image
```

- dimX: integer
- dimY: integer
- dimZ: integer
- source_points: txt file with coordinates of source points
- input_domain_image: input image in raw data format: float 
- ouput_image: output image in raw data format: float 

Example With opened domain (11 source points):
```
geodesicDT3d 80 80 80 mapfile.3d domain3d_80.vol mapa3d.vol
```

Example With closed domain (9 source points):
```
geodesicDT3d 80 80 80 mapfile.3dnew domain3d_new.vol mapa3dnew.vol
```

## Notes
The input domain should be always an image with the same size as the output 
in raw data format as float, and the domain region of interest are pixels equal to zero

