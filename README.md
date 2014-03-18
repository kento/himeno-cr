
Himemo Benchmark (with Checkpoint/Restart)
==========================

This code implements a himen benchmark with simple checkpoint/restart.

About himeno benchmark
------------------
This benchmark test program is measuring a cpu performance
of floating point operation by a Poisson equation solver.

If you have any question, please ask me via email.
written by Ryutaro HIMENO, November 26, 2001.
Version 3.0

    Ryutaro Himeno, Dr. of Eng.
    Head of Computer Information Division,
    RIKEN (The Institute of Pysical and Chemical Research)
    Email : himeno@postman.riken.go.jp

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

     Finite-difference method, curvilinear coodinate system
     Vectorizable and parallelizable on each grid point
     No. of grid points : imax x jmax x kmax including boundaries

A,B,C:coefficient matrix, wrk1: source term of Poisson equation
wrk2 : working area, OMEGA : relaxation parameter
BND:control variable for boundaries and objects ( = 0 or 1)
P: pressure (this array is checkpointed)

HOW to Build & Install & Run
------------
Run commands below:   

    $ ./paramset.sh <Grid size> <X> <Y> <Z> <Checkpoint dir>
    $ make

* `<Grid size>`           : XS (32x32x64), S  (64x64x128), M  (128x128x256), L  (256x256x512), XL (512x512x1024)
* `<X> <Y> <Z>`           : Partition size
* `Checkpoint dir`: Path to Checkpoint/Restart directory


    $  mpirun -np * ./bmt <Restart #> <Checkpoint interval (steps)>
* `Restart #`:  Checkpiont id at which bmt starts. 0 indicates run without restart
* `Checkpoint interval (steps)`: # of Steps to skip checkpointing . 0 indicates no checkpoints.

Example
-------------
Run commands below:  
  
    $ ./paramset.sh M 1 1 2 /path/to/checkpoint/restart/dir
    $ make
    $ mpirun -np 2 ./bmt 0 100 
    $ mpirun -np 2 ./bmt 1000 100 #restart from step 1000
