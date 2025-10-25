#ifndef TARGET_SELECTOR_HPP
#define TARGET_SELECTOR_HPP

#include "Position.hpp"

#include <string>
#include <vector>

namespace tutorial
{
	class Entity;
	class Engine;

	// Base class for target selection strategies
	class TargetSelector
	{
	public:
		virtual ~TargetSelector() = default;

		// Select targets and add them to the list
		// Returns true if at least one valid target was selected
		virtual bool SelectTargets(
		    Entity& user, Engine& engine,
		    std::vector<Entity*>& targets) const = 0;

	protected:
		// Helper: Check if there's line-of-sight from origin to target
		bool HasLineOfSight(Engine& engine, pos_t origin,
		                    pos_t target) const;
	};

	// Targets the item user (wearer)
	class SelfTargetSelector : public TargetSelector
	{
	public:
		bool SelectTargets(
		    Entity& user, Engine& engine,
		    std::vector<Entity*>& targets) const override;
	};

	// Targets the closest enemy within range
	class ClosestEnemySelector : public TargetSelector
	{
	public:
		explicit ClosestEnemySelector(float range);

		bool SelectTargets(
		    Entity& user, Engine& engine,
		    std::vector<Entity*>& targets) const override;

	private:
		float range_;
	};

	// Targets a single entity selected by the player
	class SingleTargetSelector : public TargetSelector
	{
	public:
		explicit SingleTargetSelector(float range);

		bool SelectTargets(
		    Entity& user, Engine& engine,
		    std::vector<Entity*>& targets) const override;

	private:
		float range_;
	};

	// Targets all entities within range of a selected tile
	class AreaTargetSelector : public TargetSelector
	{
	public:
		AreaTargetSelector(float pickRange, float effectRadius);

		bool SelectTargets(
		    Entity& user, Engine& engine,
		    std::vector<Entity*>& targets) const override;

	private:
		float pickRange_;    // How far the player can target
		float effectRadius_; // Radius of effect around chosen tile
	};

	// Targets all entities in a straight line (beam/ray)
	class BeamTargetSelector : public TargetSelector
	{
	public:
		explicit BeamTargetSelector(float range);

		bool SelectTargets(
		    Entity& user, Engine& engine,
		    std::vector<Entity*>& targets) const override;

	private:
		float range_; // Maximum beam range
	};

	// Targets ONLY the first entity in a straight line (beam/ray)
	class FirstInBeamTargetSelector : public TargetSelector
	{
	public:
		explicit FirstInBeamTargetSelector(float range);

		bool SelectTargets(
		    Entity& user, Engine& engine,
		    std::vector<Entity*>& targets) const override;

	private:
		float range_; // Maximum beam range
	};

} // namespace tutorial

#endif // TARGET_SELECTOR_HPP
