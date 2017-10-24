/* *****************************************************************************
 *	circular_buffer.h									  			2014 AUG 23
 *
 * 	General Purpose Circular Buffer Function Set
 *		Includes Circular Buffers for floats and shorts
 *
 *  Created on: Aug 21, 2014
 *      Author: Graham / KE9H
 *
 * *****************************************************************************
 *
 *	Copyright (C) 2014 FlexRadio Systems.
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 *	You should have received a copy of the GNU General Public License
 *	along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 **************************************************************************** */


#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_


#include <stdbool.h>
#include <complex.h>

/* Circular buffer objects */
typedef struct {
    unsigned int   size;    // Maximum number of elements + 1
    unsigned int   start;   // Index of oldest element
    unsigned int   end;     // Index at which to write new element
    short         *elems;   // Vector of elements
} circular_short_buffer, *Circular_Short_Buffer;

typedef struct {
    unsigned int   size;    // Maximum number of elements + 1
    unsigned int   start;   // Index of oldest element
    unsigned int   end;     // Index at which to write new element
    float         *elems;   // Vector of elements
} circular_float_buffer, *Circular_Float_Buffer;

typedef struct {
    unsigned int   size;    // Maximum number of elements + 1
    unsigned int   start;   // Index of oldest element
    unsigned int   end;     // Index at which to write new element
    Complex       *elems;   // Vector of elements
} circular_comp_buffer, *Circular_Comp_Buffer;

/* *****************************************************************************
 *  Prototype Declarations
 */

//  Returns TRUE if buffer is full
bool cfbIsFull(Circular_Float_Buffer cb);
bool csbIsFull(Circular_Short_Buffer cb);
bool ccbIsFull(Circular_Comp_Buffer cb);

//  Returns TRUE if buffer is empty
bool cfbIsEmpty(Circular_Float_Buffer cb);
bool csbIsEmpty(Circular_Short_Buffer cb);
bool ccbIsEmpty(Circular_Comp_Buffer cb);

//  Write an element, overwriting oldest element if buffer is full.
//  App can choose to avoid the overwrite by checking cbIsFull().
void cbWriteFloat(Circular_Float_Buffer cb, float sample);
void cbWriteShort(Circular_Short_Buffer cb, short sample);
void cbWriteComp(Circular_Comp_Buffer cb, Complex sample);

//Read oldest element. App must ensure !cbIsEmpty() first.
float cbReadFloat(Circular_Float_Buffer cb);
short cbReadShort(Circular_Short_Buffer cb);
Complex cbReadComp(Circular_Comp_Buffer cb);

// Clear buffer
void zero_cfb(Circular_Float_Buffer cb);
void zero_csb(Circular_Short_Buffer cb);
void zero_ccb(Circular_Comp_Buffer cb);

// Returns number of samples in buffer
int cfbContains(Circular_Float_Buffer cb);
int csbContains(Circular_Short_Buffer cb);
int ccbContains(Circular_Comp_Buffer cb);

Circular_Float_Buffer cfbCreate(size_t size);
Circular_Short_Buffer csbCreate(size_t size);
Circular_Comp_Buffer  ccbCreate(size_t size);

void cfbDestroy(Circular_Float_Buffer buff);
void csbDestroy(Circular_Short_Buffer buff);
void ccbDestroy(Circular_Comp_Buffer  buff);


#endif /* CIRCULAR_BUFFER_H_ */

// EoF =====-----

