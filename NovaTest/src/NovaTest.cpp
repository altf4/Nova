#include <gtest/gtest.h>


#include "tester_Config.h"
#include "tester_FeatureSet.h"
#include "tester_Suspect.h"
#include "tester_SuspectTable.h"
#include "tester_ClassificationEngine.h"
#include "tester_RequestMessage.h"
#include "tester_HoneydConfiguration.h"

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
