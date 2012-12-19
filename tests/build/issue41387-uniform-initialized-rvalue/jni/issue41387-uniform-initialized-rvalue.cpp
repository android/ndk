struct pair {
    int a, b;
    void method() const {}
};

int main() {
    (pair { 0, 0 }).method();
}

