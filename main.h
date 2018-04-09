//d3d11 w2s for some games by n7

//DX Includes
#include <DirectXMath.h>
using namespace DirectX;

//==========================================================================================================================

//features deafult settings
int Folder1 = 1;
int Item1 = 1; //sOptions[0].Function //wallhack
int Item2 = 1; //sOptions[1].Function //chams
int Item3 = 1; //sOptions[2].Function //esp
int Item4 = 1; //sOptions[3].Function //aimbot
int Item5 = 0; //sOptions[4].Function //aimkey 0 = RMouse
int Item6 = 3; //sOptions[5].Function //aimsens
int Item7 = 3; //sOptions[6].Function //aimfov
int Item8 = 0; //sOptions[7].Function //aimheight
int Item9 = 0; //sOptions[8].Function //autoshoot
int Item10 = 0; //sOptions[9].Function //crosshair
int Item11 = 2; //sOptions[10].Function //preaim

//globals
DWORD Daimkey = VK_RBUTTON;		//aimkey
int aimheight = 0;				//aim height value
int preaim = 0;					//praim to not aim behind
unsigned int asdelay = 90;		//use x-999 (shoot for xx millisecs, looks more legit)
bool IsPressed = false;			//
DWORD astime = timeGetTime();	//autoshoot timer

//init only once
bool firstTime = true;
bool firstTime2 = true;

//viewport
UINT vps = 1;
D3D11_VIEWPORT viewport;
float ScreenCenterX;
float ScreenCenterY;

//vertex
UINT veStartSlot;
UINT veNumBuffers;
ID3D11Buffer *veBuffer;
UINT Stride;
UINT veBufferOffset;
D3D11_BUFFER_DESC vedesc;

//index
ID3D11Buffer *inBuffer;
DXGI_FORMAT inFormat;
UINT        inOffset;
D3D11_BUFFER_DESC indesc;

//rendertarget
ID3D11Texture2D* RenderTargetTexture;
ID3D11RenderTargetView* RenderTargetView = NULL;

//shader
ID3D11PixelShader* psRed = NULL;
ID3D11PixelShader* psGreen = NULL;

//pssetshaderresources
UINT pssrStartSlot;
ID3D11ShaderResourceView* pShaderResView;
ID3D11Resource *Resource;
D3D11_SHADER_RESOURCE_VIEW_DESC  Descr;
D3D11_TEXTURE2D_DESC texdesc;

//psgetConstantbuffers
UINT pscStartSlot;
UINT pscNumBuffers;
ID3D11Buffer *pscBuffer;
D3D11_BUFFER_DESC pscdesc;

//vsgetconstantbuffers
ID3D11Buffer *vsBuffer;
UINT vsNumBuffers;
D3D11_BUFFER_DESC vsdesc;

UINT psStartSlot;
UINT vsStartSlot;

//create texture
ID3D11Texture2D* texGreen = nullptr;
ID3D11Texture2D* texRed = nullptr;

//create shaderresourcevew
ID3D11ShaderResourceView* texSRVg;
ID3D11ShaderResourceView* texSRVr;

//create samplerstate
ID3D11SamplerState *pSamplerState;

static BOOL performance_loss = FALSE;

//used for logging/cycling through values
bool logger = false;
UINT countnum = -1;
char szString[64];

#define SAFE_RELEASE(x) if (x) { x->Release(); x = NULL; }
HRESULT hr;

//==========================================================================================================================

//get dir
using namespace std;
#include <fstream>
char dlldir[320];
char *GetDirectoryFile(char *filename)
{
	static char path[320];
	strcpy_s(path, dlldir);
	strcat_s(path, filename);
	return path;
}

//log
void Log(const char *fmt, ...)
{
	if (!fmt)	return;

	char		text[4096];
	va_list		ap;
	va_start(ap, fmt);
	vsprintf_s(text, fmt, ap);
	va_end(ap);

	ofstream logfile(GetDirectoryFile("log.txt"), ios::app);
	if (logfile.is_open() && text)	logfile << text << endl;
	logfile.close();
}

//==========================================================================================================================

//generate shader func
HRESULT GenerateShader(ID3D11Device* pD3DDevice, ID3D11PixelShader** pShader, float r, float g, float b)
{
	/*
	//texture sample chams bright
	const char szCast[] =
		"struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            sampler sampler0;\
            Texture2D texture0;\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
            float4 out_col = input.col.bgra + texture0.Sample(sampler0, input.uv); \
			out_col.g = %f; \
			out_col.a = 1.0f; \
            return out_col; \
            }";
	*/
	
	char szCast[] = "struct VS_OUT"
		"{"
		" float4 Position : SV_Position;"
		" float4 Color : COLOR1;"
		"};"

		"float4 main( VS_OUT input ) : SV_Target"
		"{"
		" float4 fake;"
		" fake.a = 1.0f;"
		" fake.r = %f;"
		" fake.g = %f;"
		" fake.b = %f;"
		" return fake;"
		"}";
	
	ID3D10Blob* pBlob;
	char szPixelShader[1000];

	sprintf_s(szPixelShader, szCast, r, g, b);

	ID3DBlob* d3dErrorMsgBlob;

	HRESULT hr = D3DCompile(szPixelShader, sizeof(szPixelShader), "shader", NULL, NULL, "main", "ps_4_0", NULL, NULL, &pBlob, &d3dErrorMsgBlob);

	if (FAILED(hr))
		return hr;

	hr = pD3DDevice->CreatePixelShader((DWORD*)pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, pShader);

	if (FAILED(hr))
		return hr;

	return S_OK;
}

//==========================================================================================================================

UINT stencilRef = 0;
ID3D11DepthStencilState* pDepthStencilState = NULL;
D3D11_DEPTH_STENCIL_DESC Desc;
ID3D11DepthStencilState* pHackedDepthStencilState = NULL;

ID3D11DepthStencilState* depthStencilState;
ID3D11DepthStencilState* depthStencilStatefalse;

//wh
char *state;
ID3D11RasterizerState * rwState;
ID3D11RasterizerState * rsState;

//==========================================================================================================================

//menu
#define MAX_ITEMS 25

#define T_FOLDER 1
#define T_OPTION 2
#define T_MULTIOPTION 3
#define T_AIMKEYOPTION 4

#define LineH 15

struct Options {
	LPCWSTR Name;
	int	Function;
	BYTE Type;
};

struct Menu {
	LPCWSTR Title;
	int x;
	int y;
	int w;
};

DWORD Color_Font;
DWORD Color_On;
DWORD Color_Off;
DWORD Color_Folder;
DWORD Color_Current;

bool Is_Ready, Visible;
int Items, Cur_Pos;

Options sOptions[MAX_ITEMS];
Menu sMenu;

#include <string>
#include <fstream>
void SaveCfg()
{
	ofstream fout;
	fout.open(GetDirectoryFile("w2sf.ini"), ios::trunc);
	fout << "Item1 " << sOptions[0].Function << endl;
	fout << "Item2 " << sOptions[1].Function << endl;
	fout << "Item3 " << sOptions[2].Function << endl;
	fout << "Item4 " << sOptions[3].Function << endl;
	fout << "Item5 " << sOptions[4].Function << endl;
	fout << "Item6 " << sOptions[5].Function << endl;
	fout << "Item7 " << sOptions[6].Function << endl;
	fout << "Item8 " << sOptions[7].Function << endl;
	fout << "Item9 " << sOptions[8].Function << endl;
	fout << "Item10 " << sOptions[9].Function << endl;
	fout << "Item11 " << sOptions[10].Function << endl;
	fout.close();
}

void LoadCfg()
{
	ifstream fin;
	string Word = "";
	fin.open(GetDirectoryFile("w2sf.ini"), ifstream::in);
	fin >> Word >> Item1;
	fin >> Word >> Item2;
	fin >> Word >> Item3;
	fin >> Word >> Item4;
	fin >> Word >> Item5;
	fin >> Word >> Item6;
	fin >> Word >> Item7;
	fin >> Word >> Item8;
	fin >> Word >> Item9;
	fin >> Word >> Item10;
	fin >> Word >> Item11;
	fin.close();
}

void JBMenu(void)
{
	Visible = true;
}

void Init_Menu(ID3D11DeviceContext *pContext, LPCWSTR Title, int x, int y)
{
	Is_Ready = true;
	sMenu.Title = Title;
	sMenu.x = x;
	sMenu.y = y;
}

void AddFolder(LPCWSTR Name, int Pointer)
{
	sOptions[Items].Name = (LPCWSTR)Name;
	sOptions[Items].Function = Pointer;
	sOptions[Items].Type = T_FOLDER;
	Items++;
}

void AddOption(LPCWSTR Name, int Pointer, int *Folder)
{
	if (*Folder == 0)
		return;
	sOptions[Items].Name = Name;
	sOptions[Items].Function = Pointer;
	sOptions[Items].Type = T_OPTION;
	Items++;
}

void AddMultiOption(LPCWSTR Name, int Pointer, int *Folder)
{
	if (*Folder == 0)
		return;
	sOptions[Items].Name = Name;
	sOptions[Items].Function = Pointer;
	sOptions[Items].Type = T_MULTIOPTION;
	Items++;
}

void AddMultiOptionText(LPCWSTR Name, int Pointer, int *Folder)
{
	if (*Folder == 0)
		return;
	sOptions[Items].Name = Name;
	sOptions[Items].Function = Pointer;
	sOptions[Items].Type = T_AIMKEYOPTION;
	Items++;
}

void Navigation()
{
	if (GetAsyncKeyState(VK_INSERT) & 1)
	{
		SaveCfg(); //save settings
		Visible = !Visible;
	}

	if (!Visible)
		return;

	int value = 0;

	if (GetAsyncKeyState(VK_DOWN) & 1)
	{
		Cur_Pos++;
		if (sOptions[Cur_Pos].Name == 0)
			Cur_Pos--;
	}

	if (GetAsyncKeyState(VK_UP) & 1)
	{
		Cur_Pos--;
		if (Cur_Pos == -1)
			Cur_Pos++;
	}

	else if (sOptions[Cur_Pos].Type == T_OPTION && GetAsyncKeyState(VK_RIGHT) & 1)
	{
		if (sOptions[Cur_Pos].Function == 0)
			value++;
	}

	else if (sOptions[Cur_Pos].Type == T_OPTION && (GetAsyncKeyState(VK_LEFT) & 1) && sOptions[Cur_Pos].Function == 1)
	{
		value--;
	}

	else if (sOptions[Cur_Pos].Type == T_MULTIOPTION && GetAsyncKeyState(VK_RIGHT) & 1 && sOptions[Cur_Pos].Function <= 6)//max
	{
		value++;
	}

	else if (sOptions[Cur_Pos].Type == T_MULTIOPTION && (GetAsyncKeyState(VK_LEFT) & 1) && sOptions[Cur_Pos].Function != 0)
	{
		value--;
	}

	else if (sOptions[Cur_Pos].Type == T_AIMKEYOPTION && GetAsyncKeyState(VK_RIGHT) & 1 && sOptions[Cur_Pos].Function <= 6)//max
	{
		value++;
	}

	else if (sOptions[Cur_Pos].Type == T_AIMKEYOPTION && (GetAsyncKeyState(VK_LEFT) & 1) && sOptions[Cur_Pos].Function != 0)
	{
		value--;
	}


	if (value) {
		sOptions[Cur_Pos].Function += value;
		if (sOptions[Cur_Pos].Type == T_FOLDER)
		{
			memset(&sOptions, 0, sizeof(sOptions));
			Items = 0;
		}
	}

}

bool IsReady()
{
	if (Items)
		return true;
	return false;
}

void DrawTextF(ID3D11DeviceContext* pContext, LPCWSTR text, int FontSize, int x, int y, DWORD Col)
{
	if (Is_Ready == false)
		MessageBoxA(0, "Error, you dont initialize the menu!", "Error", MB_OK);

	if (pFontWrapper)
		pFontWrapper->DrawString(pContext, text, (float)FontSize, (float)x, (float)y, Col, FW1_RESTORESTATE);
}

void Draw_Menu()
{
	if (!Visible)
		return;

	DrawTextF(pContext, sMenu.Title, 14, sMenu.x + 10, sMenu.y, Color_Font);
	for (int i = 0; i < Items; i++)
	{
		if (sOptions[i].Type == T_OPTION)
		{
			if (sOptions[i].Function)
			{
				DrawTextF(pContext, L"On", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);
			}
			else {
				DrawTextF(pContext, L"Off", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_Off);
			}
		}

		if (sOptions[i].Type == T_AIMKEYOPTION)
		{
			if (sOptions[i].Function == 0)
				DrawTextF(pContext, L"Right Mouse", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 1)
				DrawTextF(pContext, L"Left Mouse", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 2)
				DrawTextF(pContext, L"Shift", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 3)
				DrawTextF(pContext, L"Ctrl", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 4)
				DrawTextF(pContext, L"Alt", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 5)
				DrawTextF(pContext, L"Space", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 6)
				DrawTextF(pContext, L"X", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 7)
				DrawTextF(pContext, L"C", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);
		}

		if (sOptions[i].Type == T_MULTIOPTION)
		{
			if (sOptions[i].Function == 0)
				DrawTextF(pContext, L"0", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 1)
				DrawTextF(pContext, L"1", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 2)
				DrawTextF(pContext, L"2", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 3)
				DrawTextF(pContext, L"3", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 4)
				DrawTextF(pContext, L"4", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 5)
				DrawTextF(pContext, L"5", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 6)
				DrawTextF(pContext, L"6", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);

			if (sOptions[i].Function == 7)
				DrawTextF(pContext, L"7", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_On);
		}

		if (sOptions[i].Type == T_FOLDER)
		{
			if (sOptions[i].Function)
			{
				DrawTextF(pContext, L"Open", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_Folder);
			}
			else {
				DrawTextF(pContext, L"Closed", 14, sMenu.x + 150, sMenu.y + LineH*(i + 2), Color_Folder);
			}
		}
		DWORD Color = Color_Font;
		if (Cur_Pos == i)
			Color = Color_Current;
		DrawTextF(pContext, sOptions[i].Name, 14, sMenu.x + 6, sMenu.y + 1 + LineH*(i + 2), 0xFF2F4F4F);
		DrawTextF(pContext, sOptions[i].Name, 14, sMenu.x + 5, sMenu.y + LineH*(i + 2), Color);

	}
}

#define ORANGE 0xFF00BFFF
#define BLACK 0xFF000000
#define WHITE 0xFFFFFFFF
#define GREEN 0xFF00FF00 
#define RED 0xFFFF0000 
#define GRAY 0xFF2F4F4F

void Do_Menu()
{
	AddOption(L"Wallhack", Item1, &Folder1);
	AddOption(L"Chams", Item2, &Folder1);
	AddOption(L"Esp", Item3, &Folder1);
	AddOption(L"Aimbot", Item4, &Folder1);
	AddMultiOptionText(L"Aimkey", Item5, &Folder1);
	AddMultiOption(L"Aimsens", Item6, &Folder1);
	AddMultiOption(L"Aimfov", Item7, &Folder1);
	AddMultiOption(L"Aimheight", Item8, &Folder1);
	AddOption(L"Autoshoot", Item9, &Folder1);
	AddOption(L"Crosshair", Item10, &Folder1);
	AddMultiOption(L"PreAim", Item11, &Folder1);
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

void MapBuffer(ID3D11Buffer* pStageBuffer, void** ppData, UINT* pByteWidth)
{
	D3D11_MAPPED_SUBRESOURCE subRes;
	HRESULT res = pContext->Map(pStageBuffer, 0, D3D11_MAP_READ, 0, &subRes);

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
	pContext->Unmap(pStageBuffer, 0);
}

void CopyBufferToCpu(ID3D11Buffer* pInBuffer, ID3D11Buffer*& pOutBuffer)
{
	D3D11_BUFFER_DESC CBDesc;
	pInBuffer->GetDesc(&CBDesc);
	if (pOutBuffer != nullptr) {	// this probably is not needed
		D3D11_BUFFER_DESC outDesc;
		pOutBuffer->GetDesc(&outDesc);
		if (outDesc.ByteWidth != CBDesc.ByteWidth)
			SAFE_RELEASE(pOutBuffer);
			//return;
	}

	//performance_loss = false;
	if (pOutBuffer == nullptr)
	{ // create shadow buffer.
	  //Log("called once");
		performance_loss = true;
		D3D11_BUFFER_DESC desc;
		desc.BindFlags = 0;
		desc.ByteWidth = CBDesc.ByteWidth;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;
		desc.Usage = D3D11_USAGE_STAGING;
		if (FAILED(pDevice->CreateBuffer(&desc, NULL, &pOutBuffer)))
		{
			Log("CreateBuffer failed when CopyBufferToCpu {%d}", CBDesc.ByteWidth);
			SAFE_RELEASE(pOutBuffer);
		}
	}
	if (pOutBuffer != nullptr) {
		pContext->CopyResource(pOutBuffer, pInBuffer);
	}
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
int WorldViewCBnum = 2;
int ProjCBnum = 1;
int matProjnum = 16;

ID3D11Buffer* pWorldViewCB = nullptr;
ID3D11Buffer* pProjCB = nullptr;
ID3D11Buffer* m_pCurWorldViewCB = NULL;
ID3D11Buffer* m_pCurProjCB = NULL;

void AddModel(ID3D11DeviceContext* pContext)
{
	pContext->VSGetConstantBuffers(WorldViewCBnum, 1, &pWorldViewCB);//WorldViewCBnum changed to 2

	if (pWorldViewCB != nullptr)
		CopyBufferToCpu(pWorldViewCB, m_pCurWorldViewCB);
	if (m_pCurWorldViewCB == nullptr) {
		SAFE_RELEASE(pWorldViewCB);
		return;
	}

	pContext->VSGetConstantBuffers(ProjCBnum, 1, &pProjCB);//ProjCBnum changed to 1

	if (pProjCB != nullptr)
		CopyBufferToCpu(pProjCB, m_pCurProjCB);

	if (m_pCurProjCB == nullptr) {
		SAFE_RELEASE(pWorldViewCB);
		SAFE_RELEASE(pProjCB);
		return;
	}

	float matWorldView[4][4];
	{
		float* worldview;
		MapBuffer(m_pCurWorldViewCB, (void**)&worldview, NULL);
		memcpy(matWorldView, &worldview[0], sizeof(matWorldView));
		//matWorldView[3][2] = matWorldView[3][2] + (aimheight * 10); //aimheight alternative in some games
		UnmapBuffer(m_pCurWorldViewCB);
		//SAFE_RELEASE(m_pCurWorldViewCB);
	}

	float matProj[4][4];
	{
		float *proj;
		MapBuffer(m_pCurProjCB, (void**)&proj, NULL);
		memcpy(matProj, &proj[matProjnum], sizeof(matProj));//matProjnum changed to 16
		UnmapBuffer(m_pCurProjCB);
		//SAFE_RELEASE(m_pCurProjCB);
	}
	//SAFE_RELEASE(pWorldViewCB);
	//SAFE_RELEASE(pProjCB);

	//======================
	//w2s 1 (ut4)
	DirectX::XMVECTOR Pos = XMVectorSet(0.0f, (float)preaim*7.0f, (float)aimheight*30.0f, 1.0f);//preaim, aimheight

	DirectX::XMFLOAT4 pOut2d;
	DirectX::XMMATRIX pWorld = DirectX::XMMatrixIdentity();

	//transpose or not, depends on the game
	//DirectX::XMMATRIX pWorldView = DirectX::XMMatrixTranspose((FXMMATRIX)*matWorldView); //transpose
	//DirectX::XMMATRIX pProj = DirectX::XMMatrixTranspose((FXMMATRIX)*matProj); //transpose
	//DirectX::XMVECTOR pOut = DirectX::XMVector3Project(Pos, 0, 0, viewport.Width, viewport.Height, 0, 1, pProj, pWorldView, pWorld); //transposed

	//distance
	DirectX::XMMATRIX pWorldViewProj = (FXMMATRIX)*matWorldView * (FXMMATRIX)*matProj;
	float w = Pos.m128_f32[0] * pWorldViewProj.r[0].m128_f32[3] + Pos.m128_f32[1] * pWorldViewProj.r[1].m128_f32[3] + Pos.m128_f32[2] * pWorldViewProj.r[2].m128_f32[3] + pWorldViewProj.r[3].m128_f32[3];

	//normal
	DirectX::XMVECTOR pOut = DirectX::XMVector3Project(Pos, 0, 0, viewport.Width, viewport.Height, 0, 1, (FXMMATRIX)*matProj, (FXMMATRIX)*matWorldView, pWorld); //normal

	DirectX::XMStoreFloat4(&pOut2d, pOut);

	vec2 o;
	if (pOut2d.z < 1.0f)
	{
		o.x = pOut2d.x;
		o.y = pOut2d.y;
	}
	//AimEspInfo_t pAimEspInfo = { static_cast<float>(pOut.m128_f32[0]), static_cast<float>(pOut.m128_f32[1]) };
	AimEspInfo_t pAimEspInfo = { static_cast<float>(o.x), static_cast<float>(o.y), static_cast<float>(w*0.1f) };
	AimEspInfo.push_back(pAimEspInfo);
	//======================

	/*
	//======================
	//w2s 2
	DirectX::XMVECTOR Pos = XMVectorSet(0.0f, 0.0f, (float)aimheight, 1.0f);//aimheight

	DirectX::XMMATRIX ViewProjectionMatrix = DirectX::XMMatrixMultiply((FXMMATRIX)*matWorldView, (FXMMATRIX)*matProj);

	//transpose or not, depends on the game
	///DirectX::XMMATRIX pWorldViewProj = DirectX::XMMatrixTranspose(ViewProjectionMatrix); //transpose
	//DirectX::XMVECTOR temp = DirectX::XMVector4Transform(Pos, pWorldViewProj); //transposed

	//normal
	DirectX::XMVECTOR temp = DirectX::XMVector4Transform(Pos, ViewProjectionMatrix); //normal

	float tempInvW2 = 1.0f / temp.m128_f32[3];

	float outx = (1.0f + (temp.m128_f32[0] * tempInvW2)) * viewport.Width / 2.0f;
	float outy = (1.0f + (temp.m128_f32[1] * tempInvW2)) * viewport.Height / 2.0f; //- or + depends on the game
	float outz = temp.m128_f32[2];

	AimEspInfo_t pAimEspInfo = { static_cast<float>(outx), static_cast<float>(outy), static_cast<float>(Pos.m128_f32[3]) };
	AimEspInfo.push_back(pAimEspInfo);
	//======================
	*/

	/*
	//======================
	//w2s 3 (unity)
	//DirectX::XMVECTOR Pos = XMVectorSet(-(float)aimheight/20.0f, -(float)aimheight/20.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR Pos = XMVectorSet(0.0f, (float)aimheight / 10.0f, 0.1f, 1.0f);//verdun(game) aimheight with pre aim 0.1
	DirectX::XMMATRIX viewProjMatrix = DirectX::XMMatrixMultiply((FXMMATRIX)*matWorldView, (FXMMATRIX)*matProj);

	//transpose or not, depends on the game
	//DirectX::XMMATRIX WorldViewProj = DirectX::XMMatrixTranspose(viewProjMatrix); //transpose

	//normal
	DirectX::XMMATRIX WorldViewProj = viewProjMatrix; //normal

	float mx = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[0] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[0] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[0] + WorldViewProj.r[3].m128_f32[0];
	float my = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[1] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[1] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[1] + WorldViewProj.r[3].m128_f32[1];
	float mz = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[2] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[2] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[2] + WorldViewProj.r[3].m128_f32[2];
	float mw = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[3] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[3] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[3] + WorldViewProj.r[3].m128_f32[3];

	float xx, yy;
	if (mw > 0.2f)
	{
		xx = ((mx / mw) * (viewport.Width / 2.0f)) + (viewport.Width / 2.0f);
		yy = (viewport.Height / 2.0f) + ((my / mw) * (viewport.Height / 2.0f)); //- or + depends on the game
	}
	else
	{
		xx = -1;
		yy = -1;
	}
	AimEspInfo_t pAimEspInfo = { static_cast<float>(xx), static_cast<float>(yy), static_cast<float>(mw) };
	AimEspInfo.push_back(pAimEspInfo);
	//======================
	*/

	/*
	//======================
	//w2s 4
	vec3 vStart;
	vec3 vOut;
	float *flMatrix;
	DirectX::XMMATRIX Matrix = DirectX::XMMatrixMultiply((FXMMATRIX)*matWorldView, (FXMMATRIX)*matProj);

	//transpose or not, depends on the game
	//DirectX::XMMATRIX Matrixx = DirectX::XMMatrixTranspose(Matrix); //transpose

	flMatrix = (float*)&Matrix;

	vOut.x = flMatrix[0] * vStart.x + flMatrix[1] * vStart.y + flMatrix[2] * vStart.z + flMatrix[3];
	vOut.y = flMatrix[4] * vStart.x + flMatrix[5] * vStart.y + flMatrix[6] * vStart.z + flMatrix[7];

	float flTemp = flMatrix[12] * vStart.x + flMatrix[13] * vStart.y + flMatrix[14] * vStart.z + flMatrix[15];//w

	if (flTemp > 0.01f)
	{
		float x = (float)viewport.Width / 2.0f;
		float y = (float)viewport.Height / 2.0f;

		x += 0.5f * vOut.x * (float)viewport.Width + 0.5f;
		y -= 0.5f * vOut.y * (float)viewport.Height + 0.5f; //- or + depends on the game

		vOut.x = x;
		vOut.y = y;
	}
	else
	{
		vOut.x = -1.0f;
		vOut.y = -1.0f;
	}

	AimEspInfo_t pAimEspInfo = { static_cast<float>(vOut.x), static_cast<float>(vOut.y) };
	AimEspInfo.push_back(pAimEspInfo);
	//======================
	*/

	/*
	//======================
	//w2s 5 
	DirectX::XMVECTOR Pos = XMVectorSet(0.0f, 0.0f, (float)countnum, 1.0f);//aimheight
	float xx, yy;
	DirectX::XMFLOAT3 Pos;
	DirectX::XMMATRIX viewProjMatrix = DirectX::XMMatrixMultiply((FXMMATRIX)*matWorldView, (FXMMATRIX)*matProj);//DirectX::XMMatrixTranspose((FXMMATRIX)*matWorldView);
	//DirectX::XMMATRIX ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), (viewport.Width / viewport.Height), 0.1f, 10000.0f); // Creating Perspective Matrix
	//XMMATRIX viewProjMatrix = DirectX::XMMatrixMultiply(pWorldView, ProjectionMatrix); // View Projective Matrix = View Matrix * Projection Matrix

	float w = viewProjMatrix.r[3].m128_f32[0] * Pos.m128_f32[0] +
		viewProjMatrix.r[3].m128_f32[1] * Pos.m128_f32[1] +
		viewProjMatrix.r[3].m128_f32[2] * Pos.m128_f32[2] +
		viewProjMatrix.r[3].m128_f32[3];

	if (w > 0.01f) {     // check the angles
		float inversew = 1 / w;

		float tempX = viewProjMatrix.r[0].m128_f32[0] * Pos.m128_f32[0] +
			viewProjMatrix.r[0].m128_f32[1] * Pos.m128_f32[1] +
			viewProjMatrix.r[0].m128_f32[2] * Pos.m128_f32[2] +
			viewProjMatrix.r[0].m128_f32[3];

		float tempY = viewProjMatrix.r[1].m128_f32[0] * Pos.m128_f32[0] +
			viewProjMatrix.r[1].m128_f32[1] * Pos.m128_f32[1] +
			viewProjMatrix.r[1].m128_f32[2] * Pos.m128_f32[2] +
			viewProjMatrix.r[1].m128_f32[3];

		xx = (viewport.Width / 2.0f) + (0.5f * (tempX * inversew) * viewport.Width + 0.5f);
		yy = (viewport.Height / 2.0f) - (0.5f * (tempY * inversew) * viewport.Height + 0.5f);
	}
	else
	{
		xx = -1.0f;
		yy = -1.0f;
	}

	AimEspInfo_t pAimEspInfo = { static_cast<float>(xx), static_cast<float>(yy) };
	AimEspInfo.push_back(pAimEspInfo);
	*/

	/*
	//======================
	//w2s 6
	DirectX::XMVECTOR position{ 0 };
	float x, y;

	DirectX::XMMATRIX view = (FXMMATRIX)*matWorldView;
	DirectX::XMMATRIX projection = (FXMMATRIX)*matProj;
	DirectX::XMMATRIX viewProjection;
	viewProjection = view * projection;

	DirectX::XMVECTOR screenPosition = XMVector3Transform(position, viewProjection);
	//D3DXVec3TransformCoord(&screenPosition, &position, &viewProjection);
	x = (screenPosition.m128_f32[0] + 1) * viewport.Width / 2;
	y = (-screenPosition.m128_f32[1] + 1) * viewport.Height / 2;

	AimEspInfo_t pAimEspInfo = { static_cast<float>(x), static_cast<float>(y) };
	AimEspInfo.push_back(pAimEspInfo);
	//======================
	*/
}

//==========================================================================================================================
/*
//if (pWorldViewCB != nullptr && pProjCB != nullptr)
//W2S(pWorldViewCB, pProjCB);
void W2S(ID3D11Buffer* pWorldViewCB, ID3D11Buffer* pProjCB)
{
	float matWorldView[4][4];
	{
		float* worldview;
		MapBuffer(m_pCurWorldViewCB, (void**)&worldview, NULL);
		memcpy(matWorldView, &worldview[0], sizeof(matWorldView));
		//matWorldView[3][2] = matWorldView[3][2] + (aimheight * 10); //aimheight alternative in some games
		UnmapBuffer(m_pCurWorldViewCB);
		//SAFE_RELEASE(m_pCurWorldViewCB);
	}

	float matProj[4][4];
	{
		float *proj;
		MapBuffer(m_pCurProjCB, (void**)&proj, NULL);
		memcpy(matProj, &proj[16], sizeof(matProj));//matProjnum changed to 16
		UnmapBuffer(m_pCurProjCB);
		//SAFE_RELEASE(m_pCurProjCB);
	}

	//======================
	//w2s 1 (ut4)
	DirectX::XMVECTOR Pos = XMVectorSet(0.0f, (float)preaim*7.0f, (float)aimheight*30.0f, 1.0f);//preaim, aimheight

	DirectX::XMFLOAT4 pOut2d;
	DirectX::XMMATRIX pWorld = DirectX::XMMatrixIdentity();

	//transpose or not, depends on the game
	//DirectX::XMMATRIX pWorldView = DirectX::XMMatrixTranspose((FXMMATRIX)*matWorldView); //transpose
	//DirectX::XMMATRIX pProj = DirectX::XMMatrixTranspose((FXMMATRIX)*matProj); //transpose
	//DirectX::XMVECTOR pOut = DirectX::XMVector3Project(Pos, 0, 0, viewport.Width, viewport.Height, 0, 1, pProj, pWorldView, pWorld); //transposed

	//distance
	DirectX::XMMATRIX pWorldViewProj = (FXMMATRIX)*matWorldView * (FXMMATRIX)*matProj;
	float w = Pos.m128_f32[0] * pWorldViewProj.r[0].m128_f32[3] + Pos.m128_f32[1] * pWorldViewProj.r[1].m128_f32[3] + Pos.m128_f32[2] * pWorldViewProj.r[2].m128_f32[3] + pWorldViewProj.r[3].m128_f32[3];

	//normal
	DirectX::XMVECTOR pOut = DirectX::XMVector3Project(Pos, 0, 0, viewport.Width, viewport.Height, 0, 1, (FXMMATRIX)*matProj, (FXMMATRIX)*matWorldView, pWorld); //normal

	DirectX::XMStoreFloat4(&pOut2d, pOut);

	vec2 o;
	if (pOut2d.z < 1.0f)
	{
		o.x = pOut2d.x;
		o.y = pOut2d.y;
	}
	//AimEspInfo_t pAimEspInfo = { static_cast<float>(pOut.m128_f32[0]), static_cast<float>(pOut.m128_f32[1]) };
	AimEspInfo_t pAimEspInfo = { static_cast<float>(o.x), static_cast<float>(o.y), static_cast<float>(w*0.1f) };
	AimEspInfo.push_back(pAimEspInfo);
	//======================
}
*/