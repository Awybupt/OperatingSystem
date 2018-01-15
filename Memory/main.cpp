#include <thread>
#include <array>
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <iomanip>


#include <Windows.h>
#include <vector>

struct Trace
{
	LPVOID start;
	long size;
};
#define PAGE_UNKNOWN 0x0
#define MEM_UNKNOWN_TYPE 0x0

#define EXTRACT(VAR) {VAR, #VAR}

int main (int argc, char *argv[])
{
	try
	{
		// Shared info
		const std::vector<std::pair<DWORD, std::string>> protectArray
		{
			EXTRACT (PAGE_READONLY),
			EXTRACT (PAGE_READWRITE),
			EXTRACT (PAGE_EXECUTE),
			EXTRACT (PAGE_EXECUTE_READ),
			EXTRACT (PAGE_EXECUTE_READWRITE)
		};
		std::vector<Trace> traceInfo (protectArray.size ());

		size_t outputCount = 0;
		auto outputStatus = [&] (std::ostream &os)
		{
			constexpr auto outputAlignment = 25;
			const static std::vector<std::string> actionArray
			{
				"Reverse", "Commit", "Lock", "Unlock", "Decommit", "Release"
			};
			const static std::unordered_map<DWORD, std::string> protectionMap
			{
				EXTRACT (PAGE_UNKNOWN),
				EXTRACT (PAGE_NOACCESS),
				EXTRACT (PAGE_READONLY),
				EXTRACT (PAGE_READWRITE),
				EXTRACT (PAGE_WRITECOPY),
				EXTRACT (PAGE_EXECUTE),
				EXTRACT (PAGE_EXECUTE_READ),
				EXTRACT (PAGE_EXECUTE_READWRITE),
				EXTRACT (PAGE_EXECUTE_WRITECOPY),
				EXTRACT (PAGE_GUARD),
				EXTRACT (PAGE_NOCACHE),
				EXTRACT (PAGE_WRITECOMBINE),
				//EXTRACT (PAGE_REVERT_TO_FILE_MAP)
			};
			const static std::unordered_map<DWORD, std::string> stateMap
			{
				EXTRACT (MEM_COMMIT),
				EXTRACT (MEM_RESERVE),
				EXTRACT (MEM_FREE)
			};
			const static std::unordered_map<DWORD, std::string> typeMap
			{
				EXTRACT (MEM_UNKNOWN_TYPE),
				EXTRACT (MEM_IMAGE),
				EXTRACT (MEM_MAPPED),
				EXTRACT (MEM_PRIVATE)
			};

			SYSTEM_INFO info;
			GetSystemInfo (&info);
			if (!outputCount)
				os << std::setw (outputAlignment) << "PageSize " << info.dwPageSize << std::endl;

			if (outputCount % actionArray.size () == 0)
				os << "\nCase: " << protectArray[outputCount / actionArray.size ()].second << std::endl;
			os << actionArray[outputCount % actionArray.size ()] << std::endl;

			MEMORYSTATUS status;
			GlobalMemoryStatus (&status);
			MEMORY_BASIC_INFORMATION mem;
			VirtualQuery (traceInfo[outputCount / actionArray.size ()].start, &mem, sizeof (MEMORY_BASIC_INFORMATION));
			os << std::setw (outputAlignment) << "BaseAddress " << mem.BaseAddress << std::endl;
			os << std::setw (outputAlignment) << "AllocationBase " << mem.AllocationBase << std::endl;
			os << std::setw (outputAlignment) << "AllocationProtect " << protectionMap.at (mem.AllocationProtect) << std::endl;
			os << std::setw (outputAlignment) << "RegionSize / PageSize " << mem.RegionSize / info.dwPageSize << std::endl;
			os << std::setw (outputAlignment) << "Protect " << protectionMap.at (mem.Protect) << std::endl;
			os << std::setw (outputAlignment) << "State " << stateMap.at (mem.State) << std::endl;
			os << std::setw (outputAlignment) << "Type " << typeMap.at (mem.Type) << std::endl;

			outputCount++;
		};

		auto outputError = [] (std::ostream &os)
		{
			auto dwError = GetLastError ();
			HLOCAL hlocal = NULL;
			DWORD systemLocale = MAKELANGID (LANG_NEUTRAL, SUBLANG_NEUTRAL);
			BOOL fOk = FormatMessageA (
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
				FORMAT_MESSAGE_ALLOCATE_BUFFER,
				NULL, dwError, systemLocale,
				(char *) &hlocal, 0, NULL);
			if (!fOk)
			{
				// Is it a network-related error?
				HMODULE hDll = LoadLibraryEx (TEXT ("netmsg.dll"), NULL,
											  DONT_RESOLVE_DLL_REFERENCES);
				if (hDll != NULL)
				{
					fOk = FormatMessageA (
						FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS |
						FORMAT_MESSAGE_ALLOCATE_BUFFER,
						hDll, dwError, systemLocale,
						(char *) &hlocal, 0, NULL);
					FreeLibrary (hDll);
				}
			}

			if (fOk && (hlocal != NULL))
			{
				os << (const char *) LocalLock (hlocal) << std::endl;
				LocalFree (hlocal);
			}
			else
				os << "Could not Get Error Msg for " << dwError << std::endl;
		};

		auto semaphoreAlloc = CreateSemaphore (NULL, 1, 1, NULL);
		auto semaphoreTrack = CreateSemaphore (NULL, 0, 1, NULL);
		auto isEndTrack = false;

		auto threadAlloc = std::thread ([&] ()
		{
			SYSTEM_INFO info;
			GetSystemInfo (&info);

			size_t index = 0;
			for (const auto protection : protectArray)
			{
				auto syncWrapper = [&] (auto callback)
				{
					WaitForSingleObject (semaphoreAlloc, INFINITE);
					callback ();
					ReleaseSemaphore (semaphoreTrack, 1, NULL);
				};

				// Reference:
				// https://msdn.microsoft.com/en-us/library/windows/desktop/aa366887(v=vs.85).aspx

				// Reverse
				syncWrapper ([&] ()
				{
					traceInfo[index].size = (index + 1) * info.dwPageSize;
					traceInfo[index].start = VirtualAlloc (NULL, traceInfo[index].size,
														   MEM_RESERVE, PAGE_NOACCESS);
				});

				// Commit
				syncWrapper ([&] ()
				{
					traceInfo[index].start = VirtualAlloc (traceInfo[index].start,
														   traceInfo[index].size,
														   MEM_COMMIT, protection.first);
				});

				// Lock
				syncWrapper ([&] ()
				{
					if (!VirtualLock (traceInfo[index].start,
									  traceInfo[index].size))
						outputError (std::cout);
				});

				// Unlock
				syncWrapper ([&] ()
				{
					if (!VirtualUnlock (traceInfo[index].start,
										traceInfo[index].size))
						outputError (std::cout);
				});

				// Decommit
				syncWrapper ([&] ()
				{
					if (!VirtualFree (traceInfo[index].start,
									  traceInfo[index].size, MEM_DECOMMIT))
						outputError (std::cout);
				});

				// Release
				syncWrapper ([&] ()
				{
					if (!VirtualFree (traceInfo[index].start, 0, MEM_RELEASE))
						outputError (std::cout);
				});

				index++;
			}
			isEndTrack = true;
		});

		auto threadTrack = std::thread ([&] ()
		{
			std::ofstream ofs ("MemState.log");
			while (!isEndTrack)
			{
				WaitForSingleObject (semaphoreTrack, INFINITE);
				outputStatus (ofs);
				ReleaseSemaphore (semaphoreAlloc, 1, NULL);
			}
		});

		if (threadAlloc.joinable ()) threadAlloc.join ();
		if (threadTrack.joinable ()) threadTrack.join ();

		CloseHandle (semaphoreAlloc);
		CloseHandle (semaphoreTrack);
	}
	catch (const std::exception &ex)
	{
		std::cout << ex.what () << std::endl;
	}

	return 0;
}
