.intel_syntax noprefix
.section .text

/*
 * rt_support_call
 *
 * Parameters:
 *   %edx: Method name as a symbol
 *   %esp + 4: The object to look up in
 */

.global _rt_support_call
_rt_support_call:
.func _rt_support_call
	sub esp, 4
	push esp
	push edx
	push dword ptr [esp + 16]
	call _rt_lookup
	pop edx
	pop edx
	pop ecx
	pop ecx
	jmp eax
.endfunc

/*
 * rt_support_super
 *
 * Parameters (bottom up):
 *   rt_value method_name
 *   rt_value method_module
 *   rt_value self: The object to look up
 */

.global _rt_support_super
_rt_support_super:
.func _rt_support_super
	sub esp, 4
	push esp
	push dword ptr [esp + 12]
	push dword ptr [esp + 20]
	call _rt_lookup_super
	add esp, 28
	mov ecx, [esp - 4]
	mov edx, [esp - 8]
	push dword ptr [esp - 12]
	jmp eax
.endfunc
