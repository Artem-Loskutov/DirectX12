#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include "wc.h"

LRESULT CALLBACK WinProc(HWND windowHandler, UINT umessage, WPARAM wparam, LPARAM lparam) {
	if (umessage == WM_CREATE)
	{
		const auto* const createStruct = reinterpret_cast<CREATESTRUCT*>(lparam);
		::SetWindowLongPtr(windowHandler, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
	}
	switch (umessage)
	{
		case WM_DESTROY:
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}
		default:
		{
			return DefWindowProc(windowHandler, umessage, wparam, lparam);
		}
	}
}

int Run()
{
	MSG msg = {};
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"DX12 call failed", L"Error", MB_OK);
		exit(-1);
	}
}

int WINAPI WinMain(HINSTANCE hInstance,	HINSTANCE hPrevInstance, PSTR pCmdLine, int nShowCmd)
{
	WindowClass win(hInstance, WinProc);
	win.CreateWinEx();
	win.ShowWin();

	//Some
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	ThrowIfFailed(
		D3D12CreateDevice(
			nullptr,
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
		)
	);

	//Commands
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	ThrowIfFailed(
		device->CreateCommandQueue(
			&queueDesc,
			IID_PPV_ARGS(&mCommandQueue)
		));
	ThrowIfFailed(
		device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&mCommandAllocator)
		));
	ThrowIfFailed(
		device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			mCommandAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(&mCommandList)
		));
	ThrowIfFailed(mCommandList->Close());

	//Fence
	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mFenceValue = 0;
	HANDLE mFenceEvent = nullptr;

	ThrowIfFailed(
		device->CreateFence(
			0,
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&mFence)
		)
	);

	mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	
	//SwapChain
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	ThrowIfFailed(
		CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory))
	);

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Width = 400;
	swapChainDesc.Height = 400;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;

	ThrowIfFailed(
		dxgiFactory->CreateSwapChainForHwnd(
			mCommandQueue.Get(),
			win.GetHWND(),
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain1
		)
	);

	Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapChain;
	ThrowIfFailed(
		swapChain1.As(&mSwapChain)
	);

	UINT mCurrentBackBuffer = mSwapChain->GetCurrentBackBufferIndex();




	UINT rtvDescriptorSize =
		device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ThrowIfFailed(
		device->CreateDescriptorHeap(
			&rtvHeapDesc,
			IID_PPV_ARGS(&mRtvHeap)
		)
	);

	Microsoft::WRL::ComPtr<ID3D12Resource> mBackBuffers[2];

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < 2; i++)
	{
		ThrowIfFailed(
			mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBackBuffers[i]))
		);

		device->CreateRenderTargetView(
			mBackBuffers[i].Get(),
			nullptr,
			rtvHandle
		);

		rtvHandle.ptr += rtvDescriptorSize;
	}


	D3D12_VIEWPORT mViewport = {};
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.Width = 400;
	mViewport.Height = 400;
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	D3D12_RECT mScissorRect = { 0, 0, 400, 400 };

	ThrowIfFailed(mCommandAllocator->Reset());
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	UINT backIndex = mSwapChain->GetCurrentBackBufferIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleAll = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += backIndex * rtvDescriptorSize;

	mCommandList->RSSetViewports(1, &mViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	FLOAT clearColor[] = { 0.0f, 0.9f, 0.8f, 1.0f };
	mCommandList->ClearRenderTargetView(
		rtvHandleAll,
		clearColor,
		0,
		nullptr
	);

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* lists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(1, lists);

	ThrowIfFailed(
		mSwapChain->Present(1, 0)
	);


	return Run();
}