//d3d11 w2s for ut4 engine games by n7

//==========================================================================================================================

//features
int aimbot = 1;
DWORD Daimkey = VK_RBUTTON;		//aimkey
int aimfov = 3;				//aim field of view in % 
int aimsens = 3;			//aim sensitivity, makes aim smoother
static int aimheight = 46;		//aim height value, low value = aims lower, high values aims heigher
int autoshoot = 0;			//autoshoot
unsigned int asdelay = 90;		//use x-999 (shoot for xx millisecs, looks more legit)
bool IsPressed = false;			//
DWORD astime = timeGetTime();		//auto_shoot

//init only once
bool firstTime = true;

//viewport
UINT vps = 1;
D3D11_VIEWPORT viewport;
float ScreenCenterX;
float ScreenCenterY;

//vertex
ID3D11Buffer *veBuffer;
UINT Stride = 0;
UINT veBufferOffset = 0;
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
D3D11_SHADER_RESOURCE_VIEW_DESC  Descr;
ID3D11ShaderResourceView* ShaderResourceView;

//psgetConstantbuffers
ID3D11Buffer *pcsBuffer;
D3D11_BUFFER_DESC pscdesc;
UINT pscStartSlot;

//vsgetconstantbuffers
ID3D11Buffer *mConstantBuffers;
UINT vsConstant_StartSlot;

//used for logging/cycling through values
bool logger = false;
static int countnum = -1;
char szString[64];

UINT psStartSlot;

#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = nullptr; } }

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
	char szCast[] = "struct VS_OUT"
		"{"
		" float4 Position : SV_Position;"
		" float4 Color : COLOR0;"
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

//wh
char *state;
ID3D11RasterizerState * rwState;
ID3D11RasterizerState * rsState;

enum eDepthState
{
	ENABLED,
	DISABLED,
	READ_NO_WRITE,
	NO_READ_NO_WRITE,
	_DEPTH_COUNT
};

ID3D11DepthStencilState* myDepthStencilStates[static_cast<int>(eDepthState::_DEPTH_COUNT)];

void SetDepthStencilState(eDepthState aState)
{
	pContext->OMSetDepthStencilState(myDepthStencilStates[aState], 1);
}

//==========================================================================================================================

//w2s stuff
struct Vec2
{
	float x, y;
};

struct Vec3
{
	float x, y, z;
};

struct Vec4
{
	float x, y, z, w;
};

static Vec4 Vec4MulMat4x4(const Vec4& v, float(*mat4x4)[4])
{
	Vec4 o;
	
	//inv
	o.x = v.x * mat4x4[0][0] + v.y * mat4x4[1][0] + v.z * mat4x4[2][0] + v.w * mat4x4[3][0];
	o.y = v.x * mat4x4[0][1] + v.y * mat4x4[1][1] + v.z * mat4x4[2][1] + v.w * mat4x4[3][1];//+aimheight;
	o.z = v.x * mat4x4[0][2] + v.y * mat4x4[1][2] + v.z * mat4x4[2][2] + v.w * mat4x4[3][2];
	o.w = v.x * mat4x4[0][3] + v.y * mat4x4[1][3] + v.z * mat4x4[2][3] + v.w * mat4x4[3][3];
	
	/*
	//normal?
	o.x = v.x * mat4x4[0][0] + v.y * mat4x4[0][1] + v.z * mat4x4[0][2] + v.w * mat4x4[0][3];
	o.y = v.x * mat4x4[1][0] + v.y * mat4x4[1][1] + v.z * mat4x4[1][2] + v.w * mat4x4[1][3];
	o.z = v.x * mat4x4[2][0] + v.y * mat4x4[2][1] + v.z * mat4x4[2][2] + v.w * mat4x4[2][3];
	o.w = v.x * mat4x4[3][0] + v.y * mat4x4[3][1] + v.z * mat4x4[3][2] + v.w * mat4x4[3][3];
	*/
	return o;
}

static Vec4 Vec3MulMat4x4(const Vec3& v, float(*mat4x4)[4])
{
	Vec4 o;
	
	//inv
	o.x = v.x * mat4x4[0][0] + v.y * mat4x4[1][0] + v.z * mat4x4[2][0] + mat4x4[3][0];
	o.y = v.x * mat4x4[0][1] + v.y * mat4x4[1][1] + v.z * mat4x4[2][1] + mat4x4[3][1];
	o.z = v.x * mat4x4[0][2] + v.y * mat4x4[1][2] + v.z * mat4x4[2][2] + mat4x4[3][2];
	o.w = v.x * mat4x4[0][3] + v.y * mat4x4[1][3] + v.z * mat4x4[2][3] + mat4x4[3][3];
	
	/*
	//normal?
	o.x = mat4x4[0][0] * v.x + mat4x4[0][1] * v.y + mat4x4[0][2] * v.z + mat4x4[0][3];
	o.y = mat4x4[1][0] * v.x + mat4x4[1][1] * v.y + mat4x4[1][2] * v.z + mat4x4[1][3];
	o.z = mat4x4[2][0] * v.x + mat4x4[2][1] * v.y + mat4x4[2][2] * v.z + mat4x4[2][3];
	o.w = mat4x4[3][0] * v.x + mat4x4[3][1] * v.y + mat4x4[3][2] * v.z + mat4x4[3][3];
	*/
	return o;
}

static Vec3 Vec3MulMat4x3(const Vec3& v, float(*mat4x3)[3])
{
	Vec3 o;
	o.x = v.x * mat4x3[0][0] + v.y * mat4x3[1][0] + v.z * mat4x3[2][0] + mat4x3[3][0];
	o.y = v.x * mat4x3[0][1] + v.y * mat4x3[1][1] + v.z * mat4x3[2][1] + mat4x3[3][1];
	o.z = v.x * mat4x3[0][2] + v.y * mat4x3[1][2] + v.z * mat4x3[2][2] + mat4x3[3][2];
	return o;
}

void MapBuffer(ID3D11Buffer* pStageBuffer, void** ppData, UINT* pByteWidth)
{
	D3D11_MAPPED_SUBRESOURCE subRes;
	HRESULT res = pContext->Map(pStageBuffer, 0, D3D11_MAP_READ, 0, &subRes);

	D3D11_BUFFER_DESC desc;
	pStageBuffer->GetDesc(&desc);

	if (FAILED(res))
	{
		Log("Map stage buffer failed {%d} {%d} {%d} {%d} {%d}", (void*)pStageBuffer, desc.ByteWidth, desc.BindFlags, desc.CPUAccessFlags, desc.Usage);
	}

	*ppData = subRes.pData;

	if (pByteWidth)
		*pByteWidth = desc.ByteWidth;
}

void UnmapBuffer(ID3D11Buffer* pStageBuffer)
{
	pContext->Unmap(pStageBuffer, 0);
}

ID3D11Buffer* CopyBufferToCpu(ID3D11Buffer* pBuffer)
{
	D3D11_BUFFER_DESC CBDesc;
	pBuffer->GetDesc(&CBDesc);

	ID3D11Buffer* pStageBuffer = NULL;
	{ // create shadow buffer.
		D3D11_BUFFER_DESC desc;
		desc.BindFlags = 0;
		desc.ByteWidth = CBDesc.ByteWidth;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;
		desc.Usage = D3D11_USAGE_STAGING;

		if (FAILED(pDevice->CreateBuffer(&desc, NULL, &pStageBuffer)))
		{
			Log("CreateBuffer failed when CopyBufferToCpu {%d}", CBDesc.ByteWidth);
		}
	}

	if (pStageBuffer != NULL)
		pContext->CopyResource(pStageBuffer, pBuffer);

	return pStageBuffer;
}

//get distance
float GetmDst(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

struct AimEspInfo_t
{
	float vOutX, vOutY;
	//INT       iTeam;
	float CrosshairDst;
};
std::vector<AimEspInfo_t>AimEspInfo;

//w2s
int WorldViewCBnum = 2;
int ProjCBnum = 1;
int matProjnum = 16;

//Game			WorldViewCBnum		ProjCBnum		matProjnum
//UT4 Alpha		2			1			16		(4=world, 2=view, 1=proj, 16=proj[xx])
//Outlast 		0			1			0 and 16
//Overwatch		7			9			0		(untested)
//Warframe		0			0			0 or 4
ID3D11Buffer* pWorldViewCB = nullptr;
ID3D11Buffer* pProjCB = nullptr;
ID3D11Buffer* m_pCurWorldViewCB;
ID3D11Buffer* m_pCurProjCB;
void AddModel(ID3D11DeviceContext* pContext)
{
	//Warning, this is NOT optimized:

	//pContext->VSGetConstantBuffers(countnum, 1, &pWorldCB);//4works (UT4) (not needed) //world

	pContext->VSGetConstantBuffers(WorldViewCBnum, 1, &pWorldViewCB);//2works (UT4)		//worldview

	pContext->VSGetConstantBuffers(ProjCBnum, 1, &pProjCB);//1works (UT4)				//proj

	if (pWorldViewCB == NULL|| pProjCB == NULL)
	{
		return;
	}

	//WORLDVIEW
	if(pWorldViewCB != NULL)
	m_pCurWorldViewCB = CopyBufferToCpu(pWorldViewCB);
	SAFE_RELEASE(pWorldViewCB);

	float matWorldView[4][4];
	{
		float* WorldViewCB;
		MapBuffer(m_pCurWorldViewCB, (void**)&WorldViewCB, NULL);
		memcpy(matWorldView, &WorldViewCB[0], sizeof(matWorldView));
		//matWorldView[3][2] = matWorldView[3][2] + (aimheight*20);	//aimheight is usually done here for body parts
		UnmapBuffer(m_pCurWorldViewCB);
		SAFE_RELEASE(m_pCurWorldViewCB);
	}
	Vec3 v;
	Vec4 vWorldView = Vec3MulMat4x4(v, matWorldView);
	

	//PROJECTION
	if(pProjCB != NULL)
	m_pCurProjCB = CopyBufferToCpu(pProjCB);
	SAFE_RELEASE(pProjCB);

	float matProj[4][4];
	{
		float* pProjCB;
		MapBuffer(m_pCurProjCB, (void**)&pProjCB, NULL);
		memcpy(matProj, &pProjCB[matProjnum], sizeof(matProj));//16works (UT4)
		UnmapBuffer(m_pCurProjCB);
		SAFE_RELEASE(m_pCurProjCB);
	}
	Vec4 vWorldViewProj = Vec4MulMat4x4(vWorldView, matProj);


	Vec2 o;
	o.x = ScreenCenterX + ScreenCenterX * vWorldViewProj.x / vWorldViewProj.w;
	o.y = ScreenCenterY + ScreenCenterY * -vWorldViewProj.y / vWorldViewProj.w;

	AimEspInfo_t pAimEspInfo = { static_cast<float>(o.x), static_cast<float>(o.y) };
	AimEspInfo.push_back(pAimEspInfo);
}
/*
void TransformToScreenSpace(ID3D11DeviceContext* pContext)
{
	
}
*/
