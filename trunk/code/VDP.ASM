;----------------------------------------------------------------------------
; Sega Master System & Game Gear - VDP functions
; by Omar Cornut / Bock|Zoop (c) in February 2001
; extracted from 'Happy Looser!' demo.
;----------------------------------------------------------------------------

; VDP_INIT() ----------------------------------------------------------------
; Initialize default VDP registers, clear VRAM, clear sprites
;----------------------------------------------------------------------------
; no parameters
;----------------------------------------------------------------------------
.section "VDP Init" free
vdp_init:
                in      a, (VDP_STATUS)         ; Read VDP status once
                ld      hl, vdp_init_table      ; before using VDP
                ld      b, 11*2                 ;
                ld      c, VDP_ADDR             ; Then upload default
                otir                            ; registers.
; VDP_CLEAR() ---------------------------------------------------------------
; Clear VRAM, clear sprites
;----------------------------------------------------------------------------
; no parameters
;----------------------------------------------------------------------------
vdp_clear:                                      ;
                ld      de, $0000               ; Clear VRAM
                rst     $18                     ; Set VDP address to DE
                ld      bc, VRAM_SIZE           ;
vdp_clear_loop:                                 ;
                xor     a
                out     (VDP_DATA), a           ;
                dec     bc                      ;
                ld      a, b                    ;
                or      c                       ;
                jr      nz, vdp_clear_loop      ;	
; VDP_DISABLE_SPRITES() -----------------------------------------------------
; Clear sprites (by setting the first sprite position to 208)
;----------------------------------------------------------------------------
; no parameters
;----------------------------------------------------------------------------
vdp_disable_sprites:
                ld      de, VRAM_SPR_MAP+1      ; Disable sprites
                rst     $18                     ;
                ld      a, VRAM_SPR_LAST        ;
                out     (VDP_DATA), a           ;
                ret
.ends	
			
;----------------------------------------------------------------------------
.section "VDP Init Table" free
vdp_init_table:
.db             $16, $80,  $80, $81,  $FF, $82,  $FF, $83,  $FF, $84
.db             $FF, $85,  $FB, $86,  $00, $87,  $00, $88,  $00, $89
.db             $00, $8A
.ends
;----------------------------------------------------------------------------

; VDP_LOAD_DATA() -----------------------------------------------------------
; Load data from given source to video memory
;----------------------------------------------------------------------------
; bc = number of bytes
; hl = source in ROM/RAM
; de = destination in VRAM
;----------------------------------------------------------------------------
.section "VDP Load Data" free
vdp_load_data:
                push    hl
                rst     $18                     ; Set VDP address to DE
vdp_load_data_loop:
                ld      a, (hl)
                inc     hl
                out     (VDP_DATA), a
                dec     bc
                ld      a, b
                or      c
                jr      nz, vdp_load_data_loop
                pop     hl
                ret
.ends
;----------------------------------------------------------------------------

; VDP_BG_PUTTILE() ----------------------------------------------------------
; Put a tile to background tile map, starting at *current* VRAM address
;----------------------------------------------------------------------------
;  b = number of times
;  d = tile number
;  e = attribute (bit 0 must not be set)
; hl = VRAM address
;----------------------------------------------------------------------------
.section "VDP Put Tile" free
vdp_bg_puttiles:
                rst     $28                     ; Set VDP address to HL
                push    ix
                pop     ix
vdp_bg_puttile_x:
                ld      a, d
                out     (VDP_DATA), a
                push    ix
                pop     ix
                ld      a, e
                out     (VDP_DATA), a
                djnz    vdp_bg_puttile_x
                ret
.ends

.section "VDP Put Single Tile" free
vdp_bg_puttile:
                rst     $28                     ; Set VDP address to HL
                push    ix
                pop     ix
                ld      a, d
                out     (VDP_DATA), a
                push    ix
                pop     ix
                ld      a, e
                out     (VDP_DATA), a
                ret
.ends

;----------------------------------------------------------------------------
;  Wait for the vrame update..
;
;----------------------------------------------------------------------------
.section "VDP One Frame" free
vdp_frame_one:
        ;ld      b, 1
vdp_frame:
        xor     a
        ld      (VAR_frame_cnt), a
vdp_frame_loop:
        ei
        halt
        di
        ld      a, (VAR_frame_cnt)
        and     a
        jr      z, vdp_frame_loop
        ;djnz    vdp_frame          ; <-- why is this not hitting a zero flag?
        ;dec     b
		;jr      nz, vdp_frame
		ret
.ends
