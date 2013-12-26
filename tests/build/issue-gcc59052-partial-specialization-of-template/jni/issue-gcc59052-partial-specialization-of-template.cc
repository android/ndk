namespace N {

template <class T>
struct C {
    template <class U, T t>
    struct Implementation {};
};

template <class T>
template <T t>
struct C<T>::Implementation<void, t> {
    static void method() {}
};

}

int main() {
    N::C<int>::Implementation<void, 0>::method();
}
