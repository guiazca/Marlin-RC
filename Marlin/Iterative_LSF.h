//  This code is an alternate implementation of the Least Squares Best Fit algorythm that
//  is used to calculate the plane the print bed lies in.   It is an optional replacement for the
//  more proven and much larger QR_SOLVE code base that is being used in the Marlin Code base.
//
//  Using this code as a solution for the Least Squares Best fit by the Auto Bed Leveling code
//  will save you almost exactly 10 Kb of firmware space.   It will cost you several seconds of
//  time after all the points are sampled as it iteratively converges on a Least Squares Error
//  solution for the plane.
//
//  This code was donated by Roxy-3DPrintBoard!

#include "configuration.h"

double summation_of_least_square_error_data(double , double , double , double , int , double *, double * );
//double * iterative_LSF(int , int , double *, double *);
double *qr_solve(int , int , double *, double *);
