#include "SpawnTable.hpp"

#include <libtcod/mersenne.hpp>

#include <numeric>

namespace tutorial
{
	void SpawnTable::AddEntry(const std::string& id, int weight)
	{
		if (weight > 0) {
			entries_.push_back({ id, weight });
		}
	}

	std::string SpawnTable::Roll() const
	{
		if (entries_.empty()) {
			return "";
		}

		// Calculate total weight
		int totalWeight = GetTotalWeight();

		// Roll random number from 0 to totalWeight-1
		auto* rand = TCODRandom::getInstance();
		int roll = rand->getInt(0, totalWeight - 1);

		// Find which entry the roll lands in
		int currentWeight = 0;
		for (const auto& entry : entries_) {
			currentWeight += entry.weight;
			if (roll < currentWeight) {
				return entry.id;
			}
		}

		// Fallback (shouldn't happen)
		return entries_[0].id;
	}

	int SpawnTable::GetTotalWeight() const
	{
		return std::accumulate(entries_.begin(), entries_.end(), 0,
		                       [](int sum, const SpawnEntry& entry) {
			                       return sum + entry.weight;
		                       });
	}
} // namespace tutorial
