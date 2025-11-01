#ifndef PATHFINDING_HPP
#define PATHFINDING_HPP

#include "Position.hpp"

#include <vector>

namespace tutorial
{
	// Forward declaration
	class Map;

	// Holds pathfinding state - similar to DCSS's travel_point_distance
	// grid Negative values encode both "visited" status and parent
	// direction
	struct PathfindingContext {
		std::vector<int> distances;
		int width;
		int height;

		PathfindingContext(int w, int h);

		// Get/set distance value at position
		int GetDistance(pos_t pos) const;
		void SetDistance(pos_t pos, int value);

		// Check if position is in bounds
		bool InBounds(pos_t pos) const;
	};

	// Find path between two points using DCSS-style best-first search
	// Returns empty vector if no path exists
	// Path includes start and end positions
	std::vector<pos_t> FindPath(const Map& map, pos_t start, pos_t end);

} // namespace tutorial

#endif // PATHFINDING_HPP
