.include "C:\Program Files (x86)\Atmel\AVR Tools\AvrAssembler2\Appnotes\tn13Adef.inc"

main:

	sbis pinB, pinB1
	rjmp disableGenerator

	sbi portB, pinB0
	nop;
	nop;
	nop;
	nop;
	nop;
	nop;

disableGenerator:
	cbi portB, pinB0
	nop;
	nop;

rjmp main

