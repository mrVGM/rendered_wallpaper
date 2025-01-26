#include "ipc.h"
#include <errhandlingapi.h>
#include <handleapi.h>
#include <memoryapi.h>
#include <winbase.h>
#include <winerror.h>

ipc::SharedMem::SharedMem()
{
    m_mappedObject = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        sizeof(SharedData),
        TEXT("adfa94ad-bfdd-4e50-971d-108d860ab4d2"));

    bool shouldInit = GetLastError() != ERROR_ALREADY_EXISTS;

    void* tmp = MapViewOfFile(
        m_mappedObject,
        FILE_MAP_WRITE,
        0,
        0,
        0);

    m_data = static_cast<SharedData*>(tmp);

    if (shouldInit)
    {
        *m_data = SharedData();
    }
}

ipc::SharedMem::~SharedMem()
{
    UnmapViewOfFile(m_data);
    CloseHandle(m_mappedObject);
}

ipc::SharedData& ipc::SharedMem::GetSharedData()
{
    return *m_data;
}
