.intel_syntax noprefix
.text

/*
 * mirb_arch_support_jit_stub
 */
.global mirb_arch_support_jit_stub
mirb_arch_support_jit_stub:
.func mirb_arch_support_jit_stub
	call mirb_arch_support_jit_invoke
	jmp eax
.endfunc

/*
 * mirb_arch_support_call
 */
.global mirb_arch_support_call
mirb_arch_support_call:
.func mirb_arch_support_call
	lea eax, dword ptr [esp + 12]
	push eax
	push dword ptr [esp + 12]
	push dword ptr [esp + 12]
	call mirb_arch_support_lookup
	test eax, eax
	jz unknown_call_method
	jmp eax

unknown_call_method:
	mov ecx, ebp
	jmp mirb_arch_support_unknown_call_method
.endfunc

/*
 * mirb_arch_support_super
 */
.global mirb_arch_support_super
mirb_arch_support_super:
.func mirb_arch_support_super
	lea eax, dword ptr [esp + 12]
	push eax
	push dword ptr [esp + 12]
	call mirb_arch_support_lookup_super
	test eax, eax
	jz unknown_super_method
	jmp eax

unknown_super_method:
	mov ecx, ebp
	jmp mirb_arch_support_unknown_super_method
.endfunc
