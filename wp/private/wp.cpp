#include <Windows.h>

#include <corecrt_terminate.h>
#include <cstring>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#include "renderWindow.h"
#include "render.h"
#include "ipc.h"
#include "tasks.h"
#include <string>

#include <sstream>

const wchar_t* shaderName = nullptr;
int maxFPS;

int main(int args, char** argv)
{
    std::wstring shader(L"pixel.hlsl");

    if (args > 1)
    {
        std::string tmp(argv[1]);
        shader = std::wstring(tmp.begin(), tmp.end());
    }

    if (args > 2)
    {
        std::stringstream ss;
        ss << argv[2];

        ss >> maxFPS;
    }

    shaderName = shader.c_str();

    shaderName = shader.c_str();

    HWND w = wp::creteRenderWindow();

    wp::CreateDXObjects(w);

    tasks::ThreadPool tp(1);
    tp.run([]() {
        wp::RenderLoop();
    });

    ipc::SharedMem mem;
    ipc::Receiver<bool> terminate;

    mem.GetSharedData().m_address = terminate.GetAddress();

    bool tmp;
    terminate.Receive(tmp);

    return 0;
}

