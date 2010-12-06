.intel_syntax noprefix
.text

/*
 * mirb_arch_support_jit_stub
 *
 * Parameters:::
 *   [%esp + 20]: argv
 *   [%esp + 16]: argc
 *   [%esp + 12]: name
 *   [%esp + 8]: obj
 *   [%esp + 4]: Return address
 *   [%esp]: The block to compile
 *   %eax: Scopes
 *   %ecx: Passed block
 *   %edx: Method module
 */
.global mirb_arch_support_jit_stub
mirb_arch_support_jit_stub:
.func mirb_arch_support_jit_stub
	push ebx

	push eax
	push ecx
	push edx
	mov ecx, [esp + 16]
	call mirb_arch_support_jit_invoke
	mov ebx, eax
	pop edx
	pop ecx
	pop eax

	push [esp + 24]
	push [esp + 24]
	push [esp + 24]
	push [esp + 24]
	call ebx

	pop ebx
	add esp, 4
	ret 16
.endfunc

/*
 * mirb_arch_support_call
 */
.global mirb_arch_support_call
mirb_arch_support_call:
.func mirb_arch_support_call
	push ecx
	sub esp, 4
	push esp
	push dword ptr [esp + 20]
	push dword ptr [esp + 20]
	call mirb_arch_support_lookup
	pop edx
	pop ecx
	jmp eax
.endfunc

/*
 * mirb_arch_support_super
 */
.global mirb_arch_support_super
mirb_arch_support_super:
.func mirb_arch_support_super
	push ecx
	sub esp, 4
	push esp
	push dword ptr [esp + 20]
	push edx
	call mirb_arch_support_lookup_super
	pop edx
	pop ecx
	jmp eax
.endfunc
