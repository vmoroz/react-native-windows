#include "pch.h"

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main(int argc, char *argv[]) {
  winrt::init_apartment();
  return Catch::Session().run(argc, argv);
}
