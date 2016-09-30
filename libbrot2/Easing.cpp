/* Robert Penner's easing equations.
 * See http://robertpenner.com/easing/ for details.
 * C++ port from https://github.com/jesusgollonet/ofpennereasing/
 * Fixes for g++ 5.4 with -Werror=sequence-point by Ross Younger.
 */

/*
   TERMS OF USE - EASING EQUATIONS

   Open source under the BSD License.

   Copyright © 2001 Robert Penner
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

   Neither the name of the author nor the names of contributors may be used to
   endorse or promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
 */

#include "Easing.h"
#include <cmath>

float Cubic::easeIn (float t, float b, float c, float d) {
	t/=d;
	return c*t*t*t + b;
}
float Cubic::easeOut(float t, float b, float c, float d) {
	t=t/d-1;
	return c*(t*t*t + 1) + b;
}

float Cubic::easeInOut(float t, float b, float c, float d) {
	t/=d/2;
	if (t < 1)
		return c/2*t*t*t + b;
	t-=2;
	return c/2*(t*t*t + 2) + b;
}

float Linear::easeIn (float t, float b, float c, float d) {
	return c*t/d + b;
}
float Linear::easeOut(float t, float b, float c, float d) {
	return c*t/d + b;
}
float Linear::easeInOut(float t, float b, float c, float d) {
	return c*t/d + b;
}

float Sine::easeIn (float t,float b , float c, float d) {
		return -c * cos(t/d * (M_PI/2)) + c + b;
}
float Sine::easeOut(float t,float b , float c, float d) {
		return c * sin(t/d * (M_PI/2)) + b;
}

float Sine::easeInOut(float t,float b , float c, float d) {
		return -c/2 * (cos(M_PI*t/d) - 1) + b;
}

/* The discrete speed of the function is the distance between two successive steps */
float Sine::SpeedIn(float t, float c, float d) {
	return Sine::easeIn(t+1,0,c,d) - Sine::easeIn(t,0,c,d);
}
float Sine::SpeedOut(float t, float c, float d) {
	return Sine::easeOut(t+1,0,c,d) - Sine::easeOut(t,0,c,d);
}
float Sine::SpeedInOut(float t, float c, float d) {
	return Sine::easeInOut(t+1,0,c,d) - Sine::easeInOut(t,0,c,d);
}