#include "Z-OS.h"
#include "Shell.h"

//#define TestTimerDefine
//#define TestMutexDefine
//#define TestEventDefine
//#define TestDelegateAsyncDefine
//#define RunShellDefine
//#define TestMathErrorDefine
//#define TestSymLinkDefine

void zputs(char* str)
{
#ifdef TestMutexDefine
	UInt16 mutexHandle = 0;
	ISemaphore* inter = 0;
	Int16 ret;
	if ((ret = OpenObject("zputs",&mutexHandle)))
	{
		if ((ret = CreateObject(TypeSemaphore,&mutexHandle,"zputs")))
		{
			return;
		}
	}
	if ((ret = GetInterface(mutexHandle,CodeISemaphore,(void**)&inter)))
	{
		ReleaseObject(mutexHandle);
		return;
	}
	inter->QueueCapture(mutexHandle);
	WaitForObject(mutexHandle);
#endif
	puts(str);
#ifdef TestMutexDefine
	inter->ReleaseCapture(mutexHandle);
	ReleaseObject(mutexHandle);
#endif
}

void Handler1(void* arg)
{
	Int16 i;
	for (i = 0; i < 256; i++) puts("Handler 1\r\n");
}

void Handler2(void* arg)
{
	Int16 i;
	for (i = 0; i < 256; i++) puts("Handler 2\r\n");
}

void Handler3(void* arg)
{
	Int16 i;
	for (i = 0; i < 256; i++) puts("Handler 3\r\n");
}

void Thread2Func(void)
{
#ifdef TestEventDefine
	{
		UInt16 handle;
		IEvent* inter;
		Int16 ret;
		if ((ret = OpenObject("StartupFinished",&handle))) return;
		if ((ret = GetInterface(handle,CodeIEvent,(void**)&inter))) return;
		WaitForObject(handle);
	}
#endif
#ifdef TestSymLinkDefine
	{
		UInt16 handle;
		IThread* thrInt;
		if (OpenObject("CurrentThread",&handle)) return;
		if (GetInterface(handle,CodeIThread,(void**)&thrInt)) return;
		thrInt->SetPriority(handle,5);
		ReleaseObject(handle);
	}
#endif
#ifdef TestDelegateAsyncDefine
	{
		UInt16 handle;
		IDelegate* delInt;
		if (CreateObject(TypeDelegate,&handle,(char*)0)) return;
		if (GetInterface(handle,CodeIDelegate,(void**)&delInt)) return;
		delInt->AddHandler(handle,Handler1);
		delInt->AddHandler(handle,Handler2);
		delInt->AddHandler(handle,Handler3);
		delInt->InvokeAsync(handle,(void*)0);
		WaitForObject(handle);
		zputs("Done with those handlers.\r\n");
	}
#endif
//	for(;;) zputs("My New Thread");
}

Int16 CompareNums(void* num1, void* num2)
{
	if ((Int16)num1 < (Int16)num2) return -1;
	if ((Int16)num1 == (Int16)num2) return 0;
	return 1;
}

void ListTest(void)
{
	List list = {0};
	Int16 i;
	puts("Random numbers:\r\n");
	while (i++ < 25)
	{
		Int16 val = rand();
		AddListItem(&list,(void*)val);
		printf("%d\r\n",val);
	}
	SortList(&list,HeapSort,(CompareItemsProc)&CompareNums);
	puts("Sorted items:\r\n");
	for (i = 0; i < list.Length; i++)
	{
		printf("%d\r\n",(Int16)GetListItem(&list,i));
	}
}

void __attribute__((__weak__)) StartupThreadProc(void)
{
	UInt16 handle;
	Int16 ret = 0;
	IThread* interface;
#ifdef TestEventDefine
	IEvent* eveInt;
	UInt16 eveHandle;
	
	if ((ret = CreateObject(TypeEvent,&eveHandle,"StartupFinished"))) return;
	if ((ret = GetInterface(eveHandle,CodeIEvent,(void**)&eveInt))) return;
#endif
	
	// Create the "My New Thread" thread
	if ((ret = CreateObject(TypeThread,&handle,(char*)0))) return;
	if ((ret = GetInterface(handle,CodeIThread,(void**)&interface))) return;
	interface->Start(handle,(ThreadFunction)Thread2Func,0);
	ReleaseObject(handle);
	
#ifdef RunShellDefine
	// Create the shell thread
	if ((ret = CreateObject(TypeThread,&handle,"ShellThread"))) return;
	if ((ret = GetInterface(handle,CodeIThread,(void**)&interface))) return;
	interface->Start(handle,(ThreadFunction)ShellThreadProc,0);
	ReleaseObject(handle);
#endif
	
#ifdef TestTimerDefine
	{
		ITimer* timInt;
		
		// Create the timer
		if ((ret = CreateObject(TypeTimer,&handle,(char*)0))) return;
		if ((ret = GetInterface(handle,CodeITimer,(void**)&timInt))) return;
		
		// Wait using the timer
		timInt->SetInterval(handle,50);
		timInt->Start(handle);
		WaitForObject(handle);
		ReleaseObject(handle);
	}
#endif
	
//	ListTest();
	
#ifdef TestSimpleDeviceDefine
	{
		UInt16 handle;
		ISimpleIO* inter;
		if ((ret = OpenObject("SerialPort1",&handle))) return;
		if ((ret = GetInterface(handle,CodeISimpleIO,(void**)&inter))) return;
		inter->Write(handle,(Int8*)"Hello, world!\r\n",15);
		ReleaseObject(handle);
	}
#endif
	
#ifdef TestEventDefine
	eveInt->Set(eveHandle);
#endif
	for (;;)
	{
#ifdef TestMathErrorDefine
		static Int16 i = 0;
		if (i++ > 50)
		{
			Int16 a = 1;
			Int16 b = 0;
			Int16 c = a / b;
			c++;
		}
#endif
//		zputs("My Init Thread");
	}
}
