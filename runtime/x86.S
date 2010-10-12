.intel_syntax noprefix
.text

/*
 * rt_support_call
 *
 * Parameters:
 *   %edx: Method name as a symbol
 *   %esp + 4: The object to look up in
 */
.global rt_support_call
rt_support_call:
.func rt_support_call
	sub esp, 4
	push esp
	push edx
	push dword ptr [esp + 16]
	call rt_lookup
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
.global rt_support_super
rt_support_super:
.func rt_support_super
	sub esp, 4
	push esp
	push dword ptr [esp + 12]
	push dword ptr [esp + 20]
	call rt_lookup_super
	add esp, 28
	mov ecx, [esp - 4]
	mov edx, [esp - 8]
	push dword ptr [esp - 12]
	jmp eax
.endfunc
