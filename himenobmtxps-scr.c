/********************************************************************

 This benchmark test program is measuring a cpu performance
 of floating point operation by a Poisson equation solver.

 If you have any question, please ask me via email.
 written by Ryutaro HIMENO, November 26, 2001.
 Version 3.0
 ----------------------------------------------
 Ryutaro Himeno, Dr. of Eng.
 Head of Computer Information Division,
 RIKEN (The Institute of Pysical and Chemical Research)
 Email : himeno@postman.riken.go.jp
 ---------------------------------------------------------------
 You can adjust the size of this benchmark code to fit your target
 computer. In that case, please chose following sets of
 (mimax,mjmax,mkmax):
 small : 33,33,65
 small : 65,65,129
 midium: 129,129,257
 large : 257,257,513
 ext.large: 513,513,1025
 This program is to measure a computer performance in MFLOPS
 by using a kernel which appears in a linear solver of pressure
 Poisson eq. which appears in an incompressible Navier-Stokes solver.
 A point-Jacobi method is employed in this solver as this method can 
 be easyly vectrized and be parallelized.
 ------------------
 Finite-difference method, curvilinear coodinate system
 Vectorizable and parallelizable on each grid point
 No. of grid points : imax x jmax x kmax including boundaries
 ------------------
 A,B,C:coefficient matrix, wrk1: source term of Poisson equation
 wrk2 : working area, OMEGA : relaxation parameter
 BND:control variable for boundaries and objects ( = 0 or 1)
 P: pressure
********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "param.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>

//#define SCR_ENABLE
#ifdef  SCR_ENABLE
#include  "scr.h"
#define SCR_MAX_FILENAME 1024
#define CKPT_PATH  /tmp

#endif

float jacobi(int);
int initmax(int,int,int);
void initmt(int,int);
void initcomm(int,int,int);
void sendp(int,int,int);
void sendp1();
void sendp2();
void sendp3();

double fflop(int,int,int);
double mflops(int,double,double);

static float  p[MIMAX][MJMAX][MKMAX];
static float  a[4][MIMAX][MJMAX][MKMAX],
              b[3][MIMAX][MJMAX][MKMAX],
              c[3][MIMAX][MJMAX][MKMAX];
static float  bnd[MIMAX][MJMAX][MKMAX];
static float  wrk1[MIMAX][MJMAX][MKMAX],
              wrk2[MIMAX][MJMAX][MKMAX];
static float omega;
static int npe,id;

static int ndx,ndy,ndz;
static int imax,jmax,kmax;
static int ndims=3,iop[3];
static int npx[2],npy[2],npz[2];
MPI_Comm     mpi_comm_cart;
MPI_Datatype ijvec,ikvec,jkvec;

int
main(int argc,char *argv[])
{
  int    i,j,k,nn;
  int    mx,my,mz,it;
  float  gosa;
  double cpu,cpu0,cpu1,flop,target;

  target= 60.0;
  omega= 0.8;
  mx= MX0-1;
  my= MY0-1;
  mz= MZ0-1;
  ndx= NDX0;
  ndy= NDY0;
  ndz= NDZ0;

  MPI_Init(&argc, &argv);
#ifdef SCR_ENABLE
  SCR_Init();
#endif
  
  MPI_Comm_size(MPI_COMM_WORLD, &npe);
  MPI_Comm_rank(MPI_COMM_WORLD, &id);

  initcomm(ndx,ndy,ndz);
  it= initmax(mx,my,mz);

  /*
   *    Initializing matrixes
   */
  initmt(mx,it);

  if(id==0){
    printf("Sequential version array size\n");
    printf(" mimax = %d mjmax = %d mkmax = %d\n",MX0,MY0,MZ0);
    printf("Parallel version array size\n");
    printf(" mimax = %d mjmax = %d mkmax = %d\n",MIMAX,MJMAX,MKMAX);
    printf("imax = %d jmax = %d kmax =%d\n",imax,jmax,kmax);
    printf("I-decomp = %d J-decomp = %d K-decomp =%d\n",ndx,ndy,ndz);
  }

  nn= 3;
  if(id==0){
    printf(" Start rehearsal measurement process.\n");
    printf(" Measure the performance in %d times.\n\n",nn);
  }

  MPI_Barrier(MPI_COMM_WORLD);
  cpu0= MPI_Wtime();
  gosa= jacobi(nn);
  cpu1= MPI_Wtime() - cpu0;

  MPI_Allreduce(&cpu1,
                &cpu,
                1,
                MPI_DOUBLE,
                MPI_MAX,
                MPI_COMM_WORLD);

  flop= fflop(mz,my,mx);
  if(id == 0){
    printf(" MFLOPS: %f time(s): %f %e\n\n",
           mflops(nn,cpu,flop),cpu,gosa);
  }
 nn= (int)(target/(cpu/3.0));

  if(id == 0){
    printf(" Now, start the actual measurement process.\n");
    printf(" The loop will be excuted in %d times\n",nn);
    printf(" This will take about one minute.\n");
    printf(" Wait for a while\n\n");
  }

  /*
   *    Start measuring
   */
  MPI_Barrier(MPI_COMM_WORLD);
  cpu0 = MPI_Wtime();
  //  nn = 10000000;
  gosa = jacobi(nn);
  cpu1 = MPI_Wtime() - cpu0;

  MPI_Allreduce(&cpu1,
                &cpu,
                1,
                MPI_DOUBLE,
                MPI_MAX,
                MPI_COMM_WORLD);

  if(id == 0){
    fprintf(stderr, "cpu : %f sec.\n", cpu);
    fprintf(stderr, "Loop executed for %d times\n",nn);
    fprintf(stderr, "Gosa : %e \n",gosa);
    fprintf(stderr, "GFLOPS measured : %f\n",mflops(nn,cpu,flop)/1000.0);
    fprintf(stderr, "Score based on Pentium III 600MHz : %f\n",
           mflops(nn,cpu,flop)/82.84);
  }

#ifdef SCR_ENABLE  
  SCR_Finalize();
#endif
  MPI_Finalize();
  
  return (0);
}

double
fflop(int mx,int my, int mz)
{
  return((double)(mz-2)*(double)(my-2)*(double)(mx-2)*34.0);
}

double
mflops(int nn,double cpu,double flop)
{
  return(flop/cpu*1.e-6*(double)nn);
}

void
initmt(int mx,int it)
{
  int i,j,k;

  for(i=0 ; i<MIMAX ; ++i)
    for(j=0 ; j<MJMAX ; ++j)
      for(k=0 ; k<MKMAX ; ++k){
        a[0][i][j][k]=0.0;
        a[1][i][j][k]=0.0;
        a[2][i][j][k]=0.0;
        a[3][i][j][k]=0.0;
        b[0][i][j][k]=0.0;
        b[1][i][j][k]=0.0;
        b[2][i][j][k]=0.0;
        c[0][i][j][k]=0.0;
        c[1][i][j][k]=0.0;
        c[2][i][j][k]=0.0;
        p[i][j][k]=0.0;
        wrk1[i][j][k]=0.0;
        wrk2[i][j][k]=0.0;
        bnd[i][j][k]=0.0;
      }

  for(i=0 ; i<imax ; ++i)
    for(j=0 ; j<jmax ; ++j)
      for(k=0 ; k<kmax ; ++k){
        a[0][i][j][k]=1.0;
        a[1][i][j][k]=1.0;
        a[2][i][j][k]=1.0;
        a[3][i][j][k]=1.0/6.0;
        b[0][i][j][k]=0.0;
        b[1][i][j][k]=0.0;
        b[2][i][j][k]=0.0;
        c[0][i][j][k]=1.0;
        c[1][i][j][k]=1.0;
        c[2][i][j][k]=1.0;
        p[i][j][k]=(float)((i+it)*(i+it))/(float)((mx-1)*(mx-1));
        wrk1[i][j][k]=0.0;
        wrk2[i][j][k]=0.0;
        bnd[i][j][k]=1.0;
      }
}

float
jacobi(int nn)
{
  int i,j,k,n;
  float gosa,wgosa,s0,ss;
  double s, cpu, com;

  for(n=0 ; n<nn ; ++n){
    gosa = 0.0;
    wgosa= 0.0;
    //    if (id == 0)fprintf(stderr, "%d: nn:%d, imax:%d, jmax:%d, kmax:%d\n", n, nn, imax, jmax, kmax);
    //    if (id == 128)fprintf(stderr, "%d: nn:%d, imax:%d, jmax:%d, kmax:%d\n", n, nn, imax, jmax, kmax);
    s = MPI_Wtime();
    for(i=1 ; i<imax-1 ; ++i)
      for(j=1 ; j<jmax-1 ; ++j)
        for(k=1 ; k<kmax-1 ; ++k){
          s0 = a[0][i][j][k] * p[i+1][j  ][k  ]
             + a[1][i][j][k] * p[i  ][j+1][k  ]
             + a[2][i][j][k] * p[i  ][j  ][k+1]
             + b[0][i][j][k] * ( p[i+1][j+1][k  ] - p[i+1][j-1][k  ]
                               - p[i-1][j+1][k  ] + p[i-1][j-1][k  ] )
             + b[1][i][j][k] * ( p[i  ][j+1][k+1] - p[i  ][j-1][k+1]
                               - p[i  ][j+1][k-1] + p[i  ][j-1][k-1] )
             + b[2][i][j][k] * ( p[i+1][j  ][k+1] - p[i-1][j  ][k+1]
                               - p[i+1][j  ][k-1] + p[i-1][j  ][k-1] )
             + c[0][i][j][k] * p[i-1][j  ][k  ]
             + c[1][i][j][k] * p[i  ][j-1][k  ]
             + c[2][i][j][k] * p[i  ][j  ][k-1]
             + wrk1[i][j][k];

          ss = ( s0 * a[3][i][j][k] - p[i][j][k] ) * bnd[i][j][k];
          wgosa += ss*ss;

          wrk2[i][j][k] = p[i][j][k] + omega * ss;
        }

    for(i=1 ; i<imax-1 ; ++i)
      for(j=1 ; j<jmax-1 ; ++j)
        for(k=1 ; k<kmax-1 ; ++k)
          p[i][j][k] = wrk2[i][j][k];

    cpu = MPI_Wtime() - s;
    s = MPI_Wtime();
    sendp(ndx,ndy,ndz);
    com = MPI_Wtime() - s;
    s = MPI_Wtime();
    if (id == 0) {
      fprintf(stderr, "%d: time: %f cpu: %f com: %f nn:%d, imax:%d, jmax:%d, kmax:%d\n", cpu + com, cpu, com,  n, nn, imax, jmax, kmax);
    }
    //    if (n % 10 == 0 ) fprintf(stdout, "LLLL %d  %f\n", n, s);

#ifdef SCR_ENABLE
    int flag;
    char SCR_testFileName[SCR_MAX_FILENAME];
    char     testFileName[SCR_MAX_FILENAME];
    double gs, ge, start, end, dump, encoding;
    gs = start = MPI_Wtime();
    SCR_Need_checkpoint(&flag);
    if(flag){
      SCR_Start_checkpoint();
      sprintf(testFileName, "testfile.%d", id);
      strcpy(SCR_testFileName, testFileName);
      SCR_Route_file(testFileName, SCR_testFileName);
      if(id == 0)  printf(" ***** SCR_ROUTE_FILE %s\n", SCR_testFileName);
      file_dump(SCR_testFileName, p, MIMAX * MJMAX * MKMAX * sizeof(float));
      end = MPI_Wtime();
      dump = end - start;
      start = MPI_Wtime();
      SCR_Complete_checkpoint(1);
      ge = end = MPI_Wtime();
      encoding = end - start;
      //printf(" ***** SCR_START_CHECKPOINT\n");

      if (id == 0) fprintf(stderr, "time: %f (write: %f , enco:  %f)\n", ge - gs, dump, encoding);
    }


    // SCR

#endif
    MPI_Allreduce(&wgosa,
                  &gosa,
                  1,
                  MPI_FLOAT,
                  MPI_SUM,
                  MPI_COMM_WORLD);
  } /* end n loop */

  return(gosa);
}


void
initcomm(int ndx,int ndy,int ndz)
{
  int  i,j,k,tmp;
  int  ipd[3],idm[3],ir;
  MPI_Comm  icomm;

  if(ndx*ndy*ndz != npe){
    if(id==0){
      printf("Invalid number of PE\n");
      printf("Please check partitioning pattern or number of PE\n");
      printf("ndx: %d, ndy: %d, ndz: %d, npe: %d\n", ndx, ndy, ndz, npe);
    }
    MPI_Finalize();
    exit(0);
  }

  icomm= MPI_COMM_WORLD;

  idm[0]= ndx;
  idm[1]= ndy;
  idm[2]= ndz;

  ipd[0]= 0;
  ipd[1]= 0;
  ipd[2]= 0;
  ir= 0;


  MPI_Cart_create(icomm,
                  ndims,
                  idm,
                  ipd,
                  ir,
                  &mpi_comm_cart);
  MPI_Cart_get(mpi_comm_cart,
               ndims,
               idm,
               ipd,
               iop);

  if(ndz > 1){
    MPI_Cart_shift(mpi_comm_cart,
                   2,
                   1,
                   &npz[0],
                   &npz[1]);
  }                     
  if(ndy > 1){
    MPI_Cart_shift(mpi_comm_cart,
                   1,
                   1,
                   &npy[0],
                   &npy[1]);
  }                     
  if(ndx > 1){
    MPI_Cart_shift(mpi_comm_cart,
                   0,
                   1,
                   &npx[0],
                   &npx[1]);
  }                     

}

int
initmax(int mx,int my,int mz)
{
  int  i,tmp,it;
  int  mx1[NDX0+1],my1[NDY0+1],mz1[NDZ0+1];
  int  mx2[NDX0+1],my2[NDY0+1],mz2[NDZ0+1];

  tmp= mx/ndx;
  mx1[0]= 0;
  for(i=1;i<=ndx;i++){
    if(i <= mx%ndx)
      mx1[i]= mx1[i-1] + tmp + 1;
    else
      mx1[i]= mx1[i-1] + tmp;
  }
  tmp= my/ndy;
  my1[0]= 0;
  for(i=1;i<=ndy;i++){
    if(i <= my%ndy)
      my1[i]= my1[i-1] + tmp + 1;
    else
      my1[i]= my1[i-1] + tmp;
  }
  tmp= mz/ndz;
  mz1[0]= 0;
  for(i=1;i<=ndz;i++){
    if(i <= mz%ndz)
      mz1[i]= mz1[i-1] + tmp + 1;
    else
      mz1[i]= mz1[i-1] + tmp;
  }

  for(i=0 ; i<ndx ; i++){
    mx2[i] = mx1[i+1] - mx1[i];
    if(i != 0)     mx2[i] = mx2[i] + 1;
    if(i != ndx-1) mx2[i] = mx2[i] + 1;
  }
  for(i=0 ; i<ndy ; i++){
    my2[i] = my1[i+1] - my1[i];
    if(i != 0)     my2[i] = my2[i] + 1;
    if(i != ndy-1) my2[i] = my2[i] + 1;
  }
  for(i=0 ; i<ndz ; i++){
    mz2[i] = mz1[i+1] - mz1[i];
    if(i != 0)     mz2[i] = mz2[i] + 1;
    if(i != ndz-1) mz2[i] = mz2[i] + 1;
  }

  imax = mx2[iop[0]];
  jmax = my2[iop[1]];
  kmax = mz2[iop[2]];

  if(iop[0] == 0)
    it= mx1[iop[0]];
  else
    it= mx1[iop[0]] - 1;

  if(ndx > 1){
    MPI_Type_vector(jmax,
                    kmax,
                    MKMAX,
                    MPI_FLOAT,
                    &jkvec);
    MPI_Type_commit(&jkvec);
  }                    
  if(ndy > 1){
    MPI_Type_vector(imax,
                    kmax,
                    MJMAX*MKMAX,
                    MPI_FLOAT,
                    &ikvec);
    MPI_Type_commit(&ikvec);
  }                    
  if(ndz > 1){
    MPI_Type_vector(imax*jmax,
                    1,
                    MKMAX,
                    MPI_FLOAT,
                    &ijvec);
    MPI_Type_commit(&ijvec);
  }                    

  return(it);
}

void
sendp(int ndx,int ndy,int ndz)
{
  if(ndz > 1)
    sendp3();

  if(ndy > 1)
    sendp2();

  if(ndx > 1)
    sendp1();

}

void
sendp3()
{
  MPI_Status   st[4];
  MPI_Request  req[4];

  MPI_Irecv(&p[0][0][kmax-1],
            1,
            ijvec,
            npz[1],
            1,
            mpi_comm_cart,
            req);
  MPI_Irecv(&p[0][0][0],
            1,
            ijvec,
            npz[0],
            2,
            mpi_comm_cart,
            req+1);
  MPI_Isend(&p[0][0][1],
            1,
            ijvec,
            npz[0],
            1,
            mpi_comm_cart,
            req+2);
  MPI_Isend(&p[0][0][kmax-2],
            1,
            ijvec,
            npz[1],
            2,
            mpi_comm_cart,
            req+3);

  MPI_Waitall(4,
              req,
              st);
}

void
sendp2()
{
  MPI_Status  st[4];
  MPI_Request req[4];

  MPI_Irecv(&p[0][jmax-1][0],
            1,
            ikvec,
            npy[1],
            1,
            mpi_comm_cart,
            req);
  MPI_Irecv(&p[0][0][0],
            1,
            ikvec,
            npy[0],
            2,
            mpi_comm_cart,
            req+1);
  MPI_Isend(&p[0][1][0],
            1,
            ikvec,
            npy[0],
            1,
            mpi_comm_cart,
            req+2);
  MPI_Isend(&p[0][jmax-2][0],
            1,
            ikvec,
            npy[1],
            2,
            mpi_comm_cart,
            req+3);

  MPI_Waitall(4,
              req,
              st);
}


void
sendp1()
{
  MPI_Status  st[4];
  MPI_Request req[4];

  MPI_Irecv(&p[imax-1][0][0],
            1,
            jkvec,
            npx[1],
            1,
            mpi_comm_cart,
            req);
  MPI_Irecv(&p[0][0][0],
            1,
            jkvec,
            npx[0],
            2,
            mpi_comm_cart,
            req+1);
  MPI_Isend(&p[1][0][0],
            1,
            jkvec,
            npx[0],
            1,
            mpi_comm_cart,
            req+2);
  MPI_Isend(&p[imax-2][0][0],
            1,
            jkvec,
            npx[1],
            2,
            mpi_comm_cart,
            req+3);

  MPI_Waitall(4,
              req,
              st);
}
            
int file_dump(char * file, float *buf, size_t size)
{

  ssize_t n = 0;
  int retries = 10;
  int fd;

  fd = open(file, O_WRONLY | O_CREAT, S_IREAD | S_IWRITE) ;
  if (fd < 0) {
    fprintf(stderr, "Error: %s", file);
    exit(1);
  }

  while (n < size)
    {
      ssize_t rc = write(fd, (char*) buf + n, size - n);
      //      fprintf(stderr, "data: %d\n", rc);
      if (rc > 0) {
	n += rc;
      } else if (rc == 0) {
	/* something bad happened, print an error and abort */
	scr_err("Error writing %s: write(%d, %x, %ld) returned 0 @ %s:%d",
		file, fd, (char*) buf + n, size - n, __FILE__, __LINE__
		);
	exit(1);
      } else { /* (rc < 0) */
	/* got an error, check whether it was serious */
	if (errno == EINTR || errno == EAGAIN) {
	  continue;
	}

	/* something worth printing an error about */
	retries--;
	if (retries) {
	  /* print an error and try again */
	  scr_err("Error writing %s: write(%d, %x, %ld) errno=%d %m @ %s:%d",
		  file, fd, (char*) buf + n, size - n, errno, __FILE__, __LINE__
		  );
	} else {
	  /* too many failed retries, give up */
	  scr_err("Giving up write to %s: write(%d, %x, %ld) errno=%d %m @ %s:%d",
		  file, fd, (char*) buf + n, size - n, errno, __FILE__, __LINE__
		  );
	  exit(1);
	}
      }
    }
  close(fd);
  return n;

}
