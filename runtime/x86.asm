bits 32
section .text

extern _rt_lookup
extern _rt_lookup_super

global _rt_support_call

; rt_support_call
;
; Parameters:
;   %edx: Method name as a symbol
;   %esp + 4: The object to look up in

_rt_support_call:
	sub esp, 4
	push esp
	push edx
	push dword [esp + 16]
	call _rt_lookup
	pop edx
	pop edx
	pop ecx
	pop ecx
	jmp eax
	

global _rt_support_super

; rt_support_super
;
; Parameters (bottom up):
;   rt_value method_name
;   rt_value method_module
;   rt_value self: The object to look up

_rt_support_super:
	sub esp, 4
	push esp
	push dword [esp + 12]
	push dword [esp + 20]
	call _rt_lookup_super
	add esp, 28
	mov ecx, [esp - 4]
	mov edx, [esp - 8]
	push dword [esp - 12]
	jmp eax
