section .rodata
  fmt db "Hello, %s!", 10, 0

section .text
  global  print_hello
  extern printf

  print_hello:
    sub  esp, 28
    mov  eax, [esp+32]
    mov  [esp], dword fmt
    mov  [esp+4], eax
    call printf
    add  esp, 28
    ret
