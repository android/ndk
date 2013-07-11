struct s {
    unsigned int x:1;
    unsigned int unused:15;
    unsigned short y;
};

union u {
    struct s s;
    volatile int i;
};

void foo(union u *ptr, int flag);
