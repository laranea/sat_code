/* Copyright (C) 2018, Project Pluto

'fake_ast.cpp' : generates fake astrometry from TLEs.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

   This program will generate simulated geocentric observations
for a given object from a TLE.  In theory,  one can then fit these
pseudo-observations to a higher-quality physical model to get a
considerably more accurate ephemeris for the object.  */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "norad.h"
#include "observe.h"

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923

int main( const int argc, const char **argv)
{
   FILE *ifile = fopen( argv[1], "rb");
   char line1[100], line2[100];
   const char *intl_id = NULL;
   double step_size = .1;
   int i, n_steps = 100;

   if( !ifile)
      {
      printf( "Couldn't open input file\n");
      exit( -1);
      }
   for( i = 1; i < argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 'i':
               intl_id = argv[i] + 2;
               break;
            case 'n':
               n_steps = atoi( argv[i] + 2);
               break;
            default:
               printf( "Unrecognized option '%s'\n", argv[i]);
               break;
            }
   *line1 = '\0';
   sxpx_set_implementation_param( SXPX_DUNDEE_COMPLIANCE, 1);
   while( fgets( line2, sizeof( line2), ifile))
      {
      tle_t tle; /* Pointer to two-line elements set for satellite */
      int err_val;

      if( (!intl_id || !memcmp( intl_id, line1 + 9, 6))
                && (err_val = parse_elements( line1, line2, &tle)) >= 0)
         {                  /* hey! we got a TLE! */
         int is_deep = select_ephemeris( &tle);
         double sat_params[N_SAT_PARAMS], observer_loc[3];

         if( err_val)
            printf( "WARNING: TLE parsing error %d\n", err_val);
         for( i = 0; i < 3; i++)
            observer_loc[i] = '\0';
         if( is_deep)
            SDP4_init( sat_params, &tle);
         else
            SGP4_init( sat_params, &tle);
         for( i = 0; i < n_steps; i++)
            {
            double ra, dec, dist_to_satellite;
            double pos[3]; /* Satellite position vector */
            double t_since = (double)( i - n_steps / 2) * step_size;
            double jd = tle.epoch + t_since;

            t_since *= 1440.;
            if( is_deep)
               err_val = SDP4( t_since, &tle, sat_params, pos, NULL);
            else
               err_val = SGP4( t_since, &tle, sat_params, pos, NULL);
            if( err_val)
               printf( "Ephemeris error %d\n", err_val);
            get_satellite_ra_dec_delta( observer_loc, pos,
                                 &ra, &dec, &dist_to_satellite);
            epoch_of_date_to_j2000( jd, &ra, &dec);
            printf( "%-14sC%13.5lf    %08.4lf    %+08.4lf",
                     intl_id, jd, ra * 180. / PI, dec * 180. / PI);
            printf( "                    TLEs 500\n");
            }
         }
      strcpy( line1, line2);
      }
   fclose( ifile);
   return( 0);
} /* End of main() */

