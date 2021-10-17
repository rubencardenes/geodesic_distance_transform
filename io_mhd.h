/* Copyright (c) Ruben Cardenes Almeida 08/1/2010 */
#include <string>

#ifndef UCHAR
#define UCHAR(c) ((unsigned char)(c))
#endif

struct Header {
	std::string ObjectType;
	int NDims;
	std::string BinaryData;
	std::string BinaryDataOrder;
	std::string Compression;
	float TransformMatrix[9];
	float Offset[3];
	float CenterOfRotation[3];
	std::string AnatomicalOrientation;
	float spacing[3];
	int dimsize[3];
	std::string ElementType;
	std::string DataFile;
	
};

std::string directorio(std::string s);
void readmhd_header(char* filename, struct Header *cabecera);
float* readraw_float(struct Header cabecera, std::string filename_mhd);
unsigned char* readraw_uchar(struct Header cabecera, std::string filename_mhd);
void writemhd_header(std::string filename, struct Header *cabecera);
void writeraw_uchar(unsigned char* output,std::string filename_mhd, struct Header cabecera);
void writeraw_float(float* output,std::string filename_mhd, struct Header cabecera);
int getFloatString(int numFloats, const char *flts, float *tgts);
int countFloatsInString(const char *fltString);


