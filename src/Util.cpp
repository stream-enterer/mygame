#include "Util.hpp"

#include <libtcod/mersenne.hpp>

#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace tutorial::dice
{
    // ========================
    // DiceGroup Implementation
    // ========================

    DiceGroup::DiceGroup(int count, int sides) :
        count(count),
        sides(sides),
        modifier(DiceModifier::NONE),
        modifierValue(0)
    {
    }

    int DiceGroup::Roll() const
    {
        if (count <= 0 || sides <= 0)
        {
            return 0;
        }

        auto* rng = TCODRandom::getInstance();
        std::vector<int> rolls;
        rolls.reserve(count);

        // Initial rolls
        for (int i = 0; i < count; ++i)
        {
            rolls.push_back(rng->getInt(1, sides));
        }

        // Apply exploding dice
        if (modifier == DiceModifier::EXPLODE)
        {
            std::vector<int> exploded;
            for (int roll : rolls)
            {
                int value = roll;
                while (value == sides)
                {
                    int extra = rng->getInt(1, sides);
                    value += extra;
                    if (extra != sides) break; // Stop exploding
                }
                exploded.push_back(value);
            }
            rolls = std::move(exploded);
        }

        // Apply rerolls
        if (modifier == DiceModifier::REROLL
            || modifier == DiceModifier::REROLL_ONCE)
        {
            bool rerollOnce = (modifier == DiceModifier::REROLL_ONCE);

            for (int& roll : rolls)
            {
                while (roll <= modifierValue)
                {
                    roll = rng->getInt(1, sides);
                    if (rerollOnce) break;
                }
            }
        }

        // Apply keep/drop modifiers
        if (modifier == DiceModifier::KEEP_HIGHEST
            || modifier == DiceModifier::KEEP_LOWEST
            || modifier == DiceModifier::DROP_HIGHEST
            || modifier == DiceModifier::DROP_LOWEST)
        {
            std::sort(rolls.begin(), rolls.end());

            std::vector<int> kept;

            if (modifier == DiceModifier::KEEP_HIGHEST)
            {
                // Keep N highest (last N elements after sort)
                int n = std::min(modifierValue, static_cast<int>(rolls.size()));
                kept.assign(rolls.end() - n, rolls.end());
            }
            else if (modifier == DiceModifier::KEEP_LOWEST)
            {
                // Keep N lowest (first N elements after sort)
                int n = std::min(modifierValue, static_cast<int>(rolls.size()));
                kept.assign(rolls.begin(), rolls.begin() + n);
            }
            else if (modifier == DiceModifier::DROP_HIGHEST)
            {
                // Drop N highest (keep all but last N)
                int n = std::min(modifierValue, static_cast<int>(rolls.size()));
                kept.assign(rolls.begin(), rolls.end() - n);
            }
            else if (modifier == DiceModifier::DROP_LOWEST)
            {
                // Drop N lowest (keep all but first N)
                int n = std::min(modifierValue, static_cast<int>(rolls.size()));
                kept.assign(rolls.begin() + n, rolls.end());
            }

            rolls = std::move(kept);
        }

        // Sum all kept rolls
        int sum = 0;
        for (int roll : rolls)
        {
            sum += roll;
        }

        return sum;
    }

    int DiceGroup::Min() const
    {
        if (count <= 0 || sides <= 0)
        {
            return 0;
        }

        // Minimum depends on modifiers
        int effectiveCount = count;

        if (modifier == DiceModifier::KEEP_HIGHEST
            || modifier == DiceModifier::KEEP_LOWEST)
        {
            effectiveCount = std::min(count, modifierValue);
        }
        else if (modifier == DiceModifier::DROP_HIGHEST
                 || modifier == DiceModifier::DROP_LOWEST)
        {
            effectiveCount = std::max(0, count - modifierValue);
        }

        return effectiveCount; // Minimum is 1 per die
    }

    int DiceGroup::Max() const
    {
        if (count <= 0 || sides <= 0)
        {
            return 0;
        }

        int effectiveCount = count;

        if (modifier == DiceModifier::KEEP_HIGHEST
            || modifier == DiceModifier::KEEP_LOWEST)
        {
            effectiveCount = std::min(count, modifierValue);
        }
        else if (modifier == DiceModifier::DROP_HIGHEST
                 || modifier == DiceModifier::DROP_LOWEST)
        {
            effectiveCount = std::max(0, count - modifierValue);
        }
        else if (modifier == DiceModifier::EXPLODE)
        {
            // Exploding dice have no real maximum, use approximate
            return effectiveCount * sides * 2; // Approximate
        }

        return effectiveCount * sides;
    }

    // ========================================================================
    // DiceExpression::Parser Implementation
    // ========================================================================

    // AST Node types
    namespace
    {
        struct NumberNode : public DiceExpression::Node
        {
            int value;

            explicit NumberNode(int value) : value(value)
            {
            }

            int Evaluate() const override
            {
                return value;
            }
            int Min() const override
            {
                return value;
            }
            int Max() const override
            {
                return value;
            }
            std::string ToString() const override
            {
                return std::to_string(value);
            }
        };

        struct DiceNode : public DiceExpression::Node
        {
            DiceGroup dice;

            explicit DiceNode(const DiceGroup& dice) : dice(dice)
            {
            }

            int Evaluate() const override
            {
                return dice.Roll();
            }
            int Min() const override
            {
                return dice.Min();
            }
            int Max() const override
            {
                return dice.Max();
            }
            std::string ToString() const override
            {
                std::ostringstream oss;
                oss << dice.count << "d" << dice.sides;

                switch (dice.modifier)
                {
                    case DiceModifier::KEEP_HIGHEST:
                        oss << "kh" << dice.modifierValue;
                        break;
                    case DiceModifier::KEEP_LOWEST:
                        oss << "kl" << dice.modifierValue;
                        break;
                    case DiceModifier::DROP_HIGHEST:
                        oss << "dh" << dice.modifierValue;
                        break;
                    case DiceModifier::DROP_LOWEST:
                        oss << "dl" << dice.modifierValue;
                        break;
                    case DiceModifier::EXPLODE:
                        oss << "!";
                        break;
                    case DiceModifier::REROLL:
                        oss << "r" << dice.modifierValue;
                        break;
                    case DiceModifier::REROLL_ONCE:
                        oss << "ro" << dice.modifierValue;
                        break;
                    default:
                        break;
                }

                return oss.str();
            }
        };

        struct BinaryOpNode : public DiceExpression::Node
        {
            char op;
            std::unique_ptr<DiceExpression::Node> left;
            std::unique_ptr<DiceExpression::Node> right;

            BinaryOpNode(char op, std::unique_ptr<DiceExpression::Node> left,
                         std::unique_ptr<DiceExpression::Node> right) :
                op(op), left(std::move(left)), right(std::move(right))
            {
            }

            int Evaluate() const override
            {
                int l = left->Evaluate();
                int r = right->Evaluate();

                switch (op)
                {
                    case '+':
                        return l + r;
                    case '-':
                        return l - r;
                    case '*':
                        return l * r;
                    case '/':
                        return (r != 0) ? l / r : 0;
                    default:
                        return 0;
                }
            }

            int Min() const override
            {
                int l = left->Min();
                int r = right->Min();

                switch (op)
                {
                    case '+':
                        return l + r;
                    case '-':
                        return l - r;
                    case '*':
                        return l * r;
                    case '/':
                        return (r != 0) ? l / r : 0;
                    default:
                        return 0;
                }
            }

            int Max() const override
            {
                int l = left->Max();
                int r = right->Max();

                switch (op)
                {
                    case '+':
                        return l + r;
                    case '-':
                        return l - r;
                    case '*':
                        return l * r;
                    case '/':
                        return (r != 0) ? l / r : 0;
                    default:
                        return 0;
                }
            }

            std::string ToString() const override
            {
                return "(" + left->ToString() + " " + op + " "
                       + right->ToString() + ")";
            }
        };

        struct FunctionNode : public DiceExpression::Node
        {
            std::string name;
            std::unique_ptr<DiceExpression::Node> arg1;
            std::unique_ptr<DiceExpression::Node> arg2;

            FunctionNode(const std::string& name,
                         std::unique_ptr<DiceExpression::Node> arg1,
                         std::unique_ptr<DiceExpression::Node> arg2) :
                name(name), arg1(std::move(arg1)), arg2(std::move(arg2))
            {
            }

            int Evaluate() const override
            {
                int a = arg1->Evaluate();
                int b = arg2->Evaluate();

                if (name == "max")
                {
                    return std::max(a, b);
                }
                else if (name == "min")
                {
                    return std::min(a, b);
                }

                return 0;
            }

            int Min() const override
            {
                if (name == "max")
                {
                    return std::max(arg1->Min(), arg2->Min());
                }
                else if (name == "min")
                {
                    return std::min(arg1->Min(), arg2->Min());
                }
                return 0;
            }

            int Max() const override
            {
                if (name == "max")
                {
                    return std::max(arg1->Max(), arg2->Max());
                }
                else if (name == "min")
                {
                    return std::min(arg1->Max(), arg2->Max());
                }
                return 0;
            }

            std::string ToString() const override
            {
                return name + "(" + arg1->ToString() + ", " + arg2->ToString()
                       + ")";
            }
        };

    } // anonymous namespace

    // Recursive descent parser
    class DiceExpression::Parser
    {
    public:
        explicit Parser(const std::string& input) : input_(input), pos_(0)
        {
            // Remove whitespace
            input_.erase(std::remove_if(input_.begin(), input_.end(),
                                        [](char c) { return std::isspace(c); }),
                         input_.end());
        }

        std::unique_ptr<Node> Parse()
        {
            auto result = ParseExpression();

            if (pos_ < input_.size())
            {
                throw std::runtime_error("Unexpected characters at end: "
                                         + input_.substr(pos_));
            }

            return result;
        }

    private:
        std::string input_;
        size_t pos_;

        char Peek() const
        {
            return (pos_ < input_.size()) ? input_[pos_] : '\0';
        }

        char Next()
        {
            return (pos_ < input_.size()) ? input_[pos_++] : '\0';
        }

        void Expect(char c)
        {
            if (Next() != c)
            {
                throw std::runtime_error(std::string("Expected '") + c + "'");
            }
        }

        bool Match(char c)
        {
            if (Peek() == c)
            {
                Next();
                return true;
            }
            return false;
        }

        // expression := term (('+' | '-') term)*
        std::unique_ptr<Node> ParseExpression()
        {
            auto left = ParseTerm();

            while (Peek() == '+' || Peek() == '-')
            {
                char op = Next();
                auto right = ParseTerm();
                left = std::make_unique<BinaryOpNode>(op, std::move(left),
                                                      std::move(right));
            }

            return left;
        }

        // term := factor (('*' | '/') factor)*
        std::unique_ptr<Node> ParseTerm()
        {
            auto left = ParseFactor();

            while (Peek() == '*' || Peek() == '/')
            {
                char op = Next();
                auto right = ParseFactor();
                left = std::make_unique<BinaryOpNode>(op, std::move(left),
                                                      std::move(right));
            }

            return left;
        }

        // factor := dice | number | '(' expression ')' | function
        std::unique_ptr<Node> ParseFactor()
        {
            if (Match('('))
            {
                auto expr = ParseExpression();
                Expect(')');
                return expr;
            }

            // Check for implicit dice notation FIRST (before alpha check)
            if (Peek() == 'd')
            {
                // Implicit 1dX
                return ParseDice();
            }

            if (std::isalpha(Peek()))
            {
                return ParseFunction();
            }

            if (std::isdigit(Peek()))
            {
                // Look ahead for 'd' to distinguish dice from number
                size_t saved = pos_;
                ParseNumber();

                if (Peek() == 'd')
                {
                    pos_ = saved;
                    return ParseDice();
                }
                else
                {
                    pos_ = saved;
                    return std::make_unique<NumberNode>(ParseNumber());
                }
            }

            throw std::runtime_error("Expected number, dice, or function");
        }

        // dice := number? 'd' number modifier*
        std::unique_ptr<Node> ParseDice()
        {
            DiceGroup group;

            // Count (optional, defaults to 1)
            if (std::isdigit(Peek()))
            {
                group.count = ParseNumber();
            }
            else
            {
                group.count = 1;
            }

            Expect('d');

            // Sides
            if (!std::isdigit(Peek()))
            {
                throw std::runtime_error("Expected number after 'd'");
            }
            group.sides = ParseNumber();

            // Modifiers (optional)
            ParseModifier(group);

            return std::make_unique<DiceNode>(group);
        }

        void ParseModifier(DiceGroup& group)
        {
            char c = Peek();

            if (c == '!')
            {
                Next();
                group.modifier = DiceModifier::EXPLODE;
                return;
            }

            if (c == 'k' || c == 'd' || c == 'r')
            {
                Next();
                char second = Peek();

                if (c == 'k')
                {
                    if (second == 'h')
                    {
                        Next();
                        group.modifier = DiceModifier::KEEP_HIGHEST;
                        group.modifierValue = ParseNumber();
                    }
                    else if (second == 'l')
                    {
                        Next();
                        group.modifier = DiceModifier::KEEP_LOWEST;
                        group.modifierValue = ParseNumber();
                    }
                }
                else if (c == 'd')
                {
                    if (second == 'h')
                    {
                        Next();
                        group.modifier = DiceModifier::DROP_HIGHEST;
                        group.modifierValue = ParseNumber();
                    }
                    else if (second == 'l')
                    {
                        Next();
                        group.modifier = DiceModifier::DROP_LOWEST;
                        group.modifierValue = ParseNumber();
                    }
                }
                else if (c == 'r')
                {
                    if (second == 'o')
                    {
                        Next();
                        group.modifier = DiceModifier::REROLL_ONCE;
                        group.modifierValue = ParseNumber();
                    }
                    else
                    {
                        group.modifier = DiceModifier::REROLL;
                        group.modifierValue = ParseNumber();
                    }
                }
            }
        }

        // function := 'max' '(' expression ',' expression ')'
        std::unique_ptr<Node> ParseFunction()
        {
            std::string name;
            while (std::isalpha(Peek()))
            {
                name += Next();
            }

            if (name != "max" && name != "min")
            {
                throw std::runtime_error("Unknown function: " + name);
            }

            Expect('(');
            auto arg1 = ParseExpression();
            Expect(',');
            auto arg2 = ParseExpression();
            Expect(')');

            return std::make_unique<FunctionNode>(name, std::move(arg1),
                                                  std::move(arg2));
        }

        int ParseNumber()
        {
            if (!std::isdigit(Peek()))
            {
                throw std::runtime_error("Expected number");
            }

            int value = 0;
            while (std::isdigit(Peek()))
            {
                value = value * 10 + (Next() - '0');
            }
            return value;
        }
    };

    // ========================================================================
    // DiceExpression Public Interface
    // ========================================================================

    DiceExpression DiceExpression::Parse(const std::string& expr)
    {
        Parser parser(expr);
        DiceExpression result;
        result.root_ = parser.Parse();
        return result;
    }

    int DiceExpression::Roll() const
    {
        if (!root_)
        {
            return 0;
        }
        return root_->Evaluate();
    }

    int DiceExpression::Min() const
    {
        if (!root_)
        {
            return 0;
        }
        return root_->Min();
    }

    int DiceExpression::Max() const
    {
        if (!root_)
        {
            return 0;
        }
        return root_->Max();
    }

    std::string DiceExpression::ToString() const
    {
        if (!root_)
        {
            return "0";
        }
        return root_->ToString();
    }

} // namespace tutorial::dice
