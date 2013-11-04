#include <stdio.h>

#include <exception>

bool handle_exception(std::exception_ptr exc) {
  if (exc == std::exception_ptr()) {
    fprintf(stderr, "Empty exception pointer, should not happen!\n");
    return false;
  }
  bool success = true;
  try {
    std::rethrow_exception(exc);
    success = false;
  } catch(const std::exception& e) {
    printf("Caught exception \"%s\"\n", e.what());
  }
  if (!success)
    fprintf(stderr, "Could not rethrow exception");
  return success;
}

int main() {
  std::exception_ptr exc;
  try {
      throw int(4);
  } catch (...) {
      exc = std::current_exception(); // capture
  }

  if (!handle_exception(exc))
    return 1;

  return 0;
}
