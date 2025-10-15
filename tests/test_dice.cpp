#include <catch2/catch_test_macros.hpp>

#include "../include/Util.hpp"

#include <string>

using namespace tutorial::dice;

// ========================
// Basic Dice Rolling Tests
// ========================

TEST_CASE("DiceExpression parses simple dice notation", "[dice][parser]")
{
    SECTION("Single die")
    {
        auto expr = DiceExpression::Parse("1d6");

        // Check bounds
        REQUIRE(expr.Min() == 1);
        REQUIRE(expr.Max() == 6);

        // Roll multiple times and verify range
        for (int i = 0; i < 100; ++i)
        {
            int result = expr.Roll();
            REQUIRE(result >= 1);
            REQUIRE(result <= 6);
        }
    }

    SECTION("Multiple dice")
    {
        auto expr = DiceExpression::Parse("3d6");

        REQUIRE(expr.Min() == 3);
        REQUIRE(expr.Max() == 18);

        for (int i = 0; i < 100; ++i)
        {
            int result = expr.Roll();
            REQUIRE(result >= 3);
            REQUIRE(result <= 18);
        }
    }

    SECTION("Implicit 1dX notation")
    {
        auto expr = DiceExpression::Parse("d20");

        REQUIRE(expr.Min() == 1);
        REQUIRE(expr.Max() == 20);
    }
}

TEST_CASE("DiceExpression handles arithmetic modifiers", "[dice][arithmetic]")
{
    SECTION("Addition")
    {
        auto expr = DiceExpression::Parse("1d6+3");

        REQUIRE(expr.Min() == 4); // 1+3
        REQUIRE(expr.Max() == 9); // 6+3

        for (int i = 0; i < 100; ++i)
        {
            int result = expr.Roll();
            REQUIRE(result >= 4);
            REQUIRE(result <= 9);
        }
    }

    SECTION("Subtraction")
    {
        auto expr = DiceExpression::Parse("1d6-2");

        REQUIRE(expr.Min() == -1); // 1-2 (can be negative!)
        REQUIRE(expr.Max() == 4);  // 6-2
    }

    SECTION("Multiplication")
    {
        auto expr = DiceExpression::Parse("1d6*2");

        REQUIRE(expr.Min() == 2);  // 1*2
        REQUIRE(expr.Max() == 12); // 6*2
    }

    SECTION("Division")
    {
        auto expr = DiceExpression::Parse("1d6/2");

        REQUIRE(expr.Min() == 0); // 1/2 (integer division)
        REQUIRE(expr.Max() == 3); // 6/2
    }
}

TEST_CASE("DiceExpression handles complex expressions", "[dice][complex]")
{
    SECTION("Multiple dice types")
    {
        auto expr = DiceExpression::Parse("1d8+2d6+3");

        REQUIRE(expr.Min() == 6);  // 1+2+3
        REQUIRE(expr.Max() == 23); // 8+12+3
    }

    SECTION("Parentheses for order of operations")
    {
        auto expr1 = DiceExpression::Parse("(1d4+1)*2");
        REQUIRE(expr1.Min() == 4);  // (1+1)*2
        REQUIRE(expr1.Max() == 10); // (4+1)*2

        auto expr2 = DiceExpression::Parse("1d4+1*2");
        REQUIRE(expr2.Min() == 3); // 1+(1*2)
        REQUIRE(expr2.Max() == 6); // 4+(1*2)
    }
}

TEST_CASE("DiceExpression handles keep highest modifier", "[dice][modifiers]")
{
    SECTION("4d6kh3 (D&D ability score generation)")
    {
        auto expr = DiceExpression::Parse("4d6kh3");

        REQUIRE(expr.Min() == 3);  // Keep 3 dice, minimum 1 each
        REQUIRE(expr.Max() == 18); // Keep 3 dice, maximum 6 each

        // Roll many times - result should favor higher values
        int total = 0;
        int rolls = 1000;
        for (int i = 0; i < rolls; ++i)
        {
            int result = expr.Roll();
            REQUIRE(result >= 3);
            REQUIRE(result <= 18);
            total += result;
        }

        // Average should be around 12-13
        double average = static_cast<double>(total) / rolls;
        REQUIRE(average >= 11.5);
        REQUIRE(average <= 13.5);
    }

    SECTION("2d20kh1 (D&D 5e Advantage)")
    {
        auto expr = DiceExpression::Parse("2d20kh1");

        REQUIRE(expr.Min() == 1);
        REQUIRE(expr.Max() == 20);
    }
}

TEST_CASE("DiceExpression handles exploding dice", "[dice][modifiers]")
{
    SECTION("1d6! (exploding d6)")
    {
        auto expr = DiceExpression::Parse("1d6!");

        REQUIRE(expr.Min() == 1);
        REQUIRE(expr.Max() == 12); // Approximate

        // Most rolls should be normal, some explode
        for (int i = 0; i < 100; ++i)
        {
            int result = expr.Roll();
            REQUIRE(result >= 1);
        }
    }
}

TEST_CASE("DiceExpression handles max function", "[dice][functions]")
{
    SECTION("max(1, 1d4-2) ensures minimum damage")
    {
        auto expr = DiceExpression::Parse("max(1, 1d4-2)");

        REQUIRE(expr.Min() == 1); // max(1, -1) = 1
        REQUIRE(expr.Max() == 2); // max(1, 2) = 2

        // Should never go below 1
        for (int i = 0; i < 100; ++i)
        {
            int result = expr.Roll();
            REQUIRE(result >= 1);
            REQUIRE(result <= 2);
        }
    }
}

TEST_CASE("DiceExpression throws on invalid input", "[dice][errors]")
{
    SECTION("Empty string")
    {
        REQUIRE_THROWS(DiceExpression::Parse(""));
    }

    SECTION("Invalid dice notation")
    {
        REQUIRE_THROWS(DiceExpression::Parse("d"));
        REQUIRE_THROWS(DiceExpression::Parse("3d"));
    }

    SECTION("Unmatched parentheses")
    {
        REQUIRE_THROWS(DiceExpression::Parse("(1d6"));
        REQUIRE_THROWS(DiceExpression::Parse("1d6)"));
    }
}

TEST_CASE("DiceExpression handles real RPG scenarios", "[dice][scenarios]")
{
    SECTION("D&D 5e Longsword + Sneak Attack")
    {
        // Longsword (1d8) + Dex modifier (+3) + Sneak Attack (2d6)
        auto expr = DiceExpression::Parse("1d8+3+2d6");

        REQUIRE(expr.Min() == 6);  // 1+3+2
        REQUIRE(expr.Max() == 23); // 8+3+12
    }

    SECTION("D&D 5e Fireball (8d6)")
    {
        auto expr = DiceExpression::Parse("8d6");

        REQUIRE(expr.Min() == 8);
        REQUIRE(expr.Max() == 48);
    }
}
