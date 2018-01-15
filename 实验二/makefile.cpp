/////////////////////////////////////////////////////////////////////////////////////////////
//makefile.cpp
/////////////////////////////////////////////////////////////////////////////////////////////
//文件生成程序
#include <fstream.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
struct operation
{
	int time;//起始时间
    int block;//内存页数
    int oper;//操作
	int protection;//权限
};
int main()
{
	FILE* file;
	file=fopen("opfile","wb");//“opfile”为二进制用以确定内存操作
	operation op;

	for(int j=0;j<6;j++) //0-保留；1-提交；2-锁；3-解锁；4-回收；5-释放
		for(int i=0;i<5;i++)
			//0-PAGE_READONLY;
			//1-PAGE_READWRITE;
			//2-PAGE_EXECUTE;
            //3-PAGE_EXECUTE_READ;
			//4-PAGE_EXECUTE_READWRITE;
		{
			op.time=rand()%1000;//随机生成等待时间
			op.block=rand()%5+1;//随机生成块大小
			op.oper=j;
			op.protection=i;
			fwrite(&op,sizeof(operation),1,file);//将生成的结构写入文件
		}
    return 0;
}
