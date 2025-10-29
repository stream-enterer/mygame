#ifndef SPELLCASTER_COMPONENT_HPP
#define SPELLCASTER_COMPONENT_HPP

#include <string>
#include <vector>

namespace tutorial
{
	// Component that marks an entity as capable of casting spells
	// and stores their known spells
	class SpellcasterComponent
	{
	public:
		SpellcasterComponent() = default;
		explicit SpellcasterComponent(
		    const std::vector<std::string>& spells);

		// Spell management
		void AddSpell(const std::string& spellId);
		bool KnowsSpell(const std::string& spellId) const;
		const std::vector<std::string>& GetKnownSpells() const;

	private:
		std::vector<std::string>
		    knownSpells_; // Spell IDs like "fireball"
	};

} // namespace tutorial

#endif // SPELLCASTER_COMPONENT_HPP
