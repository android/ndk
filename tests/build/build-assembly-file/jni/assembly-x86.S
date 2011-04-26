.global foo
foo:
    pushl   %ebp
    movl    %esp, %ebp
    subl    $8, %esp

    movl    $0, %eax
    leave
    ret
