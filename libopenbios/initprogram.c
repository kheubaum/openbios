/*
 *   Creation Date: <2010/04/02 13:00:00 mcayland>
 *   Time-stamp: <2010/04/02 13:00:00 mcayland>
 *
 *	<initprogram.c>
 *
 *	C implementation of (init-program) word
 *
 *   Copyright (C) 2010 Mark Cave-Ayland (mark.cave-ayland@siriusit.co.uk)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "config.h"
#include "kernel/kernel.h"
#include "libopenbios/bindings.h"
#include "libopenbios/initprogram.h"

/* Because the a.out loader requires platform-specific headers */
#ifdef CONFIG_LOADER_AOUT
#include "libopenbios/aout_load.h"
#endif

#include "libopenbios/bootcode_load.h"
#include "libopenbios/bootinfo_load.h"
#include "libopenbios/elf_load.h"
#include "libopenbios/fcode_load.h"
#include "libopenbios/forth_load.h"
#include "libopenbios/xcoff_load.h"


void init_program(void)
{
	/* Get the value of load-base and use it to determine the correct loader
           to use */
	ucell addr;

	feval("load-base");
	addr = POP();

#ifdef CONFIG_LOADER_AOUT
	if (is_aout((struct exec *)cell2pointer(addr))) {
		aout_init_program();
		return;
	}
#endif

#ifdef CONFIG_LOADER_BOOTCODE
	if (is_bootcode((char *)cell2pointer(addr))) {
		bootcode_init_program();
		return;
	}
#endif

#ifdef CONFIG_LOADER_BOOTINFO
	if (is_bootinfo((char *)cell2pointer(addr))) {
		bootinfo_init_program();
		return;
	}
#endif

#ifdef CONFIG_LOADER_ELF
	if (is_elf((Elf_ehdr *)cell2pointer(addr))) {
		elf_init_program();
		return;
	}
#endif

#ifdef CONFIG_LOADER_FCODE
	if (is_fcode((unsigned char *)cell2pointer(addr))) {
		fcode_init_program();
		return;
	}
#endif

#ifdef CONFIG_LOADER_FORTH
	if (is_forth((char *)cell2pointer(addr))) {
		forth_init_program();
		return;
	}
#endif

#ifdef CONFIG_LOADER_XCOFF
	if (is_xcoff((COFF_filehdr_t *)cell2pointer(addr))) {
		xcoff_init_program();
		return;
	}
#endif

}

void init_fcode_context(void)
{
	/* Execute FCode payload */
	printk("Evaluating FCode...\n");
	fword("load-base");
	PUSH(1);
	fword("byte-load");
}


void init_forth_context(void)
{
	/* Execute Forth payload */
	printk("Evaluating Forth...\n");
	fword("load-base");
	feval("load-state >ls.file-size @");
	fword("eval2");
}

void go(void)
{
	ucell address, type;
	int image_retval = 0;

	/* Get the entry point and the type (see forth/debugging/client.fs) */
	feval("load-state >ls.entry @");
	address = POP();
	feval("load-state >ls.file-type @");
	type = POP();

	printk("\nJumping to entry point " FMT_ucellx " for type " FMT_ucellx "...\n", address, type);

	switch (type) {
		default:
			/* Start native binary */
			image_retval = start_elf((unsigned long)address);
			break;

		case 0x10:
			/* Start Fcode image */
			image_retval = start_elf((unsigned long)&init_fcode_context);
			break;

		case 0x11:
			/* Start Forth image */
			image_retval = start_elf((unsigned long)&init_forth_context);
			break;
	}

	printk("Image returned with return value %#x\n", image_retval);
}
