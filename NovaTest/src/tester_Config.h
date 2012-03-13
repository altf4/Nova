#include "gtest/gtest.h"

#include "Config.h"
#include "math.h"

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
	Config::Inst()->setEnabledFeatures(enabledFeatureString);

	// Check the enabled feature mask string
	EXPECT_EQ(enabledFeatureString, Config::Inst()->getEnabledFeatures());

	// Check the count of enabled features
	EXPECT_EQ(5, Config::Inst()->getEnabledFeatureCount());

	// Check the squrt of the enabled features
	EXPECT_EQ(sqrt(5), Config::Inst()->getSqurtEnabledFeatures());

	// Check if it correctly set all the enabled/disabled feature values
	EXPECT_TRUE(Config::Inst()->isFeatureEnabled(0));
	EXPECT_TRUE(Config::Inst()->isFeatureEnabled(1));
	EXPECT_FALSE(Config::Inst()->isFeatureEnabled(2));
	EXPECT_FALSE(Config::Inst()->isFeatureEnabled(3));
	EXPECT_TRUE(Config::Inst()->isFeatureEnabled(4));
	EXPECT_FALSE(Config::Inst()->isFeatureEnabled(5));
	EXPECT_FALSE(Config::Inst()->isFeatureEnabled(6));
	EXPECT_TRUE(Config::Inst()->isFeatureEnabled(7));
	EXPECT_TRUE(Config::Inst()->isFeatureEnabled(8));
}

