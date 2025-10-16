#ifndef SPAWN_TABLE_HPP
#define SPAWN_TABLE_HPP

#include <string>
#include <vector>

namespace tutorial
{
    struct SpawnEntry
    {
        std::string id; // Template ID (e.g., "orc", "health_potion")
        int weight;     // Relative spawn weight (higher = more common)
    };

    class SpawnTable
    {
    public:
        SpawnTable() = default;

        // Add an entry to the spawn table
        void AddEntry(const std::string& id, int weight);

        // Roll on the table and return a random template ID
        // Returns empty string if table is empty
        std::string Roll() const;

        // Get max monsters/items for this table
        int GetMaxMonstersPerRoom() const
        {
            return maxMonstersPerRoom_;
        }
        int GetMaxItemsPerRoom() const
        {
            return maxItemsPerRoom_;
        }

        // Set max monsters/items
        void SetMaxMonstersPerRoom(int max)
        {
            maxMonstersPerRoom_ = max;
        }
        void SetMaxItemsPerRoom(int max)
        {
            maxItemsPerRoom_ = max;
        }

        // Get total weight (for debugging)
        int GetTotalWeight() const;

    private:
        std::vector<SpawnEntry> entries_;
        int maxMonstersPerRoom_ = 3;
        int maxItemsPerRoom_ = 2;
    };
} // namespace tutorial

#endif // SPAWN_TABLE_HPP
