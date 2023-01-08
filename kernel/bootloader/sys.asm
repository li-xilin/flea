;nasm -felf init.asm && i686-w64-mingw32-gcc init.o
struc SYSTAB
	.pfnLoadLibraryA    resd 1
	.pfnGetProcAddress  resd 1
	.pfnWSAStartup      resd 1
	.pfnWSASocketA      resd 1
	.pfnWSAConnect      resd 1
	.pfnWSARecv         resd 1
	.pfnVirtualAlloc    resd 1
endstruc
global _InitSysTab
extern _dld_LoadLibraryA
extern _dld_GetProcAddress
extern _dld_VirtualAlloc


section .text

_InitSysTab: ; ebp *
	push ebp
	mov ebp, esp

	call pusheip
strtab:
	sWs2_32_dll db "Ws2_32.dll", 0
	sWSAStartup db "WSAStartup", 0
	sWSASocketA db "WSASocketA", 0
	sWSAConnect db "WSAConnect", 0
	sWSARecv    db "WSARecv", 0
initfunc: ;(al = stroffset, bl = sysoffset)
	movzx eax, al
	movzx ebx, bl

	mov ecx, [ebp - 4]
	add ecx, eax ;edx = pRes->xx
	push ecx
	
	mov ecx, [ebp - 12]
	;mov [esp], ecx ;hModule
	push ecx
	
	mov edx, [ebp - 8]
	call edx
	
	mov ecx, [ebp + 8]
	mov [ecx + ebx], eax

	ret

pusheip:
	call _dld_GetProcAddress
	test eax, eax
	jz fail


	push eax ;push pGetProcAddress

	mov ebx, [ebp + 8]
	mov [ebx + SYSTAB.pfnGetProcAddress], eax
	; pRes->pfnGetProcAddress = pfnGetProcAddress


	call _dld_VirtualAlloc
	; eax = pLoadLibraryA
	mov ebx, [ebp + 8]
	mov [ebx + SYSTAB.pfnVirtualAlloc], eax
	; pRes->pfnVirtualAlloc = pfnVirtualAlloc

	call _dld_LoadLibraryA
	; eax = pLoadLibraryA
	mov ebx, [ebp + 8]
	mov [ebx + SYSTAB.pfnLoadLibraryA], eax
	; pRes->pfnLoadLibraryA = pfnLoadLibraryA


	
	mov edx, [ebp - 4]
	lea ebx, [edx + sWs2_32_dll - strtab]
	push ebx 
	call eax; LoadLibrary
	;add esp, 4
	push eax ;push hModule
	
	;mov edi, initfunc

	mov al, sWSAStartup - strtab
	mov bl, SYSTAB.pfnWSAStartup
	call initfunc

	mov al, sWSASocketA - strtab
	mov bl, SYSTAB.pfnWSASocketA
	call initfunc

	mov al, sWSAConnect - strtab
	mov bl, SYSTAB.pfnWSAConnect
	call initfunc

	mov al, sWSARecv - strtab
	mov bl, SYSTAB.pfnWSARecv
	call initfunc


	mov ebx, [ebp + 8]
	xor eax, eax
	mov ecx, 6
loop_tab:
	mov edx, [ebx + eax * 4]
	test edx, edx
	jz fail
	;add ebx, 4
	loop loop_tab

	jmp out
fail:
	mov eax, 1
out:
	mov esp, ebp
	pop ebp
	ret

