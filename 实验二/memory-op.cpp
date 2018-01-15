 


////////////////////////////////////////////////////////////////////////////////////////////////////////
//memory-op.cpp

//内存管理实习
//将程序从文件读入每次的操作，并将结果输入到out.txt文件中
////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <fstream.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <iostream.h>

struct operation
{
	int time;//起始时间
    int block;//内存页数
    int oper;//操作
	int protection;//权限
};

struct trace //跟踪每一次分配活动的数据结构
{
	LPVOID start;//起始地址
	long size;   //分配的大小
};

HANDLE allo,trac; //信号量的句柄

DWORD Tracker(LPDWORD lpdwparm) //跟踪allocator线程的内存行为，并输出必要信息
{
	ofstream outfile;//输出文件
	outfile.open("out.txt");

	for(int i=0;i<=30;i++)
	{
		WaitForSingleObject(trac,INFINITE); //等待allocator一次内存分配活动结束
		//打印内存状况和系统状况
		outfile<<i<<endl;
		//以下一段显示系统信息，每次执行操作后系统信息不变
		//如果要查看系统信息，可以取消注释

		SYSTEM_INFO info;  //系统信息
		GetSystemInfo(&info);
		outfile<<"dwActiveProcessorMask"<<'\t'<<info.dwActiveProcessorMask<<endl;
		outfile<<"dwAllocationGranularity"<<'\t'<<info.dwAllocationGranularity<<endl;		
        outfile<<"dwNumberOfProcessors"<<'\t'<<info.dwNumberOfProcessors<<endl;
		outfile<<"dwOemId"<<'\t'<<info.dwOemId<<endl;
		outfile<<"dwPageSize"<<'\t'<<info.dwPageSize<<endl;
		outfile<<"dwProcessorType"<<'\t'<<info.dwProcessorType<<endl;
		outfile<<"lpMaximumApplicationAddress"<<'\t'<<info.lpMaximumApplicationAddress<<endl;
        outfile<<"lpMinimumApplicationAddress"<<'\t'<<info.lpMinimumApplicationAddress<<endl;
		outfile<<"wProcessorArchitecture"<<'\t'<<info.wProcessorArchitecture<<endl;
		outfile<<"wProcessorLevel"<<'\t'<<info.wProcessorLevel<<endl;
		outfile<<"wProcessorRevision"<<'\t'<<info.wProcessorRevision<<endl;
		outfile<<"wReserved"<<'\t'<<info.wReserved<<endl;
		outfile<<"************************************************************************************"<<endl;

        //内存状况
		MEMORYSTATUS status; //内存状态
		GlobalMemoryStatus(&status);
		outfile<<"dwAvailPageFile"<<'\t'<<status.dwAvailPageFile<<endl;
		outfile<<"dwAvailPhys"<<'\t'<<status.dwAvailPhys<<endl;
        outfile<<"dwAvailVirtual"<<'\t'<<status.dwAvailVirtual<<endl;
        outfile<<"dwLength"<<'\t'<<status.dwLength<<endl;
        outfile<<"dwMemoryLoad"<<'\t'<<status.dwMemoryLoad<<endl;
		outfile<<"dwTotalPageFile"<<'\t'<<status.dwTotalPageFile<<endl;
        outfile<<"dwTotalPhys"<<'\t'<<status.dwTotalPhys<<endl;
        outfile<<"dwTotalVirtual"<<'\t'<<status.dwTotalVirtual<<endl;
        outfile<<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&"<<endl;

		//以下一段显示内存基本信息，每次操作后内存基本信息不变
		//如要查看内存基本信息，可以取消注释
        		
		MEMORY_BASIC_INFORMATION mem;//内存基本信息
		VirtualQuery(     info.lpMinimumApplicationAddress,&mem,
		                  sizeof(MEMORY_BASIC_INFORMATION)      );
		outfile<<"AllocationBase"<<'\t'<<mem.AllocationBase<<endl;
        outfile<<"AllocationProtect"<<'\t'<<mem.AllocationProtect<<endl;
        outfile<<"BaseAddress"<<'\t'<<mem.BaseAddress<<endl;
        outfile<<"Protect"<<'\t'<<mem.Protect<<endl;
        outfile<<"RegionSize"<<'\t'<<mem.RegionSize<<endl;
        outfile<<"State"<<'\t'<<mem.State<<endl;
        outfile<<"Type"<<'\t'<<mem.Type<<endl;
		outfile<<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<endl;
		
		//释放信号量通知allocator可以执行下一次内存分配活动
		ReleaseSemaphore(allo,1,NULL);
	}
	return 0;
}
void Allocator() //模拟内存分配活动的线程
{
	trace traceArray[5];
	int index=0;
	FILE* file;
	file=fopen("opfile","rb");//读入文件
	operation op;
	SYSTEM_INFO info;
	DWORD temp;
	GetSystemInfo(&info);
	for(int i=0;i<30;i++)
	{
       WaitForSingleObject(allo,INFINITE);  //等待tracker打印结束的信号量
	   cout<<i<<':';
	   fread(&op,sizeof(operation),1,file);
	   Sleep(op.time); //执行时间，如果想在指定时间执行可以取消注释
	   GetSystemInfo(&info);
	   switch(op.protection) //根据文件内容确定权限
	   {
	   case 0:
		   {
			   index=0;
			   temp=PAGE_READONLY;
			   break;
		   }
       case 1:
		   temp=PAGE_READWRITE;
		   break;
	   case 2:
		   temp=PAGE_EXECUTE;
		   break;
	   case 3:
		   temp=PAGE_EXECUTE_READ;
		   break;
	   case 4:
		   temp=PAGE_EXECUTE_READWRITE;
		   break;
	   default:
		   temp=PAGE_READONLY;
	   }
	   switch(op.oper)
	   {
	   case 0://保留一个区域
		   {
			   cout<<"reserve now"<<endl;

			   traceArray[index].start=VirtualAlloc(NULL,op.block*info.dwPageSize,
				                                    MEM_RESERVE,PAGE_NOACCESS);
               traceArray[index++].size=op.block*info.dwPageSize;
			   cout<<"starting address:"
				   <<traceArray[index-1].start<<'\t'<<"size:"<<traceArray[index-1].size<<endl;
			   break;
		   }
       case 1://提交一个区域
		   {
			   cout<<"commit now"<<endl;

			   traceArray[index].start=VirtualAlloc(traceArray[index].start,traceArray[index].size,MEM_COMMIT,temp);
			   index++;
			   cout<<"starting address:"
				   <<traceArray[index-1].start<<'\t'<<"size:"<<traceArray[index-1].size<<endl;
			   break;
		   }
       case 2: //锁一个区域
		   {
			   cout<<"lock now"<<endl;
			   cout<<"starting address:"<<traceArray[index].start<<'\t'<<"size:"<<traceArray[index].size<<endl;
			   if(!VirtualLock(traceArray[index].start,traceArray[index++].size))
				   cout<<GetLastError()<<endl;//GetLastError()函数返回错误号
			   break;
		   }
       case 3: //解锁一个区域
		   {
               cout<<"unlock now"<<endl;
               cout<<"starting address:"<<traceArray[index].start<<'\t'<<"size:"<<traceArray[index].size<<endl;
               if(!VirtualUnlock(traceArray[index].start,traceArray[index++].size))
                   cout<<GetLastError()<<endl;
               break;
		   }
	   case 4: //回收一个区域
		   {
               cout<<"decommit now"<<endl;
               cout<<"starting address:"<<traceArray[index].start<<'\t'<<"size:"<<traceArray[index].size<<endl;
               if(!VirtualFree(traceArray[index].start,traceArray[index++].size,MEM_DECOMMIT))
                   cout<<GetLastError()<<endl;
               break;
		   }
	   case 5: //释放一个区域
		   {
               cout<<"release now"<<endl;
			   cout<<"starting address:"<<traceArray[index].start<<'\t'<<"size:"<<traceArray[index].size<<endl;
               if(!VirtualFree(traceArray[index++].start,0,MEM_RELEASE))
                   cout<<GetLastError()<<endl;
			   break;
		   }
	   default:
		   cout<<"error"<<endl;
	   }
	   ReleaseSemaphore(trac,1,NULL); //释放信号量通知tracker可以打印信息
	}
}
int main()
{
	DWORD dwThread;
	HANDLE handle[2];
	//生成两个线程
	handle[0]=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Tracker,NULL,0,&dwThread);
    handle[1]=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Allocator,NULL,0,&dwThread);
	
	//生成两个信号量
	allo=CreateSemaphore(NULL,0,1,"allo");
    trac=CreateSemaphore(NULL,1,1,"trac");
	//等待线程执行的执行结束后，再退出
	WaitForMultipleObjects(2,handle,TRUE,INFINITE);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//The End
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	
	

