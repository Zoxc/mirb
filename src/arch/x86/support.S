.intel_syntax noprefix
.text

/*
 * mirb_arch_support_jit_stub
 *
 * Parameters:::
 *   [%esp + 20]: argv
 *   [%esp + 16]: argc
 *   [%esp + 12]: block
 *   [%esp + 8]: obj
 *   [%esp + 4]: Return address
 *   [%esp]: The block to compile
 *   %eax: Scopes
 *   %ecx: Method name
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
