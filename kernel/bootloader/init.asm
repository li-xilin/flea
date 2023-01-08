section .text
	extern _Load
%ifndef DEBUG
	global _start
_start:
%else
	global _WinMain@16
_WinMain@16:
%endif
	call pusheip;
	ipaddr dd 0x0100007F
pusheip:
	pop eax
	push ebp
	mov ebp, esp

	; mov eax, [ebp + 4]
	mov eax, [eax]
	push eax
	call _Load
	mov esp, ebp
	pop ebp
	; add esp, 4

%ifdef DEBUG
	ret 16
%else
	ret
%endif

