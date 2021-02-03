.syntax unified

  .global Init_section_fs_data

  .section .init, "ax"
  .code 16
  .balign 2
  .thumb_func
Init_section_fs_data:

ldr r0, =__fs_data_load_start__
ldr r1, =__fs_data_start__
ldr r2, =__fs_data_end__
bl memory_copy
ldr r2, =main
blx r2

  .thumb_func
memory_copy:
  cmp r0, r1
  beq 2f
  subs r2, r2, r1
  beq 2f
1:
  ldrb r3, [r0]
  adds r0, r0, #1
  strb r3, [r1]
  adds r1, r1, #1
  subs r2, r2, #1
  bne 1b
2:
  bx lr