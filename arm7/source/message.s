	.global msg_init
	.global xmit_fib_msg
	.global xmit_string

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

xmit_string:
	sub sp, sp, #4
	str lr, [sp]

	ldr r3, =xmit_string_buf
	add r2, r3, #52
	mov r1, #0

clear_msg:
	str r1, [r3]
	add r3, r3, #4
	cmp r3, r2
	bne clear_msg

	ldr r3, =xmit_string_buf
load_msg:
	ldrb r1, [r0]
	strb r1, [r3]
	add r3, r3, #1
	add r0, r0, #1
	cmp r3, r2
	beq msg_loaded
	cmp r1, #0
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

	ldr lr, [sp]
	add sp, sp, #4

	mov pc, lr

xmit_string_buf:
	.zero 52

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
	cmp r3, #52/4
	bne fib_loop

	@@ restore r4 from the stack
	ldr r4, [r13]
	add r13, r13, #4

	@@ return
	mov r15, r14

fib_msg_addr:
	.long fib_msg


xmit_pkt:
	@@ r0 points to a 52-byte message string
	@@ r1 contains the opcode

	@@ push r4 onto the stack
	sub r13, r13, #4
	str r4, [r13]

	@@ first write out the opcode
	ldr r2, pkt_out_addr
	add r3, r2, #8
	str r1, [r3]

	@@ now write the 52-byte message
	add r3, r3, #4
	mov r4, #0
put_long:
	ldr r1, [r0]

	str r1, [r3]

	add r3, r3, #4
	add r0, r0, #4
	add r4, r4, #1
	cmp r4, #52/4
	bne put_long

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
	@@ third four bytes: opcode
	@@ other 52 bytes: message data
	.align 4
pkt_out_addr:
	.long 0x00100000
next_seqno:
	.long 1
ack_seqno:
	.long 0
fib_msg:
	.zero 52
