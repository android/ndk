#ifndef GLOBAL_CTOR_H
#define GLOBAL_CTOR_H

class GlobalCtor
{
private:
    int Value;

public:
    GlobalCtor();

    bool IsInitialized() const
    {
        return (Value == 42);
    }
};

#endif // GLOBAL_CTOR_H
