#include <iostream>
#include <Windows.h>
#include <ddk/ntifs.h>
#include <tlhelp32.h>
#include <process.h>
#include <string.h>

using namespace std;
#define InitOA(ptr, root, attrib, name, desc, qos) { (ptr)->Length = sizeof(OBJECT_ATTRIBUTES);  (ptr)->RootDirectory = root; (ptr)->Attributes = attrib; (ptr)->ObjectName = name; (ptr)->SecurityDescriptor = desc; (ptr)->SecurityQualityOfService = qos; }
typedef NTSYSAPI NTSTATUS(NTAPI * __ntOpenProcess)(
	PHANDLE            ProcessHandle,
	ACCESS_MASK        DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PCLIENT_ID         ClientId
	);

typedef NTSYSAPI NTSTATUS(NTAPI *__ntResumeProcess)(
    HANDLE ProcessHandle
);

typedef NTSYSAPI NTSTATUS(NTAPI *__ntSuspendProcess)(
    HANDLE ProcessHandle
);

typedef NTSYSAPI NTSTATUS (NTAPI *__ntOpenThread)(
    PHANDLE            ThreadHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PCLIENT_ID         ClientId
);

typedef NTSYSAPI NTSTATUS (NTAPI *__ntQueryInformationThread)(
   HANDLE          ThreadHandle,
   THREADINFOCLASS ThreadInformationClass,
   PVOID          ThreadInformation,
   ULONG           ThreadInformationLength,
   PULONG         ReturnLength
);

__ntOpenProcess openProc;
__ntResumeProcess resumeProc;
__ntSuspendProcess suspendProc;
__ntOpenThread openThread;
__ntQueryInformationThread queryInformationThread;

HINSTANCE ntModule;
DWORD PIDProcess;

PROCESSENTRY32 GetPIDByName(char* szName){
    PROCESSENTRY32 processo;
    HANDLE hSys = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if(Process32First(hSys, &processo)){
        do {
           if(!strcmp(processo.szExeFile, szName)){
            //cout << "Threads: " << processo.cntThreads << endl;
            PIDProcess = processo.th32ProcessID;
            return processo;
           }
        } while(Process32Next(hSys, &processo));
    }
    memset(&processo, 0, sizeof(PROCESSENTRY32));
    return processo;
}

BOOL MatchAddressToModule( DWORD dwProcId, LPTSTR lpstrModule,  DWORD dwThreadStartAddr,  PDWORD pModuleStartAddr) // by Echo
{
    BOOL bRet = FALSE;
    HANDLE hSnapshot;
    MODULEENTRY32 moduleEntry32;

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE , dwProcId);

    moduleEntry32.dwSize = sizeof(MODULEENTRY32);
    moduleEntry32.th32ModuleID = 1;

    if(Module32First(hSnapshot, &moduleEntry32)){
        if(moduleEntry32.th32ProcessID == dwProcId)
            if(dwThreadStartAddr >= (DWORD)moduleEntry32.modBaseAddr && dwThreadStartAddr <= ((DWORD)moduleEntry32.modBaseAddr + moduleEntry32.modBaseSize)){
                strcpy(lpstrModule, moduleEntry32.szExePath);
                bRet=true;
        }else{
            while(Module32Next(hSnapshot, &moduleEntry32)){
                if(moduleEntry32.th32ProcessID == dwProcId){
                    if(dwThreadStartAddr >= (DWORD)moduleEntry32.modBaseAddr && dwThreadStartAddr <= ((DWORD)moduleEntry32.modBaseAddr + moduleEntry32.modBaseSize)){
                        strcpy(lpstrModule, moduleEntry32.szExePath);
                        bRet=true;
                        break;
                    }
                }
            }
        }
    }

    if(pModuleStartAddr) *pModuleStartAddr = (DWORD)moduleEntry32.modBaseAddr;
    CloseHandle(hSnapshot);

    return bRet;
}

void GetListThreads(DWORD pid){
    THREADENTRY32 thread;
    thread.dwSize = sizeof(THREADENTRY32);
    HANDLE hSys = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);
    if(hSys == INVALID_HANDLE_VALUE)
    {
        cout << "Invalid snapshot " << endl;
        return;
    }

    if(Thread32First(hSys, &thread)){
        do {
                if(thread.th32OwnerProcessID == pid){
                    openThread = (__ntOpenThread)GetProcAddress(ntModule, "NtOpenThread");
                    if(openThread != NULL){
                    HANDLE hThread = INVALID_HANDLE_VALUE;
                    CLIENT_ID uPid = { NULL };
                    uPid.UniqueProcess = ( PDWORD )thread.th32OwnerProcessID;
                    uPid.UniqueThread = ( PDWORD )thread.th32ThreadID;
                    OBJECT_ATTRIBUTES attribs;
                    InitOA(&attribs, NULL, NULL, NULL, NULL, NULL);

                    openThread(&hThread, THREAD_ALL_ACCESS, &attribs, &uPid);
                    if(hThread == INVALID_HANDLE_VALUE){
                        cout << "Thr identificador invalido" << endl;
                        continue;
                    }
                    queryInformationThread = (__ntQueryInformationThread)GetProcAddress(ntModule, "NtQueryInformationThread");
                    if(queryInformationThread == NULL){
                        cout << "Problema address query" << endl;
                        continue;
                    }
                    THREADINFOCLASS infosthread;
                    DWORD hThreadStartAddr, hThreadBaseAddress;
                    queryInformationThread(hThread, ThreadQuerySetWin32StartAddress , &hThreadStartAddr, sizeof(DWORD), NULL);
                    char moduleName[MAX_PATH];

                    if(MatchAddressToModule(thread.th32OwnerProcessID, moduleName, hThreadStartAddr,  &hThreadBaseAddress)){
                         if(strcspn("Main.exe", moduleName) == 0){
                            ResumeThread(hThread);
                            //cout << "Thread: " << thread.th32ThreadID << " Handle: " << hThread << " Nome: " << moduleName << " Local: " << hThreadBaseAddress <<  endl;
                         }
                    }

                    }
                }
           // }else{
            //    cout << "Unable address OpenThread" << endl;
            //}
        }while(Thread32Next(hSys, &thread));
    }
}

HANDLE GetHandle()
{
    PROCESSENTRY32 hWindow = GetPIDByName("Main.exe");
	if (hWindow.th32ProcessID == 0)
		return INVALID_HANDLE_VALUE;

	CLIENT_ID uPid = { NULL };
	uPid.UniqueProcess = ( PDWORD )hWindow.th32ProcessID;

	if (ntModule == NULL)
		return INVALID_HANDLE_VALUE;

    openProc = (__ntOpenProcess)GetProcAddress(ntModule, "NtOpenProcess"); // get ntopenproc

	if (openProc == NULL)
		return INVALID_HANDLE_VALUE;

	OBJECT_ATTRIBUTES attribs;
	InitOA(&attribs, NULL, NULL, NULL, NULL, NULL);

	HANDLE hProc = INVALID_HANDLE_VALUE;
	NTSTATUS _openStatus = openProc(&hProc, PROCESS_SUSPEND_RESUME, &attribs, &uPid); // open it now
	SetLastError(_openStatus); // set last error for correct debugging, otherwise getlasterror will be 0x0

	return hProc;
}

int main()
{
    ntModule = LoadLibraryA("ntdll");
    cout << "FireProtect procurando..." << endl;
	HANDLE hProc = GetHandle();

	if (hProc != INVALID_HANDLE_VALUE){
		cout << "Encontrado: 0x" << hProc << endl;
        suspendProc = (__ntSuspendProcess)GetProcAddress(ntModule, "NtSuspendProcess");
        if(suspendProc != NULL){
                suspendProc(hProc);
                GetListThreads(PIDProcess);
                cout << "Protecao suspensa." << endl;
        }else{
                cout << "Problema suspend" << endl;
        }
	}else
		cout << "(Jogo nao encontrado), Error Code: 0x" << cout <<  GetLastError() << endl;

	system("PAUSE");
    return 0;
}
