#ifndef ROOM_HPP
#define ROOM_HPP

#include "Position.hpp"

#include <vector>

namespace tutorial
{
	class Room
	{
	public:
		Room(pos_t origin, int width, int height);

		pos_t GetCenter() const;
		pos_t GetEnd() const;
		pos_t GetOrigin() const;
		std::vector<pos_t> GetInner() const;
		bool Intersects(const Room& other) const;

	private:
		pos_t origin_; // Top-left corner
		pos_t end_;    // Bottom-right corner
	};
} // namespace tutorial

#endif // ROOM_HPP
