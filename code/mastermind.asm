;
; Mastermind (SMS)
; by Team YUS: Cthulhu32, Ptoing - April 2011
;

.INCLUDE "sms.inc"

.EMPTYFILL $00
.SMSTAG
.COMPUTESMSCHECKSUM

.MEMORYMAP
	DEFAULTSLOT     0
	SLOTSIZE        PAGE_SIZE
	SLOT            0               PAGE_0
	SLOT			1				PAGE_1
.ENDME

.ROMBANKMAP
	BANKSTOTAL      2
	banksize 		PAGE_SIZE
	banks 2
.ENDRO

.sdsctag 1.0,"Hivemind","A Mastermind SMS game","Team YUS"

.BANK 0 SLOT 0

.DEFINE VAR_frame_cnt                   (RAM + $10)     ; 1 byte
.DEFINE VAR_game_state					(RAM + $11)		; 1 byte
.DEFINE VAR_sin_cnt						(RAM + $12)     ; 1 byte
.DEFINE CurrentlyPressedButtons			(RAM + $13)		; 1 byte
.DEFINE JustPressedButtons				(RAM + $14)		; 1 byte
.DEFINE VAR_cursor_pos					(RAM + $15)		; 1 byte, [0 1 2 3]
.DEFINE VAR_cursor_anim					(RAM + $16)		; 1 byte
.DEFINE VAR_row_colors					(RAM + $17)		; 4 bytes  [0 through 5] [0 1 2 3]
.DEFINE VAR_solution_row				(RAM + $1B)		; 4 bytes  [0 through 5] [0 1 2 3]
.DEFINE VAR_cursor_depth				(RAM + $1F)		; 1 byte
.DEFINE VAR_intro_timer					(RAM + $20)		; 2 bytes
.DEFINE VAR_board_size					(RAM + $22)		; 1 byte	[8, 12, 14, 16] ?
.DEFINE VAR_next_var					(RAM + $23)		; * bytes

; Game States
.DEFINE gamestate_intro					$0
.DEFINE gamestate_title                 $1
.DEFINE gamestate_play                  $2

; Game Colors
.DEFINE grid_blank						$0
.DEFINE grid_purple						$1
.DEFINE grid_green						$2
.DEFINE grid_blue						$3
.DEFINE grid_aqua						$4
.DEFINE grid_orange						$5
.DEFINE grid_yellow						$6

; Start -----------------------------------------------------------------------
.ORGA   $0000
.section "Boot Section" force
	di
	im      1					; set the interrupt mode to 1
	ld      sp,     $DFF0		; set our stack pointer
	jp      start				; move to the starting function
.ends
	
;------------------------------------------------------------------------------

; Tools ---------------------------------------------------------------------
.ORGA   $0010
.section "Interrupt Vector Write DE" force
vdp_write_de:
	ld      a, e
	out     (VDP_ADDR), a
	ld      a, d
	out     (VDP_ADDR), a
	ret
.ends
	
.ORGA   $0018
.section "Interrupt Vector Write Addr DE" force
vdp_write_addr_de:
	ld      a, e
	out     (VDP_ADDR), a
	ld      a, d
	or      $40
	out     (VDP_ADDR), a
	ret
.ends
	
.ORGA   $0028
.section "Interrupt Vector Write Addr HL" force
vdp_write_addr_hl:
	ld      a, l
	out     (VDP_ADDR), a
	ld      a, h
	or      $40
	out     (VDP_ADDR), a
	ret
.ends
	
; Interrupt -------------------------------------------------------------------

.ORGA   $0038
.section "Interrupt" force
interrupt:
	di
	push	af
	in      a, (VDP_STATUS)
	and     $80
	jp      z, h_interrupt
	
interrupt_end:
	
	ld      a, (VAR_frame_cnt)
	inc     a
	ld      (VAR_frame_cnt), a
	
	pop 	af
	ei
	ret
.ends
	
;------------------------------------------------------------------------------

; NMI -------------------------------------------------------------------------
.ORGA   $0066
.section "Return NMI" force
        reti
.ends
;------------------------------------------------------------------------------

.section "Main Start" free
start:
	
	call    vdp_init

	; Set all these variables to 0
	xor     a
	ld		(VAR_game_state), a		; Set game state to title screen
	ld		(CurrentlyPressedButtons), a
	ld		(JustPressedButtons), a
	ld		(VAR_cursor_pos), a
	ld		(VAR_cursor_anim), a
	ld		(VAR_row_colors), a
	ld		(VAR_row_colors+1), a
	ld		(VAR_row_colors+2), a
	ld		(VAR_row_colors+3), a
	
	call	intro_screen_init
	
	ld		de, $8016
	rst		$10
	
	ld      de, $81C0               ; Enable display
    rst     $10
		
loop:
;        halt
	; wait for vblank
	call    vdp_frame_one
		
main_update:	
	; Lets check the game-state and look at what to do
	call	game_input
	
	ld		a, (VAR_game_state)

	cp		gamestate_intro
	jr		z, intro_update
	
	cp      gamestate_title
	jr      z, title_update
	
	cp      gamestate_play
	jr      z, game_update

intro_update:
	; Go to title screen if timer has expired
	ld		bc, (VAR_intro_timer)	; load timer
	dec		bc
	ld		a, b
	or		c						; has time expired?
	jr		nz, intro_check_button 
	
	ld		(VAR_intro_timer), bc   ; loads 0 into intro timer
	call	title_screen_init
	ld		a, gamestate_title
	ld		(VAR_game_state), a

intro_check_button:
	ld		a, (JustPressedButtons)
	and		P1_BUTTON1
	jr		z, intro_done
	ld		bc, $01

intro_done:
	ld		(VAR_intro_timer), bc
	jp		main_done
		
title_update:
	
	ld		a, (JustPressedButtons)
	and		P1_BUTTON1
	jr		z, title_done
	
	; Start game if button 1 pressed
	call	game_screen_init
	ld		a, gamestate_play
	ld		(VAR_game_state), a
	
title_done:
	jp		main_done
	
game_update:
	;If controller 1 gave L,R, do something
	ld		a, (JustPressedButtons)
	and		P1_UP
	jp		nz, movecursor_up
	
	ld		a, (JustPressedButtons)
	and		P1_DOWN
	jp		nz, movecursor_down
	
game_updown_done:
	
	; If controller 1 gave 1, do something
	ld		a, (JustPressedButtons)
	and		P1_BUTTON1
	jp		nz, change_peg_color
	
	; If controller 1 gave 2, do something
	ld		a, (JustPressedButtons)
	and		P1_BUTTON2
	jp		nz, game_nextline
	
game_12_done:
	
	; Update what else is needed
	call game_update_all
		
main_done:

	; Reset if its called for
	in		a, (PORT_INPUT2)
	cpl
	and		RESET_BUTTON
	jp		nz, 0				; always reset

    jp      loop
.ends

.section "Horizontal Interrupt Handler" free
h_interrupt:
	ld      a, (VAR_game_state)
	cp      gamestate_intro				; Do a sine scroller if we're in the intro state
	jr      nz, h_interrupt_cont		; need to do the sine scroller, or keep going
	
sine_scroller:
	in		a, (PORT_VLINE)

	cp      $bf							; only increment our sine-lookup index 
	jr		nz, sine_noinc				;  once per update
	ld      bc, (VAR_sin_cnt)
	inc		c
	ld      (VAR_sin_cnt), bc
	
sine_noinc:
	ld		c, a						; our load swap requires 8 cycles, but it works!
	adc		a, $98						; Only scroll the banner :3
	jp		nc, sine_noscroll			; < $98, gtfo out of here.. (carry flag is set if a >=)
	ld		a, c						; swap back
	
	; Add it to a
	ld      bc, (VAR_sin_cnt)
	add     a, c
	
	; Set X scroll (could self-modify in RAM)		
	ld      hl, wave_lut
	ld      c, a
	ld      b, $0
	add     hl, bc				; get the value at wave_lut[a]
	ld      e, (hl)				; set our de to $88 + horiz scroll wave
	ld      d, VREG_HSCROLL
	rst     10h

	jp		interrupt_end

sine_noscroll:
	ld		de, $8800			; resets this horizontal line to 0
	rst		10h

h_interrupt_cont:
	; Do something else
	jp      interrupt_end
.ends	

.INCLUDE "vdp.asm"

; Game related functions -----------------------------------------------------
.section "Game Functions" free
; User wants to move the cursor to the left
movecursor_up:
		ld		a, (VAR_cursor_pos)
		sub		1
		cp		$ff		; we hit -1
		jp		z, game_updown_done ; Don't move past here..
		ld		(VAR_cursor_pos), a	; store it and change our cursor
		
		; A has our y value, so we want to offset by Y * 32
		inc		a
		ld		b, a						; for loop (a = count)
		ld		de, $0080					; 64 tiles per row
		ld		hl, VRAM_BG_MAP+$244		; get our offset..
movecursor_up_loop:
		add		hl, de						; find offset
		djnz	movecursor_up_loop
		ld		de, $0040
		
		; Set our new tiles
		call	set_new_tiles
				
		; Set our old tiles
		add		hl, de
		call	set_old_tiles
		
		jp		game_updown_done	; done

; User wants to move the cusor to the right
movecursor_down:
		ld		a, (VAR_cursor_pos)
		add		a,1
		cp		$04
		jp		z, game_updown_done ; Don't move past here...
		ld		(VAR_cursor_pos), a ; store it and change our cursor
		
		; A has our y value, so we want to offset by Y * 32
		inc		a
		ld		b, a						; for loop (a = count)
		ld		de, $0080					; 64 tiles per row
		ld		hl, VRAM_BG_MAP+$1C4		; get our offset..
movecursor_down_loop:
		add		hl, de						; find offset
		djnz	movecursor_down_loop
		ld		de, $0040
		
		; Set our old tiles
		call	set_old_tiles
		
		; Set our new tiles
		add		hl, de
		call	set_new_tiles
		
		jp		game_updown_done    ; done
		
set_new_tiles:
		rst		$28				; set VDP_ADDR = HL | $4000
		
		ld		a, $2D
		out		(VDP_DATA), a
		xor		a
		out		(VDP_DATA), a
		
		ld		a, $2D
		out		(VDP_DATA), a
		ld		a, $02
		out		(VDP_DATA), a
		
		add		hl, de
		rst		$28
		ld		a, $2D
		out		(VDP_DATA), a
		ld		a, $04
		out		(VDP_DATA), a
		
		ld		a, $2D
		out		(VDP_DATA), a
		ld		a, $06
		out		(VDP_DATA), a
		
		ret
	
set_old_tiles:
		rst		$28				; set VDP_ADDR = HL | $4000

		ld		a, $26
		out		(VDP_DATA), a
		xor		a
		out		(VDP_DATA), a
		
		ld		a, $26
		out		(VDP_DATA), a
		ld		a, $02
		out		(VDP_DATA), a
		
		add		hl, de
		rst		$28
		ld		a, $26
		out		(VDP_DATA), a
		ld		a, $04
		out		(VDP_DATA), a
		ld		a, $26
		out		(VDP_DATA), a
		ld		a, $06
		out		(VDP_DATA), a
		
		ret
		
; User wants to change the color of this peg
change_peg_color:
		ld		hl, VAR_row_colors
		ld		b, 0
		ld		a, (VAR_cursor_pos)
		ld		c, a 
		add		hl, bc				
		ld		a, (hl)					; a = VAR_row_colors[VAR_cursor_pos]
		add		a, 1					; a += 1
		cp		$6						; make it 0 if its 6 (0..5 colors)
		jr		nz, change_peg_done
		xor		a
change_peg_done
		ld		(hl), a					; save it back into VAR_row_colors[VAR_cursor_pos]
		jp		game_12_done
		
; User wants to try their solution
game_nextline:
		jp		game_12_done
		
; Update all the game-related stuff  ------------------------------------------
game_update_all:
		ld		a, (JustPressedButtons)
		and		P1_LEFT|P1_RIGHT
		jr		nz, game_update_sprite
		
		ld		a, (JustPressedButtons)
		and		P1_BUTTON1
		jp		nz, game_update_color
		
		ld		a, (JustPressedButtons)
		and		P1_BUTTON2
		jp		nz, game_update_nextline
		
	game_update_sprite:
		; Look at our current position
		ld		a, (VAR_cursor_pos)
		
		; And what height we are at
		
		jp game_update_done
		
	game_update_color:
		; Look at what position we are in
		
		; And look at what height we are at
		
		; Then change the tile color at this point
		jp game_update_done
	
	game_update_nextline:

	game_update_done:
		
		; Update sprite animations, bgs, anything else
		
		ret
.ends		

; Palette_Set_Init -------( takes hl as the palette address ) -----------------
.section "Palette Utility" free
palette_set_init:
		ld a, $0
		out ($bf), a
		ld a, $c0
		out ($bf), a  ;setting the VDP to $c000 lets us write the palette registers 
		;ld hl, game_palette
		ld b, 32		;32 palette registers (most of them will be set to black anyway.)
		ld c,$be
		otir
        ret
.ends

.include "Phantasy Star decompressors.inc"
	
.section "Init Functions" free
; Intro_Screen_Init -----------------------------------------------------------
intro_screen_init:
	ld		hl, intro_palette
	call 	palette_set_init		; Set initial palette

	ld		de, VRAM_TILES|$4000
	ld		hl, yuscompr_data
	call	LoadTiles4BitRLENoDI
	
	ld      bc, $600
	ld      hl, yustilemap_data
	ld      de, VRAM_BG_MAP
	call    vdp_load_data
		
	xor		a						; 0 our sin counter
	ld		a, (VAR_sin_cnt)
		
	ld		bc, $400				; load an arbitrary timer
	ld		(VAR_intro_timer), bc	; load this timer into intro timer
	
	ret
		
; Title_Screen_Init -----------------------------------------------------------
title_screen_init:
	ld      de, $8100                   ; Disable display
	rst     $10
	
	ld		hl, title_palette
	call 	palette_set_init		; Set initial palette
	
	ld		de, VRAM_TILES|$4000
	ld		hl, title_data
	call	LoadTiles4BitRLENoDI
	
	ld        bc, $600
	ld        hl, titletilemap_data
	ld        de, VRAM_BG_MAP
	call      vdp_load_data
	
	ld      de, $81C0                   ; Enable display
	rst     $10
	
	ret

; Game_Screen_Init -----------------------------------------------------------
game_screen_init:
	ld      de, $8100                   ; Disable display
	rst     $10
	
	ld		hl, ingame_palette
	call 	palette_set_init			; Set initial palette
	
	ld		de, VRAM_TILES|$4000	; Load our optimized tiles.
	ld		hl, gamebg_data
	call	LoadTiles4BitRLENoDI
	
	ld		de, $4820				; magic number :/
	ld		hl, gamespr_data
	call	LoadTiles4BitRLENoDI
	
	ld      bc, $240				; Load the top of the tile map
	ld      hl, gametilemap_data
	ld      de, VRAM_BG_MAP
	call    vdp_load_data
	
	xor		a
	ld		(VAR_cursor_depth), a		; 0 out our depth (how far to the right are we?)
	
	ld		hl, VRAM_BG_MAP+$242	; offset + 4
	rst		$28
	ld		a, $0C
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, $18
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		d, $16
	ld		e, $19
	ld		b, $0B					; loop 12 times
	
loop_a:
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $04
	out		(VDP_DATA), a
	
	ld		a, e
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	                                
	djnz	loop_a                  ; loop up to Loop_a
	
	ld		a, $16
	out		(VDP_DATA), a
	ld		a, $04
	out		(VDP_DATA), a
	
	ld		b, $05					; Loop a loop of tiles..
	ld		hl, VRAM_BG_MAP+$274
	rst		$28						; load it into VDP_ADDR
	ld		d, $19
	
loop_b:								; Load right-most stuff
	inc		d
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $00
	out		(VDP_DATA), a
	djnz	loop_b
	
	; Next line..
	ld		hl, VRAM_BG_MAP+$282
	rst		$28
	ld		a, $20
	out		(VDP_DATA), a
	ld		a, $00
	out		(VDP_DATA), a

	ld		b, $0E
	ld		d, $21
	ld		e, $22
loop_c:
	ld		a, d
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, e
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	djnz	loop_c
	
	ld		a, $20
	out		(VDP_DATA), a
	ld		a, $02
	out		(VDP_DATA), a
	
	ld		hl, VRAM_BG_MAP+$2C2
	rst		$28
	ld		a, $25
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		b, $0e
	ld		d, $26		; use the same tile
loop_d:
	ld		a, d
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $02
	out		(VDP_DATA), a
	djnz	loop_d
	
	ld		a, $25
	out		(VDP_DATA), a
	ld		a, $02
	out		(VDP_DATA), a
	
	ld		hl, VRAM_BG_MAP+$302
	rst		$28
	ld		a, $0C
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		b, $0e
	ld		d, $26
loop_e:
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $04
	out		(VDP_DATA), a
	
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $06
	out		(VDP_DATA), a
	djnz	loop_e
	
	ld		a, $0C
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		hl, VRAM_BG_MAP+$342
	rst		$28
	ld		a, $0C
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	; 2 more lines like this
	ld		b, $0e
	ld		d, $26
loop_f:
	ld		a, d
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $02
	out		(VDP_DATA), a
	djnz	loop_f
	
	ld		a, $0C
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a

	ld		hl, VRAM_BG_MAP+$382
	rst		$28
	ld		a, $0C
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	; 2 more lines like this
	ld		b, $0e
	ld		d, $26
loop_g:
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $04
	out		(VDP_DATA), a
	
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $06
	out		(VDP_DATA), a
	djnz	loop_g
	
	ld		a, $0C
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		hl, VRAM_BG_MAP+$3C2
	rst		$28
	ld		a, $0C
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		b, $0e
	ld		d, $26
loop_h:
	ld		a, d
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $02
	out		(VDP_DATA), a
	djnz	loop_h
	
	ld		a, $0C
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a

	ld		hl, VRAM_BG_MAP+$402
	rst		$28
	ld		a, $0C
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		b, $0e
	ld		d, $26
loop_i:
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $04
	out		(VDP_DATA), a
	
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $06
	out		(VDP_DATA), a
	djnz	loop_i
	
	ld		a, $0C
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		hl, VRAM_BG_MAP+$442
	rst		$28
	ld		a, $0C
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		b, $0e
	ld		d, $26
loop_j:
	ld		a, d
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $02
	out		(VDP_DATA), a
	djnz	loop_j
	
	ld		a, $0C
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		hl, VRAM_BG_MAP+$482
	rst		$28
	ld		a, $34
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		b, $0e
	ld		d, $26
loop_k:
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $04
	out		(VDP_DATA), a
	
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $06
	out		(VDP_DATA), a
	djnz	loop_k
	
	ld		a, $34
	out		(VDP_DATA), a
	ld		a, $02
	out		(VDP_DATA), a
	
	ld		hl, VRAM_BG_MAP+$4C2
	rst		$28
	ld		a, $35
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		b, $0e
	ld		d, $36
	ld		e, $37
loop_l:
	ld		a, d
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, e
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	djnz	loop_l
	
	ld		a, $35
	out		(VDP_DATA), a
	ld		a, $02
	out		(VDP_DATA), a

; Load the 3As
	ld		d, $3A
	ld		e, $3B
	ld		hl, VRAM_BG_MAP+$504
	rst		$28
	ld		b, $0E
	
loop_m:
	ld		a, d
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, e
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	djnz	loop_m
	
; Load the 3Bs
	ld		d, $3E
	ld		e, $3B
	ld		b, $0E
	ld		hl, VRAM_BG_MAP+$544
	rst		$28
	
loop_n:
	ld		a, d
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, e
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	djnz	loop_n
	
; Load the 3Cs
	ld		d, $3F
	;ld		e, $3B	; already declared above
	ld		b, $0E
	ld		hl, VRAM_BG_MAP+$584
	rst		$28
	
loop_o:
	ld		a, d
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, e
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	djnz	loop_o
	
; Load the 3Cs
	ld		d, $40
	;ld		e, $3B
	ld		b, $0E
	ld		hl, VRAM_BG_MAP+$5C4
	rst		$28
	
loop_p:
	ld		a, d
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, e
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	djnz	loop_p
	
	; Load some start sprites
	ld		b, $04
	ld		hl, $3f80			; Sprite 0
	rst		$28
loop_q:
	ld		a, $10				; X = 80
	out		(VDP_DATA), a
	ld		a, $4D				; Tile $41
	out		(VDP_DATA), a
	ld		a, $18				; sprite 1, x = $88
	out		(VDP_DATA), a
	ld		a, $4E				; Tile $42
	out		(VDP_DATA), a
	ld		a, $10				; Sprite 2, x = $80
	out		(VDP_DATA), a
	ld		a, $65				; $tile 59
	out		(VDP_DATA), a
	ld		a, $18				; sprite 3, x = $88
	out		(VDP_DATA), a
	ld		a, $66				; $tile 5A
	out		(VDP_DATA), a
	djnz	loop_q
	
	ld		b, $04				;  Counter
	ld		hl, $3f00			; Base of Ys
	rst		$28					; Load DE -> VDP_ADDR
	ld 		a, $4F				; start a at $4F (increments by $8)
loop_r:
	add		a, $8
	out 	(VDP_DATA), a	;sets Y coord of sprite 0 to about mid center.
	out 	(VDP_DATA), a
	add		a, $8
	out		(VDP_DATA), a
	out		(VDP_DATA), a
	djnz	loop_r
	
	; Load some start sprites
	ld		d, $58				; Starting X
	ld		b, $04
	ld		hl, $3fA0			; Sprite 0
	rst		$28
loop_s:
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $41
	out		(VDP_DATA), a
	
	ld		a, d
	add		a, $8			; add 8, put back in d as well
	out		(VDP_DATA), a
	ld		a, $42
	out		(VDP_DATA), a
	
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $59
	out		(VDP_DATA), a
	
	ld		a, d
	add		a, $8			; add 8, put back in d as well
	out		(VDP_DATA), a
	ld		a, $5A
	out		(VDP_DATA), a
	
	; Add 16 to the mix
	ld		a, d
	add		a, $10
	ld		d, a
	
	djnz	loop_s
	
	ld		b, $04				;  Counter
	ld		hl, $3f10			; Base of Ys
	rst		$28					; Load HL|$4000 -> VDP_ADDR
	ld 		a, $4F				; start a at $4F (increments by $8)
loop_t:
	ld		a, $1F
	out 	(VDP_DATA), a		;sets Y coord of sprite 0 to about mid center.
	out 	(VDP_DATA), a
	ld		a, $27
	out		(VDP_DATA), a
	out		(VDP_DATA), a
	djnz	loop_t
	
	ld		a, $d0				; Stop displaying sprites at this point..
	out		(VDP_DATA), a
	ld		a, $00
	out		(VDP_DATA), a
	
	; Set our background on selected row ([$23, $24], $2D, [$38, $39])
	ld		hl, VRAM_BG_MAP+$284
	rst		$28
	ld		a, $23
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, $24
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		hl, VRAM_BG_MAP+$2C4
	rst		$28
	ld		a, $2D
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, $2D
	out		(VDP_DATA), a
	ld		a, $02
	out		(VDP_DATA), a
	
	ld		hl, VRAM_BG_MAP+$304
	rst		$28
	ld		a, $2D
	out		(VDP_DATA), a
	ld		a, $04
	out		(VDP_DATA), a
	
	ld		a, $2D
	out		(VDP_DATA), a
	ld		a, $06
	out		(VDP_DATA), a
	
	ld		hl, VRAM_BG_MAP+$4C4
	rst		$28
	ld		a, $38
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld		a, $39
	out		(VDP_DATA), a
	xor		a
	out		(VDP_DATA), a
	
	ld      de, $81C0                   ; Enable display
	rst     $10
	
	ld		de, $8800					; reset our horizontal scroll
	rst		$10
	
	ret
.ends
	
.section "Controller Input Function" free
game_input:
	; Read controller input
	in		a, (PORT_INPUT1)
	cpl
	
	; Only care about L, R, 1, and 2
	and		P1_UP|P1_DOWN|P1_LEFT|P1_RIGHT|P1_BUTTON1|P1_BUTTON2
	ld		b, a
	ld		hl, CurrentlyPressedButtons
	xor		(hl)			; Xor input ^ currentlypressed
	ld		(hl), b			; currentlypressed = input
	and		b				; a = all buttons since last time
	ld		(JustPressedButtons), a
		
	ret
.ends		

;------------------------------------------------------------------------------
.section "all palettes" superfree
	intro_palette:
	.db $00 $10 $14 $15 $06 $18 $1A $1B $1C $1E $0F $1F $3F $00 $00 $00 ; bg
	.db $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 ; spr

	title_palette:
	.db $10 $1C $14 $15 $06 $18 $1A $1B $1C $1E $0F $1F $3F $00 $00 $00 ; bg
	.db $00 $30 $01 $30 $03 $03 $34 $05 $17 $39 $0B $3F $0F $3F $1F $0F ; spr

	ingame_palette:
	.db $10 $11 $22 $14 $34 $17 $18 $09 $38 $0B $1C $0E $3D $1E $0F $3F ; bg
	.db $00 $10 $09 $22 $38 $14 $0E $11 $0B $34 $3D $17 $0F $00 $00 $00 ; spr
.ends

;------------------------------------------------------------------------------
;sprites_data:
;.INCLUDE "sprites.inc"
.section "Game Data" superfree
	gamebg_data:
	.INCBIN "board.pscompr"

	gamespr_data:
	.INCBIN "sprites.pscompr"
	
	gametilemap_data:
	.INCLUDE "tilemap.inc"
.ends

.section "Title Background" superfree
	title_data:
	.INCBIN "title.pscompr"
	
	titletilemap_data:
	.INCLUDE "titlemap.inc"
.ends

.section "Team YUS Background" superfree
	yuscompr_data:
	.INCBIN "yuslogo.pscompr"

	yustilemap_data:
	.INCLUDE "yustilemap.inc"
.ends

.section "Wave Look-up Table" superfree
	wave_lut:
	.include "out.inc"
.ends
