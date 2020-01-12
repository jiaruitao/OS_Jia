[bits 32]
section .text

global switch_to
switch_to:
	push esi
	push edi
	push ebx
	push ebp
	mov eax, [esp + 20]		; 得到cur
	mov [eax], esp
	
	
	mov eax, [esp + 24]		; 得到next
	mov esp, [eax]
	pop ebp
	pop ebx
	pop edi
	pop esi
	ret
