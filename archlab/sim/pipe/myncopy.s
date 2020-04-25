	.file	"myncopy.c"
	.text
	.globl	ncopy2
	.type	ncopy2, @function
ncopy2:
.LFB0:
	.cfi_startproc
	movl	$0, %r8d
	jmp	.L2
.L4:
	subl	$2, %edx
	movq	%rdi, %rsi
	movq	%r9, %rdi
.L2:
	cmpl	$1, %edx
	jle	.L6
	movl	(%rdi), %ecx
	leaq	8(%rdi), %r9
	movl	4(%rdi), %eax
	movl	%ecx, (%rsi)
	leaq	8(%rsi), %rdi
	movl	%eax, 4(%rsi)
	testl	%ecx, %ecx
	jle	.L3
	addl	$1, %r8d
.L3:
	testl	%eax, %eax
	jle	.L4
	addl	$1, %r8d
	jmp	.L4
.L8:
	movl	(%rdi), %eax
	movl	%eax, (%rsi)
	testl	%eax, %eax
	jle	.L7
	addl	$1, %r8d
.L7:
	subl	$1, %edx
.L6:
	testl	%edx, %edx
	jg	.L8
	movl	%r8d, %eax
	ret
	.cfi_endproc
.LFE0:
	.size	ncopy2, .-ncopy2
	.ident	"GCC: (Arch Linux 9.3.0-1) 9.3.0"
	.section	.note.GNU-stack,"",@progbits
