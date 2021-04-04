
#include <gtest/gtest.h>

#include "cli_args_tests.h"
#include "auxl_tests/all.h"
#include "hw_tests/all.h"
#include "alg_tests/all.h"
#include "chn_tests/all.h"
#include "ctrl_tests/all.h"
#include "plg_tests/all.h"


int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
