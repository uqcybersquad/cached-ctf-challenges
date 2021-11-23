:start
	movl cx, 10
	clr
	movl bx,  1
	movl dx,  0
	mmiv 0x0,  dx
	movl bx,  :sub4
	call bx,  0
	mmov ax, 0x0
	movl bx,  :sub5
	call bx,  0
:sub1
	movl bx,  1
	movl dx,  0
	push dx,  0
	movl bx,  :sub5
	call bx,  0
	movl bx,  1
	sub  bx,  0
	cmp  ax, ax
	movl bx, :sub1
	jnz
	movl bx,  :sub4
	call bx,  0
:sub2
	movl bx,  0
	mmiv 0x1, bx
	mmiv 0x2, bx
:sub3
	pop  ax,  0
	movl bx,  1
	movl dx,  0
	movl bx,  :sub5
	call bx,  0
	mmov bx, 0x1
	msk
	mmiv 0x1, bx
	mmov bx, 0x2
	mskb
	mmiv 0x2, bx
	movl ax,  0xff
	cmp  bx, ax
	movl bx, :sub3
	jl
	movl bx,  0
	mmov dx, 0x1
	movl cx,  1
	movl cx,  0
	movl bx,  :sub2
	jmp  bx,  0
:sub4
	movl ax,  0x05
	movl bx,  sub5
	call bx,  0
	movl bx,  1
	sub  bx,  0
	cmp  ax,  ax
	movl bx,  :sub4+1
	jnz
	ret
:sub5
	movl cx,  4
	movl cx,  0
	ret
