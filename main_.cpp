#include "common.h"

#define WINDOW_SIZE_X 1920
#define WINDOW_SIZE_Y 1080

ID3D11Device* device;
ID3D11DeviceContext* deviceContext;
IDXGISwapChain* swapChain;
ID3D11RenderTargetView* rtv;

ID3D11Buffer* vertexBuffer, * indexBuffer;

ID3D11VertexShader* vertexShader;
ID3D11InputLayout* inputLayout;
ID3D11PixelShader* pixelShader;

struct WorldBuffer
{
	XMFLOAT3 cameraPosition;
	float aspectRatio;
	float time;
	XMFLOAT3 emtpy;
	XMMATRIX cameraMatrix;
};
ID3D11Buffer* worldBuffer;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}

HWND CreateAWindow()
{
	WNDCLASSEX wcex = {};

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = &WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandleA(nullptr);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"RayMarchingClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, nullptr);

	if (RegisterClassEx(&wcex) <= 0)
	{
		printf("Failed to register win32 class\n");
		getchar();
		return 0;
	}

	HWND hWnd = CreateWindow(
		L"RayMarchingClass",
		L"Ray Marching",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		WINDOW_SIZE_X, WINDOW_SIZE_Y,
		NULL,
		NULL,
		wcex.hInstance,
		NULL
	);

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	return hWnd;
}
bool CreateDirect3D11(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	memset(&swapChainDesc, 0, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.Width = WINDOW_SIZE_X;
	swapChainDesc.BufferDesc.Height = WINDOW_SIZE_Y;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL featureLevel[] = {
		D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0,
	};
	if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr, 0,
		featureLevel, 3,
		D3D11_SDK_VERSION,
		&swapChainDesc, &swapChain,
		&device, nullptr,
		&deviceContext)))
	{
		printf("Failed to create Direct3D11 device and swapchain\n");
		getchar();
		return false;
	}

	ID3D11Texture2D* backBuffer;
	if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer)))
		return false;

	if (FAILED(device->CreateRenderTargetView(backBuffer, nullptr, &rtv)))
	{
		backBuffer->Release();

		printf("Failed to create render target view\n");
		getchar();
		return false;
	}

	backBuffer->Release();

	deviceContext->OMSetRenderTargets(1, &rtv, nullptr);

	D3D11_VIEWPORT viewport = { 0, 0, WINDOW_SIZE_X, WINDOW_SIZE_Y, 0.0f, 1.0f };
	deviceContext->RSSetViewports(1, &viewport);

	return true;
}

bool CreateRenderModel()
{
	XMFLOAT3 vertices[4] = {
		{ -1.0f,  1.0f, 0.0f },
		{  1.0f,  1.0f, 0.0f },
		{  1.0f, -1.0f, 0.0f },
		{ -1.0f, -1.0f, 0.0f },
	};
	uint32_t indices[6] = {
		0, 1, 2,
		0, 2, 3,
	};

	D3D11_BUFFER_DESC vertexBufferDesc;
	memset(&vertexBufferDesc, 0, sizeof(vertexBufferDesc));
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(XMFLOAT3) * 4;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	if (FAILED(device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer)))
	{
		printf("Failed to create vertex buffer\n");
		getchar();
		return false;
	}

	D3D11_BUFFER_DESC indexBufferDesc;
	memset(&indexBufferDesc, 0, sizeof(indexBufferDesc));
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(uint32_t) * 6;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	if (FAILED(device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer)))
	{
		printf("Failed to create index buffer\n");
		getchar();
		return false;
	}

	return true;
}

bool CreateRayMarchingShaders()
{
	ID3DBlob* errorMessage, * shaderBuffer;

	if (FAILED(D3DCompileFromFile(L"vertexshader.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &shaderBuffer, &errorMessage)))
	{
		if (errorMessage)
		{
			printf("%s\n", (const char*)errorMessage->GetBufferPointer());
			getchar();
			errorMessage->Release();
		}
		return false;
	}

	if (FAILED(device->CreateVertexShader(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), nullptr, &vertexShader)))
	{
		printf("Failed to create vertex shader\n");
		getchar();
		return false;
	}


	D3D11_INPUT_ELEMENT_DESC inputElements[1];
	memset(&inputElements, 0, sizeof(inputElements));
	inputElements[0].SemanticName = "POSITION";
	inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElements[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	if (FAILED(device->CreateInputLayout(inputElements, 1, shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), &inputLayout)))
		return false;

	shaderBuffer->Release();

	if (FAILED(D3DCompileFromFile(L"pixelshader.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &shaderBuffer, &errorMessage)))
	{
		if (errorMessage)
		{
			printf("%s\n", (const char*)errorMessage->GetBufferPointer());
			getchar();
			errorMessage->Release();
		}
		return false;
	}

	if (FAILED(device->CreatePixelShader(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), nullptr, &pixelShader)))
	{
		printf("Failed to create pixel shader\n");
		getchar();
		return false;
	}

	shaderBuffer->Release();

	D3D11_BUFFER_DESC bufferDesc;
	memset(&bufferDesc, 0, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(WorldBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	if (FAILED(device->CreateBuffer(&bufferDesc, nullptr, &worldBuffer)))
	{
		printf("Failed to create pixel shader buffer\n");
		getchar();
		return false;
	}

	return true;
}

int main()
{
	HWND hWnd = CreateAWindow();
	if (!hWnd)
		return -1;

	if (!CreateDirect3D11(hWnd))
		return -1;

	if (!CreateRenderModel())
		return -1;

	if (!CreateRayMarchingShaders())
		return -1;

	deviceContext->IASetInputLayout(inputLayout);
	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);

	clock_t startTime = clock();

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			float color[4] = { 0.0f, 1.0f, 0.0f , 1.0f };
			deviceContext->ClearRenderTargetView(rtv, color);

			//Prepare scene
			{
				D3D11_MAPPED_SUBRESOURCE map;
				if (FAILED(deviceContext->Map(worldBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
					continue;

				WorldBuffer* buffer = (WorldBuffer*)map.pData;

				float time = clock() - startTime;
				buffer->cameraPosition = { sin(time / 2000.0f) * -8.0f, 7.0f, cos(time / 2000.0f) * -8.0f };
				buffer->aspectRatio = (float)WINDOW_SIZE_X / (float)WINDOW_SIZE_Y;
				buffer->time = time;
				buffer->cameraMatrix = XMMatrixMultiply(XMMatrixRotationX(0.7f), XMMatrixRotationY(time / 2000.0f));

				deviceContext->Unmap(worldBuffer, 0);

				deviceContext->PSSetConstantBuffers(0, 1, &worldBuffer);
			}

			//Draw rectangle
			{
				unsigned int stride;
				unsigned int offset;

				stride = sizeof(XMFLOAT3);
				offset = 0;

				deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
				deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
				deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				deviceContext->DrawIndexed(6, 0, 0);
			}

			swapChain->Present(0, 0);
		}
	}

	return (int)msg.wParam;
}