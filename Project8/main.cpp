﻿#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<vector>
#include<string>
#include <cmath>

#include<d3dcompiler.h>
#include<DirectXTex.h>
#include<d3dx12.h>

#ifdef _DEBUG
#include<iostream>
#endif

#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

const char* HResultToString(HRESULT hr) {
	switch (hr) {
	case S_OK: return "S_OK";
	case E_FAIL: return "E_FAIL";
	case E_INVALIDARG: return "E_INVALIDARG";
	case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
	case DXGI_ERROR_DEVICE_REMOVED: return "DXGI_ERROR_DEVICE_REMOVED";
	case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET";
	case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
	case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL";
	default: return "UNKNOWN HRESULT";
	}
}

using namespace std;
using namespace DirectX;

///@brief コンソール画面にフォーマット付き文字列を表示
///@param format フォーマット(%dとか%fとかの)
///@param 可変長引数
///@remarksこの関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}

//面倒だけど書かなあかんやつ
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {//ウィンドウが破棄されたら呼ばれます
		PostQuitMessage(0);//OSに対して「もうこのアプリは終わるんや」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//規定の処理を行う
}

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

IDXGIFactory6* _dxgiFactory = nullptr;
ID3D12Device* _dev = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
IDXGISwapChain4* _swapchain = nullptr;

void EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))) {
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}
}

#ifdef _DEBUG
int main() {
#else
#include<Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	DebugOutputFormatString("Show window test.");
	getchar();

	HINSTANCE hInst = GetModuleHandle(nullptr);
	//ウィンドウクラス生成＆登録
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;//コールバック関数の指定
	w.lpszClassName = _T("DirectXTest");//アプリケーションクラス名(適当でいいです)
	w.hInstance = GetModuleHandle(0);//ハンドルの取得
	RegisterClassEx(&w);//アプリケーションクラス(こういうの作るからよろしくってOSに予告する)

	RECT wrc = { 0,0, window_width, window_height };//ウィンドウサイズを決める
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//ウィンドウのサイズはちょっと面倒なので関数を使って補正する
	//ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(w.lpszClassName,//クラス名指定
		_T("DX12テスト"),//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//タイトルバーと境界線があるウィンドウです
		CW_USEDEFAULT,//表示X座標はOSにお任せします
		CW_USEDEFAULT,//表示Y座標はOSにお任せします
		wrc.right - wrc.left,//ウィンドウ幅
		wrc.bottom - wrc.top,//ウィンドウ高
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		w.hInstance,//呼び出しアプリケーションハンドル
		nullptr);//追加パラメータ

#ifdef _DEBUG
	//デバッグレイヤーをオンに
	EnableDebugLayer();
#endif
	//DirectX12まわり初期化
	//フィーチャレベル列挙
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};


	//複数のグラボがある場合
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
	std::vector <IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapters.push_back(tmpAdapter);
	}
	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}





	//Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto l : levels) {
		if (D3D12CreateDevice(tmpAdapter, l, IID_PPV_ARGS(&_dev)) == S_OK) {
			featureLevel = l;
			break;
		}
	}


	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	printf("結果1: %s (0x%08X)\n", HResultToString(result), result);
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
	printf("結果2: %s (0x%08X)\n", HResultToString(result), result);
	_cmdList->Close();
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//タイムアウトなし
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//プライオリティ特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//ここはコマンドリストと合わせてください
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));//コマンドキュー生成

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


	result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapchain);
	DebugOutputFormatString("_swapchain=%p\n", _swapchain);
	printf("結果3: %s (0x%08X)\n", HResultToString(result), result);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビューなので当然RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;//表裏の２つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//特に指定なし
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
	printf("結果4: %s (0x%08X)\n", HResultToString(result), result);
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	printf("結果5: %s (0x%08X)\n", HResultToString(result), result);
	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	//SRGBレンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;


	for (size_t i = 0; i < swcDesc.BufferCount; ++i) {
		result = _swapchain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&_backBuffers[i]));
		_dev->CreateRenderTargetView(_backBuffers[i], &rtvDesc, handle);
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	printf("結果6: %s (0x%08X)\n", HResultToString(result), result);

	ShowWindow(hwnd, SW_SHOW);//ウィンドウ表示

	struct Vertex {
		XMFLOAT3 pos;//XYZ座標
		XMFLOAT2 uv;//UV座標
	};


	struct ConstBufferData {
		XMMATRIX mat;
		XMFLOAT4 color;
	};

	//四角
	std::vector<Vertex> vertices2 = {
		{{-0.5f,-0.9f,0.0f},{0.0f,1.0f} },//左下
		{{-0.5f,0.9f,0.0f} ,{0.0f,0.0f}},//左上
		{{0.5f,-0.9f,0.0f} ,{1.0f,1.0f}},//右下
		{{0.5f,0.9f,0.0f} ,{1.0f,0.0f}},//右上
	};
	//インデックス
	std::vector<unsigned short> indices2 =
	{
			0, 1, 2, // 左下、左上、右下
			2, 1, 3, // 右下、左上、右上
	};


	std::vector<Vertex> vertices;
	// 円の中心
	vertices.push_back({ {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
	float aspectRatio = static_cast<float>(window_width) / window_height;//アスペクト比
	//円周
	const int kSlice = 64;//分割数
	for (int i = 0; i <= kSlice; ++i) 
	{
		float angle = XM_2PI* i / kSlice;
		float x = cosf(angle) * 1.0f;
		float y = sinf(angle) * 1.0f;
		x /= aspectRatio;
		y /= aspectRatio;
		vertices.push_back({ {x, y, 0.0f}, {x + 1.0f, -(y + 1.0f)} });

	}


	//インデックスの作成
	std::vector<unsigned short> indices;
	//中心から扇状に結ぶ
	for (int i = 0; i < kSlice; ++i)
	{
		indices.push_back(0); //中心
		indices.push_back(i + 1);//今の頂点
		indices.push_back(i + 2);//次の頂点

	}


	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices);
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;


	//UPLOAD(確保は可能)
	ID3D12Resource* vertBuff = nullptr;
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	printf("結果7: %s (0x%08X)\n", HResultToString(result), result);

	Vertex* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	printf("結果8: %s (0x%08X)\n", HResultToString(result), result);

	std::copy(std::begin(vertices), std::end(vertices), vertMap);

	vertBuff->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//バッファの仮想アドレス
	vbView.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * vertices.size());
	vbView.StrideInBytes = sizeof(Vertex);
	//vbView.SizeInBytes = sizeof(vertices);//全バイト数
	//vbView.StrideInBytes = sizeof(vertices[0]);//1頂点あたりのバイト数

	//unsigned short indices[] = { 0,1,2, 2,1,3 };


	//ここを変えれば大きさ、場所、見え方が変わる

	XMMATRIX matrix = XMMatrixIdentity();//行優先...数学でいう4列目はXMMATRIXで4列目
	XMMATRIX matrix2 = XMMatrixIdentity();//行優先...数学でいう4列目はXMMATRIXで4列目

	//auto Worldmatrix = XMMatrixRotationY(XM_PIDIV4);//XM_PIを4で割る...45度


	//4列目派兵行為移動、それ以外の列は拡大,縮小,回転を表す
	float posX = 0;
	float posY = 0;
	matrix.r[0].m128_f32[0] = 50.0f / window_width;
	matrix.r[1].m128_f32[1] = -50.0f / window_height;
	matrix.r[3].m128_f32[0] = 0;//x軸
	matrix.r[3].m128_f32[1] = -0.5f;//y軸


	//四角の描画

	matrix2.r[0].m128_f32[0] = 300.0f / window_width;
	matrix2.r[1].m128_f32[1] = -20.0f / window_height;
	matrix2.r[3].m128_f32[0] = 0.0f;//x軸
	matrix2.r[3].m128_f32[1] = -0.7f;//y軸


	float rectLeft = -0.5f; // 四角左端
	float rectRight = 0.5f;  // 四角右端
	float rectTop = 0.9f;  // 四角上端
	float rectBottom = -0.9f; // 四角下端

	float scaleX = matrix2.r[0].m128_f32[0]; // 横スケール
	float scaleY = matrix2.r[1].m128_f32[1]; // 縦スケール
	float offsetX = matrix2.r[3].m128_f32[0]; // 中心Xオフセット
	float offsetY = matrix2.r[3].m128_f32[1]; // 中心Yオフセット



	//ワールド行列

	////ビュー行列
	//XMFLOAT3 eye(0, 0, -5);//視点
	//XMFLOAT3 target(0, 0, 0);//注視点
	//XMFLOAT3 up(0, 1, 0);//上ベクトル

	//auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));


	////プロジェクション行列
	//auto projMat = XMMatrixPerspectiveFovLH(
	//	XM_PIDIV2,//画角は90
	//	static_cast<float>(window_width) / static_cast<float>(window_height),//アスペクト比
	//	1.0f,//近いほう
	//	10.0f//遠いほう
	//);




	//頂点バッファの作成(四角)
	ID3D12Resource* vertBuff2 = nullptr;
	resdesc.Width = sizeof(Vertex) * vertices2.size();
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff2)
	);

	printf("結果8.5: %s (0x%08X)\n", HResultToString(result), result);
	//頂点バッファーにデータ転送
	Vertex* vertMap2 = nullptr;
	vertBuff2->Map(0, nullptr, (void**)&vertMap2);
	std::copy(vertices2.begin(), vertices2.end(), vertMap2);
	vertBuff2->Unmap(0, nullptr);

	//頂点バッファービュー作成
	D3D12_VERTEX_BUFFER_VIEW vbView2 = {};
	vbView2.BufferLocation = vertBuff2->GetGPUVirtualAddress();
	vbView2.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * vertices2.size());
	vbView2.StrideInBytes = sizeof(Vertex);

	//インデックスバッファ作成
	ID3D12Resource* idxBuff2 = nullptr;
	resdesc.Width = sizeof(unsigned short) * indices2.size();
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff2)
	);
	printf("結果8.5_2: %s (0x%08X)\n", HResultToString(result), result);

	// インデックスバッファにデータ転送
	unsigned short* mappedIdx2 = nullptr;
	idxBuff2->Map(0, nullptr, (void**)&mappedIdx2);
	std::copy(indices2.begin(), indices2.end(), mappedIdx2);
	idxBuff2->Unmap(0, nullptr);

	// インデックスバッファビュー作成
	D3D12_INDEX_BUFFER_VIEW ibView2 = {};
	ibView2.BufferLocation = idxBuff2->GetGPUVirtualAddress();
	ibView2.Format = DXGI_FORMAT_R16_UINT;
	ibView2.SizeInBytes = static_cast<UINT>(sizeof(unsigned short) * indices2.size());





	//XMMATRIX WorldmatrixProj = Worldmatrix * viewMat * projMat;
	//定数バッファの作成
	ID3D12Resource* constBuff = nullptr;
	ID3D12Resource* constBuff2 = nullptr;


	CD3DX12_HEAP_PROPERTIES HeapPro(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(XMMATRIX) + 0xff) & ~0xff);


	_dev->CreateCommittedResource(
		&HeapPro,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);

	ConstBufferData* mapMatrix;//マップ先を示すポインタ
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix);//マップ
	printf("結果9: %s (0x%08X)\n", HResultToString(result), result);
	mapMatrix->mat = matrix;//行列の内容をコピー
	mapMatrix->color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	constBuff->Unmap(0, nullptr); // ←これが必須

	_dev->CreateCommittedResource(
		&HeapPro,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff2)
	);

	//四角用
	ConstBufferData* mapMatrix2;//マップ先を示すポインタ
	result = constBuff2->Map(0, nullptr, (void**)&mapMatrix2);//マップ
	printf("結果9: %s (0x%08X)\n", HResultToString(result), result);
	mapMatrix2->mat = matrix2;//行列の内容をコピー
	mapMatrix2->color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	constBuff2->Unmap(0, nullptr); // ←これが必須

	ID3D12Resource* idxBuff = nullptr;


	//設定は、バッファのサイズ以外頂点バッファの設定を使いまわして
	//OKだと思います。
	resdesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff));

	printf("結果10: %s (0x%08X)\n", HResultToString(result), result);

	//作ったバッファにインデックスデータをコピー
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	//インデックスバッファビューを作成
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = static_cast<UINT>(sizeof(unsigned short) * indices.size()); //インデックスが全部で何バイトあるのかだから掛け算

	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;

	ID3DBlob* errorBlob = nullptr;
	result = D3DCompileFromFile(L"BasicVS.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_vsBlob, &errorBlob);

	printf("結果11: %s (0x%08X)\n", HResultToString(result), result);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);//行儀悪いかな…
	}
	result = D3DCompileFromFile(L"BasicPS.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_psBlob, &errorBlob);
	printf("結果12: %s (0x%08X)\n", HResultToString(result), result);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);
	}
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//中身は0xffffffff

	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};


	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	renderTargetBlendDesc.LogicOpEnable = false;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	gpipeline.RasterizerState.MultisampleEnable = false;//まだアンチェリは使わない
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true;//深度方向のクリッピングは有効に


	gpipeline.RasterizerState.FrontCounterClockwise = false;
	gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline.RasterizerState.AntialiasedLineEnable = false;
	gpipeline.RasterizerState.ForcedSampleCount = 0;
	gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;


	gpipeline.DepthStencilState.DepthEnable = false;
	gpipeline.DepthStencilState.StencilEnable = false;

	gpipeline.InputLayout.pInputElementDescs = inputLayout;//レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	gpipeline.NumRenderTargets = 1;//今は１つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

	gpipeline.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	gpipeline.SampleDesc.Quality = 0;//クオリティは最低

	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descTblRange[2] = {};
	descTblRange[0].NumDescriptors = 1;//テクスチャひとつ
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//種別はテクスチャ
	descTblRange[0].BaseShaderRegister = 0;//0番スロットから
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTblRange[1].NumDescriptors = 1;
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//種別は定数
	descTblRange[1].BaseShaderRegister = 0;//0番目
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootparam = {};
	rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam.DescriptorTable.pDescriptorRanges = descTblRange;
	rootparam.DescriptorTable.NumDescriptorRanges = 2;
	rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootSignatureDesc.pParameters = &rootparam;//ルートパラメータの先頭アドレス
	rootSignatureDesc.NumParameters = 1;//ルートパラメータ数

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//横繰り返し
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//縦繰り返し
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//奥行繰り返し
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//ボーダーの時は黒
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;//補間しない(ニアレストネイバー)
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;//ミップマップ最大値
	samplerDesc.MinLOD = 0.0f;//ミップマップ最小値
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//オーバーサンプリングの際リサンプリングしない？
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//ピクセルシェーダからのみ可視
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;

	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.NumStaticSamplers = 0;

	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	printf("結果13: %s (0x%08X)\n", HResultToString(result), result);
	result = _dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
	printf("結果14: %s (0x%08X)\n", HResultToString(result), result);
	rootSigBlob->Release();

	gpipeline.pRootSignature = rootsignature;
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));
	printf("結果15: %s (0x%08X)\n", HResultToString(result), result);

	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;//出力先の幅(ピクセル数)
	viewport.Height = window_height;//出力先の高さ(ピクセル数)
	viewport.TopLeftX = 0;//出力先の左上座標X
	viewport.TopLeftY = 0;//出力先の左上座標Y
	viewport.MaxDepth = 1.0f;//深度最大値
	viewport.MinDepth = 0.0f;//深度最小値


	D3D12_RECT scissorrect = {};
	scissorrect.top = 0;//切り抜き上座標
	scissorrect.left = 0;//切り抜き左座標
	scissorrect.right = scissorrect.left + window_width;//切り抜き右座標
	scissorrect.bottom = scissorrect.top + window_height;//切り抜き下座標


	//WICテクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	result = LoadFromWICFile(L"Img/testtext.png", WIC_FLAGS_NONE, &metadata, scratchImg);
	printf("結果16: %s (0x%08X)\n", HResultToString(result), result);
	auto img = scratchImg.GetImage(0, 0, 0);//生データ抽出

	//WriteToSubresourceで転送する用のヒープ設定
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//特殊な設定なのでdefaultでもuploadでもなく
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//ライトバックで
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//転送がL0つまりCPU側から直で
	texHeapProp.CreationNodeMask = 0;//単一アダプタのため0
	texHeapProp.VisibleNodeMask = 0;//単一アダプタのため0

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = metadata.format;//DXGI_FORMAT_R8G8B8A8_UNORM;//RGBAフォーマット
	resDesc.Width = static_cast<UINT>(metadata.width);//幅
	resDesc.Height = static_cast<UINT>(metadata.height);//高さ
	resDesc.DepthOrArraySize = static_cast<uint16_t>(metadata.arraySize);//2Dで配列でもないので１
	resDesc.SampleDesc.Count = 1;//通常テクスチャなのでアンチェリしない
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);//ミップマップしないのでミップ数は１つ
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);//2Dテクスチャ用
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトについては決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//とくにフラグなし

	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,//テクスチャ用(ピクセルシェーダから見る用)
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);
	printf("結果17: %s (0x%08X)\n", HResultToString(result), result);
	result = texbuff->WriteToSubresource(0,
		nullptr,//全領域へコピー
		img->pixels,//元データアドレス
		static_cast<UINT>(img->rowPitch),//1ラインサイズ
		static_cast<UINT>(img->slicePitch)//全サイズ
	);
	printf("結果18: %s (0x%08X)\n", HResultToString(result), result);
	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//シェーダから見えるように
	descHeapDesc.NodeMask = 0;//マスクは0
	descHeapDesc.NumDescriptors = 3;//SRVとCBVの2つ
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//シェーダリソースビュー(および定数、UAVも)
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));//生成

	//通常テクスチャビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;//DXGI_FORMAT_R8G8B8A8_UNORM;//RGBA(0.0f～1.0fに正規化)
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//後述
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;//ミップマップは使用しないので1

	_dev->CreateShaderResourceView(texbuff, //ビューと関連付けるバッファ
		&srvDesc, //先ほど設定したテクスチャ設定情報
		basicDescHeap->GetCPUDescriptorHandleForHeapStart()//ヒープのどこに割り当てるか
	);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (sizeof(XMMATRIX) + 0xff) & ~0xff;

	auto cbvHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
	cbvHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	_dev->CreateConstantBufferView(&cbvDesc, cbvHandle);

	//四角用
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc2 = {};
	cbvDesc2.BufferLocation = constBuff2->GetGPUVirtualAddress();
	cbvDesc2.SizeInBytes = (sizeof(XMMATRIX) + 0xff) & ~0xff;

	cbvHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	_dev->CreateConstantBufferView(&cbvDesc2, cbvHandle);



	//定数バッファーびゅの作成
	void CreateConstantBufferView(
		const D3D12_CONSTANT_BUFFER_VIEW_DESC * pDesc,//定数バッファ―ビューの作成 
		CD3DX12_CPU_DESCRIPTOR_HANDLE DestDescriptor//ビューを配置すべき
	);

	MSG msg = {};
	unsigned int frame = 0;
	float angle = 0.0f;
	float transX = 0.01f;
	float transY = 0.010f;
	float playerX = 0.0f;
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) {
			break;
		}

		// フェンスで前フレーム完了を待つ
		_cmdQueue->Signal(_fence, ++_fenceVal);
		if (_fence->GetCompletedValue() != _fenceVal) {
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
		//angle += 0.1f;
		//Worldmatrix = XMMatrixRotationY(angle);
		//*mapMatrix = Worldmatrix * viewMat * projMat;


		if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
			playerX -= 0.01f;
		}
		if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
			playerX += 0.01f;
		}


		if (playerX >= 0.9) 
		{
			playerX = 0.9;
		}
		else if (playerX <= -0.9)
		{
			playerX =  -0.9;
		}
		matrix2.r[3].m128_f32[0] = playerX;
		mapMatrix2->mat = matrix2;


		offsetX = matrix2.r[3].m128_f32[0]; // 中心Xオフセット
		

		//円の移動
		if (posX >= 1.0 || posX <= -1.0)
		{
			transX = -transX;
		}

		if (posY >= 1.0)
		{

			float randomY = (rand() % 50) / 10000.0f;
			transY = -0.01 - randomY;
		}
		else if (posY <= -1.0)
		{
			float randomY = (rand() % 50) / 10000.0f;
			transY = 0.01 + randomY;
		}

		posX += transX;
		matrix.r[3].m128_f32[0] = posX;

		posY += transY;
		matrix.r[3].m128_f32[1] = posY;


		mapMatrix->mat =  matrix;

		//四角の衝突判定
		// 四角の範囲を決める（X,Y方向の最小最大）

		// それに合わせて四角の範囲を再計算
		// 四角形の基準座標を保持しておく
		const float baseRectLeft = -0.5f;
		const float baseRectRight = 0.5f;
		const float baseRectTop = 0.9f;
		const float baseRectBottom = -0.9f;
		// 四角形の現在の範囲を計算
		float rectLeft = offsetX + scaleX * baseRectLeft;
		float rectRight = offsetX + scaleX * baseRectRight;
		float rectTop = offsetY + scaleY * baseRectTop;
		float rectBottom = offsetY + scaleY * baseRectBottom;


		const float circleRadius = 0.05f; // 円の半径（小さめに設定）
		// 円の中心 (posX, posY)
		// 四角 (rectLeft, rectRight, rectTop, rectBottom)

		if (posX + circleRadius >= rectLeft && posX - circleRadius <= rectRight &&
			posY + circleRadius >= rectBottom && posY - circleRadius <= rectTop) {
			// ★当たった時の処理
			transX = -transX;
			transY = -transY;
		}



		// コマンドリスト準備
		_cmdAllocator->Reset();
		_cmdList->Reset(_cmdAllocator, _pipelinestate);

		// ルートシグネチャ・ヒープ設定
		_cmdList->SetGraphicsRootSignature(rootsignature);
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);

		// SRVとCBVをそれぞれバインド
		auto srvHandle = basicDescHeap->GetGPUDescriptorHandleForHeapStart();
		//_cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);

		auto cbvHandle = basicDescHeap->GetGPUDescriptorHandleForHeapStart();
		//cbvHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		_cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);

		// バックバッファインデックス取得
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		// 画面切り替え用バリア（PRESENT→RENDER_TARGET）
		{
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = _backBuffers[bbIdx];
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			_cmdList->ResourceBarrier(1, &barrier);
		}

		// レンダーターゲット設定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvH, false, nullptr);

		// 画面クリア
		float r = (float)(0xff & frame >> 16) / 255.0f;
		float g = (float)(0xff & frame >> 8) / 255.0f;
		float b = (float)(0xff & frame >> 0) / 255.0f;
		float clearColor[] = { r, g, b, 1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		++frame;

		// ビューポート・シザー矩形設定
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);

		// 描画設定
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);

		// 描画
		_cmdList->DrawIndexedInstanced(indices.size(), 1, 0, 0, 0);

		//四角の描画
		cbvHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // ★定数バッファを「四角用」に切り替える
		_cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
		_cmdList->IASetVertexBuffers(0, 1, &vbView2);
		_cmdList->IASetIndexBuffer(&ibView2);
		_cmdList->DrawIndexedInstanced(indices2.size(), 1, 0, 0, 0);
		// バリア（RENDER_TARGET→PRESENT）
		{
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = _backBuffers[bbIdx];
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			_cmdList->ResourceBarrier(1, &barrier);
		}

		// コマンドリストを閉じる
		_cmdList->Close();

		// 実行
		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);

		// フリップ
		_swapchain->Present(1, 0);
	}
	//もうクラス使わんから登録解除してや
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
}