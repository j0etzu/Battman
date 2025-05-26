#pragma once

#include <objc/objc.h>
#include <objc/runtime.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>

#include "cpp_magic.h"

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
id objc_retain(id);
id objc_alloc(Class);
id objc_alloc_init(Class);
BOOL objc_opt_isKindOfClass(id,Class);
BOOL class_respondsToSelector(Class,SEL);

static const uint64_t _block_descriptor_1arg[2]={0,40};
#define objc_make_block(func,context) \
	({uint64_t blk_content[40]; \
		blk_content[0]=(uint64_t)&_NSConcreteStackBlock; \
		blk_content[1]=0x60000000; \
		blk_content[2]=(uint64_t)func; \
		blk_content[3]=(uint64_t)&_block_descriptor_1arg; \
		blk_content[4]=(uint64_t)context; \
		id blk=object_copy((id)&blk_content,40); \
		blk;})

extern void objc_msgSend(void);
extern void objc_msgSendSuper(void);

#define _ocall_type_expand_r(val,...) ,typeof(val) IF(HAS_ARGS(__VA_ARGS__)) ( DEFER2(_ocall_type_expand_)()(__VA_ARGS__) )
#define _ocall_type_expand_() _ocall_type_expand_r
#define _ocall_type_expand(...) IF(HAS_ARGS(__VA_ARGS__)) ( EVAL(_ocall_type_expand_r(__VA_ARGS__)) )
#define _ocall_name_expand_r(val,...) ,val IF(HAS_ARGS(__VA_ARGS__)) ( DEFER2(_ocall_name_expand_)()(__VA_ARGS__) )
#define _ocall_name_expand_() _ocall_name_expand_r
#define _ocall_name_expand(...) IF(HAS_ARGS(__VA_ARGS__)) ( EVAL(_ocall_name_expand_r(__VA_ARGS__)) )

#define _ocall(send,obj,sel,...) ((void*(*)(typeof(obj),SEL _ocall_type_expand(__VA_ARGS__)))send)(obj,oselector(sel) _ocall_name_expand(__VA_ARGS__))

#define ocall(...) _ocall(objc_msgSend,__VA_ARGS__,)
#define osupercall(obj,...) \
	({uint64_t refs[2]={(uint64_t)obj,(uint64_t)class_getSuperclass(object_getClass(obj))}; \
		_ocall(objc_msgSendSuper,refs,__VA_ARGS__,);})


#include "./cobjc_types.h"

#define DEFINE_CLASS(name, superclass) \
	asm(".section __DATA,__objc_data\n" \
		".globl _OBJC_CLASS_$_" #name "\n" \
		".p2align 3\n" \
		"_OBJC_CLASS_$_" #name ":\n" \
		".quad _OBJC_METACLASS_$_" #name "\n" \
		".quad _OBJC_CLASS_$_" #superclass "\n" \
		".quad __objc_empty_cache\n.quad 0\n" \
		".quad __OBJC_CLASS_RO_$_" #name "\n" \
		"_OBJC_METACLASS_$_" #name ":\n" \
		".quad _OBJC_METACLASS_$_NSObject\n" \
		".quad _OBJC_METACLASS_$_" #superclass "\n" \
		".quad __objc_empty_cache\n.quad 0\n" \
		".quad __OBJC_METACLASS_RO_$_" #name "\n" \
		".section __DATA,__objc_superrefs,regular,no_dead_strip\n" \
		".p2align 3\n" \
		"l_suprefs_" #name ": .quad _OBJC_CLASS_$_" #name "\n" \
		".section __DATA,__objc_classlist,regular,no_dead_strip\n" \
		".p2align 3\nl_objc_label_" #name ": .quad _OBJC_CLASS_$_" #name "\n" \
		".section __DATA,__objc_const\n" \
		"__OBJC_CLASS_RO_$_" #name ":\n" \
		".long 0\n.long 8\n.long 16\n.space 4\n.quad 0\n" \
		".quad l$$classname$$" #name "\n" \
		".quad _instance_methods." #name ".objc\n" \
		".quad 0\n.quad _instance_vars." #name ".objc\n" \
		".quad 0\n.quad 0\n" \
		"__OBJC_METACLASS_RO_$_" #name ":\n" \
		".long 1\n.long 40\n.long 40\n.space 4\n.quad 0\n" \
		".quad l$$classname$$" #name "\n" \
		".quad _class_methods." #name ".objc\n" \
		".quad 0\n.quad 0\n.quad 0\n.quad 0\n" \
		"_instance_vars." #name ".objc:\n" \
		".long 32\n.long 1\n.quad _OBJC_IVAR_$_" #name ".cobjc_struct\n" \
		".quad l_objc_ivar_name_" #name "\n.quad l_objc_ivar_type_" #name "\n.long 3\n.long 8\n" \
		".section __DATA,__objc_ivar\n.globl _OBJC_IVAR_$_" #name ".cobjc_struct\n" \
		".p2align 2\n_OBJC_IVAR_$_" #name ".cobjc_struct:\n.long 8\n" \
		".section __TEXT,__objc_methname,cstring_literals\n" \
		"l_objc_ivar_name_" #name ": .asciz \"cobjc_struct_" #name "\"\n" \
		"l_objc_ivar_type_" #name ": .asciz \"^v\"\n" \
		"l$$classname$$" #name ": .asciz \"" #name "\"\n")

#define DEFINE_CLASS_METHODS(class, count) \
	asm(".section __DATA,__objc_const\n" \
		".p2align 3\n_class_methods." #class ".objc:\n.long 24\n" \
		".long " #count "\n")
// type doesn't really matter I think
#define ADD_METHOD_WITH_TYPE(func,selector,type) \
	asm(".quad l_cls_method_name_" #func "_\n" \
		".quad l_cls_method_type_" #func "_\n"\
		".quad _" #func "\n" \
		".section __TEXT,__objc_methtype,cstring_literals\n"\
		"l_cls_method_type_" #func "_: .asciz \"" #type "\"\n"\
		"l_cls_method_name_" #func "_: .asciz \"" #selector "\"\n"\
		".section __DATA,__objc_const\n")
#define ADD_METHOD(func,selector) ADD_METHOD_WITH_TYPE(func,selector,@@:)
#define DEFINE_INSTANCE_METHODS(class, count) \
	asm(".section __DATA,__objc_const\n" \
		".p2align 3\n_instance_methods." #class ".objc:\n.long 24\n" \
		".long " #count "\n")

// To be called inside a function
#define COBJC_STRUCT(class,obj) \
	({void **val;asm("adrp x9,_OBJC_IVAR_$_" #class ".cobjc_struct@PAGE\n" \
		"ldrsw x9,[x9,_OBJC_IVAR_$_" #class ".cobjc_struct@PAGEOFF]\n" \
		"add %0,%1,x9":"=r"(val):"r"(obj):"x9");val;})
