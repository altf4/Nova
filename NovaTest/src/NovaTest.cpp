#include <gtest/gtest.h>


#include "tester_Config.h"
#include "tester_FeatureSet.h"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
