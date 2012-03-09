#include <gtest/gtest.h>


#include "tester_Config.h"
#include "tester_FeatureSet.h"
#include "tester_Suspect.h"
#include "SuspectTable.h"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
