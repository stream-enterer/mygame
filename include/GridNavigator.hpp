#ifndef GRID_NAVIGATOR_HPP
#define GRID_NAVIGATOR_HPP

namespace tutorial
{
	// Helper class for navigating 2D grids arranged in columns
	// Grid layout: items are arranged in columns from left to right
	// Example 3x4 grid (3 columns, 4 items per column):
	//   Col0  Col1  Col2
	//   [0]   [4]   [8]
	//   [1]   [5]   [9]
	//   [2]   [6]   [10]
	//   [3]   [7]   [11]
	class GridNavigator
	{
	public:
		GridNavigator(int columns, int itemsPerColumn, int totalItems);

		// Get current index
		int GetIndex() const
		{
			return currentIndex_;
		}

		// Set index directly (with bounds checking)
		void SetIndex(int index);

		// Navigation methods
		void MoveUp();
		void MoveDown();
		void MoveLeft();
		void MoveRight();

		// Get grid dimensions
		int GetColumns() const
		{
			return columns_;
		}
		int GetItemsPerColumn() const
		{
			return itemsPerColumn_;
		}
		int GetTotalItems() const
		{
			return totalItems_;
		}

	private:
		int GetColumn() const;
		int GetRow() const;
		int GetIndexFromColumnRow(int col, int row) const;

		int columns_;
		int itemsPerColumn_;
		int totalItems_;
		int currentIndex_;
	};

} // namespace tutorial

#endif // GRID_NAVIGATOR_HPP
