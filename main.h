//d3d11 w2s for ut4 engine games by n7

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

int countnum = -1;
#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = nullptr; } }

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

/*
//csgo, paladins style
static Vec4 Vec4MulMat4x4Test(const Vec4& v, float(*mat4x4)[4])
{
	Vec4 o;
	o.x = mat4x4[0][0] * o.x + mat4x4[0][1] * o.y + mat4x4[0][2] * o.z + mat4x4[0][3];
	o.y = mat4x4[1][0] * o.x + mat4x4[1][1] * o.y + mat4x4[1][2] * o.z + mat4x4[1][3];
	o.z = mat4x4[2][0] * o.x + mat4x4[2][1] * o.y + mat4x4[2][2] * o.z + mat4x4[2][3];
	o.w = mat4x4[3][0] * o.x + mat4x4[3][1] * o.y + mat4x4[3][2] * o.z + mat4x4[3][3];
	return o;
}
*/

static Vec4 Vec4MulMat4x4(const Vec4& v, float(*mat4x4)[4])
{
	Vec4 o;
	o.x = v.x * mat4x4[0][0] + v.y * mat4x4[1][0] + v.z * mat4x4[2][0] + v.w * mat4x4[3][0];
	o.y = v.x * mat4x4[0][1] + v.y * mat4x4[1][1] + v.z * mat4x4[2][1] + v.w * mat4x4[3][1];
	o.z = v.x * mat4x4[0][2] + v.y * mat4x4[1][2] + v.z * mat4x4[2][2] + v.w * mat4x4[3][2];
	o.w = v.x * mat4x4[0][3] + v.y * mat4x4[1][3] + v.z * mat4x4[2][3] + v.w * mat4x4[3][3];
	return o;
}

static Vec4 Vec3MulMat4x4(const Vec3& v, float(*mat4x4)[4])
{
	Vec4 o;
	o.x = v.x * mat4x4[0][0] + v.y * mat4x4[1][0] + v.z * mat4x4[2][0] + mat4x4[3][0];
	o.y = v.x * mat4x4[0][1] + v.y * mat4x4[1][1] + v.z * mat4x4[2][1] + mat4x4[3][1];
	o.z = v.x * mat4x4[0][2] + v.y * mat4x4[1][2] + v.z * mat4x4[2][2] + mat4x4[3][2];
	o.w = v.x * mat4x4[0][3] + v.y * mat4x4[1][3] + v.z * mat4x4[2][3] + mat4x4[3][3];
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

struct AimEspInfo_t
{
	float vOutX, vOutY;
	INT       iTeam;
	float CrosshairDst;
};
std::vector<AimEspInfo_t>AimEspInfo;

ID3D11Buffer* CopyBufferToCpu(ID3D11Buffer* pBuffer)
{
	D3D11_BUFFER_DESC CBDesc;
	pBuffer->GetDesc(&CBDesc);

	ID3D11Buffer* pStageBuffer = NULL;
	{ 
		//create shadow buffer
		D3D11_BUFFER_DESC desc;
		desc.BindFlags = 0;
		desc.ByteWidth = CBDesc.ByteWidth;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;
		desc.Usage = D3D11_USAGE_STAGING;

		if (FAILED(pDevice->CreateBuffer(&desc, NULL, &pStageBuffer)))
		{
			Log("CreateBuffer failed when CopyBufferToCpu == %d", CBDesc.ByteWidth);
		}
	}

	if (pStageBuffer != NULL)
		pContext->CopyResource(pStageBuffer, pBuffer);

	return pStageBuffer;
}

//w2s
Vec4 vWorldView;
Vec4 vClip;
int ObjectCBnum = 1;
int FrameCBnum = 1;
int matProjnum = 1;

//Game			ObjectCBnum		FrameCBnum		matProjnum
//UT4 Alpha		2				1				16
//Outlast		0				1				0 and 16
//Overwatch		7				9				0			(untested)
void AddModel(ID3D11DeviceContext* pContext, int iTeam)
{
	//bruteforce ObjectCBnum
	if (GetAsyncKeyState('Z') & 1) //-
		ObjectCBnum--;
	if (GetAsyncKeyState('U') & 1) //+
		ObjectCBnum++;
	if (GetAsyncKeyState(0x37) & 1) //7 reset, set to 0
		ObjectCBnum = 0;
	if (ObjectCBnum < 0)
		ObjectCBnum = 0;

	//bruteforce FrameCBnum
	if (GetAsyncKeyState('H') & 1) //-
		FrameCBnum--;
	if (GetAsyncKeyState('J') & 1) //+
		FrameCBnum++;
	if (GetAsyncKeyState(0x38) & 1) //8 reset, set to 0
		FrameCBnum = 0;
	if (FrameCBnum < 0)
		FrameCBnum = 0;

	//bruteforce matProjnum
	if (GetAsyncKeyState('N') & 1) //-
		matProjnum--;
	if (GetAsyncKeyState('M') & 1) //+
		matProjnum++;
	if (GetAsyncKeyState(0x39) & 1) //9 reset, set to 0
		matProjnum = 0;
	if (matProjnum < 0)
		matProjnum = 0;

	ID3D11Buffer* pObjectCB;
	ID3D11Buffer* m_pCurObjectCB;
	pContext->VSGetConstantBuffers(ObjectCBnum, 1, &pObjectCB);//2works (UT4)

	if (pObjectCB != NULL)
	{
		m_pCurObjectCB = CopyBufferToCpu(pObjectCB);

		float matWorldView[4][4];
		{
			float* pObjectCB;
			MapBuffer(m_pCurObjectCB, (void**)&pObjectCB, NULL);
			memcpy(matWorldView, &pObjectCB[0], sizeof(matWorldView));
			UnmapBuffer(m_pCurObjectCB);
		}
		Vec3 v;
		vWorldView = Vec3MulMat4x4(v, matWorldView);//
	}
	else
	{
		Log("Object CB is not set == %x", (void*)pObjectCB);
		if (pObjectCB == NULL)
			SAFE_RELEASE(pObjectCB);
		return;
	}
	//SAFE_RELEASE(pObjectCB);


	ID3D11Buffer* pFrameCB;
	ID3D11Buffer* m_pCurFrameCB;
	pContext->VSGetConstantBuffers(FrameCBnum, 1, &pFrameCB);//1works (UT4)
	if (pFrameCB != NULL)
	{
		m_pCurFrameCB = CopyBufferToCpu(pFrameCB);

		float matProj[4][4];
		{
			float* pFrameCB;
			MapBuffer(m_pCurFrameCB, (void**)&pFrameCB, NULL);
			memcpy(matProj, &pFrameCB[matProjnum], sizeof(matProj));//16! pFrameCB[countnum]
			//matProj[0][3] = 0;
			UnmapBuffer(m_pCurFrameCB);
		}
		vClip = Vec4MulMat4x4(vWorldView, matProj);//
	}
	else
	{
		Log("Frame CB is not set == %x", (void*)pFrameCB);
		if (pFrameCB != NULL)
			SAFE_RELEASE(pFrameCB);
		return;
	}
	//SAFE_RELEASE(pFrameCB);


	Vec2 o;
	UINT vps = 1;
	D3D11_VIEWPORT viewport;
	pContext->RSGetViewports(&vps, &viewport);
	float VWidth = viewport.Width / 2.0f;
	float VHeight = viewport.Height / 2.0f;
	
	//o.x = ((vClip.x / vClip.w) * (viewport.Width / 2.0f)) + viewport.TopLeftX + (viewport.Width / 2.0f);
	//o.y = viewport.TopLeftY + (viewport.Height / 2.0f) - ((vClip.y / vClip.w) * (viewport.Height / 2.0f));

	//o.x = VWidth * (1 + (vClip.x / VWidth / vClip.z));
	//o.y = VHeight * (1 - (vClip.y / VHeight / vClip.z));

	o.x = VWidth + VWidth * (vClip.x / vClip.w);
	o.y = VHeight + VHeight * -(vClip.y / vClip.w);

	AimEspInfo_t pAimEspInfo = { static_cast<float>(o.x), static_cast<float>(o.y), iTeam };
	AimEspInfo.push_back(pAimEspInfo);
}