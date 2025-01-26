#include "render.h"

#include "dim.h"

#include "d3dx12.h"
#include <combaseapi.h>
#include <cstdlib>
#include <cstring>
#include <d3d12.h>

#include <d3dcommon.h>
#include <handleapi.h>
#include <wrl.h>
#include <dxgi1_4.h>

#include <d3dcompiler.h>

#include <chrono>
#include <thread>

#include <iostream>

extern const wchar_t* shaderName;
extern int maxFPS;

namespace
{
    using Microsoft::WRL::ComPtr;

    ComPtr<ID3D12Debug> debugController;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<ID3D12CommandQueue> copyCommandQueue;
    ComPtr<IDXGISwapChain3> swapChain;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12Resource> renderTargets[2];

    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commandLists[2];

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
    
    struct Vertex
    {
        float x;
        float y;
        float u;
        float v;
    };

    Vertex verts[] = 
    {
        {-1, -1, 0, 0},
        {1, -1, 1, 0},
        {1, 1, 1, 1},
        {-1, 1, 0, 1},
    };

    int indices[] = { 0, 3, 2, 0, 2, 1 };

    ComPtr<ID3D12Heap> vertexBufferHeap;
    ComPtr<ID3D12Resource> vertexBuffer;

    ComPtr<ID3D12Heap> indexBufferHeap;
    ComPtr<ID3D12Resource> indexBuffer;

    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;

    ComPtr<ID3D12PipelineState> pipelineState;
    ComPtr<ID3D12RootSignature> rootSigniture;

    ComPtr<ID3D12Resource> settings;
    ComPtr<ID3D12Resource> uploadSettings;

    void CopyBuffers(ID3D12Resource* dest, ID3D12Resource* src, ID3D12CommandList* prepared = nullptr)
    {
        static UINT64 index = 0;

        static ComPtr<ID3D12Fence> fence;

        static ComPtr<ID3D12CommandAllocator> copyAllocator;
        static ComPtr<ID3D12GraphicsCommandList> copyList;
        static HANDLE event = CreateEvent(nullptr, false, false, nullptr);

        if (index < 1)
        {
            index = 1;
            device->CreateFence(
                0,
                D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(&fence));


            device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,
                IID_PPV_ARGS(&copyAllocator)); 

            device->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,
                copyAllocator.Get(),
                nullptr,
                IID_PPV_ARGS(&copyList));

            copyList->Close();
        }

        ID3D12CommandList* cl = prepared;

        if (!cl)
        {
            copyAllocator->Reset();
            copyList->Reset(copyAllocator.Get(), nullptr);

            {
                CD3DX12_RESOURCE_BARRIER barrier =
                    CD3DX12_RESOURCE_BARRIER::CD3DX12_RESOURCE_BARRIER::Transition(
                            dest,
                            D3D12_RESOURCE_STATE_PRESENT,
                            D3D12_RESOURCE_STATE_COPY_DEST);

                copyList->ResourceBarrier(1, &barrier);
            }

            copyList->CopyResource(dest, src);

            {
                CD3DX12_RESOURCE_BARRIER barrier =
                    CD3DX12_RESOURCE_BARRIER::CD3DX12_RESOURCE_BARRIER::Transition(
                            dest,
                            D3D12_RESOURCE_STATE_COPY_DEST,
                            D3D12_RESOURCE_STATE_PRESENT);

                copyList->ResourceBarrier(1, &barrier);
            }

            copyList->Close();
            cl = copyList.Get();
        }

        UINT64 signal = index++;

        fence->SetEventOnCompletion(signal, event);

        copyCommandQueue->ExecuteCommandLists(
            1,
            &cl);

        copyCommandQueue->Signal(fence.Get(), signal);

        WaitForSingleObject(event, INFINITE);
    }

    void CreateVertexBuffer()
    {
        {
            auto heapProps = 
                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

            auto resourceDescription =
                CD3DX12_RESOURCE_DESC::Buffer(_countof(verts) * sizeof(Vertex));

            device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
                    &resourceDescription,
                    D3D12_RESOURCE_STATE_PRESENT,
                    nullptr,
                    IID_PPV_ARGS(&vertexBuffer));

            heapProps =
                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);

            ComPtr<ID3D12Resource> uploadBuff;
            device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
                    &resourceDescription,
                    D3D12_RESOURCE_STATE_PRESENT,
                    nullptr,
                    IID_PPV_ARGS(&uploadBuff));


            void* data = nullptr;
            CD3DX12_RANGE writeRange(0, 0);
            uploadBuff->Map(0, &writeRange, &data);

            memcpy(data, verts, _countof(verts) * sizeof(Vertex));

            uploadBuff->Unmap(0, nullptr);

            CopyBuffers(vertexBuffer.Get(), uploadBuff.Get());
        }

        {
            auto heapProps = 
                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

            auto resourceDescription =
                CD3DX12_RESOURCE_DESC::Buffer(_countof(indices) * sizeof(int));

            device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
                    &resourceDescription,
                    D3D12_RESOURCE_STATE_PRESENT,
                    nullptr,
                    IID_PPV_ARGS(&indexBuffer));

            heapProps =
                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);

            ComPtr<ID3D12Resource> uploadBuff;
            device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
                    &resourceDescription,
                    D3D12_RESOURCE_STATE_PRESENT,
                    nullptr,
                    IID_PPV_ARGS(&uploadBuff));


            void* data = nullptr;
            CD3DX12_RANGE writeRange(0, 0);
            uploadBuff->Map(0, &writeRange, &data);

            memcpy(data, indices, _countof(indices) * sizeof(int));

            uploadBuff->Unmap(0, nullptr);

            CopyBuffers(indexBuffer.Get(), uploadBuff.Get());
        }
    }

    void CreateSettingsBuffer()
    {
        {
            auto heapProps = 
                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

            auto resourceDescription =
                CD3DX12_RESOURCE_DESC::Buffer(256);

            device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
                    &resourceDescription,
                    D3D12_RESOURCE_STATE_PRESENT,
                    nullptr,
                    IID_PPV_ARGS(&settings));

            heapProps =
                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);

            device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
                    &resourceDescription,
                    D3D12_RESOURCE_STATE_PRESENT,
                    nullptr,
                    IID_PPV_ARGS(&uploadSettings));
        }
    }

    void UpdateSettings()
    {
        static bool init = false;
        static ComPtr<ID3D12CommandAllocator> copyAllocator;
        static ComPtr<ID3D12GraphicsCommandList> copyList;
        static auto startTime = std::chrono::system_clock::now();

        if (!init)
        {
            init = true;

            device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,
                IID_PPV_ARGS(&copyAllocator)); 

            device->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,
                copyAllocator.Get(),
                nullptr,
                IID_PPV_ARGS(&copyList));

            {
                CD3DX12_RESOURCE_BARRIER barrier =
                    CD3DX12_RESOURCE_BARRIER::CD3DX12_RESOURCE_BARRIER::Transition(
                            settings.Get(),
                            D3D12_RESOURCE_STATE_PRESENT,
                            D3D12_RESOURCE_STATE_COPY_DEST);

                copyList->ResourceBarrier(1, &barrier);
            }

            copyList->CopyResource(settings.Get(), uploadSettings.Get());

            {
                CD3DX12_RESOURCE_BARRIER barrier =
                    CD3DX12_RESOURCE_BARRIER::CD3DX12_RESOURCE_BARRIER::Transition(
                            settings.Get(),
                            D3D12_RESOURCE_STATE_COPY_DEST,
                            D3D12_RESOURCE_STATE_PRESENT);

                copyList->ResourceBarrier(1, &barrier);
            }


            copyList->Close();
        }

        struct Data
        {
            float iResolution[3] = {
                static_cast<float>(wp::WIDTH),
                static_cast<float>(wp::HEIGHT),
                0.0f
            };
            float iTime = 0;
            float iTimeDelta = 0;

            float placeholder[3];
        };

        auto now = std::chrono::system_clock::now();
        auto elapsedMS = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime);

        static float lastFrame = 0;
        float elapsed = static_cast<float>(elapsedMS.count()) / 1000000.0f;

        void* data = nullptr;
        CD3DX12_RANGE writeRange(0, 0);
        uploadSettings->Map(0, &writeRange, &data);

        Data* d = static_cast<Data*>(data);
        *d = Data();
        d->iTime = elapsed;
        d->iTimeDelta = elapsed - lastFrame;

        lastFrame = elapsed;


        uploadSettings->Unmap(0, nullptr);

        CopyBuffers(vertexBuffer.Get(), uploadSettings.Get(), copyList.Get());
    }

    void CompileShaders()
    {
        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
        flags |= D3DCOMPILE_DEBUG;
#endif
        {
            LPCSTR profile = "vs_5_0";
            ID3DBlob* errorBlob = nullptr;
            HRESULT hr = D3DCompileFromFile(
                L"vertex.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                "main", profile,
                flags, 0, &vertexShader, &errorBlob);

            if (errorBlob)
            {
				char* ddd = static_cast<char*>(errorBlob->GetBufferPointer());
				std::cout << ddd << std::endl;
            }
        }
        {
            LPCSTR profile = "ps_5_0"; 
            ID3DBlob* errorBlob = nullptr;
            HRESULT hr = D3DCompileFromFile(
                shaderName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                "main", profile,
                flags, 0, &pixelShader, &errorBlob );

            if (errorBlob)
            {
				char* ddd = static_cast<char*>(errorBlob->GetBufferPointer());
				std::cout << ddd << std::endl;
            }

        }
    }

    void CreatePipelineStateAndRootSigniture()
    {
		{
			D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

			// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

			if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
				featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
			}

			// Allow input layout and deny uneccessary access to certain pipeline stages.
			D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
			CD3DX12_ROOT_PARAMETER1 rootParameters[1];
			rootParameters[0].InitAsConstantBufferView(0, 0);

			rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);
			//rootSignatureDesc.Init_1_1(0, nullptr, 0, nullptr, rootSignatureFlags);

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;
			D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
			device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSigniture));
		}

		{
			// Describe and create the graphics pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputLayout, _countof(inputLayout)};
			psoDesc.pRootSignature = rootSigniture.Get();
			psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

			//psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

			//psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            psoDesc.DepthStencilState.DepthEnable = false;

			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;

			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
            device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
		}
    }
}

void wp::CreateDXObjects(HWND hWnd)
{
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }

    D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_12_0,
        IID_PPV_ARGS(&device));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};

    device->CreateCommandQueue(
        &queueDesc,
        IID_PPV_ARGS(&commandQueue));

    queueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;
    device->CreateCommandQueue(
        &queueDesc,
        IID_PPV_ARGS(&copyCommandQueue)
    );

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2; // Double buffer (or more if you want triple buffer)
    swapChainDesc.Width = wp::WIDTH;    // Window width
    swapChainDesc.Height = wp::HEIGHT;   // Window height
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Pixel format
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Recommended swap effect
    swapChainDesc.SampleDesc.Count = 1; // No multi-sampling

    IDXGIFactory4* factory = nullptr;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    ComPtr<IDXGISwapChain1> swapChain1;
    factory->CreateSwapChainForHwnd(
        commandQueue.Get(),
        hWnd, // Handle to the window (created using CreateWindowEx)
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1);

    swapChain1.As(&swapChain);

    // Create Render Target Views (RTVs)
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 2;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap));

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
                    rtvHeap->GetCPUDescriptorHandleForHeapStart());

        int size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        for (UINT i = 0; i < 2; ++i)
        {
            swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
            device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, size);
        }
    }

    CreateVertexBuffer();
    CreateSettingsBuffer();
    CompileShaders();
    CreatePipelineStateAndRootSigniture();
}

void wp::RenderLoop()
{
    device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&allocator));

    for (int i = 0; i < 2; ++i)
    {
        device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            allocator.Get(),
            pipelineState.Get(),
            IID_PPV_ARGS(&commandLists[i]));

        ID3D12GraphicsCommandList* commandList = commandLists[i].Get();

        {
            //int index = swapChain->GetCurrentBackBufferIndex();

            CD3DX12_RESOURCE_BARRIER barrier =
                CD3DX12_RESOURCE_BARRIER::CD3DX12_RESOURCE_BARRIER::Transition(
                        renderTargets[i].Get(),
                        D3D12_RESOURCE_STATE_PRESENT,
                        D3D12_RESOURCE_STATE_RENDER_TARGET);

            commandList->ResourceBarrier(1, &barrier);
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
                rtvHeap->GetCPUDescriptorHandleForHeapStart());
        int size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        rtvHandle.Offset(i, size);

        float clearColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };  // Black color
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        ID3D12Resource* curRT = renderTargets[i].Get();
		commandList->SetGraphicsRootSignature(rootSigniture.Get());
        commandList->SetGraphicsRootConstantBufferView(0, settings->GetGPUVirtualAddress());

		auto viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(wp::WIDTH), static_cast<float>(wp::HEIGHT));
		auto scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(wp::WIDTH), static_cast<LONG>(wp::HEIGHT));

        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);

		D3D12_CPU_DESCRIPTOR_HANDLE handles[] = { rtvHandle };
		commandList->OMSetRenderTargets(_countof(handles), handles, FALSE, nullptr);

		D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[1];
		D3D12_VERTEX_BUFFER_VIEW& realVertexBufferView = vertexBufferViews[0];
		realVertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		realVertexBufferView.StrideInBytes = sizeof(Vertex);
		realVertexBufferView.SizeInBytes = _countof(verts) * sizeof(Vertex);

		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
		indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		indexBufferView.SizeInBytes = _countof(indices) * sizeof(int);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->IASetVertexBuffers(0, _countof(vertexBufferViews), vertexBufferViews);
		commandList->IASetIndexBuffer(&indexBufferView);

		commandList->DrawIndexedInstanced(
			_countof(indices),
			1,
			0,
			0,
			0
		);

        {
            //int index = swapChain->GetCurrentBackBufferIndex();
            CD3DX12_RESOURCE_BARRIER barrier =
                CD3DX12_RESOURCE_BARRIER::CD3DX12_RESOURCE_BARRIER::Transition(
                        renderTargets[i].Get(),
                        D3D12_RESOURCE_STATE_RENDER_TARGET,
                        D3D12_RESOURCE_STATE_PRESENT);

            commandList->ResourceBarrier(1, &barrier);
        }

        commandList->Close();
    }

    ComPtr<ID3D12Fence> fence;
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

    UINT64 frameIndex = 1;

    HANDLE h = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    int minMS = ceil(1000.0 / maxFPS);

    while (true)
    {
        auto frameStart = std::chrono::system_clock::now();

        UpdateSettings();
		int index = swapChain->GetCurrentBackBufferIndex();
		ID3D12GraphicsCommandList* commandList = commandLists[index].Get();

		commandQueue->ExecuteCommandLists(
			1,
			reinterpret_cast<ID3D12CommandList**>(&commandList));

        UINT64 signal = frameIndex++;
        commandQueue->Signal(fence.Get(), signal);

        HRESULT hRes = fence.Get()->SetEventOnCompletion(signal, h);

		DWORD res = WaitForSingleObject(h, INFINITE);
		swapChain->Present(1, 0);

        auto frameEnd = std::chrono::system_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            frameEnd - frameStart).count();

        if (elapsed < minMS)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(minMS - elapsed));
        }
    }
}
