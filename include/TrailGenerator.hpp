#ifndef TRAIL_GENERATOR_HPP
#define TRAIL_GENERATOR_HPP

#include "Position.hpp"

#include <vector>

namespace tutorial
{
	// Forward declaration
	class Map;

	// Configuration for trail generation
	struct TrailConfig {
		int edgeMargin; // Distance to stay away from map edges (DCSS
		                // uses 15)
		int minLength;  // Minimum corridor length before turning
		int maxLength;  // Maximum corridor length before turning
		float intersectChance; // Probability of turning early when
		                       // intersecting
	};

	// Generates a single winding trail through the map
	// Returns the positions that were carved (the trail path)
	std::vector<pos_t> GenerateTrail(Map& map, pos_t start, pos_t end,
	                                 const TrailConfig& config);

	// Helper: Get default trail configuration (DCSS-like values)
	TrailConfig GetDefaultTrailConfig();

} // namespace tutorial

#endif // TRAIL_GENERATOR_HPP
