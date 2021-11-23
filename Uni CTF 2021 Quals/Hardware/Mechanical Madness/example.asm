:start
	movl ax, 10
:sub1
	movl bx, 1
	sub  bx
	cmp  ax,  ax
	movl bx,  :sub1
	jnz
	rst