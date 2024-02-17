#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// https://gist.github.com/volkansalma/2972237

float atan2_approximation1(float y, float x);
float atan2_approximation2(float y, float x);

int main()
{
    float x = 1;
    float y = 0;


    for( y = 0; y < 2*M_PI; y+= 0.1 )
    {
        for(x = 0; x < 2*M_PI; x+= 0.1)
        {
            printf("atan2 for %f,%f: %f \n", y, x, atan2(y, x));
            printf("approx1 for %f,%f: %f \n", y, x, atan2_approximation1(y, x));
            printf("approx2 for %f,%f: %f \n \n", y, x, atan2_approximation2(y, x));
            getch();
        }
    }


    return 0;
}

float atan2_approximation1(float y, float x)
{
    //http://pubs.opengroup.org/onlinepubs/009695399/functions/atan2.html
    //Volkan SALMA

    const float ONEQTR_PI = M_PI / 4.0;
	const float THRQTR_PI = 3.0 * M_PI / 4.0;
	float r, angle;
	float abs_y = fabs(y) + 1e-10f;      // kludge to prevent 0/0 condition
	if ( x < 0.0f )
	{
		r = (x + abs_y) / (abs_y - x);
		angle = THRQTR_PI;
	}
	else
	{
		r = (x - abs_y) / (x + abs_y);
		angle = ONEQTR_PI;
	}
	angle += (0.1963f * r * r - 0.9817f) * r;
	if ( y < 0.0f )
		return( -angle );     // negate if in quad III or IV
	else
		return( angle );


}

#define PI_FLOAT     3.14159265f
#define PIBY2_FLOAT  1.5707963f
// |error| < 0.005
float atan2_approximation2( float y, float x )
{
	if ( x == 0.0f )
	{
		if ( y > 0.0f ) return PIBY2_FLOAT;
		if ( y == 0.0f ) return 0.0f;
		return -PIBY2_FLOAT;
	}
	float atan;
	float z = y/x;
	if ( fabs( z ) < 1.0f )
	{
		atan = z/(1.0f + 0.28f*z*z);
		if ( x < 0.0f )
		{
			if ( y < 0.0f ) return atan - PI_FLOAT;
			return atan + PI_FLOAT;
		}
	}
	else
	{
		atan = PIBY2_FLOAT - z/(z*z + 0.28f);
		if ( y < 0.0f ) return atan - PI_FLOAT;
	}
	return atan;
}