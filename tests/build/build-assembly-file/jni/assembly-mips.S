       .text
       .align  2
       .ent    foo
       .globl  foo
       .set    noreorder
foo:
       jr      $31
       nop
       .end    foo
