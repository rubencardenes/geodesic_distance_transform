/* Copyright (c) Ruben Cardenes Almeida 08/1/2010 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string>
#include <fstream>
#include <iostream>
#include <assert.h>
#include <vector>
#include "io_mhd.h"

int countFloatsInString(const char *fltString)
/* char *flts - character array of floats 
 Return -1 on a malformed string
 Return the count of the number of floating point numbers
 */
{
	char *end;
	const char *start = fltString;
	double d;
	int count = 0;
	while ((UCHAR(*start) != '\0') && isspace(UCHAR(*start))) { start++; }
	if (UCHAR(*start) == '\0') return -1; /* Don't ask to convert empty strings */
	do {
		d = strtod(start, (char **)&end);
		if (end == start) { 
			/* I want to parse strings of numbers with comments at the end */
			/* This return is executed when the next thing along can't be parsed */
			return count; 
		} 
		count++;   /* Count the number of floats */
		start = end;  /* Keep converting from the returned position. */
		while ((UCHAR(*start) != '\0') && isspace(UCHAR(*start))) { start++; }
	} while (UCHAR(*start) != '\0');
	return count; /* Success */
}

int countFloatsInString(std::string s) {
	int n = 0;
	int flag_primero = 0;
	
	// si el primer caracter es != de espacio cuento uno mas:
	if (s[0] != ' ') n=1;
	
	// Contamos los cambios entre espacio y no espacio
	for (int i=1;i<s.length();i++) {
		//cout << "Caracter: "<< s[i] << std::endl;
		if (i > 0) { 
	      if ((s[i] != ' ') && s[i-1] == ' ') {
			n++;
		  }
		} 
	}
	return n;
	//std::cout << "n: " << n << std::endl;
}

int getFloatString(int numFloats, std::string s, float *tgts) {
	std::string aux;
	int ni =0,ne=0;
	int pos_in[numFloats];
	int pos_end[numFloats];
	if (s[0] != ' ') {
		ni=1; pos_in[0] = 0;
	}
	for (int i=1;i<s.length();i++) {
		if (s[i] != ' ' && s[i-1] == ' ') {
			pos_in[ni] = i;ni++;
		}
		if (s[i] == ' ' && s[i-1] != ' ') {
			pos_end[ne] = i-1;ne++;	
		}
	}
	// si el ultimo caracter no era espacio:
	if (s[s.length()-1] != ' ') pos_end[ne] = s.length()-1; 
	
	for (int i=0;i<numFloats;i++) {
		//std::cout << " i " << i << " posics: " << pos_in[i] << " " << pos_end[i] << std::endl;
		int auxlength = pos_end[i]-pos_in[i]+1;
		aux.assign(s,pos_in[i],auxlength);
		//std::cout << ":"<< aux << ":" << std::endl;
	    tgts[i] = atof(aux.c_str());
	}
	return 0; /* Success */
}



int getFloatString(int numFloats, const char *flts, float *tgts)
/* char *flts - character array of floats */
/* float *tgts - array to save floats */
{
	char *end;
	const char *start = flts;
	double d;
	int count = 0;
	if (countFloatsInString(flts) != numFloats) return 1;
	while ((UCHAR(*start) != '\0') && isspace(UCHAR(*start))) { start++; }
	do {
		d = strtod(start, (char **)&end);
		if (end == start) { 
			/* Can't do any more conversions on this line */
			return (count == numFloats) ? 0 : 1;
		}
		tgts[count++] = d;   /* Convert from double to float */
		start = end;  /* Keep converting from the returned position. */
		while ((UCHAR(*start) != '\0') && isspace(UCHAR(*start))) { start++; }
		if (count == numFloats) return 0; /* I don't care if there are more on
		 the line as long as I got what I wanted */
	} while (UCHAR(*start) != '\0');
	return 0; /* Success */
}


void readmhd_header(char* filename, struct Header *cabecera) {
	FILE* fp;
	int n;
	std::string line,str_value;
	char *ptr1,*ptr2;
	std::ifstream file;
	
	std::string tag[13];
	tag[0] = "ObjectType";
	tag[1] = "NDims";
	tag[2] = "BinaryData";
	tag[3] = "BinaryDataByteOrderMSB";
	tag[4] = "CompressedData";
	tag[5] = "TransformMatrix";
	tag[6] = "Offset";
	tag[7] = "CenterOfRotation";
	tag[8] = "AnatomicalOrientation";
	tag[9] = "ElementSpacing";
	tag[10] = "DimSize";
	tag[11] = "ElementType";
	tag[12] = "ElementDataFile";
	
	file.open(filename);
	assert (!file.fail( ));     
	
	int i = 0;
	while ( !file.eof() ) {
		getline (file, line);
		int flag = 0;
		int pos = line.find("= ");
		//std::cout << "pos " << pos << std::endl;
		if (pos < 0) continue; 
		str_value.assign(line.begin()+pos+2,line.end());
		//std::cout << " Value " << str_value << "\n";
		switch (i) {
			case 0:
				   (*cabecera).ObjectType.assign(str_value);
					break;
			case 1: 
				   (*cabecera).NDims = atoi(str_value.c_str());
					break;
			case 2:
					(*cabecera).BinaryData.assign(str_value);
					break;
			case 3: 
					(*cabecera).BinaryDataOrder.assign(str_value);
					break;
			case 4:
					(*cabecera).Compression.assign(str_value);
					break;
			case 5: 
				    n = countFloatsInString(str_value);
				    float trans[9];
					getFloatString(n, str_value, trans);
					for (int j=0;j<n;j++) (*cabecera).TransformMatrix[j] = trans[j];
				    break;
			case 6: 
					n = countFloatsInString(str_value);
					float offset[3];
					getFloatString(n, str_value, offset);
					for (int j=0;j<n;j++) (*cabecera).Offset[j] = offset[j];
					break;
			case 7: 
					n = countFloatsInString(str_value);
					float center[3];
					getFloatString(n, str_value, center);
					for (int j=0;j<n;j++) (*cabecera).CenterOfRotation[j] = center[j];
					break;
			case 8:
					(*cabecera).AnatomicalOrientation.assign(str_value);
					break;
			case 9: 
					n = countFloatsInString(str_value);
					float spac[3];
					getFloatString(n, str_value, spac);
					for (int j=0;j<n;j++) (*cabecera).spacing[j] = spac[j];
					break;
			case 10:
				    n = countFloatsInString(str_value);
				    float dimsize[3];
					getFloatString(n, str_value, dimsize);
					for (int j=0;j<n;j++) (*cabecera).dimsize[j] = (int)dimsize[j];
					break;
			case 11:
					(*cabecera).ElementType.assign(str_value);
					break;
			case 12:
					(*cabecera).DataFile.assign(str_value);
					break;
				default:
				   break;
			}
		i++;
		//if (flag == 0) printf("Error, tag %s not present in header file: %s\n",ptr1,filename);
	}
}

std::string directorio(std::string s) {
	std::string dir;
	int pos = s.rfind("/");
	//std::cout << "pos " << pos << std::endl;
	if (pos >= 0) {
	  dir.assign(s,0,pos);
	} else {
	  dir = ".";
	}
	//std::cout << "Dir: " << dir << std::endl;
	return dir;
}

std::string filename_noextension(std::string s) {
	std::string dir;
	int pos_end = s.rfind(".");
	int pos_in = s.rfind("/");
	dir.assign(s,pos_in+1,pos_end-pos_in-1);
	//std::cout << "Name: " << dir << std::endl;
	return dir;
}


float* readraw_float(struct Header cabecera, std::string filename_mhd) {
	//std::cout << cabecera.dimsize[0] << " " <<  cabecera.dimsize[1] << " " << cabecera.dimsize[2] << " " << std::endl;
	FILE* fp;
	float* output;
	std::string filename_raw,dir;
	
	if (cabecera.ElementType.compare("MET_FLOAT") != 0 )  {
		std::cout << "Element Type not FLOAT"<< std::endl;
		return (float*)NULL;	
	}
	output = (float*)malloc((sizeof(float)*cabecera.dimsize[0]*cabecera.dimsize[1]*cabecera.dimsize[2]));
	dir = directorio(filename_mhd);
	filename_raw = dir + "/" + cabecera.DataFile;
	fp = fopen(filename_raw.c_str(),"r");
	if (fp != NULL) {
	  fread(output,cabecera.dimsize[0]*cabecera.dimsize[1]*cabecera.dimsize[2],sizeof(float),fp);
	  fclose(fp);
	} else {
		std::cout << "Failed to open " << filename_raw << std::endl;
	}
	return output;	
}

unsigned char* readraw_uchar(struct Header cabecera, std::string filename_mhd) {
	//std::cout << cabecera.dimsize[0] << " " <<  cabecera.dimsize[1] << " " << cabecera.dimsize[2] << " " << std::endl;
	FILE* fp;
	unsigned char* output;
	std::string filename_raw,dir;

	if (cabecera.ElementType.compare("MET_UCHAR") != 0 )  {
		std::cout << "Element Type not UCHAR"<< std::endl;
		return (unsigned char*)NULL;	
	}
	output = (unsigned char*)malloc((sizeof(unsigned char)*cabecera.dimsize[0]*cabecera.dimsize[1]*cabecera.dimsize[2]));
	dir = directorio(filename_mhd);
	filename_raw = dir + "/" + cabecera.DataFile;
	fp = fopen(filename_raw.c_str(),"r");
	if (fp != NULL) {
	  fread(output,cabecera.dimsize[0]*cabecera.dimsize[1]*cabecera.dimsize[2],sizeof(unsigned char),fp);
	  fclose(fp);
	} else {
		std::cout << "Failed to open " << filename_raw << std::endl;
	}
	return output;
}


void writemhd_header(std::string filename, struct Header *cabecera) {
	std::string DataFile;
	std::ofstream file;
	
	DataFile = filename_noextension(filename) + ".raw" ; 
    
	std::string tag[13];
	tag[0] = "ObjectType";
	tag[1] = "NDims";
	tag[2] = "BinaryData";
	tag[3] = "BinaryDataByteOrderMSB";
	tag[4] = "CompressedData";
	tag[5] = "TransformMatrix";
	tag[6] = "Offset";
	tag[7] = "CenterOfRotation";
	tag[8] = "AnatomicalOrientation";
	tag[9] = "ElementSpacing";
	tag[10] = "DimSize";
	tag[11] = "ElementType";
	tag[12] = "ElementDataFile";
	
	//std::cout << "Escibriendo archivo: " << filename << std::endl;
	file.open(filename.c_str(), std::ios::out);
	assert (!file.fail( ));
	file << tag[0] << " = " << (*cabecera).ObjectType << "\n";
	file << tag[1] << " = " << (*cabecera).NDims << "\n";
	file << tag[2] << " = " << (*cabecera).BinaryData << "\n";
	file << tag[3] << " = " <<  (*cabecera).BinaryDataOrder << "\n";
	file << tag[4] << " = " << (*cabecera).Compression << "\n";
	file << tag[5] << " = " << (*cabecera).TransformMatrix[0] << " " << (*cabecera).TransformMatrix[1] << " " << (*cabecera).TransformMatrix[2] << " " << (*cabecera).TransformMatrix[3] << " " << (*cabecera).TransformMatrix[4] << " " << (*cabecera).TransformMatrix[5] << " " << (*cabecera).TransformMatrix[6] << " " << (*cabecera).TransformMatrix[7] << " " << (*cabecera).TransformMatrix[8] << "\n";
	file << tag[6] << " = " << (*cabecera).Offset[0] << " " << (*cabecera).Offset[1] << " " << (*cabecera).Offset[2] << "\n";
	file << tag[7] << " = " << (*cabecera).CenterOfRotation[0] << " " << (*cabecera).CenterOfRotation[1] << " " << (*cabecera).CenterOfRotation[2] << "\n";
	file << tag[8] << " = " << (*cabecera).AnatomicalOrientation << "\n";
	file << tag[9] << " = " << (*cabecera).spacing[0] << " " << (*cabecera).spacing[1] << " " << (*cabecera).spacing[2] << "\n";
	file << tag[10] << " = " << (*cabecera).dimsize[0] <<  " " << (*cabecera).dimsize[1] <<  " " << (*cabecera).dimsize[2] << "\n";
	file << tag[11] << " = " << (*cabecera).ElementType << "\n";
	file << tag[12] << " = " << DataFile << "\n";
	
}

void writeraw_uchar(unsigned char* output,std::string filename_mhd, struct Header cabecera) {
	FILE* fp;
	std::string filename;
	
	filename = directorio(filename_mhd) + "/" + filename_noextension(filename_mhd) + ".raw"; 
	//std::cout << "Writing: " << filename << std::endl;
	fp = fopen(filename.c_str(),"w");
	if (fp == NULL) {
		std::cout << "Unable to open file " << filename << " for writing" << std::endl;	
	}
	fwrite(output,cabecera.dimsize[0]*cabecera.dimsize[1]*cabecera.dimsize[2],sizeof(unsigned char),fp);
	fclose(fp);
}

void writeraw_float(float* output,std::string filename_mhd, struct Header cabecera) {
	FILE* fp;
	std::string filename;
	
	filename = directorio(filename_mhd) + "/" + filename_noextension(filename_mhd) + ".raw"; 
	//std::cout << "Writing: " << filename << std::endl;
	fp = fopen(filename.c_str(),"w");
	if (fp == NULL) {
		std::cout << "Unable to open file " << filename << " for writing" << std::endl;	
	}
	fwrite(output,cabecera.dimsize[0]*cabecera.dimsize[1]*cabecera.dimsize[2],sizeof(float),fp);
	fclose(fp);
}
