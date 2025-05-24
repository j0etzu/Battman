#pragma once

#include <objc/objc.h>

#define oclass(cls) \
	({Class v;asm("adrp %0,_OBJC_CLASS_$_" #cls "@GOTPAGE \n" \
			"ldr %0,[%0,_OBJC_CLASS_$_" #cls "@GOTPAGEOFF]":"=r"(v));v;})

#define oselector(sel) \
	({SEL v;asm("adrp %0,\"oselector::" #sel "!%=\"@PAGE \n" \
		"ldr %0,[%0,\"oselector::" #sel "!%=\"@PAGEOFF] \n " \
		".section __TEXT,__objc_methname,cstring_literals \n" \
		"\"oselector~meth::" #sel "!%=\": .asciz \"" #sel "\" \n" \
		".section __DATA,__objc_selrefs,literal_pointers,no_dead_strip \n" \
		"\"oselector::" #sel "!%=\": .quad \"oselector~meth::" #sel "!%=\" \n" \
		".section __TEXT,__text,regular,pure_instructions":"=r"(v));v;})

void objc_release(id);
void objc_retain(id);
id objc_alloc(Class);
id objc_alloc_init(Class);