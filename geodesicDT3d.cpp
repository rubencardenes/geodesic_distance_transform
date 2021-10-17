/* Copyright (c) Ruben Cardenes Almeida 1/10/2002 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <iostream>
#include "io_mhd.h"


#define MAX_ELEM_IN_BUCKET 50000
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

struct element3d {
  int x;
  int y;
  int z;
  float dobj;
  int xobj;
  int yobj;
  int zobj;
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
int numocclusionpoints = 0;

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

float distance3d(int x1,int y1,int z1, int x2,int y2,int z2) {
  return sqrt ((x1-x2) * (x1-x2) + (y1-y2) * (y1-y2) + (z1-z2) * (z1-z2));
}

int propagar26(int mapindex, int max1, int max2, int max3, struct bucket *Lista, float* domain, int dcur,struct element3d *Element,float* maps) {
  float distreal;
  int new_mapindex,dist,x,y,z,siguiente,siguiente_obj;
  int ynew,xnew,znew,i;

  for (z=-1;z<2;z++) {
    for (x=-1;x<2;x++) {
      for (y=-1;y<2;y++) {
	if (x==0 && y==0 & z ==0) continue;
	ynew = Element[mapindex].y + y;
	xnew = Element[mapindex].x + x;
	znew = Element[mapindex].z + z;
	if (ynew >=0 && xnew >=0 && znew >=0 
	  && ynew < max1 && xnew < max2 && znew < max3) {	 
	  new_mapindex = mapIndex3D(xnew,ynew,znew,max1,max2,max3);
	  if (domain[new_mapindex] == 0 && maps[new_mapindex] == -1) {	 
	    distreal = distance3d(xnew,ynew,znew,Element[mapindex].xobj,Element[mapindex].yobj,Element[mapindex].zobj) + Element[mapindex].dobj;
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
	      Element[new_mapindex].zobj = Element[mapindex].zobj;
	      Element[new_mapindex].dobj = Element[mapindex].dobj;
	      Lista[1].index_elem[siguiente] = new_mapindex;      
	      Lista[1].num_elem++;
	      /* maps[new_mapindex] = dist; */
	      maps[new_mapindex] = distreal;
	    }	   
	    if (dist <= dcur) {
	      /* printf("cambio de objeto en x %d y %d, xnew %d ynew %d, dist %d, distreal %f Element[mapindex]: dobj %f xobj %d yobj %d\n",Element[mapindex].x,Element[mapindex].y,xnew,ynew,dist,distreal,Element[mapindex].dobj,Element[mapindex].xobj,Element[mapindex].yobj); */
	      numocclusionpoints++;
	      siguiente = Lista[2].num_elem;
	      if (siguiente >= MAX_ELEM_IN_BUCKET) {
		printf("Excedidos num elem in bucket \n");
		return 1;
	      }
	      Element[new_mapindex].xobj = Element[mapindex].x;
	      Element[new_mapindex].yobj = Element[mapindex].y;
	      Element[new_mapindex].zobj = Element[mapindex].z;
	      Element[new_mapindex].dobj = distreal;
	      Element[mapindex].dobj = distreal; 
	      Lista[2].index_elem[siguiente] = new_mapindex;
	      Lista[2].num_elem++;
	      /*maps[new_mapindex] = round(distreal + distance(xnew,ynew,Element[mapindex].x,Element[mapindex].y)); */
	      maps[new_mapindex] = distreal + distance3d(xnew,ynew,znew,Element[mapindex].x,Element[mapindex].y,Element[mapindex].z);
	    }
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

float* geodesicDT3d(char* prototypes,int max1, int max2, int max3, float* maps, float* domain) {
  int i,j,x,y,z,l,N,r,count;
  int siguiente,indice_actual;
  int d,mapindex,buckets_empty;
  struct element3d *Element;
  struct element3d *Proto;
  struct bucket Lista[3];
  float** im;
  float distancia_from_l,distancia_ultima;
  FILE *fp,*fpdist;

  /* inicializamos el mapa de distancias a -1 */ 
  for (j=0;j<max1*max2*max3;j++) {
    maps[j] = -1;
  }

  Element=(struct element3d*)malloc(sizeof(struct element3d)*max1*max2*max3);
  Proto=(struct element3d*)malloc(sizeof(struct element3d)*max1*max2*max3);
  Lista[0].index_elem = (int*)malloc(sizeof(int)*MAX_ELEM_IN_BUCKET);
  memset(Lista[0].index_elem,0,sizeof(int)*MAX_ELEM_IN_BUCKET);
  Lista[0].num_elem = 0;

  i=0;
  for (z=0;z<max3;z++) {
    for (x=0;x<max1;x++) {
      for (y=0;y<max2;y++) {
	Element[i].dobj = 0;
	Element[i].x = x;
	Element[i].y = y;
	Element[i].z = z;
	i++;
      }
    }
  }

  l = 0;
  count = 0;
  for (z=0;z<max3;z++) {
    for (x=0;x<max1;x++) {
      for (y=0;y<max2;y++) {
	if (prototypes[count] != -1 ) {
	  Proto[l].x= x;
	  Proto[l].y= y;
	  Proto[l].z= z;
	  Proto[l].icur = prototypes[count];
	  Element[count].dobj = 0;
	  Element[count].xobj = x;
	  Element[count].yobj = y;
	  Element[count].zobj = z;
	  maps[count] = 0;
	  Lista[0].index_elem[l] = count;
	  Lista[0].num_elem++;
	  l++;
	}
	count++;
      }
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
      /* printf("num_elem = %d \n",Bucket[d].num_elem); */
      indice_actual = Lista[0].index_elem[siguiente];
      Lista[0].index_elem[siguiente] = -1;
      Lista[0].num_elem--;
      mapindex =mapIndex3D(Element[indice_actual].x,Element[indice_actual].y,Element[indice_actual].z,max1,max2,max3);
      if (mapindex != indice_actual) {	
        printf("Error extrano mapindex %d != indice_actual %d\n",mapindex,indice_actual);
        return (float*)NULL;
      }
      /*printf("propagando mapindex %d x %d y %d \n",mapindex,Element[mapindex].x,Element[mapindex].y);*/
      propagar26(mapindex, max1, max2, max3, Lista, domain, d, Element, maps);   
    }
    d++;
    swap(&Lista[0],&Lista[1]);
    if (Lista[2].num_elem >0) {
      /* printf("volcando Lista2 (%d elems) en lista0 (%d elems)\n",Lista[2].num_elem,Lista[0].num_elem); */
      volcar(&Lista[2],&Lista[0]);
    }
    if (Lista[0].num_elem == 0 && Lista[1].num_elem == 0) break;
    /* if (d==43) break; */
  }
    
  printf("Distancia maxima alcanzada: %d\n",d); 
  
  /* Transformamos mapa de regiones en mapa de clases */
 
  /* liberamos memoria */
  free(Proto);
  free(Element);
  printf("geodesicDT OK\n");
  return maps;
}

int main(int argc, char* argv[]) {
  char *proto;
  float *maps,*domain;
  int i,col,row,slice,tipo_mapa,num_mapa,c,option_index, debug = 0;
  struct Header cabecera_input,cabecera_maps;
  int max1 = 256;
  int max2 = 256;
  int max3 = 1;
  int K=1;
  float a,b;
  FILE *fp,*fg;
  struct timeval startinit;
  struct timeval endinit;
  struct timeval endtotal;
  char sourcefile[200], domainfile[200], outputfile[200];
  int clase,mapindex;

  while (1) {
    static struct option long_options[] = {
      {"hx", 1, 0, 0},
      {"hy", 1, 0, 0},
      {0, 0, 0, 0}
    };

    c = getopt_long (argc, argv, "d",long_options, &option_index);

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
    case '?':
      printf("Author: Ruben Cardenes, Oct 2002\n");
      printf("Usage: geodesicDT3d [options] sourcefile.txt domain.mhd mapa.mhd\n");
      printf("              -d (debug mode)\n");
      return 1;
      break;
    
    default:
      printf ("?? getopt returned character code 0%o ??\n", c);
    }
  }
  
  if ((argc - optind) != 3) {
    printf ("Incorrect number of arguments: ");
    printf("Author: Ruben Cardenes, Oct 2002 \n");
    printf("Usage: geodesicDT3d [options] sourcefile.txt domain.mhd mapa.mhd\n");
    printf("              -d (debug mode)\n");
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

  gettimeofday(&startinit,NULL);

  // Read domain
  //domain = (float*)malloc(sizeof(float)*max1*max2*max3);
  //fp = fopen(domainfile,"r");
  //fread(domain,sizeof(float),max1*max2*max3,fp);
  //fclose(fp);

  // Read domain mhd format 
  readmhd_header(domainfile, &cabecera_input);
  std::cout << domainfile << std::endl;
  std::cout << "Cabecera input type " << cabecera_input.ElementType << std::endl;
  std::cout << "Cabecera input dims " << cabecera_input.dimsize[0] << std::endl;
  std::cout << "Cabecera input dims " << cabecera_input.dimsize[1] << std::endl;
  std::cout << "Cabecera input dims " << cabecera_input.dimsize[2] << std::endl;
  domain = readraw_float(cabecera_input,domainfile);

  max1 = cabecera_input.dimsize[1];
  max2 = cabecera_input.dimsize[0];
  max3 = cabecera_input.dimsize[2];

  maps = (float*)malloc(sizeof(float)*max1*max2*max3);
  proto = (char*)malloc(sizeof(char)*max1*max2*max3);

  for (i=0;i<max1*max2*max3;i++) {
    proto[i] = -1;
  }

  fp = fopen(sourcefile,"rb");
  if (fp == NULL) {
    fprintf(stderr,"Failed reading prototypes %s\n",sourcefile);
    exit(1);
  }

  printf("Leyendo prototipos \n");
  initializeFromTrainingDataEspacial(fp,100000000,0,max1,max2,max3);
  for (i=0;i<numPrototypes;i++) {   
    row=(int)prototypeNodeData[i].row;
    col=(int)prototypeNodeData[i].col;    
    slice=(int)prototypeNodeData[i].slice;  
    if (prototypeNodeData[i].pclass != 0) {
      mapindex = mapIndex3D(row,col,slice,max1,max2,max3);
      proto[mapindex] = (char)prototypeNodeData[i].pclass;
    }
  }
  fclose(fp);

  numasignaciones = 0;
  printf("Entrando en geodesicDT\n");
  gettimeofday(&endinit,NULL);
  maps = geodesicDT3d(proto, max1, max2, max3, maps, domain);
  if (maps == (float*)NULL) {
    printf("Error en DToptimo\n");
  }
  gettimeofday(&endtotal,NULL);

  printf("Escribiendo mapa de distancias\n");
  cabecera_maps = cabecera_input;
  cabecera_maps.ElementType.assign("MET_FLOAT");
  writemhd_header(outputfile,&cabecera_maps);
  writeraw_float(maps,outputfile,cabecera_maps);

  //fg=fopen(outputfile,"w");
  //fwrite(maps,sizeof(float),max1*max2*max3,fg);    
  //fclose(fg);
  
  printf("numocclusionpoints = %d\n",numocclusionpoints);
  
  free(proto);
  free(maps);
  free(domain);

  fprintf(stdout,"Initialization time: ");
  print_timing(stdout, startinit, endinit);
  fprintf(stdout,"geodesic DT time: ");
  print_timing(stdout, endinit, endtotal);
  /* printf("numasignaciones = %d, asignacionesraras =%d, numrechazos =%d\n", numasignaciones,asignacionesraras,numrechazos); */

  printf("OK\n");
}


