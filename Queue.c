#include "Z-OS.h"

static void QueueInterfaceCreate(InternalObject* obj)
{
	QueueInternal* intObj = zmalloc(sizeof(QueueInternal));
	if (intObj)
	{
		obj->Data = intObj;
	}
}

static void QueueInterfaceDestroy(InternalObject* obj)
{
	zfree(obj->Data);
}

static Int16 QueueInterfaceAddItem(UInt16 handle, void* item)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		EnterCriticalSection();
		AddListItem(&(((QueueInternal*)(obj->Data))->Items), item);
		FinishWait(obj->Data, NULL);
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
	
}

static Int16 QueueInterfacePeek(UInt16 handle, void** item)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		EnterCriticalSection();
		if (((QueueInternal*)(obj->Data))->Items.Length == 0)
		{
			*item = 0;
			ExitCriticalSection();
			return ErrorEmpty;
		}
		*item = GetListItem(&(((QueueInternal*)(obj->Data))->Items),0);
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 QueueInterfaceGetItem(UInt16 handle, void** item)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		EnterCriticalSection();
		if (((QueueInternal*)(obj->Data))->Items.Length == 0)
		{
			*item = 0;
			ExitCriticalSection();
			return ErrorEmpty;
		}
		*item = RemoveListItem(&(((QueueInternal*)(obj->Data))->Items),0);
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 QueueInterfaceStartWait(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		EnterCriticalSection();
		if (((QueueInternal*)(obj->Data))->Items.Length > 0)
		{
			ExitCriticalSection();
			return ErrorNoWait;
		}
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 QueueGetInterface(Int16 code, void** interface)
{
	switch (code)
	{
	case CodeIGeneric:
	{
		static const IGeneric inter = 
		{
			QueueInterfaceCreate,
			QueueInterfaceDestroy
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeIQueue:
	{
		static const IQueue inter = 
		{
			QueueInterfaceAddItem,
			QueueInterfacePeek,
			QueueInterfaceGetItem
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeIWaitable:
	{
		static const IWaitable inter = 
		{
			QueueInterfaceStartWait
		};
		*interface = (void*)&inter;
		break;
	}
	default:
		return ErrorInvalidInterface;
	}
	return ErrorSuccess;
}

void InitializeQueues(void)
{
	TypeRegistration type;
	type.Type = TypeQueue;
	type.GetInterface = (void*)QueueGetInterface;
	RegisterTypeManager(type);
}
