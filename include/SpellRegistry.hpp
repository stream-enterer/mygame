#ifndef SPELL_REGISTRY_HPP
#define SPELL_REGISTRY_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace tutorial
{
	class Effect;
	class TargetSelector;

	// Spell data structure loaded from JSON
	struct SpellData {
		std::string id;
		std::string name;
		unsigned int manaCost;

		// Targeting configuration
		std::string targetingType;
		float range;
		float radius;

		// Effect configuration (stored as JSON strings for lazy
		// loading)
		std::vector<std::string> effectTypes;
		std::vector<int> effectAmounts;
		std::vector<std::string> effectMessages;

		// Create runtime objects from data
		std::unique_ptr<TargetSelector> CreateTargetSelector() const;
		std::vector<std::unique_ptr<Effect>> CreateEffects() const;
	};

	// Singleton registry for spell definitions
	class SpellRegistry
	{
	public:
		static SpellRegistry& Instance();

		// Load spells from directory
		void LoadFromDirectory(const std::string& dirPath);

		// Clear all loaded spells
		void Clear();

		// Access spells
		const SpellData* Get(const std::string& id) const;
		bool Has(const std::string& id) const;
		std::vector<std::string> GetAllIds() const;

	private:
		SpellRegistry() = default;
		~SpellRegistry() = default;

		// Prevent copying
		SpellRegistry(const SpellRegistry&) = delete;
		SpellRegistry& operator=(const SpellRegistry&) = delete;

		// Load single spell from JSON
		SpellData LoadSpell(const std::string& id,
		                    const std::string& filePath);

		std::unordered_map<std::string, SpellData> spells_;
	};

} // namespace tutorial

#endif // SPELL_REGISTRY_HPP
