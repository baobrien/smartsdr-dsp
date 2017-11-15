///*!   \file sched_waveform.c
// *    \brief Schedule Wavefrom Streams
// *
// *    \copyright  Copyright 2012-2014 FlexRadio Systems.  All Rights Reserved.
// *                Unauthorized use, duplication or distribution of this software is
// *                strictly prohibited by law.
// *
// *    \date 29-AUG-2014
// *    \author 	Ed Gonzalez
// *    \mangler 	Graham / KE9H
// *
// */

/* *****************************************************************************
 *
 *  Copyright (C) 2014 FlexRadio Systems.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *  Contact Information:
 *  email: gpl<at>flexradiosystems.com
 *  Mail:  FlexRadio Systems, Suite 1-150, 4616 W. Howard LN, Austin, TX 78728
 *
 * ************************************************************************** */

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>		// for memset
#include <unistd.h>
#include <complex.h>

#include "common.h"
#include "datatypes.h"
#include "hal_buffer.h"
#include "sched_waveform.h"
#include "vita_output.h"
#include "freedv_api_internal.h"
#include "fsk.h"

//static Queue sched_fft_queue;
static pthread_rwlock_t _list_lock;
static BufferDescriptor _root;

static pthread_t _waveform_thread;
static BOOL _waveform_thread_abort = FALSE;

static sem_t sched_waveform_sem;


static void _dsp_convertBufEndian(BufferDescriptor buf_desc)
{
	int i;

	if(buf_desc->sample_size != 8)
	{
		//TODO: horrendous error here
		return;
	}

	for(i = 0; i < buf_desc->num_samples*2; i++)
		((int32*)buf_desc->buf_ptr)[i] = htonl(((int32*)buf_desc->buf_ptr)[i]);
}

static BufferDescriptor _WaveformList_UnlinkHead(void)
{
	BufferDescriptor buf_desc = NULL;
	pthread_rwlock_wrlock(&_list_lock);

	if (_root == NULL || _root->next == NULL)
	{
		output("Attempt to unlink from a NULL head");
		pthread_rwlock_unlock(&_list_lock);
		return NULL;
	}

	if(_root->next != _root)
		buf_desc = _root->next;

	if(buf_desc != NULL)
	{
		// make sure buffer exists and is actually linked
		if(!buf_desc || !buf_desc->prev || !buf_desc->next)
		{
			output( "Invalid buffer descriptor");
			buf_desc = NULL;
		}
		else
		{
			buf_desc->next->prev = buf_desc->prev;
			buf_desc->prev->next = buf_desc->next;
			buf_desc->next = NULL;
			buf_desc->prev = NULL;
		}
	}

	pthread_rwlock_unlock(&_list_lock);
	return buf_desc;
}

static void _WaveformList_LinkTail(BufferDescriptor buf_desc)
{
	pthread_rwlock_wrlock(&_list_lock);
	buf_desc->next = _root;
	buf_desc->prev = _root->prev;
	_root->prev->next = buf_desc;
	_root->prev = buf_desc;
	pthread_rwlock_unlock(&_list_lock);
}

void sched_waveform_Schedule(BufferDescriptor buf_desc)
{
	_WaveformList_LinkTail(buf_desc);
	sem_post(&sched_waveform_sem);
}

void sched_waveform_signal()
{
	sem_post(&sched_waveform_sem);
}


/* *********************************************************************************************
 * *********************************************************************************************
 * *********************                                                 ***********************
 * *********************  LOCATION OF MODULATOR / DEMODULATOR INTERFACE  ***********************
 * *********************                                                 ***********************
 * *********************************************************************************************
 * ****************************************************************************************** */

#include <stdio.h>
#include "freedv_api.h"
#include "circular_buffer.h"
#include "resampler.h"
#include "comp_prim.h"

#define PACKET_SAMPLES  128

#define SCALE_RX_IN  	 8000.0 	// Multiplier   // Was 16000 GGH Jan 30, 2015
#define SCALE_RX_OUT     8000.0		// Divisor
#define SCALE_TX_IN     24000.0 	// Multiplier   // Was 16000 GGH Jan 30, 2015
#define SCALE_TX_OUT    24000.0 	// Divisor

#define FILTER_TAPS	48
#define DECIMATION_FACTOR 	3

/* These are offsets for the input buffers to decimator */
#define MEM_24		FILTER_TAPS					   /* Memory required in 24kHz buffer */
#define MEM_8		FILTER_TAPS/DECIMATION_FACTOR   /* Memory required in 8kHz buffer */

#define UPPER_SIDEBAND 0
#define LOWER_SIDEBAND 1

static struct freedv *_freedvS	= NULL;         // Initialize Coder structure
static int fdv_mode = FREEDV_MODE_1600;			// FreeDV Mode
static int sideband_mode = UPPER_SIDEBAND;
static struct my_callback_state  _my_cb_state;
#define MAX_RX_STRING_LENGTH 40
static char _rx_string[MAX_RX_STRING_LENGTH + 5];

static BOOL _end_of_transmission = FALSE;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	Circular Buffer Declarations

Circular_Float_Buffer RX1_cb;
Circular_Float_Buffer RX2_cb;
Circular_Short_Buffer RX3_cb;
Circular_Float_Buffer RX4_cb;

Circular_Float_Buffer TX1_cb;
Circular_Short_Buffer TX2_cb;
Circular_Comp_Buffer  TX3_cb;
Circular_Comp_Buffer  TX4_cb;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Callbacks for embedded ASCII stream, transmit and receive

void my_put_next_rx_char(void *callback_state, char c)
{
    char new_char[2];
    if ( (uint32) c < 32 || (uint32) c > 126 ) {
    	/* Treat all control chars as spaces */
    	//output(ANSI_YELLOW "Non-valid RX_STRING char. ASCII code = %d\n", (uint32) c);
    	new_char[0] = (char) 0x7F;
    } else if ( c == ' ' ) {
    	/* Encode spaces differently */
    	new_char[0] = (char) 0x7F;
    } else {
    	new_char[0] = c;
    }

    new_char[1] = 0;

    strncat(_rx_string, new_char, MAX_RX_STRING_LENGTH+4);
    if (strlen(_rx_string) > MAX_RX_STRING_LENGTH)
    {
        // lop off first character
        strcpy(_rx_string, _rx_string+1);
    }
    //output(ANSI_MAGENTA "new string = '%s'\n",_rx_string);

    char* api_cmd = safe_malloc(80);
    sprintf(api_cmd, "waveform status slice=%d string=\"%s\"",0,_rx_string);
    tc_sendSmartSDRcommand(api_cmd,FALSE,NULL);
    safe_free(api_cmd);
}

struct my_callback_state
{
    char  tx_str[80];
    char *ptx_str;
};

char my_get_next_tx_char(void *callback_state)
{
    struct my_callback_state* pstate = (struct my_callback_state*)callback_state;
    char  c = *pstate->ptx_str++;

    if (*pstate->ptx_str == 0)
    {
        pstate->ptx_str = pstate->tx_str;
    }

    return c;
}

void freedv_set_string(uint32 slice, char* string)
{
    strcpy(_my_cb_state.tx_str, string);
    _my_cb_state.ptx_str = _my_cb_state.tx_str;
    output(ANSI_MAGENTA "new TX string is '%s'\n",string);
}

void sched_waveform_setEndOfTX(BOOL end_of_transmission)
{
    _end_of_transmission = TRUE;
}


// Set FDV sideband. 'U' for USB, 'L' for LSB.
void freedv_set_sideband(uint32 slice,char sideband){
	size_t i;
	freedv_mode_info_t modeinfo = {"", -1, 0, 0, 0};
	for(i=0; i<FreeDV_modes_count; i++){
		if( FreeDV_modes[i].fdv_mode_idx == fdv_mode ){
			modeinfo = FreeDV_modes[i];
		}
	}

	if(sideband == 'L' && modeinfo.lower_sideband_allowed){
		sideband_mode = LOWER_SIDEBAND;
	}
	if(sideband == 'U'){
		sideband_mode = UPPER_SIDEBAND;
	}
	freedv_setup_filter(slice);
}

// Set rx filter to match FreeDV mode
void freedv_setup_filter(uint32 slice){
	size_t i;
	int filter_low, filter_high, t;
	freedv_mode_info_t modeinfo = {"", -1, 0, 0, 0};
	for(i=0; i<FreeDV_modes_count; i++){
		if( FreeDV_modes[i].fdv_mode_idx == fdv_mode ){
			modeinfo = FreeDV_modes[i];
		}
	}
	if(modeinfo.fdv_mode_idx<0)
		return;

	filter_low = modeinfo.filter_low;
	filter_high = modeinfo.filter_high;
	//set filtering for LSB
	if(sideband_mode == LOWER_SIDEBAND && modeinfo.lower_sideband_allowed){
		t = filter_low;
		filter_low = -filter_high;
		filter_high = -t;
	}

	if(sideband_mode == LOWER_SIDEBAND && !modeinfo.lower_sideband_allowed){
		sideband_mode = UPPER_SIDEBAND;
	}
    char api_cmd[80];

    snprintf(api_cmd, 80, "filt %d %d %d",slice,filter_low,filter_high);
    tc_sendSmartSDRcommand(api_cmd,FALSE,NULL);

}

void freedv_set_mode(uint32 slice, int mode){
	size_t i;
	for(i=0; i<FreeDV_modes_count; i++){
		if( FreeDV_modes[i].fdv_mode_idx == mode ){
			fdv_mode = mode;
		    output(ANSI_MAGENTA "FreeDV mode set to '%s'\n",FreeDV_modes[i].fdv_mode_name);
		    return;
		}
	}

	output(ANSI_RED "Invalid FreeDV mode %d\n",mode);

}

// Send a mode string status update to whatever's listening
static void _sched_waveform_send_mode_status(){

	size_t i;
	freedv_mode_info_t modeinfo = {"", -1, 0, 0, 0};
	for(i=0; i<FreeDV_modes_count; i++){
		if( FreeDV_modes[i].fdv_mode_idx == fdv_mode ){
			modeinfo = FreeDV_modes[i];
		}
	}

    char api_cmd[80];
    char* sideband = (sideband_mode == UPPER_SIDEBAND)?"U":"L";

    snprintf(api_cmd, 80, "waveform status slice=%d fdvmode=\"%s%s\"",0,sideband,modeinfo.fdv_mode_name);
    tc_sendSmartSDRcommand(api_cmd,FALSE,NULL);

}

//Set up or change the FreeDV struct after mode change
static void _sched_waveform_change_fdv_mode(){
	//Default to 1600 if we're not in an implemented or valid mode
	if(! ( fdv_mode == FREEDV_MODE_1600 || fdv_mode == FREEDV_MODE_700C || fdv_mode == FREEDV_MODE_2400A)){
		fdv_mode = FREEDV_MODE_1600;
	}
	// Tear down FreeDV struct if it's already in place
	if(_freedvS != NULL){
		freedv_close(_freedvS);
	}

	output(ANSI_MAGENTA "Changing FDV mode to %d\n",fdv_mode);
	_freedvS = freedv_open(fdv_mode);

	if( fdv_mode == FREEDV_MODE_2400A ){
		freedv_set_alt_modem_samp_rate(_freedvS,24000);
		//fsk_set_est_limits(_freedvS->fsk,-3000,3000);
		_freedvS->fsk->f1_tx = -1800;
	}

    freedv_set_callback_txt(_freedvS, my_put_next_rx_char, my_get_next_tx_char, &_my_cb_state);
}

static void* _sched_waveform_thread(void* param)
{
    int 	nin, nout;

    int		i;			// for loop counter
    float	fsample;	// a float sample
    Complex	cssample;
//    float   Sig2Noise;	// Signal to noise ratio

    // Flags ...
    int		initial_tx = 1; 		// Flags for TX circular buffer, clear if starting transmit
    int		initial_rx = 1;			// Flags for RX circular buffer, clear if starting receive

    int				mode_status_countdown = 0;	// Timer to keep track of loop iterations before re-sending the FDV mode status -- NOTE: only deincremented during freedv rx or tx
    const int		mode_status_time	  = 25; // How may loop iterations before resending mode status

    // RX RESAMPLER I/O BUFFERS
    float 	float_in_8k[PACKET_SAMPLES + FILTER_TAPS];
    float 	float_out_8k[PACKET_SAMPLES];

    float 	float_in_24k[PACKET_SAMPLES * DECIMATION_FACTOR + FILTER_TAPS];
    float 	float_out_24k[PACKET_SAMPLES * DECIMATION_FACTOR ];

    // TX RESAMPLER I/O BUFFERS
    float 	tx_float_in_8k_r[PACKET_SAMPLES + FILTER_TAPS];
    float 	tx_float_in_8k_i[PACKET_SAMPLES + FILTER_TAPS];
    float 	tx_float_out_8k[PACKET_SAMPLES];

    float 	tx_float_in_24k[PACKET_SAMPLES * DECIMATION_FACTOR + FILTER_TAPS];
    float 	tx_float_out_24k_r[PACKET_SAMPLES * DECIMATION_FACTOR ];
    float 	tx_float_out_24k_i[PACKET_SAMPLES * DECIMATION_FACTOR ];

    float	tx_float_cleanup_r[PACKET_SAMPLES * DECIMATION_FACTOR + FILTER_TAPS];
    float	tx_float_cleanup_i[PACKET_SAMPLES * DECIMATION_FACTOR + FILTER_TAPS];

    BOOL inhibit_tx = FALSE;
    BOOL flush_tx = FALSE;


    // =======================  Initialization Section =========================

    // Initialize the Circular Buffers

	RX1_cb = cfbCreate(PACKET_SAMPLES*24 +1);
	RX2_cb = cfbCreate(PACKET_SAMPLES*16 +1);
	RX3_cb = csbCreate(PACKET_SAMPLES*16 +1);
	RX4_cb = cfbCreate(PACKET_SAMPLES*24 +1);

	TX1_cb = cfbCreate(PACKET_SAMPLES*18 +1);
	TX2_cb = csbCreate(PACKET_SAMPLES*12 +1);
	TX3_cb = ccbCreate(PACKET_SAMPLES*12 +1);
	TX4_cb = ccbCreate(PACKET_SAMPLES*18 +1);

	initial_tx = TRUE;
	initial_rx = TRUE;

    fdv_mode = FREEDV_MODE_700C;
    sideband_mode = UPPER_SIDEBAND;
    int fdv_mode_old = fdv_mode;
    _sched_waveform_change_fdv_mode();

    // Set up callback for txt msg chars
    // clear tx_string
    memset(_my_cb_state.tx_str,0,80);
    _my_cb_state.ptx_str = _my_cb_state.tx_str;

    uint32 bypass_count = 0;
    BOOL bypass_demod = TRUE;

    // In 2400A, we don't convert RF to/from 8KHz
    BOOL bypass_8kconv = FALSE;

    // NCO to shift 2400A waveform up in frequency because the modem doesn't like negative frequencies
    const float fshift_2400a = -3000.0;
    COMP dphi_nco_2400a = comp_exp_j(fshift_2400a*2.0*M_PI/24000.0);
    COMP nco_2400a;
    nco_2400a.real = 1;
    nco_2400a.imag = 0;

	// show that we are running
	BufferDescriptor buf_desc;

	while( !_waveform_thread_abort )
	{
		// wait for a buffer descriptor to get posted
		sem_wait(&sched_waveform_sem);
		if(!_waveform_thread_abort)
		{
			do {
				// change freedv mode when freedv mode is changed
				if(fdv_mode != fdv_mode_old){
					_sched_waveform_change_fdv_mode();
				}
				fdv_mode_old = fdv_mode;
				if(fdv_mode == FREEDV_MODE_2400A){
					bypass_8kconv = TRUE;
				} else {
					bypass_8kconv = FALSE;
				}

				// speech/mod buffers moved into main loop to allow for n_sample changes
			    size_t freedv_nsamp_audio = freedv_get_n_speech_samples(_freedvS);
			    size_t freedv_nsamp_rf = freedv_get_n_max_modem_samples(_freedvS);

				// VOCODER I/O BUFFERS
			    short	speech_in[freedv_nsamp_audio];
			    short 	speech_out[freedv_nsamp_audio];
			    float 	demod_in[freedv_nsamp_rf];
			    COMP 	mod_out[freedv_nsamp_rf];

				//while(sem_trywait(&sched_waveform_sem) == 0);

				buf_desc = _WaveformList_UnlinkHead();
				//Zero out semaphore so we don't get stuck in this while loop and accumulate a lrage count

				// if we got signalled, but there was no new data, something's wrong
				// and we'll just wait for the next packet
				if (buf_desc == NULL)
				{
					//output( "We were signaled that there was another buffer descriptor, but there's not one here");
					break;
				}
				else
				{
					// convert the buffer to little endian
					_dsp_convertBufEndian(buf_desc);


					if( (buf_desc->stream_id & 1) == 0) { //RX BUFFER


						//	If 'initial_rx' flag, clear buffers RX1, RX2, RX3, RX4
						if(initial_rx)
						{
							RX1_cb->start = 0;	// Clear buffers RX1, RX2, RX3, RX4
							RX1_cb->end	  = 0;
							RX2_cb->start = 0;
							RX2_cb->end	  = 0;
							RX3_cb->start = 0;
							RX3_cb->end	  = 0;
							RX4_cb->start = 0;
							RX4_cb->end	  = 0;


							/* Clear filter memory */
							memset(float_in_24k, 0, MEM_24 * sizeof(float));
							memset(float_in_8k, 0, MEM_8 * sizeof(float));

							/* Requires us to set initial_rx to FALSE which we do at the end of
							 * the first loop
							 */
						}

						//	Set the transmit 'initial' flag
						initial_tx = TRUE;
						inhibit_tx = FALSE;
						flush_tx = FALSE;
						_end_of_transmission = FALSE;
						// Check for new receiver input packet & move to RX1_cb.
						// TODO - If transmit packet, discard here?

						// Since we're just converting to real and the spectrum is already filtered, we can just cheat
						// with LSB conversion
						for( i = 0 ; i < PACKET_SAMPLES ; i++)
						{
							//output("Outputting ")
							//	fsample = Get next float from packet;
							COMP csample = ((COMP*)buf_desc->buf_ptr)[i];
							// Shift the frequency if we're in 2400A
							if(fdv_mode == FREEDV_MODE_2400A){
								nco_2400a = cmult(nco_2400a,dphi_nco_2400a);
								csample = cmult(csample,nco_2400a);
							}
							cbWriteFloat(RX1_cb, csample.real);

						}
						//Normalize the NCO for stability if we're in 2400A mode
						if(fdv_mode == FREEDV_MODE_2400A){
							nco_2400a = comp_normalize(nco_2400a);
						}
//
						// Check for >= 384 samples in RX1_cb and spin downsampler
						//	Convert to shorts and move to RX2_cb.
						while(cfbContains(RX1_cb) >= 384)
						{
							if(!bypass_8kconv){
								for(i=0 ; i<384 ; i++)
								{
									float_in_24k[i + MEM_24] = cbReadFloat(RX1_cb);
								}

								fdmdv_24_to_8(float_out_8k, &float_in_24k[MEM_24], 128);

								for(i=0 ; i<128 ; i++)
								{
									cbWriteFloat(RX2_cb, float_out_8k[i]);

								}
							}else{
								for(i=0 ; i<384 ; i++)
								{
									cbWriteFloat(RX2_cb,cbReadFloat(RX1_cb));
								}
							}

						}
//
//						// Check for >= 320 samples in RX2_cb and spin vocoder
						// 	Move output to RX3_cb.
						//do {
							// freedv_nin is really necessary as nin may change between freedv_rx calls
							// the modem uses nin changes to account for differences in sample clock rates
							nin = freedv_nin(_freedvS); // TODO Is nin, nout really necessary?
							int n_speech = freedv_get_n_speech_samples(_freedvS);

							if ( cfbContains(RX2_cb) >= nin )
							{
	//
								for( i=0 ; i< nin ; i++)
								{
									demod_in[i] = cbReadFloat(RX2_cb);
								}

								nout = freedv_floatrx(_freedvS, speech_out, demod_in);


								if ( freedv_get_sync(_freedvS) ) {
									/* Increase count for turning bypass off */
									if ( bypass_count < 10) bypass_count++;
								} else {
									if ( bypass_count > 0 ) bypass_count--;
								}

								if ( bypass_count > 7 ) {
									//if ( bypass_demod ) output("baypass_demod transitioning to FALSE\n");

									bypass_demod = FALSE;
								}
								else if ( bypass_count < 2 ) {
									//if ( !bypass_demod ) output("baypass_demod transitioning to TRUE \n");
									bypass_demod = TRUE;
								}

								if ( bypass_demod && !bypass_8kconv) {
									for ( i = 0 ; i < nin ; i++ ) {
										cbWriteShort(RX3_cb, (short)(demod_in[i] * SCALE_RX_OUT));
									}
								} else if ( bypass_demod && bypass_8kconv) {
									/* TEMPORARY */
									for ( i = 0 ; i < n_speech ; i++ ) {
										cbWriteShort(RX3_cb, 0);
									}
								} else {
									for( i=0 ; i < nout ; i++)
									{
										cbWriteShort(RX3_cb, speech_out[i]);
									}
								}

								// Do mode status string send
								mode_status_countdown--;
								if(mode_status_countdown <= 0){
									mode_status_countdown = mode_status_time;
									_sched_waveform_send_mode_status();
								}


								//output("%d\n", bypass_count);

							}
							//} else {
							//	break; /* Break out of while loop */
							//}
						//} while (1);
//
						// Check for >= 128 samples in RX3_cb, convert to floats
						//	and spin the upsampler. Move output to RX4_cb.

						if(csbContains(RX3_cb) >= 128)
						{
							for( i=0 ; i<128 ; i++)
							{
								float_in_8k[i+MEM_8] = ((float)  (cbReadShort(RX3_cb) / SCALE_RX_OUT)     );
							}

							fdmdv_8_to_24(float_out_24k, &float_in_8k[MEM_8], 128);

							for( i=0 ; i<384 ; i++)
							{
								cbWriteFloat(RX4_cb, float_out_24k[i]);
							}
							//Sig2Noise = (_freedvS->fdmdv_stats.snr_est);
						}

						// Check for >= 128 samples in RX4_cb. Form packet and
						//	export.

						uint32 check_samples = PACKET_SAMPLES;

						if(initial_rx)
							check_samples = PACKET_SAMPLES * 3;

						if(cfbContains(RX4_cb) >= check_samples )
						{
							for( i=0 ; i<128 ; i++)
							{
								//output("Fetching from end buffer \n");
								// Set up the outbound packet
								fsample = cbReadFloat(RX4_cb);
								// put the fsample into the outbound packet

								((Complex*)buf_desc->buf_ptr)[i].real = fsample;
								((Complex*)buf_desc->buf_ptr)[i].imag = fsample;

							}
						} else {

							memset( buf_desc->buf_ptr, 0, PACKET_SAMPLES * sizeof(Complex));

							if(initial_rx)
								initial_rx = FALSE;
						}

						emit_waveform_output(buf_desc);

						int sval;
						//sem_getvalue(&sched_waveform_sem,&sval);
//						if(sval>0 && bypass_demod == TRUE){
//							//while(sem_trywait(&sched_waveform_sem) == 0);
//							buf_desc = _WaveformList_UnlinkHead();
//							while(buf_desc != NULL){
//								_dsp_convertBufEndian(buf_desc);
//								emit_waveform_output(buf_desc);
//								buf_desc = _WaveformList_UnlinkHead();
//								initial_rx = TRUE;
//							}
//							break;
//						}


					} else if ( (buf_desc->stream_id & 1) == 1) { //TX BUFFER

						//	If 'initial_rx' flag, clear buffers TX1, TX2, TX3, TX4
						if(initial_tx)
						{
							TX1_cb->start = 0;	// Clear buffers RX1, RX2, RX3, RX4
							TX1_cb->end	  = 0;
							TX2_cb->start = 0;
							TX2_cb->end	  = 0;
							TX3_cb->start = 0;
							TX3_cb->end	  = 0;
							TX4_cb->start = 0;
							TX4_cb->end	  = 0;


							/* Clear filter memory */

							memset(tx_float_in_24k, 0, MEM_24 * sizeof(float));
							memset(tx_float_in_8k_r, 0, MEM_8 * sizeof(float));
							memset(tx_float_in_8k_i, 0, MEM_8 * sizeof(float));

							memset(tx_float_cleanup_r, 0, MEM_24 * sizeof(float));
							memset(tx_float_cleanup_i, 0, MEM_24 * sizeof(float));

							/* Requires us to set initial_rx to FALSE which we do at the end of
							 * the first loop
							 */
						}


						initial_rx = TRUE;
						// Check for new receiver input packet & move to TX1_cb.
						// TODO - If transmit packet, discard here?

						if ( !inhibit_tx ) {
                            for( i = 0 ; i < PACKET_SAMPLES ; i++ )
                            {
                                //output("Outputting ")
                                //	fsample = Get next float from packet;
                                cbWriteFloat(TX1_cb, ((Complex*)buf_desc->buf_ptr)[i].real);

                            }

    //
                            // Check for >= 384 samples in TX1_cb and spin downsampler
                            //	Convert to shorts and move to TX2_cb.
                            if(cfbContains(TX1_cb) >= 384)
                            {
                                for(i=0 ; i<384 ; i++)
                                {
                                    tx_float_in_24k[i + MEM_24] = cbReadFloat(TX1_cb);
                                }

                                fdmdv_24_to_8(tx_float_out_8k, &tx_float_in_24k[MEM_24], 128);

                                for(i=0 ; i<128 ; i++)
                                {
                                    cbWriteShort(TX2_cb, (short) (tx_float_out_8k[i]*SCALE_TX_IN));

                                }

                            }
    //
    //						// Check for >= 320 samples in TX2_cb and spin vocoder
                            // 	Move output to TX3_cb.

                            int n_speech = freedv_get_n_speech_samples(_freedvS);
                            int n_modem_nom = freedv_get_n_nom_modem_samples(_freedvS);
							if ( csbContains(TX2_cb) >= n_speech )
							{
								for( i=0 ; i< n_speech ; i++)
								{
									speech_in[i] = cbReadShort(TX2_cb);
								}

								freedv_comptx(_freedvS, mod_out, speech_in);
								for( i=0 ; i < n_modem_nom ; i++)
								{
									cbWriteComp(TX3_cb, ((Complex*)mod_out)[i]);
								}

								// Do mode status string send
								mode_status_countdown--;
								if(mode_status_countdown <= 0){
									mode_status_countdown = mode_status_time;
									_sched_waveform_send_mode_status();
								}

							}

                            // Check for >= 128 samples in TX3_cb, convert to floats
                            //	and spin the upsampler. Move output to TX4_cb.


                            while(ccbContains(TX3_cb) >= (bypass_8kconv?384:128))
                            {
                            	if(!bypass_8kconv){
									for( i=0 ; i<128 ; i++)
									{
										cssample = cbReadComp(TX3_cb);
										tx_float_in_8k_r[i+MEM_8] = cssample.real;
										tx_float_in_8k_i[i+MEM_8] = cssample.imag;
									}

									fdmdv_8_to_24(tx_float_out_24k_r, &tx_float_in_8k_r[MEM_8], 128);
									fdmdv_8_to_24(tx_float_out_24k_i, &tx_float_in_8k_i[MEM_8], 128);

									for( i=0 ; i<384 ; i++)
									{
										if(sideband_mode == UPPER_SIDEBAND){
											cssample.real = tx_float_out_24k_r[i];
											cssample.imag = tx_float_out_24k_i[i];
										}else{
											cssample.real = tx_float_out_24k_i[i];
											cssample.imag = tx_float_out_24k_r[i];
										}
										cbWriteComp(TX4_cb, cssample);
									}
									//Sig2Noise = (_freedvS->fdmdv_stats.snr_est);
                            	}else{
                            		for( i=0; i<384; i++)
                            		{
										cssample = cbReadComp(TX3_cb);
                            			tx_float_cleanup_r[i+MEM_24] = cssample.real;
                            			tx_float_cleanup_i[i+MEM_24] = cssample.imag;

                            			tx_float_out_24k_r[i] = tx_float_cleanup_r[i+MEM_24];
                            			tx_float_out_24k_i[i] = tx_float_cleanup_i[i+MEM_24];
                            		}
                            		fdmdv_5k_cleanup(tx_float_out_24k_r, &tx_float_cleanup_r[MEM_24], 384);
                            		fdmdv_5k_cleanup(tx_float_out_24k_i, &tx_float_cleanup_i[MEM_24], 384);

									for( i=0 ; i<384 ; i++)
									{
										cssample.real = tx_float_out_24k_r[i];
										cssample.imag = tx_float_out_24k_i[i];
										//cssample = cbReadComp(TX3_cb);
										if(sideband_mode == LOWER_SIDEBAND){
											fsample = cssample.real;
											cssample.real = cssample.imag;
											cssample.imag = fsample;
										}
										cbWriteComp(TX4_cb, cssample);
									}
                            	}
                            }
					    }
						// Check for >= 128 samples in RX4_cb. Form packet and
						//	export.

						uint32 tx_check_samples = PACKET_SAMPLES;

						if(initial_tx)
							tx_check_samples = PACKET_SAMPLES * 3;

						if ( _end_of_transmission )
						    flush_tx = TRUE;

						if ( !inhibit_tx ) {
                            if(ccbContains(TX4_cb) >= tx_check_samples )
                            {
                                for( i = 0 ; i < PACKET_SAMPLES ; i++)
                                {
                                    // Set up the outbound packet
                                    ((Complex*)buf_desc->buf_ptr)[i] = cbReadComp(TX4_cb);
                                }
                            } else {
                                memset( buf_desc->buf_ptr, 0, PACKET_SAMPLES * sizeof(Complex));

                                if(initial_tx)
                                    initial_tx = FALSE;
                            }

                            emit_waveform_output(buf_desc);
                            if ( flush_tx ) {
                                inhibit_tx = TRUE;

                                while ( ccbContains(TX4_cb) > 0 ) {

                                    if ( ccbContains(TX4_cb) > PACKET_SAMPLES ) {
                                        for( i = 0 ; i < PACKET_SAMPLES ; i++)
                                        {
                                            // put the fsample into the outbound packet
                                            ((Complex*)buf_desc->buf_ptr)[i] = cbReadComp(TX4_cb);

                                        }
                                    } else {
                                        int end_index = 0;
                                        for ( i = 0 ; i <= ccbContains(TX4_cb); i++ ) {
                                            ((Complex*)buf_desc->buf_ptr)[i] = cbReadComp(TX4_cb);
                                            end_index = i+1;
                                        }

                                        for ( i = end_index ; i < PACKET_SAMPLES ; i++ ) {
                                            ((Complex*)buf_desc->buf_ptr)[i].real = 0.0f;
                                            ((Complex*)buf_desc->buf_ptr)[i].imag = 0.0f;
                                        }
                                    }
                                    emit_waveform_output(buf_desc);
                                }
                            }
						}
					}

					hal_BufferRelease(&buf_desc);
				}

			} while(1); // Seems infinite loop but will exit once there are no longer any buffers linked in _Waveformlist
		}
	}
	_waveform_thread_abort = TRUE;
	freedv_close(_freedvS);
	cfbDestroy(RX1_cb);
	cfbDestroy(RX2_cb);
	csbDestroy(RX3_cb);
	cfbDestroy(RX4_cb);

	cfbDestroy(TX1_cb);
	csbDestroy(TX2_cb);
	ccbDestroy(TX3_cb);
	ccbDestroy(TX4_cb);

	return NULL;
}

void sched_waveform_Init(void)
{
	pthread_rwlock_init(&_list_lock, NULL);

	pthread_rwlock_wrlock(&_list_lock);
	_root = (BufferDescriptor)safe_malloc(sizeof(buffer_descriptor));
	memset(_root, 0, sizeof(buffer_descriptor));
	_root->next = _root;
	_root->prev = _root;
	pthread_rwlock_unlock(&_list_lock);

	sem_init(&sched_waveform_sem, 0, 0);

	pthread_create(&_waveform_thread, NULL, &_sched_waveform_thread, NULL);

	struct sched_param fifo_param;
	fifo_param.sched_priority = 30;
	pthread_setschedparam(_waveform_thread, SCHED_FIFO, &fifo_param);
}

void sched_waveformThreadExit()
{
	_waveform_thread_abort = TRUE;
	sem_post(&sched_waveform_sem);
}
