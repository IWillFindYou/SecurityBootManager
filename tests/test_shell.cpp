#include <iostream>
using namespace std;

extern bool assert_func();

int main() {
  if (!assert_func())
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}

