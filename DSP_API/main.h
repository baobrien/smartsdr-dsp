/* *****************************************************************************
 *	main.h															2014 AUG 23
 *
 *  Author: Graham / KE9H
 *
 *	Date created: August 5, 2014
 *
 *	Wrapper program for "Embedded FreeDV" including CODEC2
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

#ifndef FDV_MAIN_H_
#define FDV_MAIN_H_

#include "freedv_api.h"


typedef struct {
	char	fdv_mode_name[7];		//FreeDV mode string
	int		fdv_mode_idx;			//FreeDV API mode index
	int		lower_sideband_allowed;	//Allow LSB operation of this mode
	int 	filter_low;				//Low end of rx filter
	int		filter_high;			//High end of rx filter
} freedv_mode_info_t;

const int FreeDV_modes_count;

const freedv_mode_info_t FreeDV_modes[4];

void freedv_set_string(uint32 slice, char* string);
void freedv_set_mode(uint32 slice, int mode);
void freedv_setup_filter(uint32 slice);

#endif /* MAIN_H_ */
