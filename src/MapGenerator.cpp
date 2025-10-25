#include "MapGenerator.hpp"

#include "libtcod/bresenham.hpp"
#include "libtcod/mersenne.hpp"

#include <algorithm>

namespace tutorial
{
	inline namespace
	{
		constexpr float kHalfChance = 0.5F;
	}

	std::vector<pos_t> tunnelBetween(pos_t start, pos_t end)
	{
		auto* rand = TCODRandom::getInstance();

		pos_t corner { 0, 0 };

		if (rand->get(0.0F, 1.0F) < kHalfChance) {
			corner = pos_t { end.x, start.y };
		} else {
			corner = pos_t { start.x, end.y };
		}

		std::vector<pos_t> tunnel {};

		for (const auto&& [x, y] : tcod::BresenhamLine(
		         { start.x, start.y }, { corner.x, corner.y })) {
			tunnel.push_back({ x, y });
		}

		for (const auto&& [x, y] : tcod::BresenhamLine(
		         { corner.x, corner.y }, { end.x, end.y })) {
			tunnel.push_back({ x, y });
		}

		return tunnel;
	}

	Map::Generator::Generator(const MapParameters& params) : params_(params)
	{
	}

	void Map::Generator::Generate(Map& map)
	{
		auto* rand = TCODRandom::getInstance();

		for (int i = 0; i < params_.maxRooms; ++i) {
			int roomWidth = rand->getInt(params_.minRoomSize,
			                             params_.maxRoomSize);
			int roomHeight = rand->getInt(params_.minRoomSize,
			                              params_.maxRoomSize);

			pos_t roomOrigin {
				rand->getInt(0, params_.width - roomWidth - 1),
				rand->getInt(0, params_.height - roomHeight - 1)
			};

			auto room = Room(roomOrigin, roomWidth, roomHeight);

			auto it =
			    std::find_if(map.rooms_.begin(), map.rooms_.end(),
			                 [&room](Room other) {
				                 return room.Intersects(other);
			                 });

			if (it != map.rooms_.end()) {
				continue;
			}

			for (auto pos : room.GetInner()) {
				if (map.IsWall(pos)) {
					map.SetTileType(pos, TileType::FLOOR);
				}
			}

			if (!map.rooms_.empty()) {
				auto tunnel = tunnelBetween(
				    room.GetCenter(),
				    map.rooms_.at(map.rooms_.size() - 1)
				        .GetCenter());

				for (auto pos : tunnel) {
					if (map.IsWall(pos)) {
						map.SetTileType(
						    pos, TileType::FLOOR);
					}
				}
			}

			map.rooms_.push_back(room);
		}
	}
} // namespace tutorial
