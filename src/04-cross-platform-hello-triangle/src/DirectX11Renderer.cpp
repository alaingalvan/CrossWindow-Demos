#include "Renderer.h"

// DirectX utils

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

// Renderer

Renderer::Renderer(xwin::Window& window)
{
	mVsync = true;
	mWindow = nullptr;

	mFactory = nullptr;
	mAdapter = nullptr;
	mAdapterOutput = nullptr;
	mDevice = nullptr;
	mDeviceContext = nullptr;
#if defined(_DEBUG)
	mDebugController = nullptr;
#endif

	mVertexBuffer = nullptr;
	mIndexBuffer = nullptr;
	mLayout = nullptr;
	mUniformBuffer = nullptr;
	mVertexShader = nullptr;
	mPixelShader = nullptr;

	mSwapchain = nullptr;
	mBackbufferTex = nullptr;
	mDepthStencilBuffer = nullptr;
	mRenderTargetView = nullptr;

	mDepthStencilState = nullptr;
	mDepthStencilView = nullptr;
	mRasterState = nullptr;

	initializeAPI(window);
	resize(mWidth, mHeight);
	initializeResources();
	tStart = std::chrono::high_resolution_clock::now();
}

Renderer::~Renderer()
{
	if (mSwapchain != nullptr)
	{
		mSwapchain->SetFullscreenState(false, nullptr);
		mSwapchain->Release();
		mSwapchain = nullptr;
	}
	destroyFrameBuffer();
	destroyResources();
	destroyAPI();
}

void Renderer::initializeAPI(xwin::Window& window)
{
	// The renderer needs the window when resizing the swapchain
	mWindow = &window;
	xwin::WindowDesc desc = window.getDesc();
	mWidth = desc.width;
	mHeight = desc.height;

	ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&mFactory)));

	ThrowIfFailed(mFactory->EnumAdapters(0, &mAdapter));

	ThrowIfFailed(mAdapter->EnumOutputs(0, &mAdapterOutput));

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	unsigned int numModes;
	DXGI_MODE_DESC* displayModeList;
	ThrowIfFailed(mAdapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL));

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	displayModeList = new DXGI_MODE_DESC[numModes];

	// Now fill the display mode list structures.
	ThrowIfFailed(mAdapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList));

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	mNumerator = 0;
	mDenominator = 1;
	for (size_t i = 0; i < numModes; i++)
	{
		if (displayModeList[i].Width == mWidth && displayModeList[i].Height == mHeight)
		{
				mNumerator = displayModeList[i].RefreshRate.Numerator;
				mDenominator = displayModeList[i].RefreshRate.Denominator;
				break;
		}
	}

	// Release the display mode list.
	delete[] displayModeList;
	displayModeList = nullptr;

	D3D_FEATURE_LEVEL featureLevelInputs[7] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	D3D_FEATURE_LEVEL featureLevelOutputs = D3D_FEATURE_LEVEL_11_1;

	ThrowIfFailed(D3D11CreateDevice(
		mAdapter,
		D3D_DRIVER_TYPE_UNKNOWN,
		0,
		D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG,
		featureLevelInputs,
		7u,
		D3D11_SDK_VERSION,
		&mDevice,
		&featureLevelOutputs,
		&mDeviceContext
	));

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	ThrowIfFailed(mDevice->QueryInterface(IID_PPV_ARGS(&mDebugController)));
#endif
}

void Renderer::destroyAPI()
{
	mDeviceContext->ClearState();
	mDeviceContext->Flush();

	mDevice->Release();
	mDevice = nullptr;

	mDeviceContext->Release();
	mDeviceContext = nullptr;

	mAdapterOutput->Release();
	mAdapterOutput = nullptr;

	mAdapter->Release();
	mAdapter = nullptr;

	mFactory->Release();
	mFactory = nullptr;

#if defined(_DEBUG)
	// Report on any remaining objects that haven't been deallocated

	D3D11_RLDO_FLAGS rldoFlags = D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL;
	mDebugController->ReportLiveDeviceObjects(rldoFlags);

	mDebugController->Release();
	mDebugController = nullptr;
#endif
}

void Renderer::initializeResources()
{
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
	unsigned int numElements;
	D3D11_BUFFER_DESC uniformBufferDesc;


	// Initialize the pointers this function will use to null.

	ID3DBlob* vertexShader;
	ID3DBlob* pixelShader;
	ID3DBlob* errors;

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	std::string path = "";
	char pBuf[1024];

	_getcwd(pBuf, 1024);
	path = pBuf;
	path += "\\";
	std::wstring wpath = std::wstring(path.begin(), path.end());

	std::wstring vertPath = wpath + L"assets/shaders/triangle.vert.hlsl";
	std::wstring fragPath = wpath + L"assets/shaders/triangle.frag.hlsl";

	try
	{
		ThrowIfFailed(D3DCompileFromFile(vertPath.c_str(), nullptr, nullptr, "main", "vs_5_0", compileFlags, 0, &vertexShader, &errors));
		ThrowIfFailed(D3DCompileFromFile(fragPath.c_str(), nullptr, nullptr, "main", "ps_5_0", compileFlags, 0, &pixelShader, &errors));
	}
	catch (std::exception e)
	{
		const char* errStr = (const char*)errors->GetBufferPointer();
		std::cout << errStr;
	}

	// Create the vertex shader from the buffer.
	ThrowIfFailed(mDevice->CreateVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), NULL, &mVertexShader));

	// Create the pixel shader from the buffer.
	ThrowIfFailed(mDevice->CreatePixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize(), NULL, &mPixelShader));

	// Create the vertex input layout description.
	// This setup needs to match the VertexType stucture in the ModelClass and in the shader.
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "COLOR";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Create the vertex input layout.
	ThrowIfFailed(mDevice->CreateInputLayout(polygonLayout, numElements, vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), &mLayout));

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShader->Release();
	vertexShader = 0;

	pixelShader->Release();
	pixelShader = 0;

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	uniformBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	uniformBufferDesc.ByteWidth = sizeof(uboVS);
	uniformBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	uniformBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	uniformBufferDesc.MiscFlags = 0;
	uniformBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	ThrowIfFailed(mDevice->CreateBuffer(&uniformBufferDesc, NULL, &mUniformBuffer));

	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;

	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * 3;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = mVertexBufferData;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	ThrowIfFailed(mDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &mVertexBuffer));

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned) * 3;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = mIndexBufferData;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	ThrowIfFailed(mDevice->CreateBuffer(&indexBufferDesc, &indexData, &mIndexBuffer));

	// Graphics Pipeline
	D3D11_RASTERIZER_DESC rasterDesc;

	// Setup the raster description which will determine how and what polygons will be drawn.
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	ThrowIfFailed(mDevice->CreateRasterizerState(&rasterDesc, &mRasterState));

	// Now set the rasterizer state.
	mDeviceContext->RSSetState(mRasterState);
}

void Renderer::destroyResources()
{
	if (mVertexBuffer)
	{
		mVertexBuffer->Release();
		mVertexBuffer = nullptr;
	}

	if (mIndexBuffer)
	{
		mIndexBuffer->Release();
		mIndexBuffer = nullptr;
	}

	if (mLayout)
	{
		mLayout->Release();
		mLayout = nullptr;
	}

	if (mUniformBuffer)
	{
		mUniformBuffer->Release();
		mUniformBuffer = nullptr;
	}

	if (mVertexShader)
	{
		mVertexShader->Release();
		mVertexShader = nullptr;
	}

	if (mPixelShader)
	{
		mPixelShader->Release();
		mPixelShader = nullptr;
	}

	if (mRasterState)
	{
		mRasterState->Release();
		mRasterState = nullptr;
	}
}

void Renderer::initFrameBuffer()
{
	xwin::WindowDesc desc = mWindow->getDesc();

	// Get the pointer to the back buffer.
	ThrowIfFailed(mSwapchain->GetBuffer(0, IID_PPV_ARGS(&mBackbufferTex)));

	// Create the render target view with the back buffer pointer.
	ThrowIfFailed(mDevice->CreateRenderTargetView(mBackbufferTex, NULL, &mRenderTargetView));

	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;

	// Initialize the description of the depth buffer.
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = mWidth;
	depthBufferDesc.Height = mHeight;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	// Create the texture for the depth buffer using the filled out description.
	ThrowIfFailed(mDevice->CreateTexture2D(&depthBufferDesc, NULL, &mDepthStencilBuffer));

	// Initialize the description of the stencil state.
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	ThrowIfFailed(mDevice->CreateDepthStencilState(&depthStencilDesc, &mDepthStencilState));

	// Set the depth stencil state.
	mDeviceContext->OMSetDepthStencilState(mDepthStencilState, 1);

	// Initialize the depth stencil view.
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	ThrowIfFailed(mDevice->CreateDepthStencilView(mDepthStencilBuffer, &depthStencilViewDesc, &mDepthStencilView));

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);

	mDeviceContext->RSSetViewports(1, &mViewport);

}

void Renderer::destroyFrameBuffer()
{
	if (mBackbufferTex)
	{
		mBackbufferTex->Release();
		mBackbufferTex = nullptr;
	}

	if (mDepthStencilView)
	{
		mDepthStencilView->Release();
		mDepthStencilView = nullptr;
	}

	if (mDepthStencilState)
	{
		mDepthStencilState->Release();
		mDepthStencilState = nullptr;
	}

	if (mDepthStencilBuffer)
	{
		mDepthStencilBuffer->Release();
		mDepthStencilBuffer = nullptr;
	}

	if (mRenderTargetView)
	{
		mRenderTargetView->Release();
		mRenderTargetView = nullptr;
	}
}

void Renderer::setupSwapchain(unsigned width, unsigned height)
{
	mViewport.TopLeftX = 0.0f;
	mViewport.TopLeftY = 0.0f;
	mViewport.Width = static_cast<float>(mWidth);
	mViewport.Height = static_cast<float>(mHeight);
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	// Update Uniforms
	float zoom = -2.5f;

	// Update matrices
	uboVS.projectionMatrix = Matrix4::perspective(45.0f, static_cast<float>(mWidth) / static_cast<float>(mHeight), 0.01f, 1024.0f);

	uboVS.viewMatrix = Matrix4::translation(Vector3(0.0f, 0.0f, zoom)) * Matrix4::rotationZ(3.14f);

	uboVS.modelMatrix = Matrix4::identity();

	if (mSwapchain != nullptr)
	{
		mSwapchain->SetFullscreenState(false, nullptr);
		mSwapchain->Release();
		mSwapchain = nullptr;
	}

	DXGI_SWAP_CHAIN_DESC swapchainDesc;
	ZeroMemory(&swapchainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapchainDesc.BufferCount = 1;

	// Set the width and height of the back buffer.
	swapchainDesc.BufferDesc.Width = mWidth;
	swapchainDesc.BufferDesc.Height = mHeight;

	// Set regular 32-bit surface for the back buffer.
	swapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.

	if (mVsync)
	{
		swapchainDesc.BufferDesc.RefreshRate.Numerator = mNumerator;
		swapchainDesc.BufferDesc.RefreshRate.Denominator = mDenominator;
	}
	else
	{
		swapchainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Turn multisampling off.
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;

	// Set the scan line ordering and scaling to unspecified.
	swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	swapchainDesc.Flags = 0;

	mSwapchain = xgfx::createSwapchain(mWindow, mFactory, mDevice, &swapchainDesc);
}

void Renderer::resize(unsigned width, unsigned height)
{
	mWidth = clamp(width, 1u, 0xffffu);
	mHeight = clamp(height, 1u, 0xffffu);

	destroyFrameBuffer();
	setupSwapchain(width, height);
	initFrameBuffer();
}

void Renderer::render()
{
	// Framelimit set to 60 fps
	tEnd = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::milli>(tEnd - tStart).count();
	if (time < (1000.0f / 60.0f))
	{
		return;
	}
	tStart = std::chrono::high_resolution_clock::now();

	{
		// Update Uniforms
		mElapsedTime += 0.001f * time;
		mElapsedTime = fmodf(mElapsedTime, 6.283185307179586f);
		uboVS.modelMatrix = Matrix4::rotationY(mElapsedTime);

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		unsigned int bufferNumber;


		// Lock the constant buffer so it can be written to.
		ThrowIfFailed(mDeviceContext->Map(mUniformBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

		// Get a pointer to the data in the constant buffer.
		memcpy(mappedResource.pData, &uboVS, sizeof(uboVS));

		// Unlock the constant buffer.
		mDeviceContext->Unmap(mUniformBuffer, 0);

		// Set the position of the constant buffer in the vertex shader.
		bufferNumber = 0;

		// Finanly set the constant buffer in the vertex shader with the updated values.
		mDeviceContext->VSSetConstantBuffers(bufferNumber, 1, &mUniformBuffer);

	}

	float color[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	mDeviceContext->ClearRenderTargetView(mRenderTargetView, color);

	// Clear the depth buffer.
	mDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Set the vertex input layout.
	mDeviceContext->IASetInputLayout(mLayout);

	// Set the vertex and pixel shaders that will be used to render this assets/shaders/triangle.
	mDeviceContext->VSSetShader(mVertexShader, NULL, 0);
	mDeviceContext->PSSetShader(mPixelShader, NULL, 0);

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	unsigned stride = sizeof(Vertex);
	unsigned offset = 0;
	mDeviceContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	mDeviceContext->IASetIndexBuffer(mIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Render the assets/shaders/triangle.
	mDeviceContext->DrawIndexed(3, 0, 0);

	if (mVsync)
	{
		mSwapchain->Present(1, 0);
	}
	else
	{
		mSwapchain->Present(0, 0);
	}
}

/**
 * While most modern graphics APIs have a command queue, sync, and render passes, DirectX 11 does not.
 * So these functions are just stubs:
 */

void Renderer::createCommands()
{
	// DirectX 11 doesn't have a queue, but rather a context that's set at render time
}

void Renderer::setupCommands()
{
	// DirectX 11 doesn't have commands
}

void Renderer::destroyCommands()
{
	//DirectX 11 doesn't have commands
}

void Renderer::createRenderPass()
{
	// DirectX 11 doesn't have render passes, just framebuffer outputs
}

void Renderer::createSynchronization()
{
	// DirectX 11 doesn't have synchronization primitives
}
