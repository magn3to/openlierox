/*
 *  SystemFunctions.cpp
 *  OpenLieroX
 *
 *  Created by Albert Zeyer on 01.12.09.
 *  code under LGPL
 *
 */



#include <iomanip>
#include <time.h>
#include <SDL.h>
#define Font Font_Xlib // Hack to prevent name clash in precompiled header
#include <SDL_syswm.h>
#undef Font
#ifdef REAL_OPENGL
#include <SDL_opengl.h>
#endif
#include <cstdlib>
#include <sstream>
#include <cstring>

#if defined(__APPLE__)
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <sys/sysctl.h>
#include <mach/mach_traps.h>
#include <pthread.h>
#elif defined(WIN32) || defined(WIN64)
#include <windows.h>
#else
#include <cstdio>
#include <unistd.h>
#endif

#ifdef __FREEBSD__
#include <sys/sysctl.h>
#include <vm/vm_param.h>
#include <sys/vmmeter.h>
#endif

#ifndef WIN32
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#endif

#include "StringUtils.h"


#ifdef THREADNAME_VIA_SIGUSR1
#ifndef _MSC_VER
static void setCurThreadName__signalraiser(const char* name) {
	// this signal is only for debuggers, we should ignore it
	signal( SIGUSR1, SIG_IGN );

	// KDevelop will catch this, read the 'name' parameter and continue execution
	raise( SIGUSR1 );
}
#endif
#endif


//////////////////
// Gives a name to the thread
// Code taken from http://www.codeproject.com/KB/threads/Name_threads_in_debugger.aspx
void setCurThreadName(const std::string& name)
{
#ifdef _MSC_VER // Currently only for MSVC, haven't found a Windows-general way (I doubt there is one)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	} THREADNAME_INFO;
	
	THREADNAME_INFO info;
	{
		info.dwType = 0x1000;
		info.szName = name.c_str();
		info.dwThreadID = (DWORD)-1;
		info.dwFlags = 0;
	}
	
	__try
	{
		RaiseException( 0x406D1388 /* MSVC EXCEPTION */, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info );
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}
#else
	
#ifdef THREADNAME_VIA_SIGUSR1
	setCurThreadName__signalraiser(name.c_str());
#endif

#ifdef __APPLE__
	// pthread_setname_np only available since SDK 10.6
#if defined(MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
	pthread_setname_np(name.c_str());
#endif
#endif
	
#endif
}

void setCurThreadPriority(float p) {
#ifdef WIN32
	
#elif defined(__APPLE__)
	//int curp = getpriority(PRIO_DARWIN_THREAD, 0); 
	//int newp = p >= 0 ? 0 : 1;
	//notes << "curp:" << curp << ", newp:" << newp << endl;
	//setpriority(PRIO_DARWIN_THREAD, 0, newp);
#endif
}



size_t GetFreeSysMemory() {
#if defined(__APPLE__)
	vm_statistics_data_t page_info;
	vm_size_t pagesize;
	mach_msg_type_number_t count;
	kern_return_t kret;
	
	pagesize = 0;
	kret = host_page_size (mach_host_self(), &pagesize);
	count = HOST_VM_INFO_COUNT;
	
	kret = host_statistics (mach_host_self(), HOST_VM_INFO,(host_info_t)&page_info, &count);
	return page_info.free_count * pagesize;
#elif defined(__WIN64__)
	MEMORYSTATUSEX memStatex;
	memStatex.dwLength = sizeof (memStatex);
	::GlobalMemoryStatusEx (&memStatex);
	return memStatex.ullAvailPhys;
#elif defined(__WIN32__)
	MEMORYSTATUS memStatus;
	memStatus.dwLength = sizeof(MEMORYSTATUS);
	::GlobalMemoryStatus(&memStatus);
	return memStatus.dwAvailPhys;
#elif defined(__SUN__) && defined(_SC_AVPHYS_PAGES)
	return sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE);
#elif defined(__FREEBSD__)
	int vm_vmtotal[] = { CTL_VM, VM_METER };
	struct vmtotal vmdata;
	
	size_t len = sizeof(vmdata);
	int result = sysctl(vm_vmtotal, sizeof(vm_vmtotal) / sizeof(*vm_vmtotal), &vmdata, &len, NULL, 0);
	if(result != 0) return 0;
	
	return vmdata.t_free * sysconf(_SC_PAGESIZE);
#elif defined(__linux__)
	
	// get it from /proc/meminfo
	FILE *fp = fopen("/proc/meminfo", "r");
	if ( fp )
	{
		unsigned long freeMem = 0;
		unsigned long buffersMem = 0;
		unsigned long cachedMem = 0;
		struct SearchTerm { const char* name; unsigned long* var; };
		SearchTerm terms[] = { {"MemFree:", &freeMem}, {"Buffers:", &buffersMem}, {"Cached:", &cachedMem} };
		
		char buf[1024];
		int n = fread(buf, sizeof(char), sizeof(buf) - 1, fp);
		buf[sizeof(buf)-1] = '\0';
		if(n > 0) {
			for(unsigned int i = 0; i < sizeof(terms) / sizeof(SearchTerm); ++i) {
				char* p = strstr(buf, terms[i].name);
				if(p) {
					p += strlen(terms[i].name);
					*terms[i].var = strtoul(p, NULL, 10);
				}
			}
		}
		
		fclose(fp);
		// it's written in KB in meminfo
		return ((size_t)freeMem + (size_t)buffersMem + (size_t)cachedMem) * 1024;
	}
	
	return 0;
#else
#warning No GetFreeSysMemory implementation for this arch/sys
	return 50 * 1024 * 1024; // return 50 MB (really randomly made up, but helps for cache system)
#endif
}




std::string GetDateTimeText()
{
	time_t t = time(NULL);
	
	if (t == (time_t)-1)
		return "TIME-ERROR1";
	
	char* timeCstr = ctime(&t);
	if(timeCstr == NULL)
		return "TIME-ERROR2";
	
	std::string timeStr(timeCstr);
	TrimSpaces(timeStr);
	return timeStr;
}

std::string GetDateTimeFilename()
{
	// Add date and server name to screenshot filename
	time_t curtime1 = time(NULL);
	if (curtime1 == (time_t)-1)
		return "TIME-ERROR1";
	struct tm *curtime = localtime(&curtime1);
	if (curtime == NULL)
		return "TIME-ERROR2";
	char filePrefixTime[200] = {0};
	strftime(filePrefixTime, sizeof(filePrefixTime), "%Y%m%d-%H%M", curtime);
	return filePrefixTime;
}


