#pragma once

#include <Windows.h>
#include <fileapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <namedpipeapi.h>
#include <processthreadsapi.h>
#include <winnt.h>

namespace ipc
{
    struct Address
    {
        DWORD m_pid = 0;
        HANDLE m_handle = INVALID_HANDLE_VALUE;

        bool IsValid() const
        {
            return m_pid > 0 && m_handle != INVALID_HANDLE_VALUE;
        }
    };

    struct SharedData
    {
        Address m_address;
    };

    class SharedMem
    {
    private:
        SharedData* m_data = nullptr;
        HANDLE m_mappedObject = INVALID_HANDLE_VALUE;

    public:
        SharedMem();
        ~SharedMem();

        SharedData& GetSharedData();
    };

    template <typename T>
    class Receiver
    {
    private:
        HANDLE m_read = INVALID_HANDLE_VALUE;
        HANDLE m_write = INVALID_HANDLE_VALUE;

    public:
        Receiver()
        {
            CreatePipe(
                &m_read,
                &m_write,
                nullptr,
                0);
        }

        ~Receiver()
        {
            CloseHandle(m_write);
            CloseHandle(m_read);
        }

        bool Receive(T& item)
        {
            DWORD read;
            ReadFile(
                m_read,
                &item,
                sizeof(T),
                &read,
                nullptr);

            return read == sizeof(T);
        }

        Address GetAddress() const
        {
            return Address { GetCurrentProcessId(), m_write };
        }

    };

    template <typename T>
    class Sender
    {
    private:
        HANDLE m_write = INVALID_HANDLE_VALUE;

    public:
        Sender(const Address& address)
        {
            HANDLE proc = OpenProcess(
                PROCESS_ALL_ACCESS,
                false,
                address.m_pid);

            DuplicateHandle(
                proc,
                address.m_handle,
                GetCurrentProcess(),
                &m_write,
                DUPLICATE_SAME_ACCESS,
                false,
                0);

            CloseHandle(proc);
        }

        ~Sender()
        {
            CloseHandle(m_write);
        }

        bool Send(const T& item)
        {
            DWORD written;
            WriteFile(
                m_write,
                &item,
                sizeof(T),
                &written,
                nullptr);

            return written == sizeof(T);
        }
    };
}

