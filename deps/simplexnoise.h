// ! This dependency was modified from the original
// ! (http://staffwww.itn.liu.se/~stegu/aqsis/aqsis-newnoise/simplexnoise1234.h)
// ! to remove uneeded functions (backdash only uses 1d noise).
//
// SimplexNoise1234
// Copyright © 2003-2011, Stefan Gustavson
//
// Contact: stegu@itn.liu.se
//
// This library is public domain software, released by the author
// into the public domain in February 2011. You may do anything
// you like with it. You may even remove all attributions,
// but of course I'd appreciate it if you kept my name somewhere.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

/** \file
		\brief Declares the SimplexNoise1234 class for producing Perlin simplex noise.
		\author Stefan Gustavson (stegu@itn.liu.se)
*/

/*
 * This is a clean, fast, modern and free Perlin Simplex noise class in C++.
 * Being a stand-alone class with no external dependencies, it is
 * highly reusable without source code modifications.
 *
 *
 * Note:
 * Replacing the "float" type with "double" can actually make this run faster
 * on some platforms. A templatized version of SimplexNoise1234 could be useful.
 */

class SNoise {

  public:
    SNoise() {}
    ~SNoise() {}

/** 1D float Perlin noise
 */
    static double noise( double x );

/** 1D float Perlin noise, with a specified integer period
 */
    //static float pnoise( float x, int px );

  private:
    static unsigned char perm[];
    static double grad( int hash, double x );
};

