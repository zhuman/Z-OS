#ifndef _LISTS_HEADER_
#define _LISTS_HEADER_

#include "Types.h"

// This provides the basis for the dynamic
// array implementation.

struct t_ListItem
{
	void* Object;
	struct t_ListItem* NextItem;
};

typedef struct t_ListItem ListItem;

// This structure represents a list and can be
// easily instantiated.
typedef struct
{
	UInt16 Length;
	ListItem* FirstItem;
} List;

// Selects the sorting algorithm used; transitions refer to 
// pre-semi-sorted data vs. random data
typedef enum
{
	QuickSort, // O(n^2) in the worst case, O(n log n) on average, recursive, memory requirements
	HeapSort, // O(n log n) in the worst case
	InsertionSort, // O(n^2) in the worst case, transitions to O(n+d)
	SelectionSort, // O(n^2) always
//	MergeSort, // O(n log n), but averages O(n) - doesn't work
	BubbleSort, // O(n^2) in the worst case
	CocktailSort // O(n^2) in the worst case and average, but transitions to O(n)
} SortMethodEnum;

// A user-defined callback that compares two items, returning:
//  -1 if item1 < item2
//   0 if item1 == item2
//  +1 if item1 > item2
typedef Int8 (*CompareItemsProc)(void* item1, void* item2);

// Returns an item from a list at the given index.
void* GetListItem(List* list, UInt16 index);

// Returns the length of a list.
UInt16 GetListLength(List* list);

// Adds an item to a list.
void AddListItem(List* list, void* item);

// Removes the item at the specified index and returns it
void* RemoveListItem(List* list, UInt16 index);

// Inserts a new list item at the specified index
void InsertListItem(List* list, UInt16 index, void* newItem);

// Replaces the list item at the specified index with the new value, returning the old value
void* ReplaceListItem(List* list, UInt16 index, void* newItem);

// Returns the index of a given item in the list (-1 is returned for non-existant items)
Int16 GetIndexOf(List* list, void* item);

// Sorts a list with the specified sort algorithm using a callback function to compare items
void SortList(List* list, SortMethodEnum method, CompareItemsProc compareFunc);

// Swaps two list items at the specified indices
void SwapListItems(List* list, UInt16 item1, UInt16 item2);

// Clears all list items from a list (note, this does not touch any of the 
// list items, so you must free them all beforehand)
void ClearList(List* list);

// Combines two or more lists into a newly malloc'ed list
List* ConcatenateLists(List** lists, UInt16 listCount);

#endif
