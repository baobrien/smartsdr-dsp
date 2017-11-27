
 	AREA  |.text|, CODE, READONLY

	EXPORT cohpsk_filt_kern


cohpsk_filt_kern PROC
	STMFD	sp!, {r4-r6, lr}

	;Loop end var
	ADD	r4, r0, #4800
	vmov.i32 q8,#0

	ikloop:
	vldr.32 q9, [r0]!
	vldr.32 {d2[],d3[]}, [r1]!
	vmla.f32 q8, q0, q1
	cmp r4, r0
	ble ikloop
	vadd.f32 d16, d16, d17
	vst1.32 d16, [r2]

	LDMFD	sp!, {r4-r6, pc}
  ENDP

