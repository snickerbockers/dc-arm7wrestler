	.global msg_init
	.global xmit_fib_msg
	.global xmit_string
	.global get_btns
	.global clear_screen
	.global notify_exception

	.set DATA_LEN, 112

	.align 4
msg_init:
	sub sp, sp, #4
	str lr, [sp]
	bl init_fib_msg
	ldr lr, [sp]
	add sp, sp, #4
	mov pc, lr

xmit_fib_msg:
	@@ save the link register
	sub sp, sp, #4
	str lr, [sp]

	ldr r0, fib_msg_addr
	mov r1, #69
	bl xmit_pkt

	@@ restore the link register
	ldr lr, [sp]
	add sp, sp, #4

	ldr r0, ack_seqno
	ldr r1, pkt_out_addr
	add r1, r1, #4
wait_for_ack:
	ldr r2, [r1]
	cmp r2, r0
	bne wait_for_ack

	mov pc, lr

	@@ Transmit a text string to the SH4
	@@ parameters:
	@@     r0 - string
	@@     r1 - x position
	@@     r2 - y position
	@@     r3 - color index
xmit_string:
	stmfd 	sp!,{r4-r10,lr}

	ldr r7, =xmit_string_buf

	mov r6, r7
	add r5, r6, #DATA_LEN
	mov r4, #0

clear_msg:
	str r4, [r6]
	add r6, r6, #4
	cmp r6, r5
	bne clear_msg

	@@ write out x-pos, y-pos, color (12 bytes)
	str r1, [r7]
	add r7, r7, #4
	str r2, [r7]
	add r7, r7, #4
	str r3, [r7]
	add r7, r7, #4

	mov r6, r7
load_msg:
	ldrb r4, [r0]
	strb r4, [r6]
	add r6, r6, #1
	add r0, r0, #1
	cmp r6, r5
	beq msg_loaded
	cmp r4, #0
	beq msg_loaded
	b load_msg

msg_loaded:

	ldr r0, =xmit_string_buf
	mov r1, #70
	bl xmit_pkt

	@@ now wait for the response
	ldr r0, ack_seqno
	ldr r1, pkt_out_addr
	add r1, r1, #4
xmit_string_wait_for_ack:
	ldr r2, [r1]
	cmp r2, r0
	bne xmit_string_wait_for_ack

	ldmfd 	sp!,{r4-r10,lr}
	mov pc, lr

xmit_string_buf:
	.zero DATA_LEN

	.align 4
init_fib_msg:
	@@ fills fib_msg with a fibonacci sequence
	@@ we send this to the sh4 to prove to it that
	@@ the ARM is working and has a sane environment.
	@@ It doesn't need to be this complicated, but I like the idea
	@@ of having it do some runtime calculation with a known answer
	@@ to establish that it's alive

	@@ push r4 onto the stack
	sub r13, r13, #4
	str r4, [r13]

	ldr r0, fib_msg_addr
	mov r1, #1
	mov r2, #1
	mov r3, #2

	str r1, [r0]
	add r0, r0, #4
	str r2, [r0]
	add r0, r0, #4

fib_loop:
	add r4, r1, r2

	str r4, [r0]
	add r0, r0, #4

	mov r1, r2
	mov r2, r4

	add r3, r3, #1
	cmp r3, #DATA_LEN/4
	bne fib_loop

	@@ restore r4 from the stack
	ldr r4, [r13]
	add r13, r13, #4

	@@ return
	mov r15, r14

fib_msg_addr:
	.long fib_msg

get_btns:
	@@ returns which buttons changed since the last invocation in r0
	@@ this function accepts no parameters
	stmfd 	sp!,{lr}

	mov r0, #0
	mov r1, #71

	bl xmit_pkt

	@@ now wait for the response
	ldr r0, ack_seqno
	ldr r1, pkt_out_addr
	add r1, r1, #4
get_btns_wait_for_ack:
	ldr r2, [r1]
	cmp r2, r0
	bne get_btns_wait_for_ack

	@@ get the return value
	add r1, r1, #4
	ldr r0, [r1]

	ldmfd 	sp!,{lr}
	mov pc, lr

clear_screen:
	@@ tell the SH4 to clear the screen
	@@ this function accepts no parameters and returns no values
	stmfd 	sp!,{lr}

	mov r0, #0
	mov r1, #72

	bl xmit_pkt

	@@ now wait for the response
	ldr r0, ack_seqno
	ldr r1, pkt_out_addr
	add r1, r1, #4
clear_screen_wait_for_ack:
	ldr r2, [r1]
	cmp r2, r0
	bne clear_screen_wait_for_ack

	ldmfd 	sp!,{lr}
	mov pc, lr

notify_exception:
	@@ tell the SH4 we've had a critical error
	@@ parameter r0 is an integer that identifies what kind of exception
	@@ parameter r1 is the PC
	stmfd 	sp!,{lr}

	ldr r2, =notify_exception_buf
	str r0, [r2]
	add r2, r2, #4
	str r1, [r2]

	ldr r0, =notify_exception_buf
	mov r1, #73
	bl xmit_pkt

	@@ now wait for the response
	ldr r0, ack_seqno
	ldr r1, pkt_out_addr
	add r1, r1, #4
notify_exception_wait_for_ack:
	ldr r2, [r1]
	cmp r2, r0
	bne notify_exception_wait_for_ack

	ldmfd 	sp!,{lr}
	mov pc, lr
notify_exception_buf:
	.zero DATA_LEN

.align 4
xmit_pkt:
	@@ r0 points to a DATA_LEN-byte message string
	@@ r1 contains the opcode

	@@ push r4 onto the stack
	sub r13, r13, #4
	str r4, [r13]

	@@ first write out the opcode
	ldr r2, pkt_out_addr
	add r3, r2, #12
	str r1, [r3]

	cmp r0, #0
	beq done_writing_msg

	@@ now write the message
	add r3, r3, #4
	mov r4, #0
put_long:
	ldr r1, [r0]

	str r1, [r3]

	add r3, r3, #4
	add r0, r0, #4
	add r4, r4, #1
	cmp r4, #DATA_LEN/4
	bne put_long
done_writing_msg:
	@@ now write out the sequence number
	ldr r0, next_seqno
	str r0, [r2]
	str r0, ack_seqno
	add r0, r0, #1
	str r0, next_seqno

	@@ restore r4 from the stack
	ldr r4, [r13]
	add r13, r13, #4

	@@ return
	mov r15, r14

	@@ communication protocol:
	@@ outbound packets (from ARM's perspective) are placed at
	@@ 0x00100000 (1MB)
	@@ first four bytes: sequence number (zero is considered invalid)
	@@ second four bytes: sequence number ack (sh4 writes to this)
	@@ third four bytes: return value (sh4 writes to this)
	@@ third four bytes: opcode
	@@ other DATA_LEN bytes: message data
	.align 4
pkt_out_addr:
	.long 0x00100000
next_seqno:
	.long 1
ack_seqno:
	.long 0
fib_msg:
	.zero DATA_LEN
