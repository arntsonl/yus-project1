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
	SLOTSIZE        $4000
	SLOT            0               $0000	; ROM page 0
	SLOTSIZE		$4000
	SLOT			1				$4000	; ROM page 1
	SLOTSIZE		$4000
	SLOT			2				$8000	; ROM page 2
	SLOTSIZE		$2000
	SLOT			3				$C000	; RAM
.ENDME

.ROMBANKMAP
	BANKSTOTAL      4
	banksize 		$4000
	banks 1
	banks 1
	banks 1
	banks 1
.ENDRO

.sdsctag 1.0,"Hivemind","A Mastermind SMS game","Team YUS"

.ramsection "RAM" slot 3

VAR_frame_cnt							db
VAR_game_state							db
VAR_sin_cnt								db
CurrentlyPressedButtons					db
JustPressedButtons						db
VAR_cursor_pos							db
VAR_cursor_anim							db
VAR_row_colors							dsb 4
VAR_solution_row						dsb 4
VAR_cursor_depth						db
VAR_intro_timer							dw
VAR_board_size							db
VAR_tmp_ctr								db
VAR_tmp_4array							dsb 4
VAR_win_counter							db

.ends

; Game States
.DEFINE gamestate_intro					$0
.DEFINE gamestate_title                 $1
.DEFINE gamestate_play                  $2
.DEFINE gamestate_ending                $3

; Game Colors
.DEFINE grid_blank						$0
.DEFINE grid_purple						$1
.DEFINE grid_green						$2
.DEFINE grid_blue						$3
.DEFINE grid_aqua						$4
.DEFINE grid_orange						$5
.DEFINE grid_yellow						$6

; Start -----------------------------------------------------------------------
.BANK 0 SLOT 0

.ORG   $0000
.section "Boot Section" force
	di
	im      1					; set the interrupt mode to 1
	ld      sp,     $DFF0		; set our stack pointer
	jp      start				; move to the starting function
.ends
	
;------------------------------------------------------------------------------

; Tools ---------------------------------------------------------------------
.ORG   $0010
.section "Interrupt Vector Write DE" force
vdp_write_de:
	ld      a, e
	out     (VDP_ADDR), a
	ld      a, d
	out     (VDP_ADDR), a
	ret
.ends
	
.ORG   $0018
.section "Interrupt Vector Write Addr DE" force
vdp_write_data_de:
	ld      a, e
	out     (VDP_DATA), a
	ld      a, d
	out     (VDP_DATA), a
	ret
.ends
	
.ORG   $0028
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

.ORG   $0038
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
.ORG    $0066
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
	
	cp      gamestate_ending
	jr      z, ending_update

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
	
	ld		a, (JustPressedButtons)
	and		P1_LEFT
	jp		nz, changecolor_left
	
	ld		a, (JustPressedButtons)
	and		P1_RIGHT
	jp		nz, changecolor_right
	
	; If controller 1 gave 1, do something
	; ld		a, (JustPressedButtons)
	; and		P1_BUTTON1
	; jp		nz, change_peg_color
	
	; If controller 1 gave 2, do something
	ld		a, (JustPressedButtons)
	and		P1_BUTTON1
	jp		nz, game_check_guess
	
game_update_done:
	jp      main_done
		
ending_update:
	ld      a, (JustPressedButtons)
	and     P1_BUTTON1
	; Reset to our title state
	; init game the screen

ending_done:
		
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
		jp		z, game_update_done ; Don't move past here..
		ld		(VAR_cursor_pos), a	; store it and change our cursor
		
		; A has our y value, so we want to offset by Y * 32
		inc		a
		ld		b, a						; for loop (a = count)
		ld		de, $0080					; 64 tiles per row
		ld		hl, VRAM_BG_MAP+$244		; get our offset..
movecursor_up_loop:
		add		hl, de						; find offset
		djnz	movecursor_up_loop
		
		; Get X-Offset
		ld      a, (VAR_cursor_depth)
		ld      e, a
		sla     e
		sla     e
		ld      d, $0
		add     hl, de                      ; X offset
		
		ld		de, $0040
		ld      c, $2D                      ; Y offset from top
		call	set_bg_4square_tiles        ; Set our new tiles

		ld      c, $26                      ; Y offset
		add		hl, de 
		call	set_bg_4square_tiles        ; Set our old tiles
		
		jp		game_update_done	; done

; User wants to move the cusor to the right
movecursor_down:
		ld		a, (VAR_cursor_pos)
		add		a,1
		cp		$04
		jp		z, game_update_done ; Don't move past here...
		ld		(VAR_cursor_pos), a ; store it and change our cursor
		
		; A has our y value, so we want to offset by Y * 32
		inc		a
		ld		b, a						; for loop (a = count)
		ld		de, $0080					; 64 tiles per row
		ld		hl, VRAM_BG_MAP+$1C4		; get our offset..
movecursor_down_loop:
		add		hl, de						; find offset
		djnz	movecursor_down_loop
		
		; Get X-Offset
		ld      a, (VAR_cursor_depth)
		ld      e, a
		sla     e
		sla     e
		ld      d, $0
		add     hl, de                      ; X offset
		
		ld		de, $0040
		ld      c, $26
		call	set_bg_4square_tiles           ; Set our old tiles
		
		ld      c, $2D
		add		hl, de
		call	set_bg_4square_tiles           ; Set our new tiles
		
		jp		game_update_done    ; done
		
; 4 square means each corner is the same tile, but flipped, so  Normal  Horizontal  (next line) Vertical  Horizontal-Vertical
set_bg_4square_tiles:
		rst		$28				; set VDP_ADDR = HL | $4000
		
		ld		a, c
		out		(VDP_DATA), a
		ld		a, $01
		out		(VDP_DATA), a
		
		ld		a, c
		out		(VDP_DATA), a
		ld		a, $03
		out		(VDP_DATA), a
		
		add		hl, de
		rst		$28
		
		ld		a, c
		out		(VDP_DATA), a
		ld		a, $05
		out		(VDP_DATA), a
		
		ld		a, c
		out		(VDP_DATA), a
		ld		a, $07
		out		(VDP_DATA), a
		
		ret
		
; Change the peg's color
changecolor_left:
		ld		hl, VAR_row_colors
		ld		a, (VAR_cursor_pos)
		ld		b, 0
		ld		c, a
		add		hl, bc
		ld		a, (hl)
		cp      grid_purple
		jr      nz, changecolor_decrement ; decrement a
		ld      a, grid_yellow
		ld      (hl), a
		jp		changecolor_update

changecolor_right:
		ld		hl, VAR_row_colors
		ld		a, (VAR_cursor_pos)
		ld		b, 0
		ld		c, a
		add		hl, bc
		ld		a, (hl)
		cp      grid_yellow
		jr      nz, changecolor_increment ; increment a
		ld      a, grid_purple
		ld      (hl), a
		jp      changecolor_update
		
changecolor_increment:
		inc     a
		ld      (hl), a             ; store a+1 into color array
		jp      changecolor_update
		
changecolor_decrement:
		dec     a
		ld      (hl), a             ; store a-1 into color array
		;jp      changecolor_update  ; no need to jump 1 line
		
changecolor_update:
        dec     a                   ; sprite tile is base 0, we're base 1
		ld      d, a                ; sprite color of tile
		sla     d                   ;   (*2 to get to the right spot)
		ld      a, (VAR_cursor_pos) ; find sprite index
		sla     a
		sla     a                   ; a *= 4
		sla     a
		ld      c, a                ; sprite offset (index*8) xnxnxnxn <-- format		
		sla     a                   ; a *= 16
		
		; ** NOTE ** Wrap this in a loop homie
		; Change our sprite up top
		add     a, $58                ; A = $58 + A
		ld      e, a
		
		ld      hl, $3fa0
		ld      b, $0               ; bc = $0+reg-a
		add     hl, bc              ; offset our sprite counter
		
		rst     $28                 ; Load our sprite into VDP_ADDR
		
		ld      a, e
		out     (VDP_DATA), a
		ld      a, d            ; load d (tile color) into a
		;add     a, $00         ; sprite starts at $00
		out     (VDP_DATA), a
		
		ld      a, e
		add     a, $8
		out     (VDP_DATA), a
		ld      a, d
		add     a, $01
		out     (VDP_DATA), a
		
		ld      a, e
		out     (VDP_DATA), a
		ld      a, d
		add     a, $0C
		out     (VDP_DATA), a
		
		ld      a, e
		add     a, $8
		out     (VDP_DATA), a
		ld      a, d
		add     a, $0D
		out     (VDP_DATA), a
		
		; Change our tile
		ld      hl, $3f80
		ld      b, $0
		add     hl, bc          ; offset our sprite counter
		rst     $28             ; Load our sprite into VDP_ADDR
		
		; Offset x
		ld      a, (VAR_cursor_depth)
		sla     a
		sla     a
		sla     a
		sla     a
		add     a, $10          ; offset of 16 pixels
		ld      e, a
		
		ld      a, e
		out     (VDP_DATA), a
		ld      a, d            ; load d (tile color) into a
		add     a, $C8          ; add our sprite tile offset
		out     (VDP_DATA), a
		
		ld      a, e
		add     a, $8
		out     (VDP_DATA), a
		ld      a, d
		add     a, $C9
		out     (VDP_DATA), a
		
		ld      a, e
		out     (VDP_DATA), a
		ld      a, d
		add     a, $D4
		out     (VDP_DATA), a
		
		ld      a, e
		add     a, $8
		out     (VDP_DATA), a
		ld      a, d
		add     a, $D5
		out     (VDP_DATA), a
		
		jp		game_update_done
.ends

.section "Game Checking Algorithm" free		

; User wants to try their solution
game_check_guess:
		
		; Compare the current guess with our actual solution
		xor     a
		ld      (VAR_win_counter), a
		ld      (VAR_tmp_4array), a    ; 0 out our temp array
		ld      (VAR_tmp_4array+1), a
		ld      (VAR_tmp_4array+2), a
        ld      (VAR_tmp_4array+3), a
		ld      e, a                   ; e is our count marker for displaying balls
		
		; First check for black markers
	    ld      a, 4                   ; 4 markers to check
check_black_markers:
		ld      (VAR_tmp_ctr), a       ; load a into temp counter
		ld      hl, VAR_row_colors-$1
		ld      b,a
black_inc_a:
        inc     hl
        djnz    black_inc_a
		ld      a, (hl)
		ld      c, e                   ; backup e
		ld      de, $04
		add     hl, de                 ; just offset our ram by 4 (cheap but works)
		ld      e, (hl)                ; look at the colors in our solution
		cp      e                      ; Are they equal
		ld      e, c                   ; restore e
		jr      nz, incorrect_black_block    ; Nope, keep going
		
		; Yes, one was in the correct spot!
		ld      a, (VAR_win_counter)   ; increase our win counter (for checking win)
		inc     a
		ld      (VAR_win_counter), a
		
		; Show the user a black dot
		ld      a, e                   ; backup e
		ld      b, e                   ; counter = e
		inc     b
		ld      hl, VRAM_BG_MAP+$4C6
		ld      de, $0040
black_offset:
        add     hl, de                ; increment the address of hl by 32
		djnz    black_offset       ; if counter == 0, done
		ld      e, a                   ; restore backup
		
		; Get X-Offset (push-pop to save de)
		push    de
		ld      a, (VAR_cursor_depth)
		ld      e, a
		sla     e
		sla     e
		ld      d, $0
		add     hl, de                      ; X offset
		pop     de
		
		rst     $28                    ; set VDP_ADDR = VRAM_BG_MAP+$266+(32*e)
		
		ld      a, $3c                 ; write our black peg
		out     (VDP_DATA), a
		ld      a, $01
		out     (VDP_DATA), a
		
		inc     a                      ; set a to 1
		ld      hl, VAR_tmp_4array     ; load our temp array
		add     hl, de                 ; offset to array[index]
        ld      (hl), a                ; set array[index] = 1
		
		inc     e                      ; Finally, increment our marker
		
incorrect_black_block:
        ld      a, (VAR_tmp_ctr)
		dec     a                       ; Memory is set back at top
		jp      nz, check_black_markers
		
; Done looking for matching, now we want pegs that are in the same domain
		
		; e is our offset counter
		ld      a, 4
check_white_markers:                   ; Look for white markers
        ld      (VAR_tmp_ctr), a       ; load a into temp counter
        
		ld      b, $0
		ld      c, a
		dec     c
		ld      hl, VAR_tmp_4array
		add     hl, bc                 ; lets look at this array, if its 1, we're done
		ld      a, (hl)
		and     $1                     ; was this a black peg?
		jr      nz, no_white_block     ; if it is, check the next block
		
		ld      hl, VAR_row_colors     ; what color is this peg?
		add     hl, bc                 ; row_colors[index]
		ld      c, (hl)                ; c is our color (how clever :3)
		ld      b, $4                  ; check every other peg
check_white_loop:
        ld      a, (VAR_tmp_ctr)       ; load a into temp counter
		cp      b                      ; are we on the same peg?
		jr      z, next_white_loop     ; skip this peg
		
		ld      hl, VAR_tmp_4array     ; does this peg have a 1 (already found)
		ld      a, e                   ; backup e
		ld      d, $0                  ; set de to counter-1
		ld      e, b                   ; e = counter
		dec     e                      ; e = counter-1
		add     hl, de                 ; hl + (counter-1)
		ld      e, a                   ; restore e
		ld      a, (hl)                ; load our toggler into a
		and     $01                    ; does it == 1?
		jr      nz, next_white_loop    ; ignore it!
		
		ld      hl, VAR_solution_row   ; load our solution offset
		ld      a, e                   ; store e
		ld      d, $0
		ld      e, b
		dec     e                      ; Offset to our solution index
		add     hl, de
		ld      e, a                   ; restore e
		ld      a, (hl)                ; Get the solution color
		cp      c                      ; Do these equal?
		jr      nz, next_white_loop    ; Do not equal, go on..

        ; We found equals in different columns, add a white peg!
		ld      a, e                   ; use a as our counter
		inc     a
		ld      c, e                   ; backup e
		ld      hl, VRAM_BG_MAP+$4C6
		ld      de, $0040
white_offset:
        add     hl, de                 ; increment the address of hl by 32
		dec     a
		jr      nz, white_offset
		ld      e, c                   ; restore e
		
		; Get X-Offset (push-pop to save de)
		push    de
		ld      a, (VAR_cursor_depth)
		ld      e, a
		sla     e
		sla     e
		ld      d, $0
		add     hl, de                      ; X offset
		pop     de
		
		rst     $28                    ; set VDP_ADDR = VRAM_BG_MAP+$266+(32*e)
		
		ld      a, $3d                 ; write our black peg
		out     (VDP_DATA), a
		ld      a, $01
		out     (VDP_DATA), a

		inc     e                      ; move our marker up one
		
next_white_loop:                       ; check the next peg
        djnz    check_white_loop
		
no_white_block:                         ; go here if we know there is no white peg
        ld      a, (VAR_tmp_ctr)
		dec     a                       ; Memory is set back at top
		jp      nz, check_white_markers
		
		; check to see if WE WON!
		ld      a, (VAR_win_counter) 
		cp      $4                      
		jr      nz, not_winner           ; jump to not_winner if we did not
		
; Switch our state and display winning message!
		ld      a, gamestate_ending
		ld      (VAR_game_state), a
		
		jp      check_loop_done         ; done
		
not_winner:
; Update our cursor, depth, update sprites, do some other business...

    ; Loop through our bg tiles, fill them up with our sprite bg stuff
		ld      b, $4     ; 4 background tiles to fill in
not_winner_tile_update:
		ld      de, (VAR_cursor_depth)
		ld      hl, VRAM_BG_MAP+$284
		sla     e
		sla     e
		add     hl, de
		rst     $28
		
		ld      a, $21               ; get rid of highlighting
		out     (VDP_DATA), a
		ld		a, $01
		out     (VDP_DATA), a
		
		ld      a, $22
		out     (VDP_DATA), a
		ld		a, $01
		out     (VDP_DATA), a
		
		ld      hl, VRAM_BG_MAP+$4C4
		add     hl, de
		rst     $28
		
		ld      a, $36               ; get rid of highlighting
		out     (VDP_DATA), a
		ld		a, $01
		out     (VDP_DATA), a
		
		ld      a, $37
		out     (VDP_DATA), a
		xor     a
		out     (VDP_DATA), a
		
		ld      hl, VRAM_BG_MAP+$244
	    ;sla     c
		;sla     c
		add     hl, de    ; base-tile + cursor-depth*2
		
		; Next get the height
		ld      c, b
		ld      de, $80 ; full hop
not_winner_tile_help:
        add     hl, de
		djnz    not_winner_tile_help ; loop ^		
		rst     $28  ; load up our HL into VDP_ADDR
		ld      b, c
		
		push    hl
		call    load_color_d    ; get our color into d
		pop     hl
		
		; Write our tiles
		ld      a, d            ; load d (tile color) into a
		add     a, $27          ; add our sprite tile offset
		out     (VDP_DATA), a
        xor     a               ; no flipping
		out     (VDP_DATA), a
		
		ld      a, d            ; load d (tile color) into a
		add     a, $27          ; add our sprite tile offset
		out     (VDP_DATA), a
        ld      a, $02          ; flip-h
		out     (VDP_DATA), a
		
		ld      c, d
		ld      de, $40
		add     hl, de         ; move VRAM look-up
		rst     $28             ; re-load VRAM_addr
		ld      d, c
		
		ld      a, d            ; load d (tile color) into a
		add     a, $2E          ; add our sprite tile offset
		out     (VDP_DATA), a
        xor     a               ; no flipping
		out     (VDP_DATA), a
		
		ld      a, d            ; load d (tile color) into a
		add     a, $2E          ; add our sprite tile offset
		out     (VDP_DATA), a
        ld      a, $02          ; flip-h
		out     (VDP_DATA), a
		
		djnz    not_winner_tile_update

        ld      hl, VAR_cursor_depth
		inc     (hl)                    ; increment our depth
		
		; Update the highlighted tile and cursor position
		ld      d, $0
		ld      e, (hl)
		sla     e
		sla     e
		ld      hl, VRAM_BG_MAP+$2C4
		add     hl, de
		rst     $28
		
		call    helper_draw_highlight
		
		ld      de, (VAR_cursor_depth)
		ld      hl, VRAM_BG_MAP+$284
		sla     e
		sla     e
		add     hl, de
		rst     $28
		
		ld      a, $23               ; add highlighting
		out     (VDP_DATA), a
		ld		a, $01
		out     (VDP_DATA), a
		
		ld      a, $24
		out     (VDP_DATA), a
		xor     a
		out     (VDP_DATA), a
		
				ld      hl, VRAM_BG_MAP+$4C4
		add     hl, de
		rst     $28
		
		ld      a, $38               ; add highlighting
		out     (VDP_DATA), a
		ld		a, $01
		out     (VDP_DATA), a
		
		ld      a, $39
		out     (VDP_DATA), a
		xor     a
		out     (VDP_DATA), a
		
	; Loop through each of our sprites
		ld      b, $4
not_winner_spr_update:

		ld      a, $80               ; base of sprites (bottom up)
		ld      c, b                 ; load b into c
		dec     c                    ; if 4, then 3
		sla     c                    ; 3 * 2
		sla     c                    ; 3 * 4
		sla     c                    ; 3 * 8
		add     a, c                 ; a = $80 + ((b-1)*8)
		ld      l, a                 ; HL = $3f*80*
		ld      h, $3f
		rst     $28                  ; load whats in HL into vdp_addr
		
		call    load_color_d        ; get our color into d
		sla     d                   ; d = d * 2
		
		; d is now our color/array adder
		ld      a, (VAR_cursor_depth)
		sla     a ; a = a*2
		sla     a ; a = a*4
		sla     a ; a = a*8
		sla     a ; a = a*16
		add     a, $10 ; a = a + 16
		ld      e, a                       ; store a into e
		
		ld      a, e
		out     (VDP_DATA), a
		ld      a, d            ; load d (tile color) into a
		add     a, $4D          ; add our sprite tile offset
		out     (VDP_DATA), a
		
		ld      a, e
		add     a, $8
		out     (VDP_DATA), a
		ld      a, d
		add     a, $4E
		out     (VDP_DATA), a
		
		ld      a, e
		out     (VDP_DATA), a
		ld      a, d
		add     a, $65
		out     (VDP_DATA), a
		
		ld      a, e
		add     a, $8
		out     (VDP_DATA), a
		ld      a, d
		add     a, $66
		out     (VDP_DATA), a
		
		djnz    not_winner_spr_update

;   reset our VAR_cursor_pos so its at the top again
        xor     a
		ld      (VAR_cursor_pos), a

check_loop_done:                        ; Go here
		jp		game_update_done
		
; helper function, eats up hl and e
load_color_d:
		ld      hl, VAR_row_colors         ; get mem for VAR_row_colors
		ld      d, $0
		ld      e, b
		dec     e
		add     hl, de                     ; move array incrementer
		ld      d, (hl)                    ; load memory at array[index]
		dec     d                          ; color-1 = offset
		ret
.ends		

; Palette_Set_Init -------( takes hl as the palette address ) -----------------
.section "Palette Utility" free
palette_set_init:
		ld	de, RAM
		rst	$10			; set our VDP_ADDR to RAM ($c000)
		
		ld b, 32		;32 palette registers (most of them will be set to black anyway.)
		ld c, VDP_DATA  ; out (VDP_DATA), palette;  palette += 1; b--;
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
	
	ld      de, VRAM_BG_MAP|$4000
	ld      hl, yustilemap_data
	call    LoadTilemapToVRAMNoDI
		
	xor		a						; 0 our sin counter
	ld		a, (VAR_sin_cnt)
		
	ld		bc, $400				; load an arbitrary timer
	ld		(VAR_intro_timer), bc	; load this timer into intro timer
	
	ret
		
; Title_Screen_Init -----------------------------------------------------------
title_screen_init:
	ld      de, $8100                   ; Disable display
	rst     $10
	
	ld      de, $8800
	rst     $10							; Move horizontal scroll back to 0
	
	ld		hl, title_palette
	call 	palette_set_init		; Set initial palette
	
	ld		de, VRAM_TILES|$4000
	ld		hl, title_data
	call	LoadTiles4BitRLENoDI
	
	ld      de, VRAM_BG_MAP|$4000
	ld      hl, titletilemap_data
	call    LoadTilemapToVRAMNoDI
	
	ld      de, $81C0                   ; Enable display
	rst     $10
	
	ret

; Game_Screen_Init -----------------------------------------------------------
game_screen_init:
	ld      de, $8100                   ; Disable display
	rst     $10
	
	; Init our variables
	xor		a
	ld		(VAR_cursor_pos), a         ; Y of our cursor
	ld		(VAR_cursor_anim), a       ; What was this for again?
	ld      (VAR_cursor_depth), a       ; X of our cursor
	ld		a, grid_purple
	ld		(VAR_row_colors), a
	ld		(VAR_row_colors+1), a
	ld		(VAR_row_colors+2), a
	ld		(VAR_row_colors+3), a
	
	; Debug solution = green, green, blue, aqua
	ld      a, grid_green
    ld      (VAR_solution_row), a
	ld      a, grid_green
	ld      (VAR_solution_row+1), a
	ld      a, grid_blue
	ld      (VAR_solution_row+2), a
	ld      a, grid_aqua
	ld      (VAR_solution_row+3), a
	
	ld		hl, ingame_palette
	call 	palette_set_init			; Set initial palette
	
	ld		de, VRAM_TILES|$4000	; Load sprites at the beginning
	ld		hl, gamespr_data
	call	LoadTiles4BitRLENoDI
	
	ld		de, $6000	; Load our optimized tiles.
	ld		hl, gamebg_data
	call	LoadTiles4BitRLENoDI
	
;	ld      de, VRAM_BG_MAP|$4000	; Load the tile map (will be overwritten)
;	ld      hl, gametilemap_data
;	call    LoadTilemapToVRAMNoDI
	ld		hl, VRAM_BG_MAP|$4000
	rst		$28
	
	; Clear it all out
	ld		bc, $300
clear_game_bg_loop:
	ld		de, $0100 ; 0x100
	rst		$18
	dec		bc
	ld		a, c
	or		b
	jr		nz, clear_game_bg_loop
	
	xor		a
	ld		(VAR_cursor_depth), a		; 0 out our depth (how far to the right are we?)
	
	ld		hl, VRAM_BG_MAP+$3A		; first setup
	rst		$28
	
	ld		de, $0101
	rst		$18
	
	ld		de, $0301
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$78
	rst		$28
	
	ld		de, $0102
	rst		$18
	inc		de
	rst		$18
	inc		de
	rst		$18
	
	ld		de, $0302
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$84
	rst		$28
	
	ld		de, $0105
	rst		$18
	inc		de	; $0106
	rst		$18
	
	ld		b, $17
	inc		de ; $0107
loop_aa:
	rst		$18
	djnz	loop_aa
	
	inc		de	; $0108
	rst		$18
	inc		de  ; $0109
	rst		$18
	inc		de	; $010A
	rst		$18
	ld		d, $03 ; $030A
	rst		$18
	ld		e, $09	; $0309
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$C4
	rst		$28
	ld		de, $010B
	rst		$18
	
	ld		b, $8
	inc		de ; $010C
loop_bb:
	rst		$18
	djnz	loop_bb
	
	ld		b, $4
	inc		e ; $XX0D
loop_cc:
	ld		d, $01 ; $010D
	rst		$18
	ld		d, $03 ; $030D
	rst		$18
	djnz	loop_cc
	
	ld		b, $8
	ld		de, $010C
loop_dd:
	rst		$18
	djnz	loop_dd
	
	ld		de, $010E
	rst		$18
	ld		e, $09 ; $0109
	rst		$18
	ld		e, $03	; $0103
	rst		$18
	ld		e, $04	; $0104
	rst		$18
	ld		de, $0309
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$104
	rst		$28
	
	ld		b, $1A
	ld		de, $010C
loop_ee:
	rst		$18
	djnz	loop_ee
	
	ld		e, $09 ; $0109
	rst		$18
	ld		e, $0A	; $010A
	rst		$18
	ld		d, $03	; $030A
	rst		$18
	ld		e, $09	; $0309
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$144
	rst		$28
	
	ld		b, $1A
	ld		de, $010C
loop_ff:
	rst		$18
	djnz	loop_ff
	
	ld		e, $09	; $0109
	rst		$18
	ld		e, $03	; $0103
	rst		$18
	ld		e, $04	; $0104
	rst		$18
	ld		de, $0309
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$182
	rst		$28
	ld		de, $0105
	rst		$18
	
	ld		b, $09
	ld		e, $0C ; $010C
loop_gg:
	rst		$18
	djnz	loop_gg
	
	ld		b, $04
	ld		e, $0F
loop_hh:
	ld		d, $01	; $010F
	rst		$18
	ld		d, $03	; $030F
	rst		$18
	djnz	loop_hh
	
	ld		b, $09
	ld		de, $010C
loop_ii:
	rst		$18
	djnz	loop_ii
	
	ld		e, $09	; $0109
	rst		$18
	ld		e, $0A	; $010A
	rst		$18
	ld		d, $03	; $030A
	rst		$18
	ld		e, $09	; $0309
	rst		$18

	ld		hl, VRAM_BG_MAP+$1C2
	rst		$28
	ld		de, $010B
	rst		$18
	
	ld		b, $07
	ld		e, $0C	; $010C
loop_jj:
	rst		$18
	djnz	loop_jj
	
	ld		e, $10		; $0110
	rst		$18
	
	inc		e			; $0111
	rst		$18
	
	inc		e			; $0112
	rst		$18
	
	ld		d, $03		; $0312
	rst		$18
	
	ld		de, $0110
	rst		$18
	
	ld		d, $03		;$0310
	rst		$18
	
	ld		d, $01		;$0110
	rst		$18
	
	ld		d, $03		;$0310
	rst		$18
	
	ld		de, $0113	;$0113
	rst		$18
	
	inc		e			;$0114
	rst		$18
	
	inc		e			;$0115
	rst		$18
	
	ld		e, $11		;$0111
	rst		$18
	
	ld		b, $07
	ld		e, $0C		;$010C
loop_ll:
	rst		$18
	djnz	loop_ll
	
	ld		e, $09		;$0109
	rst		$18
	ld		e, $03		;$0103
	rst		$18
	inc		e			;$0104
	rst		$18
	ld		de, $0309	
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$202
	rst		$28
	
	ld		b, $08
	ld		de, $010C
loop_mm:
	rst		$18
	djnz	loop_mm
	
	ld		e, $0F	;$010F
	rst		$18
	
	ld		b, $0A
	ld		e, $16	;$0116
loop_nn:
	rst		$18
	djnz	loop_nn
	
	ld		de, $030F
	rst		$18
	
	ld		b, $06
	ld		de, $010C
loop_oo:
	rst		$18
	djnz	loop_oo
	
	ld		e, $17		; $0117
	rst		$18
	ld		e, $09
	rst		$18			; $0109
	ld		e, $0A
	rst		$18			; $010A
	ld		d, $03
	rst		$18			; $030A
	ld		e, $09		; $0309
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$242	; offset + 4
	rst		$28
	
	ld		de, $010C
	rst		$18
	
	ld		e, $18	; $0118
	rst		$18
	
	ld		b, $0b					; loop 12 times
	
loop_a:
	ld		de, $0516
	rst		$18
	
	ld		de, $0119
	rst		$18

	djnz	loop_a                  ; loop up to Loop_a
	
	ld		de, $0516
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$274
	rst		$28						; load it into VDP_ADDR
	
	ld		b, $05					; Loop a loop of tiles..
	ld		de, $0119
loop_b:								; Load right-most stuff
	inc		e
	rst		$18
	djnz	loop_b
	
	; Next line..
	ld		hl, VRAM_BG_MAP+$282
	rst		$28
	ld		de, $0120
	rst		$18

	ld		b, $0E
	ld		d, $01		; $01XX
loop_c:
	ld		e, $21		; $0121
	rst		$18	
	ld		e, $22		; $0122
	rst		$18
	djnz	loop_c
	
	ld		de, $0320
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$2C2
	rst		$28
	ld		de, $0125
	rst		$18
	
	ld		b, $0e
	ld		e, $26		; use the same tile $XX26
loop_d:
	ld		d, $01		; $0126
	rst		$18

	ld		d, $03		; $0326
	rst		$18
	djnz	loop_d
	
	ld		e, $25		; $0325
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$302
	rst		$28
	
	ld		de, $010C
	rst		$18
	
	ld		b, $0e	; $0E loops
	ld		e, $26
loop_e:
	ld		d, $05	; $0526
	rst		$18
	ld		d, $07	; $0726
	rst		$18
	
	djnz	loop_e
	
	ld		de, $010C
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$342
	rst		$28
	
	rst		$18		; $010C is already loaded in de..
	
	; 2 more lines like this
	ld		b, $0e
	ld		e, $26
loop_f:
	ld		d, $01	; $0126
	rst		$18
	ld		d, $03	; $0326
	rst		$18
	djnz	loop_f
	
	ld		de, $010C
	rst		$18

	ld		hl, VRAM_BG_MAP+$382
	rst		$28
	
	rst		$18		; $010C is already loaded in de..
	
	; 2 more lines like this
	ld		b, $0e
	ld		e, $26
loop_g:
	ld		d, $05	;$0526
	rst		$18
	ld		d, $07	;$0726
	rst		$18
	
	djnz	loop_g
	
	ld		de, $010C
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$3C2
	rst		$28
	
	rst		$18		; $010C is already loaded in de..
	
	ld		b, $0e
	ld		e, $26
loop_h:
	ld		d, $01
	rst		$18
	ld		d, $03
	rst		$18
	
	djnz	loop_h
	
	ld		de, $010C
	rst		$18

	ld		hl, VRAM_BG_MAP+$402
	rst		$28
	
	rst		$18		; $010C is already loaded in de..
	
	ld		b, $0e
	ld		e, $26
loop_i:
	ld		d, $05	; $0526
	rst		$18
	ld		d, $07	; $0726
	rst		$18
	
	djnz	loop_i
	
	ld		de, $010C
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$442
	rst		$28
	
	rst		$18		; $010C is already loaded in de..
	
	ld		b, $0e
	ld		e, $26
loop_j:
	ld		d, $01	; $0126
	rst		$18
	
	ld		d, $03	; $0326
	rst		$18
	djnz	loop_j
	
	ld		de, $010C
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$482
	rst		$28
	
	ld		e, $34
	rst		$18
	
	ld		b, $0e
	ld		e, $26
loop_k:
	ld		d, $05	; $0526
	rst		$18
	ld		d, $07	; $0726
	rst		$18
	
	djnz	loop_k
	
	ld		de, $0334
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$4C2
	rst		$28
	
	ld		de, $0135
	rst		$18
	
	ld		b, $0e
loop_l:
	ld		e, $36	;$0136
	rst		$18
	ld		e, $37	;$0137
	rst		$18
	
	djnz	loop_l
	
	ld		de, $0335
	rst		$18

; Load the 3As
	ld		hl, VRAM_BG_MAP+$504
	rst		$28
	
	ld		b, $0E
	ld		d, $01
loop_m:
	ld		e, $3a 	; $013A
	rst		$18
	ld		e, $3b	; $013B
	rst		$18
	
	djnz	loop_m
	
; Load the 3Bs
	ld		hl, VRAM_BG_MAP+$544
	rst		$28
	
	ld		b, $0E
loop_n:
	ld		e, $3e	; $013e
	rst		$18
	ld		e, $3b	; $013b
	rst		$18
	
	djnz	loop_n
	
; Load the 3Cs
	ld		hl, VRAM_BG_MAP+$584
	rst		$28
	
	ld		b, $0E
loop_o:
	ld		e, $3f	; $013f
	rst		$18
	ld		e, $3b	; $013b
	rst		$18
	
	djnz	loop_o
	
; Load the 3Cs
	ld		hl, VRAM_BG_MAP+$5C4
	rst		$28
	
	ld		b, $0E
loop_p:
	ld		e, $40 ; $0140
	rst		$18
	ld		e, $3b ; $013b
	rst		$18
	
	djnz	loop_p
	
	; Load some start sprites
	ld		hl, $3f80			; Sprite 0
	rst		$28
	
	ld		b, $04
loop_q:
	ld		de, $C810			; Tile $00, Sprite 0, X=$80
	rst		$18
	ld		de, $C918			; Tile $01, Sprite 1, X=$88
	rst		$18
	ld		de, $D410			; Tile $0C, Sprite 2, X=$80
	rst		$18
	ld		de, $D518			; Tile $0D, Sprite 3, X=$88
	rst		$18
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
	ld		a, $00
	out		(VDP_DATA), a
	
	ld		a, d
	add		a, $8			; add 8, put back in d as well
	out		(VDP_DATA), a
	ld		a, $01
	out		(VDP_DATA), a
	
	ld		a, d
	out		(VDP_DATA), a
	ld		a, $0C
	out		(VDP_DATA), a
	
	ld		a, d
	add		a, $8			; add 8, put back in d as well
	out		(VDP_DATA), a
	ld		a, $0D
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
	
	ld		de, $00d0			; Stop displaying sprites at this point..
	
	; Set our background on selected row ([$23, $24], $2D, [$38, $39])
	ld		hl, VRAM_BG_MAP+$284
	rst		$28
	
	ld		de, $0123
	rst		$18
	
	inc		de		; $0124
	rst		$18
	
	ld		hl, VRAM_BG_MAP+$2C4
	rst		$28

	call    helper_draw_highlight ; expects HL
	
	ld		hl, VRAM_BG_MAP+$4C4
	rst		$28
	
	ld		de, $0138
	rst		$18
	inc		de		; $0139
	rst		$18
	
	ld      de, $81C0                   ; Enable display
	rst     $10
	
	ld		de, $8800					; reset our horizontal scroll
	rst		$10
	
	ret
	
helper_draw_highlight:
	ld		de, $012D
	rst		$18
	
	ld		d, $03	; $032D
	rst		$18
	
	ld      de, $0040
	add     hl, de
	rst		$28
	
	ld		de, $052D
	rst		$18
	
	ld		d, $07		; $072D
	rst		$18
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

.bank 2 slot 2
;------------------------------------------------------------------------------
.section "all palettes" force
	intro_palette:
	.db $00 $18 $3F $1C $14 $0F $1A $06 $1E $10 $15 $1B $1F $00 $00 $00 ; bg
	.db $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 ; spr

	title_palette:
	.db $00 $14 $18 $29 $1C $3D $1E $3F $00 $00 $00 $00 $00 $00 $00 $00 ; bg
	.db $00 $30 $01 $30 $03 $03 $34 $05 $17 $39 $0B $3F $0F $3F $1F $0F ; spr

	ingame_palette:
	.db $10 $11 $22 $14 $34 $17 $18 $09 $38 $0B $1C $0E $3D $1E $0F $3F ; bg
	.db $00 $10 $20 $30 $11 $22 $14 $34 $17 $09 $38 $0B $0E $3D $0F $3F ; spr
.ends

;------------------------------------------------------------------------------
;sprites_data:
;.INCLUDE "sprites.inc"
.section "Game Data" free
	gamebg_data:
	.INCBIN "board.pscompr"

	gamespr_data:
	.INCBIN "sprites.pscompr"
.ends

.section "Title Background" free
	title_data:
	.INCBIN "title.pscompr"
	
	titletilemap_data:
	.INCBIN "title_tiles.pscompr"
.ends

.section "Team YUS Background" free
	yuscompr_data:
	.INCBIN "yuslogo.pscompr"

	yustilemap_data:
	.INCBIN "yuslogo_tiles.pscompr"
.ends

.section "Wave Look-up Table" free
	wave_lut:
	.include "out.inc"
.ends
