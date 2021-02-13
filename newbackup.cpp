//new w2s finder concept, more fps (backup test code)
#include "includes.h"
#include "imgui_memory_editor.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


typedef HRESULT(__stdcall* ResizeBuffers) (IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
ResizeBuffers oResizeBuffers;

typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
Present oPresent;

typedef void(__stdcall* DrawIndexed) (ID3D11DeviceContext* pContextD3D11, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
DrawIndexed oDrawIndexed;


HWND window = NULL;
WNDPROC oWndProc;

ID3D11Device* pDeviceD3D11 = NULL;
ID3D11DeviceContext* pContextD3D11 = NULL;
ID3D11RenderTargetView* mainRenderTargetViewD3D11;


//globals
bool showmenu = false;
bool initonce = false;

//item states
bool wallhack = 1;
bool chams = 1;
bool esp = 1;
bool aimbot = 1;
int aimkey = 1;
int aimsens = 1;
bool modelrecfinder = 1;
int countStride = 0;
int countIndexCount = 0;
int countveWidth = 0;
bool wtsfinder = 1;


//wh
ID3D11DepthStencilState* DepthStencilState_TRUE = NULL; //depth off
ID3D11DepthStencilState* DepthStencilState_FALSE = NULL; //depth off
ID3D11DepthStencilState* DepthStencilState_ORIG = NULL; //depth on

//shader
ID3D11PixelShader* sGreen = NULL;
ID3D11PixelShader* sMagenta = NULL;

//Viewport
float ViewportWidth;
float ViewportHeight;

int countnum = -1;

//==========================================================================================================================

//get dir
using namespace std;
#include <fstream>

// getdir & log
char dir[320];
char* GetDirFile(char* name)
{
	static char pldir[320];
	strcpy_s(pldir, dir);
	strcat_s(pldir, name);
	return pldir;
}

//log
void Log(const char* fmt, ...)
{
	if (!fmt)	return;

	char		text[4096];
	va_list		ap;
	va_start(ap, fmt);
	vsprintf_s(text, fmt, ap);
	va_end(ap);

	ofstream logfile((PCHAR)"log.txt", ios::app);
	if (logfile.is_open() && text)	logfile << text << endl;
	logfile.close();
}


#include <D3Dcompiler.h> //generateshader
#pragma comment(lib, "D3dcompiler.lib")
//generate shader func
HRESULT GenerateShader(ID3D11Device* pDeviceD3D11, ID3D11PixelShader** pShader, float r, float g, float b)
{
	char szCast[] = "struct VS_OUT"
		"{"
		" float4 Position : SV_Position;"
		" float4 Color : COLOR0;"
		"};"

		"float4 main( VS_OUT input ) : SV_Target"
		"{"
		" float4 col;"
		" col.a = 1.0f;"
		" col.r = %f;"
		" col.g = %f;"
		" col.b = %f;"
		" return col;"
		"}";

	ID3D10Blob* pBlob;
	char szPixelShader[1000];

	sprintf_s(szPixelShader, szCast, r, g, b);

	ID3DBlob* error;

	HRESULT hr = D3DCompile(szPixelShader, sizeof(szPixelShader), "shader", NULL, NULL, "main", "ps_4_0", NULL, NULL, &pBlob, &error);

	if (FAILED(hr))
		return hr;

	hr = pDeviceD3D11->CreatePixelShader((DWORD*)pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, pShader);

	if (FAILED(hr))
		return hr;

	return S_OK;
}

ID3D11Texture2D* texc = nullptr;
ID3D11ShaderResourceView* textureColor;
void GenerateTexture(uint32_t pixelcolor, DXGI_FORMAT format)//DXGI_FORMAT_R32G32B32A32_FLOAT DXGI_FORMAT_R8G8B8A8_UNORM
{
	//static const uint32_t pixelcolor = 0xff00ff00; //0xff00ff00 green, 0xffff0000 blue, 0xff0000ff red
	D3D11_SUBRESOURCE_DATA initData = { &pixelcolor, sizeof(uint32_t), 0 };

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = 1;
	desc.Height = 1;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	pDeviceD3D11->CreateTexture2D(&desc, &initData, &texc);

	D3D11_SHADER_RESOURCE_VIEW_DESC srdes;
	memset(&srdes, 0, sizeof(srdes));
	srdes.Format = format;
	srdes.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srdes.Texture2D.MostDetailedMip = 0;
	srdes.Texture2D.MipLevels = 1;
	pDeviceD3D11->CreateShaderResourceView(texc, &srdes, &textureColor);
}

//==========================================================================================================================

//w2s stuff
struct vec2
{
	float x, y;
};

struct vec3
{
	float x, y, z;
};

struct vec4
{
	float x, y, z, w;
};

static vec4 Vec4MulMat4x4(const vec4& v, float(*mat4x4)[4])
{
	vec4 o;
	
	o.x = v.x * mat4x4[0][0] + v.y * mat4x4[1][0] + v.z * mat4x4[2][0] + v.w * mat4x4[3][0];
	o.y = v.x * mat4x4[0][1] + v.y * mat4x4[1][1] + v.z * mat4x4[2][1] + v.w * mat4x4[3][1];
	o.z = v.x * mat4x4[0][2] + v.y * mat4x4[1][2] + v.z * mat4x4[2][2] + v.w * mat4x4[3][2];
	o.w = v.x * mat4x4[0][3] + v.y * mat4x4[1][3] + v.z * mat4x4[2][3] + v.w * mat4x4[3][3];
	
	return o;
}

static vec4 Vec3MulMat4x4(const vec3& v, float(*mat4x4)[4])
{
	vec4 o;
	
	o.x = v.x * mat4x4[0][0] + v.y * mat4x4[1][0] + v.z * mat4x4[2][0] + mat4x4[3][0];
	o.y = v.x * mat4x4[0][1] + v.y * mat4x4[1][1] + v.z * mat4x4[2][1] + mat4x4[3][1];
	o.z = v.x * mat4x4[0][2] + v.y * mat4x4[1][2] + v.z * mat4x4[2][2] + mat4x4[3][2];
	o.w = v.x * mat4x4[0][3] + v.y * mat4x4[1][3] + v.z * mat4x4[2][3] + mat4x4[3][3];
	
	return o;
}

static vec3 Vec3MulMat4x3(const vec3& v, float(*mat4x3)[3])
{
	vec3 o;
	o.x = v.x * mat4x3[0][0] + v.y * mat4x3[1][0] + v.z * mat4x3[2][0] + mat4x3[3][0];
	o.y = v.x * mat4x3[0][1] + v.y * mat4x3[1][1] + v.z * mat4x3[2][1] + mat4x3[3][1];
	o.z = v.x * mat4x3[0][2] + v.y * mat4x3[1][2] + v.z * mat4x3[2][2] + mat4x3[3][2];
	return o;
}

void MapBuffer(ID3D11Buffer* pStageBuffer, void** ppData, UINT* pByteWidth)
{
	D3D11_MAPPED_SUBRESOURCE subRes;
	HRESULT res = pContextD3D11->Map(pStageBuffer, 0, D3D11_MAP_READ, 0, &subRes);

	D3D11_BUFFER_DESC desc;
	pStageBuffer->GetDesc(&desc);

	if (FAILED(res))
	{
		Log("Map stage buffer failed {%d} {%d} {%d} {%d} {%d}", (void*)pStageBuffer, desc.ByteWidth, desc.BindFlags, desc.CPUAccessFlags, desc.Usage);
		SAFE_RELEASE(pStageBuffer);
		return;
	}

	*ppData = subRes.pData;

	if (pByteWidth)
		*pByteWidth = desc.ByteWidth;
}

void UnmapBuffer(ID3D11Buffer* pStageBuffer)
{
	pContextD3D11->Unmap(pStageBuffer, 0);
}

#include <unordered_set>
//Used for capturing processed buffers
//So we don't process the same buffers over and over
std::unordered_set<ID3D11Buffer*> seenParams;

#include <map>
using namespace std;
ID3D11Buffer* pStageBufferA = NULL;
ID3D11Buffer* CopyBufferToCpuA(ID3D11Buffer* pBufferA)
{
	D3D11_BUFFER_DESC CBDescA;
	pBufferA->GetDesc(&CBDescA);

	if (pStageBufferA == NULL)
	{
		//Log("onceA");
		// create shadow buffer
		D3D11_BUFFER_DESC descA;
		descA.BindFlags = 0;
		descA.ByteWidth = CBDescA.ByteWidth;
		descA.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		descA.MiscFlags = 0;
		descA.StructureByteStride = 0;
		descA.Usage = D3D11_USAGE_STAGING;

		if (FAILED(pDeviceD3D11->CreateBuffer(&descA, NULL, &pStageBufferA)))
		{
			Log("CreateBuffer failed when CopyBufferToCpuA {}");
		}
	}
	
	if (pStageBufferA != NULL)
		pContextD3D11->CopyResource(pStageBufferA, pBufferA);

	return pStageBufferA;
}

ID3D11Buffer* pStageBufferB = NULL;
ID3D11Buffer* CopyBufferToCpuB(ID3D11Buffer* pBufferB)
{
	D3D11_BUFFER_DESC CBDescB;
	pBufferB->GetDesc(&CBDescB);

	if (pStageBufferB == NULL)
	{
		//Log("onceB");
		// create shadow buffer
		D3D11_BUFFER_DESC descB;
		descB.BindFlags = 0;
		descB.ByteWidth = CBDescB.ByteWidth;
		descB.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		descB.MiscFlags = 0;
		descB.StructureByteStride = 0;
		descB.Usage = D3D11_USAGE_STAGING;

		if (FAILED(pDeviceD3D11->CreateBuffer(&descB, NULL, &pStageBufferB)))
		{
			Log("CreateBuffer failed when CopyBufferToCpuB {}");
		}
	}

	if (pStageBufferB != NULL)
		pContextD3D11->CopyResource(pStageBufferB, pBufferB);

	return pStageBufferB;
}

//get distance
float GetDst(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

struct AimEspInfo_t
{
	float vOutX, vOutY, vOutZ;
	float CrosshairDst;
};
std::vector<AimEspInfo_t>AimEspInfo;


//w2s
int WorldViewCBnum = 0; //2
int ProjCBnum = 0;//1
int matProjnum = 0;//16

ID3D11Buffer* pWorldViewCB = nullptr;
ID3D11Buffer* pProjCB = nullptr;

ID3D11Buffer* m_pCurWorldViewCB = NULL;
ID3D11Buffer* m_pCurProjCB = NULL;

float matWorldView[4][4];
float matProj[4][4];

float* worldview;
float* proj;

void AddModel(ID3D11DeviceContext* pContextD3D11)
{
	pContextD3D11->VSGetConstantBuffers(WorldViewCBnum, 1, &pWorldViewCB);	//WorldViewCBnum
	pContextD3D11->VSGetConstantBuffers(ProjCBnum, 1, &pProjCB);			//ProjCBnum

	if (pWorldViewCB == nullptr)
	{
		SAFE_RELEASE(pWorldViewCB);
	}
	if (pProjCB == nullptr)
	{
		SAFE_RELEASE(pProjCB);
	}
	

	if (pWorldViewCB != nullptr && pProjCB != nullptr)
	{
		m_pCurWorldViewCB = CopyBufferToCpuA(pWorldViewCB);
		m_pCurProjCB = CopyBufferToCpuB(pProjCB);
	}


	if (m_pCurWorldViewCB == nullptr)
	{
		SAFE_RELEASE(m_pCurWorldViewCB);
	}
	if (m_pCurProjCB == nullptr)
	{
		SAFE_RELEASE(m_pCurProjCB);
	}


	if (m_pCurWorldViewCB != nullptr && m_pCurProjCB != nullptr)
	{
		MapBuffer(m_pCurWorldViewCB, (void**)&worldview, NULL);
		memcpy(matWorldView, &worldview[0], sizeof(matWorldView));
		UnmapBuffer(m_pCurWorldViewCB);

		MapBuffer(m_pCurProjCB, (void**)&proj, NULL);
		memcpy(matProj, &proj[matProjnum], sizeof(matProj));			//matProjnum
		UnmapBuffer(m_pCurProjCB);
	}

	
	//======================
	//w2s 1 (ut4 engine)
	DirectX::XMVECTOR Pos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);//preaim, aimheight

	DirectX::XMFLOAT4 pOut2d;
	DirectX::XMMATRIX pWorld = DirectX::XMMatrixIdentity();

	//transpose or not, depends on the game
	//DirectX::XMMATRIX pWorldView = DirectX::XMMatrixTranspose((FXMMATRIX)*matWorldView); //transpose
	//DirectX::XMMATRIX pProj = DirectX::XMMatrixTranspose((FXMMATRIX)*matProj); //transpose
	//DirectX::XMVECTOR pOut = DirectX::XMVector3Project(Pos, 0, 0, ViewportWidth, ViewportHeight, 0, 1, pProj, pWorldView, pWorld); //transposed

	//distance
	DirectX::XMMATRIX pWorldViewProj = (FXMMATRIX)*matWorldView * (FXMMATRIX)*matProj;
	float w = Pos.m128_f32[0] * pWorldViewProj.r[0].m128_f32[3] + Pos.m128_f32[1] * pWorldViewProj.r[1].m128_f32[3] + Pos.m128_f32[2] * pWorldViewProj.r[2].m128_f32[3] + pWorldViewProj.r[3].m128_f32[3];

	//normal
	DirectX::XMVECTOR pOut = DirectX::XMVector3Project(Pos, 0, 0, ViewportWidth, ViewportHeight, 0, 1, (FXMMATRIX)*matProj, (FXMMATRIX)*matWorldView, pWorld); //normal

	DirectX::XMStoreFloat4(&pOut2d, pOut);

	vec2 o;
	//if (pOut2d.z < 1.0f)
	//{
		o.x = pOut2d.x;
		o.y = pOut2d.y; //or -pOut2d.y;
	//}
	//AimEspInfo_t pAimEspInfo = { static_cast<float>(pOut.m128_f32[0]), static_cast<float>(pOut.m128_f32[1]) };
	AimEspInfo_t pAimEspInfo = { static_cast<float>(o.x), static_cast<float>(o.y), static_cast<float>(w * 0.1f) };
	AimEspInfo.push_back(pAimEspInfo);
	//======================
	
	/*
	//same
	vec3 v;
	vec4 vWorldView = Vec3MulMat4x4(v, matWorldView);
	vec4 vClip = Vec4MulMat4x4(vWorldView, matProj);

	vec2 o;
	o.x = vClip.x / vClip.w * (ViewportWidth * 0.5f) + (ViewportWidth * 0.5f);
	o.y = 1.0f + (vClip.y / vClip.w * (ViewportHeight * 0.5f) + (ViewportHeight * 0.5f)); //or -

	AimEspInfo_t pAimEspInfo = { static_cast<float>(o.x), static_cast<float>(o.y) };
	AimEspInfo.push_back(pAimEspInfo);
	*/
}


void InitImGuiD3D11()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	
	ImGui::StyleColorsClassic();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDeviceD3D11, pContextD3D11);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
		return true;
	}
	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

HRESULT __stdcall hkResizeBuffers_D3D11(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	ImGui_ImplDX11_InvalidateDeviceObjects();
	if (nullptr != mainRenderTargetViewD3D11) { mainRenderTargetViewD3D11->Release(); mainRenderTargetViewD3D11 = nullptr; }

	HRESULT toReturn = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	ImGui_ImplDX11_CreateDeviceObjects();

	return toReturn;
}

HRESULT __stdcall hkPresent_D3D11(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!initonce)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)& pDeviceD3D11)))
		{
			pDeviceD3D11->GetImmediateContext(&pContextD3D11);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)& pBackBuffer);
			pDeviceD3D11->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetViewD3D11);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGuiD3D11();

			// Create depthstencil state
			D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
			depthStencilDesc.DepthEnable = TRUE;
			depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			depthStencilDesc.StencilEnable = FALSE;
			depthStencilDesc.StencilReadMask = 0xFF;
			depthStencilDesc.StencilWriteMask = 0xFF;
			// Stencil operations if pixel is front-facing
			depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			// Stencil operations if pixel is back-facing
			depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
			depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			pDeviceD3D11->CreateDepthStencilState(&depthStencilDesc, &DepthStencilState_FALSE);

			GenerateTexture(0xff00ff00, DXGI_FORMAT_R10G10B10A2_UNORM); //DXGI_FORMAT_R32G32B32A32_FLOAT); //DXGI_FORMAT_R8G8B8A8_UNORM; 

			initonce = true;
		}
		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	//create shaders
	if (!sGreen)
		GenerateShader(pDeviceD3D11, &sGreen, 0.0f, 1.0f, 0.0f); //green

	if (!sMagenta)
		GenerateShader(pDeviceD3D11, &sMagenta, 1.0f, 0.0f, 1.0f); //magenta

	//recreate rendertarget on reset
	if (mainRenderTargetViewD3D11 == NULL)
	{
		ID3D11Texture2D* pBackBuffer = NULL;
		pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		pDeviceD3D11->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetViewD3D11);
		pBackBuffer->Release();
	}

	//get imgui viewport
	ImGuiIO io = ImGui::GetIO();
	ViewportWidth = io.DisplaySize.x;
	ViewportHeight = io.DisplaySize.y;

	if (GetAsyncKeyState(VK_INSERT) & 1) {
		showmenu = !showmenu;
	}

	//if (GetAsyncKeyState(VK_END)) {
		//kiero::shutdown();
		//return 0;
	//}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (showmenu) {
		
		ImGui::Begin("Hack Menu");
		ImGui::Checkbox("Wallhack", &wallhack);
		ImGui::Checkbox("Chams", &chams);	
		ImGui::Checkbox("Esp", &esp);
		//circle esp
		//line esp
		//text esp
		//distance esp

		ImGui::Checkbox("Aimbot", &aimbot);
		ImGui::SliderInt("Aimsens", &aimsens, 0, 10);

		ImGui::Text("Aimkey");
		const char* aimkey_Options[] = { "Off", "Shift", "Right Mouse", "Left Mouse", "Ctrl", "Tab", "Space", "X", "C" };
		/*
		aimkey 16 is Shift because VK_SHIFT = (hex) 0x10 = (decimal) 16 (virtual keycodes)
		aimkey 1 is Left Mouse
		aimkey 2 is Right Mouse
		aimkey 4 is Middle Mouse
		aimkey 17 is Ctrl
		aimkey 18 is Alt
		aimkey 20 is Caps Lock
		aimkey 32 is Space
		aimkey 88 is X key
		aimkey 67 is C Key
		aimkey 86 is V Key
		*/

		ImGui::SameLine();
		ImGui::Combo("##AimKey", (int*)&aimkey, aimkey_Options, IM_ARRAYSIZE(aimkey_Options));
		//aimfov
		//aimspeed_isbasedon_distance
		//aimspeed
		//aimheight
		//autoshoot
		//as_xhairds
		//as_compensatedst

		ImGui::Checkbox("Modelrecfinder", &modelrecfinder);
		if (modelrecfinder == 1)
		{
			//bruteforce
			ImGui::SliderInt("find Stride", &countStride, 0, 100);
			ImGui::SliderInt("find IndexCount", &countIndexCount, 0, 100);
			ImGui::SliderInt("find veWidth", &countveWidth, 0, 100);
		}

		ImGui::Checkbox("Wtsfinder", &wtsfinder);
		if (wtsfinder == 1)
		{
			//reset to avoid wrong values
			pStageBufferA = NULL;
			pStageBufferB = NULL;

			//bruteforce
			ImGui::SliderInt("WorldViewCBnum", &WorldViewCBnum, 0, 10);
			ImGui::SliderInt("ProjCBnum", &ProjCBnum, 0, 10);
			ImGui::SliderInt("matProjnum", &matProjnum, 0, 100);//240
		}

		ImGui::End();
	}

	//do esp
	if (esp==1)
	{
		ImGui::Begin("Transparent", reinterpret_cast<bool*>(true), ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
		ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);

		if (AimEspInfo.size() != NULL)
		{
			for (unsigned int i = 0; i < AimEspInfo.size(); i++)
			{
				if (AimEspInfo[i].vOutX > 1 && AimEspInfo[i].vOutY > 1 && AimEspInfo[i].vOutX < ViewportWidth && AimEspInfo[i].vOutY < ViewportHeight)
				{
					ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(AimEspInfo[i].vOutX, AimEspInfo[i].vOutY), ImColor(255, 255, 0, 255), "Model", 0, 0.0f, 0); //draw text
				}
			}
		}
		ImGui::End();
	}

	//if (aimbot == 1)
	//.. todo
	AimEspInfo.clear();


	//ImGui::EndFrame();
	ImGui::Render();
	pContextD3D11->OMSetRenderTargets(1, &mainRenderTargetViewD3D11, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return oPresent(pSwapChain, SyncInterval, Flags);
}

void __stdcall hkDrawIndexed_D3D11(ID3D11DeviceContext* pContextD3D11, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	ID3D11Buffer* veBuffer;
	UINT veWidth;
	UINT Stride;
	UINT veBufferOffset;
	D3D11_BUFFER_DESC veDesc;

	//get models
	pContextD3D11->IAGetVertexBuffers(0, 1, &veBuffer, &Stride, &veBufferOffset);
	if (veBuffer) {
		veBuffer->GetDesc(&veDesc);
		veWidth = veDesc.ByteWidth;
	}
	if (NULL != veBuffer) {
		veBuffer->Release();
		veBuffer = NULL;
	}

	//wallhack/chams
	if (wallhack==1||chams==1) //if wallhack or chams option is enabled in menu
		if (countStride == Stride || countIndexCount / 100 || countveWidth / 1000) //model rec (replace later with the logged values) <-------------------------------------------
		//if(Stride == 24 && IndexCount == 9192 && veWidth == 47280||Stride == 24 && IndexCount == 16998 && veWidth == 82560)//model in menu
		//if(Stride == 24 && IndexCount > 300 && veDesc.ByteWidth > 30000)//models
		//if(Stride == 40)
		/*
		//unreal4 models
		if ((Stride == 32 && IndexCount == 10155) ||
			(Stride == 44 && IndexCount == 11097) ||
			(Stride == 40 && IndexCount == 11412) ||
			(Stride == 40 && IndexCount == 11487) ||
			(Stride == 44 && IndexCount == 83262) ||
			(Stride == 40 && IndexCount == 23283))
		*/
	{
		//get orig
		if (wallhack == 1)
			pContextD3D11->OMGetDepthStencilState(&DepthStencilState_ORIG, 0); //get original

		//set off
		if (wallhack == 1)
			pContextD3D11->OMSetDepthStencilState(DepthStencilState_FALSE, 0); //depthstencil off

		if (chams == 1)
			pContextD3D11->PSSetShader(sGreen, NULL, NULL);
			//pContextD3D11->PSSetShaderResources(0, 1, &textureColor); //magenta

		oDrawIndexed(pContextD3D11, IndexCount, StartIndexLocation, BaseVertexLocation); //redraw

		if (chams == 1)
			pContextD3D11->PSSetShader(sMagenta, NULL, NULL);

		//restore orig
		if (wallhack == 1)
			pContextD3D11->OMSetDepthStencilState(DepthStencilState_ORIG, 0); //depthstencil on

		//release
		if (wallhack == 1)
			SAFE_RELEASE(DepthStencilState_ORIG); //release
	}

	//esp/aimbot
	if (esp==1||aimbot==1) //if esp/aimbot option is enabled in menu
		if (countStride == Stride || countIndexCount / 100 || countveWidth / 1000) //model rec (replace later with the logged values) <-------------------------------------------
		//if(Stride == 24 && IndexCount == 9192 && veWidth == 47280||Stride == 24 && IndexCount == 16998 && veWidth == 82560)//model in menu
		//if(Stride == 24 && IndexCount > 300 && veDesc.ByteWidth > 30000)//models
		//if(Stride == 40)
		/*
		//unreal4 models
		if ((Stride == 32 && IndexCount == 10155) ||
			(Stride == 44 && IndexCount == 11097) ||
			(Stride == 40 && IndexCount == 11412) ||
			(Stride == 40 && IndexCount == 11487) ||
			(Stride == 44 && IndexCount == 83262) ||
			(Stride == 40 && IndexCount == 23283))
		*/
		{
			AddModel(pContextD3D11); //w2s
		}


	//menu logger
	if(modelrecfinder==1)
	{
	if (countStride == Stride || countIndexCount == IndexCount / 100 || countveWidth == veWidth / 1000)
		if (GetAsyncKeyState(VK_END) & 1) //press END to log to log.txt
			Log("Stride == %d && IndexCount == %d && veWidth == %d", Stride, IndexCount, veWidth);

		if (countStride == Stride || countIndexCount == IndexCount / 100 || countveWidth == veWidth / 1000)
		{
			return;
		}
	}

	/*
	//old log
	if (countnum == IndexCount / 100)
		if (GetAsyncKeyState('I') & 1)
			Log("Stride == %d && IndexCount == %d && veWidth == %d", Stride, IndexCount, veWidth);

	if (countnum == IndexCount / 100)
	{
		return;
	}
	
	if (GetAsyncKeyState('O') & 1) //-
		countnum--;
	if (GetAsyncKeyState('P') & 1) //+
		countnum++;
	if (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState('9') & 1) //reset, set to -1
		countnum = -1;
	*/
	
	return oDrawIndexed(pContextD3D11, IndexCount, StartIndexLocation, BaseVertexLocation);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	/*
	HMODULE hDXGIDLL = 0;
	do
	{
		hDXGIDLL = GetModuleHandle("dxgi.dll");
		Sleep(4000);
	} while (!hDXGIDLL);
	Sleep(100);
	*/
	oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(13, (void**)& oResizeBuffers, hkResizeBuffers_D3D11);
			kiero::bind(8, (void**)& oPresent, hkPresent_D3D11);
			kiero::bind(73, (void**)& oDrawIndexed, hkDrawIndexed_D3D11);
			init_hook = true;
		}
	} while (!init_hook);
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	}
	return TRUE;
}