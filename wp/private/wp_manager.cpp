#include "ipc.h"

#include <libloaderapi.h>
#include <minwindef.h>
#include <string>
#include <Windows.h>

int main(int args, char** argv)
{
    if (args < 2)
    {
        return 1;
    }

    std::string command(argv[1]);

    ipc::SharedMem mem;
    ipc::SharedData& data = mem.GetSharedData();

    if (command == "start")
    {
        if (data.m_address.IsValid())
        {
            return 1;
        }

        char buff[MAX_PATH + 1] = {};
        GetModuleFileName(nullptr, buff, MAX_PATH);
        
        int pathLen = strlen(buff);
        for (int i = pathLen - 1; i >= 0; --i)
        {
            if (buff[i] == '\\')
            {
                buff[i] = 0;
                break;
            }
        }

        std::string curPath(buff);

        STARTUPINFO si = {};
        si.cb = sizeof(STARTUPINFO);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        PROCESS_INFORMATION pi = {};

        std::string shaderName;
        if (args > 2)
        {
            shaderName = " ";
            shaderName += argv[2];
        }

        if (args > 3)
        {
            shaderName += " ";
            shaderName += argv[3];
        }

        std::string exe = curPath + "\\wp.exe" + shaderName;
        std::string cmd = "cmd /c " + exe;

        CreateProcess(
            nullptr,
            const_cast<char *>(cmd.c_str()),
            nullptr,
            nullptr,
            false,
            CREATE_NEW_CONSOLE,
            nullptr,
            buff,
            &si,
            &pi
        );

        return 0;
    }

    if (command == "stop")
    {
        if (!data.m_address.IsValid())
        {
            return 1;
        }

        ipc::Sender<bool> sender(data.m_address);
        sender.Send(true);
    }

    return 0;
}
