#include "Z-OS.h"

static ListItem* GetListItemInternal(List* list, UInt16 index);

// Returns an item from a list at the given index.
void* GetListItem(List* list, UInt16 index)
{
	ListItem* item =  GetListItemInternal(list,index);
	if (item)
	{
		return item->Object;
	}
	else
	{
		return NULL;
	}
}

// Returns an item from a list at the given index.
static ListItem* GetListItemInternal(List* list, UInt16 index)
{
	ListItem* listItem;
	if (!list || !(list->Length)) return NULL;
	listItem = list->FirstItem;
	while (index && listItem->NextItem)
	{
		listItem = listItem->NextItem;
		index--;
	}
	if (index) return NULL;
	return listItem;
}

// Returns the length of a list.
UInt16 GetListLength(List* list)
{
	ListItem* listItem = list->FirstItem;
	Int16 i = 0;
	while (listItem)
	{
		i++;
		listItem = listItem->NextItem;
	}
	return i;
}

// Adds an item to a list.
void AddListItem(List* list, void* item)
{
	ListItem* listItem;
	UInt16 index = list->Length-1;
	if (!list) return;
	
	if (list->FirstItem)
	{
		listItem = list->FirstItem;
		while (index && listItem)
		{
			listItem = listItem->NextItem;
			index--;
		}
		if (index) return;
		listItem->NextItem = zmalloc(sizeof(ListItem));
		if (!(listItem->NextItem)) return;
		listItem->NextItem->Object = item;
	}
	else
	{
		list->FirstItem = zmalloc(sizeof(ListItem));
		if (!(list->FirstItem)) return;
		list->FirstItem->Object = item;
	}

	list->Length = GetListLength(list);
}

// Remove the list item at the specified index.
void* RemoveListItem(List* list, UInt16 index)
{
	ListItem* item = GetListItemInternal(list,index);
	void* obj = 0;
	if (!item) return NULL;
	obj = item->Object;
	if (index == 0) { list->FirstItem = item->NextItem; }
	else { GetListItemInternal(list,index-1)->NextItem = item->NextItem; }
	zfree(item);
	list->Length = GetListLength(list);
	return obj;
}

void InsertListItem(List* list, UInt16 index, void* newItem)
{
	ListItem* oldItem = GetListItemInternal(list,index);
	ListItem* insItem = zmalloc(sizeof(ListItem));
	if (!insItem) return;
	insItem->Object = newItem;
	insItem->NextItem = oldItem;
	
	if (index == 0)
	{
		list->FirstItem = insItem;
	}
	else
	{
		ListItem* befItem = GetListItemInternal(list,index-1);
		befItem->NextItem = insItem;
	}
	list->Length = GetListLength(list);
}

void* ReplaceListItem(List* list, UInt16 index, void* newItem)
{
	void* oldItem = null;
	ListItem* intItem = GetListItemInternal(list,index);
	if (intItem)
	{
		oldItem = intItem->Object;
		intItem->Object = newItem;
	}
	return oldItem;
}

Int16 GetIndexOf(List* list, void* item)
{
	UInt16 i;
	for (i = 0; i < list->Length; i++)
	{
		if (GetListItem(list,i) == item) return i;
	}
	return -1;
}

void SwapListItems(List* list, UInt16 item1, UInt16 item2)
{
	ListItem* intItem1 = GetListItemInternal(list, item1);
	ListItem* intItem2 = GetListItemInternal(list, item2);
	if (intItem1 && intItem2)
	{
		void* temp = intItem1->Object;
		intItem1->Object = intItem2->Object;
		intItem2->Object = temp;
	}
}

void ClearList(List* list)
{
	while (list->FirstItem)
	{
		RemoveListItem(list, 0);
	}
}

/*static void Merge(List* list, List* left, List* right, CompareItemsProc compareFunc)
{
	Int16 currInd = 0;
	while (left->Length > 0 && right->Length > 0)
	{
		if (compareFunc(GetListItem(left,0),GetListItem(right,0)) <= 0)
		{
			ReplaceListItem(list,currInd++,RemoveListItem(left,0));
		}
		else
		{
			ReplaceListItem(list,currInd++,RemoveListItem(right,0));
		}
	}
	while (left->Length > 0) ReplaceListItem(list,currInd++,RemoveListItem(left,0));
	while (right->Length > 0) ReplaceListItem(list,currInd++,RemoveListItem(right,0));
}

static void MergeSortList(List* list, CompareItemsProc compareFunc)
{
	List left, right;
	Int16 mid, i;
	if (list->Length <= 1) return;
	mid = list->Length >> 1;
	for (i = 0; i <= mid; i++)
	{
		AddListItem(&left,GetListItem(list,i));
	}
	for (i = mid + 1; i < list->Length; i++)
	{
		AddListItem(&right,GetListItem(list,i));
	}
	MergeSortList(&left,compareFunc);
	MergeSortList(&right,compareFunc);
	Merge(list,&left,&right,compareFunc);
}*/

// An implementation of the recursive QuickSort algorithm
static void QuickSortList(List* list, CompareItemsProc compareFunc)
{
	List lessList = {0}, equalList = {0}, moreList = {0};
	UInt16 i, pivotInd;
	void* pivot;
	
	if (list->Length <= 1) return;
	
	// Select a value "randomly" by choosing the center item
	pivotInd = list->Length >> 1;
	pivot = GetListItem(list,pivotInd);
	AddListItem(&equalList,pivot);
	
	for (i = 0; i < list->Length; i++)
	{
		if (i != pivotInd)
		{
			void* currItem = GetListItem(list,i);
			Int16 compared = compareFunc(currItem,pivot);
			if (compared < 0) AddListItem(&lessList,currItem);
			else if (compared == 0) AddListItem(&equalList,currItem);
			else AddListItem(&moreList,currItem);
		}
	}
	QuickSortList(&lessList,compareFunc);
	QuickSortList(&moreList,compareFunc);
	for (i = 0; i < lessList.Length; i++)
	{
		ReplaceListItem(list,i,GetListItem(&lessList,i));
	}
	for (i = 0; i < equalList.Length; i++)
	{
		ReplaceListItem(list,i,GetListItem(&equalList,i));
	}
	for (i = 0; i < moreList.Length; i++)
	{
		ReplaceListItem(list,i,GetListItem(&moreList,i));
	}
	ClearList(&lessList);
	ClearList(&equalList);
	ClearList(&moreList);
}

// Used by the HeapSort algorithm
static void SiftDown(List* list, Int16 start, Int16 end, CompareItemsProc compareFunc)
{
	Int16 root = start;
	Int16 child;
	while (root + root + 1 <= end)
	{
		child = root + root + 1;
		if (child < end && compareFunc(GetListItem(list,child),GetListItem(list,child+1)) < 0)
		{
			child++;
		}
		if (compareFunc(GetListItem(list,root),GetListItem(list,child)) < 0)
		{
			SwapListItems(list,root,child);
		}
		else return;
	}
}

void SortList(List* list, SortMethodEnum method, CompareItemsProc compareFunc)
{
	if (!list || !compareFunc) return;
	switch (method)
	{
		case QuickSort:
		{
			QuickSortList(list, compareFunc);
		}
		case HeapSort:
		{
			Int16 start;
			Int16 end = list->Length - 1;
			start = end >> 1;
			while (start >= 0)
			{
				SiftDown(list,start,end,compareFunc);
				start--;
			}
			while (end > 0)
			{
				SwapListItems(list,end,0);
				end--;
				SiftDown(list,0,end,compareFunc);
			}
		}
		case InsertionSort:
		{
			UInt16 i;
			for (i = 1; i < list->Length; i++)
			{
				void* val = GetListItem(list,i);
				UInt16 j = i - 1;
				while (j >= 0 && compareFunc(GetListItem(list,j),val) > 0)
				{
					ReplaceListItem(list,j+1,GetListItem(list,j));
					j--;
				}
				ReplaceListItem(list,j+1,val);
			}
		}
		case SelectionSort:
		{
			UInt16 i;
			for (i = 0; i <= list->Length - 2; i++)
			{
				UInt16 min = i;
				UInt16 j;
				for (j = i + 1; j < list->Length; j++)
				{
					if (compareFunc(GetListItem(list,j),GetListItem(list,min)) < 0) min = j;
				}
				SwapListItems(list,i,min);
			}
		}
/*		case MergeSort:
		{
			MergeSortList(list,compareFunc);
		}
*/		case BubbleSort:
		{
			UInt8 swapped;
			UInt16 n = list->Length;
			do
			{
				UInt16 i;
				swapped = False;
				n--;
				for (i = 0; i < n; i++)
				{
					if (compareFunc(GetListItem(list,i),GetListItem(list,i+1)) > 0)
					{
						SwapListItems(list,i,i+1);
						swapped = True;
					}
				}
			} while (swapped);
		}
		case CocktailSort:
		{
			Int16 begin = -1;
			Int16 end = list->Length - 1;
			Int8 swapped = False;
			do
			{
				Int16 i;
				swapped = False;
				begin++;
				for (i = begin; i <= end; i++)
				{
					if (compareFunc(GetListItem(list,i),GetListItem(list,i+1)) > 0)
					{
						SwapListItems(list,i,i+1);
						swapped = True;
					}
				}
				if (!swapped) break;
				swapped = False;
				end--;
				for (i = end; i >= begin; i--)
				{
					if (compareFunc(GetListItem(list,i),GetListItem(list,i+1)) > 0)
					{
						SwapListItems(list,i,i+1);
						swapped = True;
					}
				}
			} while (swapped);
		}
	}
}

List* ConcatenateLists(List** lists, UInt16 listCount)
{
	List* newList = zmalloc(sizeof(List));
	if (newList)
	{
		UInt16 i;
		for (i = 0; i < listCount; i++)
		{
			if (lists[i])
			{
				UInt16 j;
				for (j = 0; j < lists[i]->Length; j++)
				{
					AddListItem(newList,GetListItem(lists[j],j));
				}
			}
		}
	}
	return newList;
}
