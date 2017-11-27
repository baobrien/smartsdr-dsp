
    .syntax unified

    .text;   .p2align 2;   .arch armv7-a
   .fpu neon
   .object_arch armv4t
   .extern gt_alpha5_root_coh

	.global cohpsk_filt_kern
	.global neon_wide_memcpy
	//.extern printf


	/*
	r0 - rx_filter_memory
	r1 - rx_filt
	r2 - j
	*/

	/*
	 r3 - gt_alpha5... pointer
	 r4 - 4800 + gt_alpha5...
	 r5 - rx_filter_memory[c]
	 r6 - rx_filter_memory[c+1]
	 r7 - NC counter

	 q0/d0-d1 - rx_filter_memory[c][k]+
	 q1/d1-d2 - rx_filter_memory[c+1][k]+

	 q8/d16-d17 - acc[c]
	 q9/d18-d19 - acc[c+1]
	 q15/d30-d31 - gt_alpha5...[k]+
	 */

	//STMFD	sp!, {r4-r8, lr}
    //
	//mov r7, #14											//set up loop counter
	//ldr r3, =gt_alpha5_root_coh_interleaved				//load coeff addr
	//add r4, r3, #4800									//set up coeff end addr
    //
	//nc_loop:
	//	ldr r3, =gt_alpha5_root_coh_interleaved			//reload coeff addr
	//	vmov.i32 q8, #0									//clear accumulators
	//	vmov.i32 q9, #0
    //
	//	ldr r5, [r0]!
	//	ldr r6, [r0]!
    //
	//	ikern_loop:
	//		vld1.32 {d30,d31}, [r3]!					//load filter coeffs
	//		vld1.32 {d0,d1},   [r5]!					//load rx_filter_memory[c][k]
	//		vmla.f32 q8, q0, q15
	//		vld1.32 {d2,d3},   [r6]!					//load rx_filter_memory[c+1][k]
	//		vmla.f32 q9, q1, q15
	//		cmp r3, r4
	//		blt ikern_loop
    //
	//	vadd.f32 d16, d16, d17							//add half accumulators
	//	ldr r8, [r1]!									//load rx_filt[c]
	//	add r8, r8, r2, lsl #3							//calc addr rx_filt[c][j]
	//	vst1.32 d16, [r8]								//store into rx_filt[c][j]
	//	//add r1, #4										//bump rx_filt pointer to c+1
    //
	//	vadd.f32 d18, d18, d19							//add half accumulators
	//	//ldr r8, [r1]!									//load rx_filt[c]
	//	add r8, r8, r2, lsl #3							//calc addr rx_filt[c][j]
	//	vst1.32 d16, [r8]								//store into rx_filt[c][j]
	//	//add r1, #4										//bump rx_filt pointer to c+1
    //
	//	sub r7, #2
	//	cmp r7, #0
	//	bgt nc_loop
    //
    //
	//ldmfd sp!, {r4-r8, pc}


	//.size cohpsk_filt_kern, .-cohpsk_filt_kern  @ ENDP

	/*
	 *
	 * r1 = rx_filtor_memory[c]
	 * r2 = &rx_filt[c][j]
	 *
	 * vadd.i32 r4, r0, #4800
	 * vmov.i32 q8,#0
	 * ikloop:
	 *   vld1.32 q0, [r0]! # { filt, filt, filt+, filt+ }
	 *   vld1.32 q1, [r1]! # { mem.r, mem.i, mem+.r, mem+.i }
	 *   vmla.f32 q8, q0, q1
	 *   cmp r4, r0
	 *   ble ikloop
	 * vadd.f32 d16, d16, d17
	 * vst1.32 d16, [r2]
	 */

	 	/*
	 r3 - gt_alpha5... pointer
	 r4 - 4800 + gt_alpha5...
	 r5 - rx_filter_memory[c]
	 r6 - rx_filter_memory[c+1]
	 r7 - NC counter

	 q0/d0-d1 - rx_filter_memory[c][k]+
	 q1/d1-d2 - rx_filter_memory[c+1][k]+

	 q8/d16-d17 - acc[c]
	 q9/d18-d19 - acc[c+1]
	 q15/d30-d31 - gt_alpha5...[k]+
	 */


	//cohpsk_filt_kern(&rx_filter_memory[c][0],&rx_filt[c][j],&rx_filter_memory[c+1][0],&rx_filt[c+1][j]);
	//cohpsk_filt_kern(&rx_filter_memory[c][0],&rx_filter_memory[c+1][0],&rx_filt[c][j],&rx_filt[c+1][j]);
	.type	cohpsk_filt_kern, %function; cohpsk_filt_kern: @ PROC
	STMFD	sp!, {r4-r5, lr}

	@Loop end var
	//ldr r4, =gt_alpha5_root_coh_interleaved
	//add	r5, r4, #4800

	ldr r4, =gt_alpha5_root_coh
	add	r5, r4, #2400

	vmov.i32 q8,#0
	vmov.i32 q9,#0
	ikloop:

		//vld1.32 {d30,d31}, [r4]!
		// q15: {filt, filt+1, filt+2, filt+3}
		vld2.32 {d30[],d31[]}, [r4]!
		//vld1.32 {d31[]}, [r4]!

		vld1.32 {d0,d1}, [r0]!
		vmla.f32 q8, q0, q15
		vld1.32 {d2,d3}, [r1]!
		vmla.f32 q9, q1, q15


		cmp r5, r4
		bgt ikloop

	vadd.f32 d16, d16, d17
	vadd.f32 d18, d18, d19
	vst1.32 d16, [r2]
	vst1.32 d18, [r3]

	LDMFD	sp!, {r4-r5, pc}
	.size cohpsk_filt_kern, .-cohpsk_filt_kern  @ ENDP


	// neon_wide_memcpy(void* dest,void* src,size_t size)
	.type neon_wide_memcpy, %function; neon_wide_memcpy: @ PROC
	//STMFD	sp!, {r4-r6, lr}

	neon_wdcl:
		vld1.32 {d0,d1}, [r1]!
		vst1.32 {d0,d1}, [r0]!
		subs r2,r2,#16
		cmp r2, #16
		bgt neon_wdcl

	cmp r2,#0
	beq neon_wdexit
	vld1.32 d0, [r1]
	vst1.32 d0, [r0]

	neon_wdexit:
	//LDMFD	sp!, {r4-r6, pc}
	mov pc,lr
	.size neon_wide_memcpy, .-neon_wide_memcpy  @ ENDP


    .section	.note.GNU-stack,"",%progbits
