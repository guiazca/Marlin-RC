//  This code is an alternate implementation of the Least Squares Best Fit algorythm that
//  is used to calculate the plane the print bed lies in.   It is an optional replacement for the
//  more proven and much larger QR_SOLVE code base that is being used in the Marlin Code base.
//
//  Using this code as a solution for the Least Squares Best fit by the Auto Bed Leveling code
//  will save you almost exactly 10 Kb of firmware space.   It will cost you several seconds of
//  time after all the points are sampled as it iteratively converges on a Least Squares Error
//  solution for the plane.
//
//  This code was donated by Roxy-3D!


#include "Marlin.h"
#include "configuration.h"

#include "ultralcd.h"
#include "planner.h"
#include "stepper.h"
#include "endstops.h"
#include "temperature.h"
#include "cardreader.h"
#include "configuration_store.h"
#include "language.h"
#include "pins_arduino.h"
#include "math.h"
#include "nozzle.h"
#include "duration_t.h"
#include "types.h"
#include "Iterative_LSF.h"


void idle();
double coefficients[3];         // We need a place to return the coefficients we found.
                    // This is where we do that!

//double * iterative_LSF(int abl2, int n, double *eqnA, double *eqnB)
double *qr_solve(double coeff[3], int abl2, int n, double *eqnA, double *eqnB)
{
double a= 0.0, b= 0.0, c=1.0, d=Z_PROBE_OFFSET_FROM_EXTRUDER;       // Start with some reasonable values for our coefficients
double current, plus_next, minus_next, delta_size=.1;
int flag, dflag, dir=1;
int cnt=0;

    do {
        idle();         // or we risk the system thinking we are locked up!
        cnt++;          // How many times we have gone through the loop.  Only used for debug print outs

        flag = 0;       // a flag to tell us if anything got adjusted this time through the loop.

//
//  The strategy is we figure out our current error amount based on the current values of the coefficient.
//  We then look at what our error will be if we adjust the current coefficient by the delta amount.
//
//  We adjust the 'a' coeffiecent first:
//
        current = summation_of_least_square_error_data(a, b, c, d, abl2, eqnA, eqnB);
        plus_next = summation_of_least_square_error_data(a+delta_size, b, c, d, abl2, eqnA, eqnB);

        if ( plus_next<current) {
            a = a+delta_size;
            flag++;
        }
//
//  If we didn't adjust the 'a' coefficient in the plus direction, we have to compute where we will be if
//  we move it in the minus direction.  As a speed optimization, we don't do the calculation unless the
//  previous plus movement will not help our values.
//
        else {
            minus_next = summation_of_least_square_error_data(a-delta_size, b, c, d, abl2, eqnA, eqnB);
            if (minus_next<current) {
                a = a-delta_size;
                flag++;
            }
        }
//
//  Now we adjust the 'b' coefficient:  It is the same logic as above for the 'a' coefficient
//
        current = summation_of_least_square_error_data(a, b, c, d, abl2, eqnA, eqnB);
        plus_next = summation_of_least_square_error_data(a, b+delta_size, c, d, abl2, eqnA, eqnB);
        if ( plus_next<current) {
            b = b+delta_size;
            flag++;
        }
        else {
            minus_next = summation_of_least_square_error_data(a, b-delta_size, c, d, abl2, eqnA, eqnB);
            if (minus_next<current) {
                b = b-delta_size;
                flag++;
            }
        }

        //
        // We handle the 'd' coefficient in a special manner.  Especially towards the end of the iteration
        // when we have several significant digits of precision on 'a' and 'b', there is no reason to
        // adjust 'a' and 'b' while we adjust 'd'.   Doing so just slows down the algorythm dramatically.
        // Instead, what we do is adjust 'd' as much as we can before going back and adjusting 'a' and 'b'.
        // Also, because we are just focused on 'd', we can keep the last value we moved to as our 'current' value
        // for the next comparision.  As a result of this, we can adjust the 'd' value much faster.

        dflag = 0;  // flag that tells us if we are done adjusting 'd'

        current = summation_of_least_square_error_data(a, b, c, d, abl2, eqnA, eqnB);
        do {
            plus_next = summation_of_least_square_error_data(a,b,c,d+delta_size, abl2, eqnA, eqnB );
            if ( plus_next<current) {
                d = d+delta_size;
                current = plus_next;
                flag++;
            }
            else {
                minus_next = summation_of_least_square_error_data(a,b,c,d-delta_size, abl2, eqnA, eqnB );
                if (minus_next<current) {
                    d = d-delta_size;
                    current = minus_next;
                    flag++;
                } else
                    dflag++;    // If we get here, we did not adjust the 'd' value up or down
                            // So, we want to exit the special case loop and go back to
                            // adjusting 'a' and 'b'.
            }
        } while (dflag==0);

//  If flag==0 it means that we did not find a better value for the Least Squares Error by shifting any of
//  the coefficients this time through the loop.  What this means is the amount we were trying to shift the
//  coefficients by was too big.  We have to back off some.  We will try again the next time through the loop
//  with a smaller delta amount!

        if (flag==0)  {
//
//  The following debug code can be uncommented to help understand the convergance process.
//

/*
current = summation_of_least_square_error_data(a, b, c, d, abl2, eqnA, eqnB);
SERIAL_PROTOCOLPGM("error=");
SERIAL_PROTOCOL_F(current, 8);
SERIAL_PROTOCOLPGM(" delta_size=");
SERIAL_PROTOCOL_F(delta_size, 8);
SERIAL_PROTOCOLPGM(" a=");
SERIAL_PROTOCOL_F(a, 8);
SERIAL_PROTOCOLPGM(" b=");
SERIAL_PROTOCOL_F(b, 8);
SERIAL_PROTOCOLPGM(" d=");
SERIAL_PROTOCOL_F(d, 8);
SERIAL_PROTOCOLPGM(" cnt=");
SERIAL_PROTOCOL_F(cnt, 8);
SERIAL_PROTOCOLPGM("\n");
*/

            delta_size = delta_size / 2.0;
        }


    } while ( flag!=0 || delta_size>.00000005 );        // We keep running the loop until nothing is changing
                                // and the size of delta_size gets so small it is almost
                                // pointless to continue.

    coefficients[0] = -a;       // Save the values we have found
    coefficients[1] = -b;       // and return them to the caller!
    coefficients[2] = -d;

    coeff[0] = -a;       // Save the values we have found
    coeff[1] = -b;       // and return them to the caller!
    coeff[2] = -d;
    return coefficients;
}


//
//  This function loops through all of the supplied data points and uses the specified coefficients to
//  calculation the summation of the least squares error for the data.  It uses the formula
//  D = (ax + by +cz + d) / sqrt( a*a + b*b + c*c)
//  For speed, the normalization [  sqrt( a*a + b*b + c*c) ] is just calculated once.
//


double summation_of_least_square_error_data(double A, double B, double C, double D, int n, double *eqn_A, double *eqn_B )
{
    int i;
    double normal, distance, sum = 0.0;

    normal = sqrt( A*A + B*B + C*C);
    for(i=0; i<n; i++) {
        distance = abs( A*eqn_A[i] + B*eqn_A[i+n] + C*eqn_B[i] + D) / normal ;
        sum += distance*distance;
    }

    return sum;
}
