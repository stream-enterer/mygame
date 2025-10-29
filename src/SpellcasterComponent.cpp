#include "SpellcasterComponent.hpp"

#include <algorithm>

namespace tutorial
{
	SpellcasterComponent::SpellcasterComponent(
	    const std::vector<std::string>& spells)
	    : knownSpells_(spells)
	{
	}

	void SpellcasterComponent::AddSpell(const std::string& spellId)
	{
		// Don't add duplicates
		if (!KnowsSpell(spellId)) {
			knownSpells_.push_back(spellId);
		}
	}

	bool SpellcasterComponent::KnowsSpell(const std::string& spellId) const
	{
		return std::find(knownSpells_.begin(), knownSpells_.end(),
		                 spellId)
		       != knownSpells_.end();
	}

	const std::vector<std::string>& SpellcasterComponent::GetKnownSpells()
	    const
	{
		return knownSpells_;
	}

} // namespace tutorial
