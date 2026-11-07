    .set noreorder
    .set noat

    .text
    .balign 4

/* Asia retains the unreachable return without the padding of Europe and Japan. */
    .globl GameModeMainLoop
    .type GameModeMainLoop, @function
GameModeMainLoop:
    addiu      $sp, $sp, -0x18
    sw         $ra, 0x14($sp)
.Lgame_mode_loop:
    jal        CallCurrentGameMode
    nop
    b          .Lgame_mode_loop
    nop
#if TGR_REGION != TGR_REGION_ASIA
    .rept 3
    nop
    .endr
#endif
    lw         $ra, 0x14($sp)
    addiu      $sp, $sp, 0x18
    jr         $ra
    nop
    .size GameModeMainLoop, . - GameModeMainLoop
