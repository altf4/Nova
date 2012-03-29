#include "gtest/gtest.h"

#include "Config.h"
#include "math.h"

using namespace std;
using namespace Nova;

class ConfigTest : public ::testing::Test {
protected:
	virtual void SetUp() {
		// Let Config call it's own constructor and get an instance
		Config::Inst();
	}

};

// Make sure the instance is real
TEST_F(ConfigTest, test_instanceNotNull)
{
	EXPECT_NE((Config*)0, Config::Inst());
}


// Tests that changing the enabled features sets all needed config options
TEST_F(ConfigTest, test_setEnabledFeatures) {
	string enabledFeatureString = "110010011";
	Config::Inst()->SetEnabledFeatures(enabledFeatureString);

	// Check the enabled feature mask string
	EXPECT_EQ(enabledFeatureString, Config::Inst()->GetEnabledFeatures());

	// Check the count of enabled features
	EXPECT_EQ((uint)5, Config::Inst()->GetEnabledFeatureCount());

	// Check the squrt of the enabled features
	EXPECT_EQ(sqrt(5), Config::Inst()->GetSqurtEnabledFeatures());

	// Check if it correctly set all the enabled/disabled feature values
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(0));
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(1));
	EXPECT_FALSE(Config::Inst()->IsFeatureEnabled(2));
	EXPECT_FALSE(Config::Inst()->IsFeatureEnabled(3));
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(4));
	EXPECT_FALSE(Config::Inst()->IsFeatureEnabled(5));
	EXPECT_FALSE(Config::Inst()->IsFeatureEnabled(6));
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(7));
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(8));
}

