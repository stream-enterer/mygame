#ifndef UTIL_HPP
#define UTIL_HPP

#include "Position.hpp"

#include <memory>
#include <string>

namespace tutorial
{
    namespace util
    {
        inline std::string capitalize(const std::string& string)
        {
            auto ret = string;

            auto ch = ret[0];
            ret[0] = std::toupper(ch);

            return ret;
        }

        constexpr int posToIndex(pos_t pos, int width)
        {
            return (pos.x + pos.y * width);
        }

        constexpr pos_t indexToPos(int i, int width)
        {
            int y = i / width;
            int x = i - y * width;

            return pos_t{ x, y };
        };
    } // namespace util

    // Dice expression parser for RPG-style dice notation
    // Supports: 3d6+4, 4d6kh3, 2d20kh1 (advantage), 1d6!, etc.
    namespace dice
    {
        // Modifiers that can be applied to a dice roll
        enum class DiceModifier
        {
            NONE,
            KEEP_HIGHEST, // kh3: keep highest 3 dice
            KEEP_LOWEST,  // kl3: keep lowest 3 dice
            DROP_HIGHEST, // dh1: drop highest die
            DROP_LOWEST,  // dl1: drop lowest die
            EXPLODE,      // !: reroll and add on max
            REROLL,       // r1: reroll values <= N
            REROLL_ONCE   // ro1: reroll values <= N (once)
        };

        // A single dice group like "3d6" or "4d6kh3"
        struct DiceGroup
        {
            int count;             // Number of dice (3 in "3d6")
            int sides;             // Die size (6 in "3d6")
            DiceModifier modifier; // Optional modifier
            int modifierValue;     // Parameter for modifier (3 in "kh3")

            DiceGroup(int count = 1, int sides = 6);

            int Roll() const; // Roll and apply modifiers
            int Min() const;  // Minimum possible result
            int Max() const;  // Maximum possible result (approx for exploding)
        };

        // A complete dice expression like "2d6+1d4+3" or "max(1, 1d4-2)"
        class DiceExpression
        {
        public:
            DiceExpression() = default;

            // Parse from string
            static DiceExpression Parse(const std::string& expr);

            // Evaluate the expression
            int Roll() const;
            int Min() const;
            int Max() const;

            // For debugging
            std::string ToString() const;

        public:
            // Abstract syntax tree node
            struct Node
            {
                virtual ~Node() = default;
                virtual int Evaluate() const = 0;
                virtual int Min() const = 0;
                virtual int Max() const = 0;
                virtual std::string ToString() const = 0;
            };

        private:
            std::unique_ptr<Node> root_;

            // Parser implementation
            class Parser;
        };

    } // namespace dice

} // namespace tutorial

#endif // UTIL_HPP
