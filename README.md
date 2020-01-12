# ARM7WRESTLER DC

arm7wrestler DC is a CPU test for the ARM7DI CPU in the Dreamcast's AICA audio
system.  It was originally developed for the Nintendo DS by micol972.  This
port is based on Arisotura's fork at
https://github.com/arisotura/arm7wrestler , so it also includes the improvements
they made to the original DS version.

Please direct all criticism to snickerbockers@washemu.org .  The source code
for this test may be found on github at
https://github.com/snickerbockers/dc-arm7wrestler .

Since the Dreamcast's ARM7 is only able to access the audio hardware directly,
this test implements a simple packet communication system to allow the ARM7 to
request that the SH4 to display text to the framebuffer and access the maple
bus on its behalf.  The source for the ARM7-side of this can be
found in arm7/source/message.s .  The SH4-side of the communication system is
implemented in sh4/main.c .

Other than the communication system and the bootstrap code, the only changes
required to get this running on the Dreamcast were to remove all the
ARMv4/v5 and thumb opcodes which aren't available on the Dreamcast's ARM7DI.

## KNOWN BUGS
* If you get an error message stating "ERROR - ARM7 CPU EXCEPTION RAISED",
  the included PC address is probably wrong.  You should never see this error
  anyways unless there's something wrong with your Dreamcast emulator because
  this test leaves interrupts permanently masked on the ARM7.
* On some emulators, you may see text slowly appear in increments across
  multiple frames.  This is caused by the emulator not interleaving ARM7 and SH4
  execution tightly enough.  The communication system I implemented between the
  two CPUs serialises them so that the ARM7 can't do anything while it's waiting
  for the SH4 to respond.  Some emulators will run each CPU one at a time and
  alternate between them every N cycles; this introduces a latency into the
  communication system since it takes up to 2*N cycles for a round-trip message
  between CPUs and that latency can cause it to take more than one frame to
  update the screen.  This is not a problem on real hardware because the two
  CPUs can actually execute concurrently.  This bug will not impact your test
  results and can be safely ignored.

--snickerbockers
