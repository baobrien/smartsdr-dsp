/* *****************************************************************************
 *	circular_float_buffer.c											2014 AUG 20
 *
 *	General Purpose Circular Buffer Function Set
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
 *******************************************************************************
 *
 *	Example usage:
 *
 *	Circular buffer object, declared in circular_buffer.h
 *
 * 		typedef struct {
 *			unsigned int  size;		// Maximum number of elements + 1
 *			unsigned int  start;	// Index of oldest element
 *			unsigned int  end;		// Index at which to write new element
 *			unsigned char *elems;	// Vector of elements
 *		} circular_buffer, *Circular_Buffer;
 *
 *
 *	Circular Buffer Declaration
 *
 *		unsigned char CL_Buff[2048];		// Example: Command Line Buffer
 *		Note: Make sure declaration matches data size
 *
 *		circular_buffer CommandLine_cb;
 *
 *		Circular_Buffer CL_cb = &CommandLine_cb;
 *
 *
 *	Initialize circular buffer for Command Line buffer
 *	// Includes one empty element
 *		CL_cb->size	 = 2048;		// size = no.elements in array
 *		CL_cb->start = 0;
 *		CL_cb->end	 = 0;
 *		CL_cb->elems = CL_Buff;
 *
 *	Call Read or Write function, appropriate to the data size
 *
 **************************************************************************** */


#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "circular_buffer.h"



/* *****************************************************************************
 *	BOOL cfbIsFull(Circular_Float_Buffer cb)
 *		Test to check if the buffer is FULL
 *		Returns TRUE if full
 */

bool cfbIsFull(Circular_Float_Buffer cb)
{
	return ((cb->end + 1) % cb->size == cb->start);
}


/* *****************************************************************************
 *	BOOL cfbIsEmpty(Circular_Float_Buffer cb)
 *		Test to check if the buffer is EMPTY
 *		Returns TRUE if empty
 */

bool cfbIsEmpty(Circular_Float_Buffer cb)
{
	return cb->end == cb->start;
}


/* *****************************************************************************
 *	void cbWriteChar(Circular_Buffer cb, unsigned char temp)
 *		Write an element, overwriting oldest element if buffer is full.
 *		App can choose to avoid the overwrite by checking cbIsFull().
 */

void cbWriteFloat(Circular_Float_Buffer cb, float sample)
{
	cb->elems[cb->end] = sample;
	cb->end = (cb->end + 1) % cb->size;
	if (cb->end == cb->start)
		cb->start = (cb->start + 1) % cb->size;		/* full, overwrite */
}


/* *****************************************************************************
 *	unsigned char cbReadChar(CircularBuffer *cb)
 *		Returns oldest element.
 *		Calling function must ensure [cbIsEmpty() != TRUE], first.
 */

float cbReadFloat(Circular_Float_Buffer cb)
{
	float temp;

	temp = cb->elems[cb->start];
	cb->start = (cb->start + 1) % cb->size;
	return temp;
}


/* *****************************************************************************
 *	void zero_cfb(Circular_Float_Buffer cb)
 *
 *		Empties circular buffer
 */

void zero_cfb(Circular_Float_Buffer cb)
{
	cb->start = 0;
	cb->end	 = 0;
}


/* *****************************************************************************
 *	int cfbContains(Circular_Float_Buffer cb)
 *
 *		Returns the number of samples in the circular buffer
 *
 */

int cfbContains(Circular_Float_Buffer cb)
{
	int contains;

	contains = (cb->end - cb->start);
	if (contains < 0)
		contains += cb->size;

	return contains;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  SHORT BUFFER ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


/* *****************************************************************************
 *	BOOL cfbIsFull(Circular_Float_Buffer cb)
 *		Test to check if the buffer is FULL
 *		Returns TRUE if full
 */

bool csbIsFull(Circular_Short_Buffer cb)
{
	return ((cb->end + 1) % cb->size == cb->start);
}


/* *****************************************************************************
 *	BOOL csbIsEmpty(Circular_Short_Buffer cb)
 *		Test to check if the buffer is EMPTY
 *		Returns TRUE if empty
 */

bool csbIsEmpty(Circular_Short_Buffer cb)
{
	return cb->end == cb->start;
}


/* *****************************************************************************
 *	void cbWriteChar(Circular_Buffer cb, unsigned char temp)
 *		Write an element, overwriting oldest element if buffer is full.
 *		App can choose to avoid the overwrite by checking cbIsFull().
 */

void cbWriteShort(Circular_Short_Buffer cb, short sample)
{
	cb->elems[cb->end] = sample;
	cb->end = (cb->end + 1) % cb->size;
	if (cb->end == cb->start)
		cb->start = (cb->start + 1) % cb->size;		/* full, overwrite */
}


/* *****************************************************************************
 *	unsigned char cbReadShort(Circular_Short_Buffer *cb)
 *		Returns oldest element.
 *		Calling function must ensure [csbIsEmpty() != TRUE], first.
 */

short cbReadShort(Circular_Short_Buffer cb)
{
	short temp;

	temp = cb->elems[cb->start];
	cb->start = (cb->start + 1) % cb->size;
	return temp;
}


/* *****************************************************************************
 *	void zero_cfb(Circular_Short_Buffer cb)
 *
 *		Empties circular buffer
 */

void zero_csb(Circular_Short_Buffer cb)
{
	cb->start = 0;
	cb->end	 = 0;
}


/* *****************************************************************************
 *	int csbContains(Circular_Short_Buffer cb)
 *
 *		Returns the number of samples in the circular buffer
 *
 */

int csbContains(Circular_Short_Buffer cb)
{
	int contains;

	contains = (cb->end - cb->start);
	if (contains < 0)
		contains += cb->size;

	return contains;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  Complex BUFFER ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


/* *****************************************************************************
 *	BOOL cfbIsFull(Circular_Comp_Buffer cb)
 *		Test to check if the buffer is FULL
 *		Returns TRUE if full
 */

bool ccbIsFull(Circular_Comp_Buffer cb)
{
	return ((cb->end + 1) % cb->size == cb->start);
}


/* *****************************************************************************
 *	BOOL ccbIsEmpty(Circular_Comp_Buffer cb)
 *		Test to check if the buffer is EMPTY
 *		Returns TRUE if empty
 */

bool ccbIsEmpty(Circular_Comp_Buffer cb)
{
	return cb->end == cb->start;
}


/* *****************************************************************************
 *	void cbWriteChar(Circular_Buffer cb, unsigned char temp)
 *		Write an element, overwriting oldest element if buffer is full.
 *		App can choose to avoid the overwrite by checking cbIsFull().
 */

void cbWriteComp(Circular_Comp_Buffer cb, Complex sample)
{
	cb->elems[cb->end] = sample;
	cb->end = (cb->end + 1) % cb->size;
	if (cb->end == cb->start)
		cb->start = (cb->start + 1) % cb->size;		/* full, overwrite */
}


/* *****************************************************************************
 *	unsigned char cbReadComp(Circular_Comp_Buffer *cb)
 *		Returns oldest element.
 *		Calling function must ensure [ccbIsEmpty() != TRUE], first.
 */

Complex cbReadComp(Circular_Comp_Buffer cb)
{
	Complex temp;

	temp = cb->elems[cb->start];
	cb->start = (cb->start + 1) % cb->size;
	return temp;
}


/* *****************************************************************************
 *	void zero_cfb(Circular_Comp_Buffer cb)
 *
 *		Empties circular buffer
 */

void zero_ccb(Circular_Comp_Buffer cb)
{
	cb->start = 0;
	cb->end	 = 0;
}


/* *****************************************************************************
 *	int ccbContains(Circular_Comp_Buffer cb)
 *
 *		Returns the number of samples in the circular buffer
 *
 */

int ccbContains(Circular_Comp_Buffer cb)
{
	int contains;

	contains = (cb->end - cb->start);
	if (contains < 0)
		contains += cb->size;

	return contains;
}


Circular_Float_Buffer cfbCreate(size_t size){
	float * elems = malloc(size*sizeof(float));
	if(elems == NULL) return NULL;

	Circular_Float_Buffer cfb = malloc(sizeof(circular_float_buffer));
	if(cfb == NULL){
		free(elems);
		return NULL;
	}
	cfb->elems = elems;
	cfb->size = size;
	cfb->start = 0;
	cfb->end = 0;

	return cfb;
}

Circular_Short_Buffer csbCreate(size_t size){
	short * elems = malloc(size*sizeof(short));
	if(elems == NULL) return NULL;

	Circular_Short_Buffer csb = malloc(sizeof(circular_short_buffer));
	if(csb == NULL){
		free(elems);
		return NULL;
	}
	csb->elems = elems;
	csb->size = size;
	csb->start = 0;
	csb->end = 0;

	return csb;
}

Circular_Comp_Buffer  ccbCreate(size_t size){
	Complex * elems = malloc(size*sizeof(Complex));
	if(elems == NULL) return NULL;

	Circular_Comp_Buffer ccb = malloc(sizeof(circular_comp_buffer));
	if(ccb == NULL){
		free(elems);
		return NULL;
	}
	ccb->elems = elems;
	ccb->size = size;
	ccb->start = 0;
	ccb->end = 0;

	return ccb;
}

static void cbDestroy(void * circbuf, void * elems){
	if(circbuf != NULL) free(circbuf);
	if(elems   != NULL) free(elems);
}

void cfbDestroy(Circular_Float_Buffer buff){
	void * elems = (void*) buff->elems;
	cbDestroy((void*) buff, elems);
}

void csbDestroy(Circular_Short_Buffer buff){
	void * elems = (void*) buff->elems;
	cbDestroy((void*) buff, elems);
}

void ccbDestroy(Circular_Comp_Buffer  buff){
	void * elems = (void*) buff->elems;
	cbDestroy((void*) buff, elems);
}

// EoF =====-----

