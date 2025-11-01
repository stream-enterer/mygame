#include "PathFinding.hpp"

#include "Map.hpp"
#include "Tile.hpp"
#include "Util.hpp"

#include <libtcod.h>

#include <algorithm>
#include <queue>
#include <random>

namespace tutorial
{
	inline namespace
	{
		// Orthogonal directions only (no diagonals)
		constexpr pos_t kOrthogonalDirs[4] = {
			{ 0, -1 }, // North
			{ 1, 0 },  // East
			{ 0, 1 },  // South
			{ -1, 0 }  // West
		};

		// Priority queue entry for best-first search
		struct PathNode {
			pos_t pos;
			int distanceToGoal;

			bool operator>(const PathNode& other) const
			{
				return distanceToGoal > other.distanceToGoal;
			}
		};

		// Manhattan distance heuristic
		int ManhattanDistance(pos_t a, pos_t b)
		{
			return std::abs(a.x - b.x) + std::abs(a.y - b.y);
		}

		// Encode parent direction as DCSS does: (-dx + 2) * 4 + (-dy +
		// 2) This gives us a unique negative number for each direction
		int EncodeDirection(pos_t dir)
		{
			return -((-dir.x + 2) * 4 + (-dir.y + 2));
		}

		// Decode parent direction from encoded value
		pos_t DecodeDirection(int encoded)
		{
			int val = -encoded;
			int dx = -(val / 4 - 2);
			int dy = -(val % 4 - 2);
			return pos_t { dx, dy };
		}
	} // namespace

	// PathfindingContext implementation
	PathfindingContext::PathfindingContext(int w, int h)
	    : distances(w * h, 0), width(w), height(h)
	{
	}

	int PathfindingContext::GetDistance(pos_t pos) const
	{
		return distances[util::posToIndex(pos, width)];
	}

	void PathfindingContext::SetDistance(pos_t pos, int value)
	{
		distances[util::posToIndex(pos, width)] = value;
	}

	bool PathfindingContext::InBounds(pos_t pos) const
	{
		return pos.x >= 0 && pos.y >= 0 && pos.x < width
		       && pos.y < height;
	}

	// Main pathfinding function - DCSS-style best-first search
	std::vector<pos_t> FindPath(const Map& map, pos_t start, pos_t end)
	{
		// Early exit if start or end is invalid
		if (!map.IsInBounds(start) || !map.IsInBounds(end)) {
			return {};
		}

		if (map.IsWall(start) || map.IsWall(end)) {
			return {};
		}

		// If already at destination
		if (start == end) {
			return { start };
		}

		// Initialize pathfinding context
		PathfindingContext context(map.GetWidth(), map.GetHeight());

		// Priority queue for best-first search
		std::priority_queue<PathNode, std::vector<PathNode>,
		                    std::greater<PathNode>>
		    queue;

		// Start searching
		queue.push({ start, ManhattanDistance(start, end) });
		context.SetDistance(start, 1); // Mark as visited

		auto* rand = TCODRandom::getInstance();
		bool foundPath = false;

		while (!queue.empty()) {
			PathNode current = queue.top();
			queue.pop();

			// Check if we reached the goal
			if (current.pos == end) {
				foundPath = true;
				break;
			}

			// Randomize neighbor order (DCSS does this for variety)
			std::vector<int> dirIndices = { 0, 1, 2, 3 };
			std::shuffle(dirIndices.begin(), dirIndices.end(),
			             std::default_random_engine(
			                 rand->getInt(0, 999999)));

			// Try all orthogonal neighbors
			for (int dirIdx : dirIndices) {
				pos_t dir = kOrthogonalDirs[dirIdx];
				pos_t neighbor = current.pos + dir;

				// Skip if out of bounds
				if (!context.InBounds(neighbor)) {
					continue;
				}

				// Skip if wall
				if (map.IsWall(neighbor)) {
					continue;
				}

				// Skip if already visited
				if (context.GetDistance(neighbor) != 0) {
					continue;
				}

				// Mark as visited and encode parent direction
				context.SetDistance(neighbor,
				                    EncodeDirection(dir));

				// Add to queue
				int distToGoal =
				    ManhattanDistance(neighbor, end);
				queue.push({ neighbor, distToGoal });
			}
		}

		// If no path found, return empty vector
		if (!foundPath) {
			return {};
		}

		// Reconstruct path by walking backwards from end to start
		std::vector<pos_t> path;
		pos_t current = end;

		while (current != start) {
			path.push_back(current);

			// Decode parent direction and move backwards
			int encoded = context.GetDistance(current);
			pos_t dir = DecodeDirection(encoded);
			current =
			    current - dir; // Subtract to go back to parent
		}

		// Add start position
		path.push_back(start);

		// Reverse to get start->end order
		std::reverse(path.begin(), path.end());

		return path;
	}

} // namespace tutorial
