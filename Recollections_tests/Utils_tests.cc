#include "../Utils.h"

#include <gtest/gtest.h>

// Utils::keyQuadrant()
TEST(UtilsTests, BasicAssertions) {
  EXPECT_EQ(Utils::keyQuadrant(0), QUADRANT.NW);
  EXPECT_EQ(Utils::keyQuadrant(5), QUADRANT.NW);

  EXPECT_EQ(Utils::keyQuadrant(3), QUADRANT.NE);
  EXPECT_EQ(Utils::keyQuadrant(6), QUADRANT.NE);

  EXPECT_EQ(Utils::keyQuadrant(8), QUADRANT.SW);
  EXPECT_EQ(Utils::keyQuadrant(13), QUADRANT.SW);

  EXPECT_EQ(Utils::keyQuadrant(11), QUADRANT.SE);
  EXPECT_EQ(Utils::keyQuadrant(15), QUADRANT.SE);

  EXPECT_EQ(Utils::keyQuadrant(16), QUADRANT.INVALID);
}