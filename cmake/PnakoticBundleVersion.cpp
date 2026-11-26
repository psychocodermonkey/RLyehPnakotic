#include "PnakoticBuildVersion.h"

#include <iostream>

int main() {
  using namespace RLyeh::Pnakotic;

  std::cout << Build::Version.version.view();
}
