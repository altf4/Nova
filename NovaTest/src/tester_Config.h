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

TEST_F(ConfigTest, test_ReaderWriter)
{
	Config::Inst();
	EXPECT_TRUE(Config::Inst()->WriteSetting("INTERFACE", "foobar"));
	EXPECT_EQ(Config::Inst()->ReadSetting("INTERFACE"), "foobar");

	EXPECT_TRUE(Config::Inst()->WriteSetting("INTERFACE", "eth0"));
	EXPECT_EQ(Config::Inst()->ReadSetting("INTERFACE"), "eth0");
}


// Tests that changing the enabled features sets all needed config options
TEST_F(ConfigTest, test_setEnabledFeatures) {
	string enabledFeatureString = "1100100111100";
	Config::Inst()->SetEnabledFeatures(enabledFeatureString);

	// Check the enabled feature mask string
	EXPECT_EQ(enabledFeatureString, Config::Inst()->GetEnabledFeatures());

	// Check the count of enabled features
	EXPECT_EQ((uint)8, Config::Inst()->GetEnabledFeatureCount());

	// Check the squrt of the enabled features
	EXPECT_EQ(sqrt(8), Config::Inst()->GetSqurtEnabledFeatures());

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
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(9));
	EXPECT_TRUE(Config::Inst()->IsFeatureEnabled(10));
	EXPECT_FALSE(Config::Inst()->IsFeatureEnabled(11));
	EXPECT_FALSE(Config::Inst()->IsFeatureEnabled(12));


}

