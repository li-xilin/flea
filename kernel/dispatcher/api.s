	.file	"api.c"
	.text
	.p2align 4
	.globl	_InitializeWin32Api
	.def	_InitializeWin32Api;	.scl	2;	.type	32;	.endef
_InitializeWin32Api:
LFB589:
	.cfi_startproc
	leal	4(%esp), %ecx
	.cfi_def_cfa 1, 0
	andl	$-16, %esp
	pushl	-4(%ecx)
	pushl	%ebp
	movl	%esp, %ebp
	.cfi_escape 0x10,0x5,0x2,0x75,0
	pushl	%edi
	pushl	%esi
	leal	-60(%ebp), %edx
	.cfi_escape 0x10,0x7,0x2,0x75,0x7c
	.cfi_escape 0x10,0x6,0x2,0x75,0x78
	leal	-47(%ebp), %esi
	pushl	%ebx
	pushl	%ecx
	.cfi_escape 0xf,0x3,0x75,0x70,0x6
	.cfi_escape 0x10,0x3,0x2,0x75,0x74
	subl	$104, %esp
	movl	(%ecx), %ebx
	movl	$842230885, -56(%ebp)
	movl	$1852990827, -60(%ebp)
	movl	$775041900, -55(%ebp)
	movl	(%ebx), %eax
	movl	$7105636, -51(%ebp)
	movl	%edx, (%esp)
	call	*%eax
	leal	-82(%ebp), %edx
	movl	$1680749107, -78(%ebp)
	subl	$4, %esp
	movl	$1597141879, -82(%ebp)
	movl	%eax, %edi
	movl	(%ebx), %eax
	movl	$7105636, -75(%ebp)
	movl	%edx, (%esp)
	call	*%eax
	leal	-71(%ebp), %edx
	movl	$1680763248, -67(%ebp)
	subl	$4, %esp
	movl	%eax, -92(%ebp)
	movl	(%ebx), %eax
	movl	$1634954852, -71(%ebp)
	movl	$7105636, -64(%ebp)
	movl	%edx, (%esp)
	call	*%eax
	movl	$1701147206, -47(%ebp)
	subl	$4, %esp
	movl	%eax, -96(%ebp)
	movl	8(%ebx), %eax
	movl	$1919052108, -43(%ebp)
	movl	$7959137, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 4(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1165259617, -43(%ebp)
	movl	8(%ebx), %eax
	movl	$1282696519, -47(%ebp)
	movl	$1917154419, -42(%ebp)
	movl	$7499634, -38(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 16(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$99, %ecx
	movl	$1885431112, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$1869376577, -43(%ebp)
	movw	%cx, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 24(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1885431112, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$1701147206, -43(%ebp)
	movb	$0, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 32(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movdqu	LC0, %xmm0
	movl	8(%ebx), %eax
	movl	$7496040, -31(%ebp)
	movups	%xmm0, -47(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 20(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movdqu	LC1, %xmm1
	movl	8(%ebx), %eax
	movl	$6648953, -31(%ebp)
	movups	%xmm1, -47(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 28(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1634038339, -43(%ebp)
	movl	8(%ebx), %eax
	movl	$1885431112, -47(%ebp)
	movl	$6648929, -40(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 36(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1984259444, -43(%ebp)
	movl	8(%ebx), %eax
	movl	$1634038339, -47(%ebp)
	movl	$1702249829, -42(%ebp)
	movl	$4289646, -38(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 40(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1968006516, -43(%ebp)
	movl	8(%ebx), %eax
	movl	$1634038339, -47(%ebp)
	movl	$1953844581, -42(%ebp)
	movl	$4290661, -38(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 44(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1298494305, -43(%ebp)
	movl	8(%ebx), %eax
	movl	$1701602642, -47(%ebp)
	movl	$1968006515, -42(%ebp)
	movl	$7890292, -38(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 48(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1750361460, -43(%ebp)
	movl	8(%ebx), %eax
	movl	$1634038339, -47(%ebp)
	movl	$1919439973, -42(%ebp)
	movl	$6578533, -38(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 96(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movdqu	LC2, %xmm2
	movl	8(%ebx), %eax
	movups	%xmm2, -47(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 104(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1920234348, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$1098477667, -43(%ebp)
	movb	$0, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 108(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1920234348, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$1467576419, -43(%ebp)
	movb	$0, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 112(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1920234348, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$1098146147, -43(%ebp)
	movb	$0, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 116(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1920234348, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$1467244899, -43(%ebp)
	movb	$0, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 120(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1920234348, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$1097887075, -43(%ebp)
	movb	$0, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 124(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1920234348, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$1466985827, -43(%ebp)
	movb	$0, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 128(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1298494305, -43(%ebp)
	movl	8(%ebx), %eax
	movl	$1701602642, -47(%ebp)
	movl	$1968006515, -42(%ebp)
	movl	$7890292, -38(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 48(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1634038339, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$1766221172, -43(%ebp)
	movl	$4285804, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 140(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$101, %edx
	movl	$1953067607, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$1818838629, -43(%ebp)
	movw	%dx, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 144(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1936682051, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$1851869285, -43(%ebp)
	movl	$6646884, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 52(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movdqu	LC3, %xmm3
	movl	8(%ebx), %eax
	movl	$7627621, -31(%ebp)
	movups	%xmm3, -47(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 80(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movdqu	LC4, %xmm4
	movl	8(%ebx), %eax
	movl	$7566435, -28(%ebp)
	movups	%xmm4, -47(%ebp)
	movl	$1701470799, -32(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 84(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movdqu	LC5, %xmm5
	movl	8(%ebx), %eax
	movups	%xmm5, -47(%ebp)
	movl	$5727597, -32(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 148(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1701602628, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$1766221172, -43(%ebp)
	movl	$5727596, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 152(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1701536623, -43(%ebp)
	movl	8(%ebx), %eax
	movl	$1396790103, -47(%ebp)
	movl	$4289637, -40(%ebp)
	movl	%esi, 4(%esp)
	movl	-92(%ebp), %edi
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 56(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1701736047, -43(%ebp)
	movl	8(%ebx), %eax
	movl	$1128354647, -47(%ebp)
	movl	$7627621, -40(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 60(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1396790103, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$6581861, -43(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 64(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1380012887, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$7758693, -43(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 68(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1701602675, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$7627621, -44(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 88(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movdqu	LC6, %xmm6
	movl	8(%ebx), %eax
	movl	$7629941, -28(%ebp)
	movups	%xmm6, -47(%ebp)
	movl	$1936020068, -32(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 92(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1953653108, -43(%ebp)
	movl	8(%ebx), %eax
	movl	$1396790103, -47(%ebp)
	movl	$7370100, -40(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 132(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1851876716, -43(%ebp)
	movl	8(%ebx), %eax
	movl	$1128354647, -47(%ebp)
	movl	$7370094, -40(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 136(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$108, %ecx
	movl	$1852798056, -47(%ebp)
	movl	8(%ebx), %eax
	movw	%cx, -43(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 72(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$115, %edx
	movl	$1852798056, -47(%ebp)
	movl	8(%ebx), %eax
	movw	%dx, -43(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 76(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$1936682083, -47(%ebp)
	movl	8(%ebx), %eax
	movl	$1668248421, -43(%ebp)
	movl	$7628139, -39(%ebp)
	movl	%esi, 4(%esp)
	movl	%edi, (%esp)
	call	*%eax
	movl	%eax, 100(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	movl	$2037540213, -43(%ebp)
	movl	8(%ebx), %eax
	movl	$1366519364, -47(%ebp)
	movl	$4284281, -40(%ebp)
	movl	%esi, 4(%esp)
	movl	-96(%ebp), %ecx
	movl	%ecx, (%esp)
	call	*%eax
	movl	%eax, 12(%ebx)
	subl	$8, %esp
	testl	%eax, %eax
	je	L3
	xorl	%eax, %eax
L5:
L1:
	leal	-16(%ebp), %esp
	popl	%ecx
	.cfi_remember_state
	.cfi_restore 1
	.cfi_def_cfa 1, 0
	popl	%ebx
	.cfi_restore 3
	popl	%esi
	.cfi_restore 6
	popl	%edi
	.cfi_restore 7
	popl	%ebp
	.cfi_restore 5
	leal	-4(%ecx), %esp
	.cfi_def_cfa 4, 4
	ret
	.p2align 4,,10
	.p2align 3
L3:
	.cfi_restore_state
	movl	$-1, %eax
	jmp	L1
	.cfi_endproc
LFE589:
	.section .rdata,"dr"
	.align 16
LC0:
	.long	1953264973
	.long	1954103913
	.long	1466913893
	.long	1130718313
	.align 16
LC1:
	.long	1701079383
	.long	1918986307
	.long	1968009044
	.long	1114207340
	.align 16
LC2:
	.long	1836213588
	.long	1952542313
	.long	1919439973
	.long	6578533
	.align 16
LC3:
	.long	1953063255
	.long	1400008518
	.long	1818717801
	.long	1784827749
	.align 16
LC4:
	.long	1953063255
	.long	1299345222
	.long	1769237621
	.long	1332046960
	.align 16
LC5:
	.long	1299473735
	.long	1819632751
	.long	1818838629
	.long	1835093605
	.align 16
LC6:
	.long	1195463511
	.long	1984918629
	.long	1634497125
	.long	1684369520
	.ident	"GCC: (Rev7, Built by MSYS2 project) 12.2.0"
