/* Copyright (c) Ruben Cardenes Almeida 27/08/2002 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <getopt.h>
#include <ctype.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

#define MAX_ELEM_IN_BUCKET 16000
#define NUM_BUCKETS 400
#define MAXPATTERNS (16384*4)
#define MAXCLASSNUMBER MAXPATTERNS
#define MAXDIM 20
#ifndef UCHAR
#define UCHAR(c) ((unsigned char)(c))
#endif

#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

struct element {
  int x;
  int y;
  float dobj;
  int xobj;
  int yobj;
  int icur;
};

struct bucket {
  int num_elem;
  int *index_elem;
  int *index_l;
};

struct nodeDataNew {
  int id;
  int pclass;
  float d[MAXDIM];
  int row;
  int col;
  int slice;
};

struct nodeDataNew prototypeNodeData[MAXPATTERNS];

int numelembucket[NUM_BUCKETS];
int numrechazos = 0;
int numasignaciones = 0;
int asignacionesraras = 0;
int numPrototypes;
int highestIndexClass;
int actualDimension; 
int numPrototypesInClass[MAXCLASSNUMBER];
char buffer[2048];
int pdim;

unsigned char* aux_out;

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

int mapIndex3D(int r,int c,int z, int nr,int nc,int nz)
{
  if (c >= nc) return -1;
  if (c < 0) return -1;
  if (r >= nr) return -1;
  if (r < 0) return -1;
  if (z >= nz) return -1;
  if (z < 0) return -1;
  return c + r * nc + z * nr * nc;
}

int mapIndex2D(int r,int c, int nr,int nc)
{
  if (c >= nc) return -1;
  if (c < 0) return -1;
  if (r >= nr) return -1;
  if (r < 0) return -1;
  return c + r * nc;
}


void read_lut(char* filename, unsigned char** lut) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr,"Failed reading file %s\n",filename);
        exit(1);
    }
    int index, r, g, b;
    printf("reading lut\n");
    fgets(buffer, sizeof(buffer),fp);
    while (!feof(fp)) {
      memset(buffer,0,sizeof(buffer));
      if (fgets(buffer, sizeof(buffer),fp) != (char *)NULL) {
        // printf("buffer %s\n", buffer);
        if (sscanf(buffer,"%d\t%d\t%d\t%d", &index, &r, &g, &b) == 0) {
            continue;
        }
      }
      // printf("index %d RGB %d %d %d\n",index, r,g,b);  
      lut[0][index] = r;
      lut[1][index] = g;
      lut[2][index] = b;
    }
    // printf("finish reading lut\n");
}

void get_redblue_lut(unsigned char** lut) {
    for (int i=0;i<256;i++) {
        lut[0][i] = 255 - i;
        lut[1][i] = 0;
        lut[2][i] = i;
    }
}

void get_random_lut(unsigned char** lut) {
    for (int i=0;i<256;i++) {
        lut[0][i] = (int)255*rand();
        lut[1][i] = (int)255*rand();
        lut[2][i] = (int)255*rand();
    }
}

int readNodeData(float* des,FILE *fp) {
  memset(buffer,0,sizeof(buffer));
  if (fgets(buffer, sizeof(buffer),fp) != (char *)NULL) {
    pdim = countFloatsInString(buffer);
    assert(pdim <= MAXDIM);
    if (pdim <= 0) return -1;
    if (getFloatString(pdim,buffer,des) != 0) {
      fprintf(stderr,"Failed to parse float string\n");
      return -1; /* failure */
    }
  } else {
    printf("Failed to read pattern\n");
    return -1; /* failure */
  }

  return pdim; /* success */
}


int initializeFromTrainingDataEspacial(FILE *td, int numberDesired, int numberExpected, int nrow, int ncol, int nslice)
{
  int pclass = -1;
  float prob = 0.0;
  int i,index;
  int* marker = (int*)malloc(sizeof(int)*nrow*ncol*nslice);
  char *straux,*end;
  memset(marker,0,sizeof(int)*nrow*ncol*nslice);

  if (numberExpected > 0) {
    prob = (float)numberDesired / (float)numberExpected;
  } else {
    prob = 1.0;
  }

  /* Seed the random number generator */
  srand(time((time_t *)NULL));

  /* Assume the data is in the format:
   * class number
   * value value value   ... value
   */
  numPrototypes = 0;
  while (numberExpected ? (numPrototypes < numberDesired) : !feof(td) ) {
    if (!feof(td)) {
      memset(buffer,0,sizeof(buffer));
      if (fgets(buffer, sizeof(buffer),td) != (char *)NULL) {
        if (sscanf(buffer,"%d", &pclass) != 1) {
          /* Skip blank lines */
          continue;
        }
        assert (pclass >= 0 && pclass < MAXCLASSNUMBER);
        memset(buffer,0,sizeof(buffer));
        assert(numPrototypes <= MAXPATTERNS);        
	/* Anadido para obtener la informacion espacial de los prototipos de entrenamiento */
	
	if (fgets(buffer, sizeof(buffer),td) != (char *)NULL) {
	  straux = strstr(buffer,"row-");
	  prototypeNodeData[numPrototypes].row = (int)strtod(straux+4,(char**)&end);
	
	  straux = strstr(buffer,"col-");
	  prototypeNodeData[numPrototypes].col = (int)strtod(straux+4,(char**)&end);
	
	  straux = strstr(buffer,"sliceindex-");
	  prototypeNodeData[numPrototypes].slice = (int)strtod(straux+11,(char**)&end);
	}

        prototypeNodeData[numPrototypes].pclass = pclass;
        
        if (pclass > highestIndexClass) {
          highestIndexClass = pclass;
        }

	/* check if we already have this prototype stored */
        if (nslice > 1 && ncol >1) {
	index = mapIndex3D((int)prototypeNodeData[numPrototypes].row,
			   (int)prototypeNodeData[numPrototypes].col,
			   (int)prototypeNodeData[numPrototypes].slice,nrow,ncol,nslice);
	}
	if (nslice == 1 && ncol >1) {
	index = mapIndex2D((int)prototypeNodeData[numPrototypes].row,
			   (int)prototypeNodeData[numPrototypes].col,
			   nrow,ncol);
	}
	if (nslice == 1 && ncol == 1) {
	  index = (int)prototypeNodeData[numPrototypes].row;
	}
	if (index == -1) {
	  printf("Error, out of bound while reading training prototypes\n");
	  printf("%d %d %d\n",(int)prototypeNodeData[numPrototypes].row,(int)prototypeNodeData[numPrototypes].col,(int)prototypeNodeData[numPrototypes].slice);
	}

	if (marker[index] == 0) {
	  marker[index] = 1;
	  /* Now decide randomly whether to use this prototype or not */
	  if ( (fabs(prob - 1.0) < 0.0001) || (rand() > (RAND_MAX) * prob)  ) {
	    numPrototypes++;
	    numPrototypesInClass[pclass] += 1;
	    prototypeNodeData[numPrototypes].id = numPrototypes; 
	    if (numPrototypes == numberDesired) break; 

	  }
	}
	
      }
    } else {
      rewind(td);
    }
  }
  printf("Initialization finished - numPrototypes is %d\n",numPrototypes);

  /* codigo  de control para ver cuantos prototipos tenemos en cada clase*/
  /* for (i=0;i<highestIndexClass+1;i++) {
     if (numPrototypesInClass[pclass] != 0) { 
     printf("Num prototypes in clas %d, is %d\n",i,numPrototypesInClass[i]);
     }
    } */
  /*  codigo  de control */

  free(marker);
  marker = (int*)NULL;
  return 0; /* Success */
}


int initializeFromTrainingDataKDT(FILE *td, int numberDesired, int numberExpected, int nrow, int ncol, int nslice)
{
  int pclass = -1;
  float prob = 0.0;
  int i,index;
  int* marker = (int*)malloc(sizeof(int)*nrow*ncol*nslice);
  char *straux,*end;
  memset(marker,0,sizeof(int)*nrow*ncol*nslice);

  if (numberExpected > 0) {
    prob = (float)numberDesired / (float)numberExpected;
  } else {
    prob = 1.0;
  }

  /* Seed the random number generator */
  srand(time((time_t *)NULL));

  /* Assume the data is in the format:
   * class number
   * value value value   ... value
   */
  numPrototypes = 0;
  while (numberExpected ? (numPrototypes < numberDesired) : !feof(td) ) {
    if (!feof(td)) {
      memset(buffer,0,sizeof(buffer));
      if (fgets(buffer, sizeof(buffer),td) != (char *)NULL) {
        if (sscanf(buffer,"%d", &pclass) != 1) {
          /* Skip blank lines */
          continue;
        }
        assert (pclass >= 0 && pclass < MAXCLASSNUMBER);
        memset(buffer,0,sizeof(buffer));
        assert(numPrototypes <= MAXPATTERNS);
        actualDimension = readNodeData(prototypeNodeData[numPrototypes].d, td);

        if (pdim == -1) {
          fprintf(stderr,"Failed reading pattern %d for pclass %d\n",numPrototypes,pclass);
        }
        prototypeNodeData[numPrototypes].pclass = pclass;
        
        if (pclass > highestIndexClass) {
          highestIndexClass = pclass;
        }

	/* check if we already have this prototype stored */
        if (actualDimension == 3) {
	index = mapIndex3D((int)prototypeNodeData[numPrototypes].d[0],
			   (int)prototypeNodeData[numPrototypes].d[1],
			   (int)prototypeNodeData[numPrototypes].d[2],nrow,ncol,nslice);
	}
	if (actualDimension == 2) {
	index = mapIndex2D((int)prototypeNodeData[numPrototypes].d[0],
			   (int)prototypeNodeData[numPrototypes].d[1],
			   nrow,ncol);
	}
	if (actualDimension == 1) {
	  index = (int)prototypeNodeData[numPrototypes].d[0];
	}
	if (index == -1) {
	  printf("Error, out of bound while reading training prototypes\n");
	  printf("%d %d %d\n",(int)prototypeNodeData[numPrototypes].d[0],(int)prototypeNodeData[numPrototypes].d[1],(int)prototypeNodeData[numPrototypes].d[2]);
	}

	if (marker[index] == 0) {
	  marker[index] = 1;
	  /* Now decide randomly whether to use this prototype or not */
	  if ( (fabs(prob - 1.0) < 0.0001) || (rand() > (RAND_MAX) * prob)  ) {
	    numPrototypes++;
	    numPrototypesInClass[pclass] += 1;
	    prototypeNodeData[numPrototypes].id = numPrototypes; 
	    if (numPrototypes == numberDesired) break; 

	  }
	}
	
      }
    } else {
      rewind(td);
    }
  }
  printf("Initialization finished - numPrototypes is %d\n",numPrototypes);

  /* codigo  de control para ver cuantos prototipos tenemos en cada clase*/
  /* for (i=0;i<highestIndexClass+1;i++) {
     if (numPrototypesInClass[pclass] != 0) { 
     printf("Num prototypes in clas %d, is %d\n",i,numPrototypesInClass[i]);
     }
    } */
  /*  codigo  de control */

  free(marker);
  marker = (int*)NULL;
  return 0; /* Success */
}

void print_timing(FILE *fp, struct timeval start, struct timeval end) 
{
  double tuend = 1e-06*(double)end.tv_usec; \
  double tustart = 1e-06*(double)start.tv_usec; \
  double tend = end.tv_sec + tuend;\
  double tstart = start.tv_sec + tustart;\
  fprintf(fp,"Elapsed time: %g\n", (tend - tstart) ); \
}

float distance(int x1,int y1,int x2,int y2) {
  return sqrt ((x1-x2) * (x1-x2) + (y1-y2) * (y1-y2));
}

int propagar8(int mapindex, int max1, int max2, struct bucket *Lista, float* domain, 
              int dcur,struct element *Element,float* maps, int *numocclusionpoints, int debug) {
  float distreal;
  int new_mapindex,dist,x,y,siguiente,siguiente_obj;
  int ynew,xnew,i;

  for (x=-1;x<2;x++) {
    for (y=-1;y<2;y++) {
      if (x==0 && y==0) continue;
      ynew = Element[mapindex].y + y;
      xnew = Element[mapindex].x + x;
      if (ynew >=0 && xnew >=0
	        && ynew < max1 && xnew < max2 ) {
	      new_mapindex = ynew*max2 + xnew;
	      if (domain[new_mapindex] == 0 && maps[new_mapindex] == -1) {	 
          distreal = distance(xnew,ynew,Element[mapindex].xobj,Element[mapindex].yobj) + Element[mapindex].dobj;
          dist = round(distreal);		
          if (dist == dcur+1) {
            /*printf("xnew %d ynew %d xobj %d yobj %d dist %d distreal %f\n",xnew,ynew,Element_p.xobj,Element_p.yobj,dist,distreal);*/
            siguiente = Lista[1].num_elem;
	        if (siguiente >= MAX_ELEM_IN_BUCKET) {
		        printf("Excedidos num elem in bucket \n");
		        return 1;
	        }
          Element[new_mapindex].xobj = Element[mapindex].xobj;
          Element[new_mapindex].yobj = Element[mapindex].yobj;
          Element[new_mapindex].dobj = Element[mapindex].dobj;
          Lista[1].index_elem[siguiente] = new_mapindex ;	      
          Lista[1].num_elem++;
          /* maps[new_mapindex] = dist; */
          maps[new_mapindex] = distreal;
        }	   
        if (dist <= dcur) {
          // printf("cambio de objeto en x %d y %d, xnew %d ynew %d, dist %d, distreal %f Element[mapindex]: dobj %f xobj %d yobj %d\n",Element[mapindex].x,Element[mapindex].y,xnew,ynew,dist,distreal,Element[mapindex].dobj,Element[mapindex].xobj,Element[mapindex].yobj);
          (*numocclusionpoints)++;
          siguiente = Lista[2].num_elem;
          if (siguiente >= MAX_ELEM_IN_BUCKET) {
              printf("Excedidos num elem in bucket \n");
              return 1;
          }
          Element[new_mapindex].xobj = Element[mapindex].x;
          Element[new_mapindex].yobj = Element[mapindex].y;
          Element[new_mapindex].dobj = distreal;
          Element[mapindex].dobj = distreal; 
          Lista[2].index_elem[siguiente] = new_mapindex;
          Lista[2].num_elem++;
          /*maps[new_mapindex] = round(distreal + distance(xnew,ynew,Element[mapindex].x,Element[mapindex].y)); */
          maps[new_mapindex] = distreal + distance(xnew,ynew,Element[mapindex].x,Element[mapindex].y);
          /* codigo control */
          if (debug == 1) {
            aux_out[Element[mapindex].y*max1+Element[mapindex].x] = 255;
          }
          /* codigo control */

	        }
	      }
      } 
    }
  }
  
  return 0;
}

void volcar(struct bucket *lista_origen, struct bucket *lista_destino) {
  int i,siguiente;
  for (i=0;i<lista_origen->num_elem;i++) {
    siguiente = lista_destino->num_elem;
    lista_destino->index_elem[siguiente] = lista_origen->index_elem[i];
    lista_origen->index_elem[i] = -1;
    lista_destino->num_elem++;
  }
  lista_origen->num_elem=0;
}

void swap(struct bucket *lista1, struct bucket *lista2) {
  int* index_aux;
  int aux;
  index_aux = lista1->index_elem;
  lista1->index_elem = lista2->index_elem;
  lista2->index_elem = index_aux;

  aux = lista1->num_elem;
  lista1->num_elem = lista2->num_elem;
  lista2->num_elem = aux;
}

float* geodesicDT(char* prototypes,int max1, int max2, float* maps, float* domain, float* dmax, int debug) {
  int i,j,x,y,l,N,r,count;
  int siguiente,indice_actual;
  int d,mapindex,buckets_empty;
  struct element *Element;
  struct element *Proto;
  struct bucket Lista[3];
  char fichero[100];
  float** im;
  float distancia_from_l,distancia_ultima;
  FILE *fp,*fpdist;
  int numocclusionpoints = 0;

  /* inicializamos el mapa de distancias a -1 */ 
  for (j=0;j<max1*max2;j++) {
    maps[j] = -1;
  }

  Element=(struct element*)malloc(sizeof(struct element)*max1*max2);
  Proto=(struct element*)malloc(sizeof(struct element)*max1*max2);
  Lista[0].index_elem = (int*)malloc(sizeof(int)*MAX_ELEM_IN_BUCKET);
  memset(Lista[0].index_elem,0,sizeof(int)*MAX_ELEM_IN_BUCKET);
  Lista[0].num_elem = 0;

  i=0;
  for (y=0;y<max1;y++) {
    for (x=0;x<max2;x++) {
      Element[i].dobj = 0;
      Element[i].x = x;
      Element[i].y = y;
      i++;
    }
  }
  l = 0;
  count = 0;
  for (y=0;y<max1;y++) {
    for (x=0;x<max2;x++) {
      if (prototypes[count] != -1 && domain[count] == 0) {
	Proto[l].x= x;
	Proto[l].y= y;
  Proto[l].icur = prototypes[count];
	Element[count].dobj = 0;
	Element[count].xobj = x;
	Element[count].yobj = y;
	maps[count] = 0;
	Lista[0].index_elem[l] = count;
	Lista[0].num_elem++;
	l++;
      }
      count++;
    }
  }

  printf("hay %d prototipos \n",l);
  Lista[1].num_elem = 0;
  Lista[1].index_elem = (int*)malloc(sizeof(int)*MAX_ELEM_IN_BUCKET);
  
  Lista[2].num_elem = 0;
  Lista[2].index_elem = (int*)malloc(sizeof(int)*MAX_ELEM_IN_BUCKET);
  /* Fin de inicializacion */

  d=0;
  while (1) {
    /* printf("num elem a propagar para d = %d, es %d\n",d,Lista[0].num_elem); */
    /* printf("Num Lista[2] %d, Num Lista[0] %d\n",Lista[2].num_elem,Lista[0].num_elem);*/   
    while (Lista[0].num_elem != 0) {
      /*printf("Distancia actual: %d\n",d);*/
      /* Sacamos elemento de lista 0 */
      /* printf("Obteniendo de bucket: %d\n",d); */
      siguiente = Lista[0].num_elem-1;
      /* printf("num_elem = %d \n",Lista[0].num_elem);*/
      indice_actual = Lista[0].index_elem[siguiente];
      Lista[0].index_elem[siguiente] = -1;
      Lista[0].num_elem--;
      mapindex = Element[indice_actual].y*max2+Element[indice_actual].x;
      if (mapindex != indice_actual) {	
        printf("Error extrano mapindex %d != indice_actual %d\n",mapindex,indice_actual);
        return (float*)NULL;
      }
      // printf("propagando mapindex %d x %d y %d \n",mapindex,Element[mapindex].x,Element[mapindex].y);
      propagar8(mapindex, max1, max2, Lista, domain, d, Element, maps, &numocclusionpoints, debug);   
    }
    d++;
    swap(&Lista[0],&Lista[1]);
    if (Lista[2].num_elem >0) {
      /* printf("volcando Lista2 (%d elems) en lista0 (%d elems)\n",Lista[2].num_elem,Lista[0].num_elem); */
      volcar(&Lista[2],&Lista[0]);
    }
    if (Lista[0].num_elem == 0 && Lista[1].num_elem == 0) break;
    // if (d == 40) break;
  }
    
  printf("Distancia maxima alcanzada: %d\n",d); 
  printf("numocclusionpoints = %d\n",numocclusionpoints);
  *dmax = d;
  /* liberamos memoria */
  free(Proto);
  printf("geodesicDT OK\n");
  return maps;
}

void run_geodesic2dDT(char* sourcefile, char* domainfile, char* outputfile, int color_mode, int debug) {
  char *proto;
  float *maps,*domain;
  int i,col,row,tipo_mapa,num_mapa;
  int width = 256;
  int height = 256;
  int depth = 1;
  int channels = 1;
  int K=1;
  struct timeval startinit;
  struct timeval endinit;
  struct timeval endtotal;
  float max_dist = 0;
  FILE *fp,*fg;

  printf("Reading domain %s \n",domainfile);
  unsigned char *domain_u = stbi_load(domainfile, &width, &height, &channels, 0);
  if(domain_u == NULL) {
        printf("Error in loading the image\n");
        exit(1);
  }
  printf("Domain width %d height %d\n",width, height);

  // Convert to float 
  domain = (float*)malloc(sizeof(float)*width*height);
  for (int i=0;i<width*height;i++) {
     domain[i] = (float)domain_u[i];
  }

  gettimeofday(&startinit,NULL);
  proto = (char*)malloc(sizeof(char)*width*height);

  for (i=0;i<width*height;i++) {
    proto[i] = -1;
  }

  fp = fopen(sourcefile,"rb");
  if (fp == NULL) {
    fprintf(stderr,"Failed reading prototypes %s\n",sourcefile);
    exit(1);
  }

  printf("Reading source points \n");
  initializeFromTrainingDataEspacial(fp,100000000,0,width,height,1);
  for (i=0;i<numPrototypes;i++) {   
    row=(int)prototypeNodeData[i].row; // x 
    col=(int)prototypeNodeData[i].col; // y    
    if (prototypeNodeData[i].pclass != 0) {
      proto[width*row+col] = (char)prototypeNodeData[i].pclass;
    }
  }
  fclose(fp);

  maps = (float*)malloc(sizeof(float)*width*height);
  /* codigo control */
  if (debug == 1) {
    aux_out = (unsigned char*)calloc(width*height,sizeof(unsigned char));
  } 
  /* codigo control */

  numasignaciones = 0;
  printf("Doing geodesicDT\n");
  gettimeofday(&endinit,NULL);
  maps = geodesicDT(proto, width, height, maps, domain, &max_dist, debug);
  if (maps == (float*)NULL) {
    printf("Error in geodesicDT\n");
  }
  gettimeofday(&endtotal,NULL);

  printf("Writing distance map\n");
  unsigned char* maps_u;
  if (color_mode > 0) {
    unsigned char **lut = (unsigned char**)malloc(sizeof(unsigned char*)*3);
    for (int i=0;i<3;i++) {
       lut[i] = (unsigned char*)malloc(sizeof(unsigned char)*256);    
    }
    if (color_mode == 1) {
      get_redblue_lut(lut);
    }
    if (color_mode == 2) {
      get_random_lut(lut);
    }
    if (color_mode == 3) {
      read_lut("cb_bluishgreen.lut", lut);
    } 
    channels = 3;    
    maps_u = (unsigned char*)malloc(sizeof(unsigned char)*width*height*channels); 
    int j = 0;
    for (int i=0;i<width*height;i++) {
       if (domain[i] == 0) {
         unsigned char value = (unsigned char)(255*maps[i]/max_dist);
         //unsigned char value = (unsigned char)((int)maps[i]%255);       
         maps_u[j] = (unsigned char)lut[0][value];
         maps_u[j+1] = (unsigned char)lut[1][value];
         maps_u[j+2] = (unsigned char)lut[2][value];
       } else {
         maps_u[j] = 0;
         maps_u[j+1] = 0;
         maps_u[j+2] = 0;
       }
       j = j+3;
    }
  } else {
    maps_u = (unsigned char*)calloc(width*height, sizeof(unsigned char));
    for (int i=0;i<width*height;i++) {
       maps_u[i] = (unsigned char)(255*maps[i]/max_dist);
    }
  }
  stbi_write_png(outputfile, width, height, channels, maps_u, width * channels);
  
  free(maps);
  free(maps_u);
  free(domain);
  free(domain_u);

  fprintf(stdout,"Initialization time: ");
  print_timing(stdout, startinit, endinit);
  fprintf(stdout,"geodesic DT time: ");
  print_timing(stdout, endinit, endtotal);
  /* printf("numasignaciones = %d, asignacionesraras =%d, numrechazos =%d\n", numasignaciones,asignacionesraras,numrechazos); */

  /* codigo control */
  if (debug == 1) {
    stbi_write_png("occlusion_points.png", width, height, 1, aux_out, width * 1);
    free(aux_out);
  }
  /* codigo control */
}


int main(int argc, char* argv[]) {
  
  float a,b;
  char sourcefile[300];
  char domainfile[300];
  char outputfile[300];
  int clase;
  int color_mode = 0;
  int option_index, c, debug;
  
  while (1) {
    static struct option long_options[] = {
      {"hx", 1, 0, 0},
      {"hy", 1, 0, 0},
      {0, 0, 0, 0}
    };

    c = getopt_long (argc, argv, "dc:",long_options, &option_index);

    if (c == -1) {
      break;
    }               
      
    switch (c) {
    case 0:      
      /* printf ("option %s = %f\n", long_options[option_index].name,threshold1);*/
      break;
    case 'd':
      debug = 1;   
      break;
    case 'c':
      color_mode = atoi(optarg);
      break;
    case '?':
      printf("Author: Ruben Cardenes, Oct 2002\n");
      printf("Usage: geodesicDT2D [options] sourcefile.txt domain.png out.png\n");
      printf("              -d (debug mode)\n");
      printf("              -c color output (0: gray, 1: red-blue, 2: random, 3: random)\n");
      return 1;
      break;
    
    default:
      printf ("?? getopt returned character code 0%o ??\n", c);
    }
  }

  if ((argc - optind) != 3) {
    printf ("Incorrect number of arguments: ");
    printf("Author: Ruben Cardenes, Oct 2002 \n");
    printf("Usage: geodesicDT2D [options] sourcefile.txt domain.png out.png\n");
    printf("              -d (debug mode)\n");
    printf("              -c color output (0: gray, 1: red-blue, 2: random, 3: yellow)\n");
    return 1;
  } else {
    while (optind < argc) {
      if (sscanf(argv[optind++], "%s", sourcefile) == 0)
        printf ("Error parsing argument \n");
      if (sscanf(argv[optind++], "%s", domainfile) == 0)
        printf ("Error parsing argument \n");
      if (sscanf(argv[optind++], "%s", outputfile) == 0)
        printf ("Error parsing argument \n");     
    }
  }
  
  run_geodesic2dDT(sourcefile, domainfile, outputfile, color_mode, debug);
  
  printf("OK\n");
}


