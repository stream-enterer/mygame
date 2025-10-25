#ifndef EFFECT_HPP
#define EFFECT_HPP

#include <memory>
#include <string>
#include <vector>

namespace tutorial
{
	class Entity;
	class Engine;

	// Base class for all effects that can be applied to entities
	class Effect
	{
	public:
		virtual ~Effect() = default;

		// Apply effect to target entity
		// Returns true if effect was successfully applied
		virtual bool ApplyTo(Entity& target, Engine& engine) const = 0;
	};

	// Effect that modifies health (healing or damage)
	class HealthEffect : public Effect
	{
	public:
		HealthEffect(int amount, const std::string& messageKey);

		bool ApplyTo(Entity& target, Engine& engine) const override;

	private:
		int amount_;             // Positive = heal, negative = damage
		std::string messageKey_; // Key for localized message (empty =
		                         // no message)
	};

	// Effect that temporarily changes AI behavior
	class AiChangeEffect : public Effect
	{
	public:
		AiChangeEffect(const std::string& aiType, int duration,
		               const std::string& messageKey);

		bool ApplyTo(Entity& target, Engine& engine) const override;

	private:
		std::string aiType_;     // "confused", etc.
		int duration_;           // Number of turns
		std::string messageKey_; // Key for localized message
	};

} // namespace tutorial

#endif // EFFECT_HPP
