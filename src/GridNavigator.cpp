#include "GridNavigator.hpp"

#include <algorithm>

namespace tutorial
{
	GridNavigator::GridNavigator(int columns, int itemsPerColumn,
	                             int totalItems)
	    : columns_(columns),
	      itemsPerColumn_(itemsPerColumn),
	      totalItems_(totalItems),
	      currentIndex_(0)
	{
	}

	void GridNavigator::SetIndex(int index)
	{
		if (index >= 0 && index < totalItems_) {
			currentIndex_ = index;
		}
	}

	void GridNavigator::MoveUp()
	{
		if (totalItems_ == 0) {
			return;
		}

		int col = GetColumn();
		int row = GetRow();

		// Move up one row, wrapping to bottom if at top
		row = (row - 1 + itemsPerColumn_) % itemsPerColumn_;

		// Calculate new index
		int newIndex = GetIndexFromColumnRow(col, row);

		// Only move if the new index is valid
		if (newIndex < totalItems_) {
			currentIndex_ = newIndex;
		}
	}

	void GridNavigator::MoveDown()
	{
		if (totalItems_ == 0) {
			return;
		}

		int col = GetColumn();
		int row = GetRow();

		// Move down one row, wrapping to top if at bottom
		row = (row + 1) % itemsPerColumn_;

		// Calculate new index
		int newIndex = GetIndexFromColumnRow(col, row);

		// Only move if the new index is valid
		if (newIndex < totalItems_) {
			currentIndex_ = newIndex;
		}
	}

	void GridNavigator::MoveLeft()
	{
		if (totalItems_ == 0) {
			return;
		}

		int col = GetColumn();
		int row = GetRow();

		// Move left one column, wrapping to rightmost if at leftmost
		col = (col - 1 + columns_) % columns_;

		// Try to maintain the same row, but if that slot doesn't
		// exist, move to the last valid item in the column
		int newIndex = GetIndexFromColumnRow(col, row);
		while (newIndex >= totalItems_ && row > 0) {
			row--;
			newIndex = GetIndexFromColumnRow(col, row);
		}

		if (newIndex < totalItems_) {
			currentIndex_ = newIndex;
		}
	}

	void GridNavigator::MoveRight()
	{
		if (totalItems_ == 0) {
			return;
		}

		int col = GetColumn();
		int row = GetRow();

		// Move right one column, wrapping to leftmost if at rightmost
		col = (col + 1) % columns_;

		// Try to maintain the same row, but if that slot doesn't
		// exist, move to the last valid item in the column
		int newIndex = GetIndexFromColumnRow(col, row);
		while (newIndex >= totalItems_ && row > 0) {
			row--;
			newIndex = GetIndexFromColumnRow(col, row);
		}

		if (newIndex < totalItems_) {
			currentIndex_ = newIndex;
		}
	}

	int GridNavigator::GetColumn() const
	{
		return currentIndex_ / itemsPerColumn_;
	}

	int GridNavigator::GetRow() const
	{
		return currentIndex_ % itemsPerColumn_;
	}

	int GridNavigator::GetIndexFromColumnRow(int col, int row) const
	{
		return col * itemsPerColumn_ + row;
	}

} // namespace tutorial
