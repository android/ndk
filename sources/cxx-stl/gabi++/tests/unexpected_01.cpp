#include <cassert>
#include <exception>

class allowed_exception {
};

void throw_allowed_exception() {
  throw allowed_exception();
}

int main() {
  std::set_unexpected(throw_allowed_exception);
  try {
    std::unexpected(); // it is OK to throw exception from std::unexpected()
    assert(false);
  } catch (const allowed_exception &) {
    assert(true);
  } catch (...) {
    assert(false);
  }
  return 0;
}
