;
; Mastermind (SMS)
; by Cthulhu32 - March 2011
; For the SMSPower.org Competition 2011
;

.INCLUDE "sms.inc"

.EMPTYFILL $00
.SMSTAG
.COMPUTESMSCHECKSUM

.MEMORYMAP
        DEFAULTSLOT     0
        SLOTSIZE        PAGE_SIZE
        SLOT            0               PAGE_0
        SLOT            1               PAGE_1
.ENDME

.ROMBANKMAP
        BANKSTOTAL      2
        BANKSIZE        PAGE_SIZE
        BANKS           2
.ENDRO

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
.DEFINE VAR_cursor_height				(RAM + $1F)		; 1 byte
.DEFINE VAR_next_var					(RAM + $20)		; * bytes

; Game States
.DEFINE gamestate_title                 $0
.DEFINE gamestate_play                  $1

; Start -----------------------------------------------------------------------
.ORGA   $0000
        di
        im      1					; set the interrupt mode to 1
        ld      sp,     $DFF0		; set our stack pointer
        jp      start				; move to the starting function
		
;------------------------------------------------------------------------------

; Tools ---------------------------------------------------------------------
.ORGA   $0010
vdp_write_de:
                ld      a, e
                out     (VDP_ADDR), a
                ld      a, d
                out     (VDP_ADDR), a
                ret
.ORGA   $0018
vdp_write_addr_de:
                ld      a, e
                out     (VDP_ADDR), a
                ld      a, d
                or      $40
                out     (VDP_ADDR), a
                ret

.ORGA   $0028
vdp_write_addr_hl:
                ld      a, l
                out     (VDP_ADDR), a
                ld      a, h
                or      $40
                out     (VDP_ADDR), a
                ret

; Interrupt -------------------------------------------------------------------
.ORGA   $0038
interrupt:
        di
		push	af
        in      a, (VDP_STATUS)
        and     $80
        jr      z, h_interrupt
		
interrupt_end:
		
		ld      a, (VAR_frame_cnt)
        inc     a
        ld      (VAR_frame_cnt), a
		
		pop 	af
        ei
        ret

;------------------------------------------------------------------------------

; NMI -------------------------------------------------------------------------
.ORGA   $0066
        reti
;------------------------------------------------------------------------------

; SDSC HEADER DATA ------------------------------------------------------------
sdsc_author:            .db     "Cthulhu32", 0
sdsc_program_name:      .db     "Mastermind SMS", 0
;------------------------------------------------------------------------------

h_interrupt:
		; Do a sine scroller if we're on the title screen
		ld      a, (VAR_game_state)
		cp      gamestate_title
		jr      nz, h_interrupt_cont		; need to do the sine scroller, or keep going
		
sine_scroller:		
		in		a, (PORT_VLINE)
		cp      $bf							; break at a guaranteed point
		jr		nz, sine_noinc
		ld      bc, (VAR_sin_cnt)
		inc     c
		ld      (VAR_sin_cnt), bc
		
sine_noinc:
		; Only scroll the banner :3
		;sub		$88
		;jp		M, h_interrupt_cont			; negative, jump!
		;in		a, (PORT_VLINE)
		
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
		
h_interrupt_cont:
		; Do something else
		
		jp      interrupt_end
		

.INCLUDE "vdp.asm"

start:
		di
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
		
		call	title_screen_init
		
        ld      de, $81C0               ; Enable display
        rst     $10
		
loop:
;        halt
	; wait for vblank
        call    vdp_frame_one
		
main_update:	
	; Lets check the game-state and look at what to do
		ld		a, (VAR_game_state)
		
		call	game_input
		
		cp      gamestate_title
		jr      z, title_update
		
		cp      gamestate_play
		jr      z, game_update
		
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
	and		P1_LEFT
	jp		nz, movecursor_left
	
	ld		a, (JustPressedButtons)
	and		P1_RIGHT
	jp		nz, movecursor_right
	
game_leftright_done:
	
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

; Game related functions -----------------------------------------------------

; User wants to move the cursor to the left
movecursor_left:
		ld		a, (VAR_cursor_pos)
		sub		1
		cp		$ff		; we hit -1
		jr		nz, movecursor_left_done
		ld		a, 3	; loop back to 3
movecursor_left_done:
		ld		(VAR_cursor_pos), a
		jp		game_leftright_done	; done

; User wants to move the cusor to the right
movecursor_right:
		ld		a, (VAR_cursor_pos)
		add		a,1
		and		$3					; loop if necessary
		ld		(VAR_cursor_pos), a
		jp		game_leftright_done
		
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
		
; Palette_Set_Init -------( takes hl as the palette address ) -----------------
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
		
; Title_Screen_Init -----------------------------------------------------------
title_screen_init:
		ld		hl, title_palette
		call 	palette_set_init		; Set initial palette
		
		ld      bc, VRAM_TILE_SIZE*$A5
		ld      hl, yuslogo_data
		ld      de, $0000
		call    vdp_load_data
		
		ld        bc, $600
		ld        hl, yustilemap_data
		ld        de, VRAM_BG_MAP
		call      vdp_load_data
		
		xor		a						; 0 our sin counter
		ld		a, (VAR_sin_cnt)
		
		ret

; Game_Screen_Init -----------------------------------------------------------
game_screen_init:
		ld      de, $8100                   ; Disable display
        rst     $10
		
		ld		hl, ingame_palette
		call 	palette_set_init		; Set initial palette
		
		ld      bc, VRAM_TILE_SIZE*$a5
		ld      hl, gamebg_data
		ld      de, $0000
		call    vdp_load_data
		
		ld        bc, $600
		ld        hl, gametilemap_data
		ld        de, VRAM_BG_MAP
		call      vdp_load_data
		
		ld      de, $81C0                   ; Enable display
        rst     $10
		
		
		ld		de, $8800					; reset our horizontal scroll
		rst		$10
		
		ret

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
		
;------------------------------------------------------------------------------
title_palette:
.db $00 $10 $14 $15 $06 $18 $1A $1B $1C $1E $0F $1F $3F $00 $00 $00 ; bg
.db $33 $30 $01 $30 $03 $03 $34 $05 $17 $39 $0B $3F $0F $3F $1F $0F ; spr

ingame_palette:
.db $10 $11 $22 $14 $34 $17 $18 $09 $38 $0B $1C $0E $3D $1E $0F $3F ; bg
.db $00 $01 $01 $00 $02 $15 $17 $15 $16 $2A $2A $27 $2A $2A $3F $3F ; spr

;------------------------------------------------------------------------------
;sprites_data:
;.INCLUDE "sprites.inc"

gamebg_data:
.INCLUDE "game_bg.inc"

gametilemap_data:
.INCLUDE "tilemap.inc"

yuslogo_data:
.include "yuslogo.inc"

yustilemap_data:
.include "yustilemap.inc"

wave_lut:
.include "out.inc"

;------------------------------------------------------------------------------

.BANK 1 SLOT 1

; SDSC HEADER -----------------------------------------------------------------
.ORGA   $7FE0
        .DB     "SDSC"                  ; Magic
        .DB     $01, $00                ; Version 1.00
        .DB     $28                     ; 27
        .DB     $03                     ; March
        .DW     $2011                   ; 2011
        .DW     sdsc_author
        .DW     sdsc_program_name
        .DW     $FFFF

; CHECKSUM --------------------------------------------------------------------
.ORGA   $7FF0
	
	.DB	"TMR SEGA"	; Trademark
    .DW $0120       ; Year
	.DW	$0000		; Checksum not correct, WLALINK corrects this!
	.DW	$0000		; Part Num not correct
	.DB $00         ; Version
    .DB $4C         ; Master System, 32k
	