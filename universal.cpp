//ut4 multihack by n7 (w2s finder)

#include <Windows.h>
#include <vector>
#include <memory>
#include <d3d11.h>
#include <D3Dcompiler.h>//generateshader
#pragma comment(lib, "D3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "winmm.lib") //timeGetTime
#include "MinHook/include/MinHook.h" //detour x86&x64
#include "FW1FontWrapper/FW1FontWrapper.h" //font
#pragma warning( disable : 4244 )


typedef HRESULT(__stdcall *D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall *D3D11ResizeBuffersHook) (IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
typedef void(__stdcall *D3D11IASetVertexBuffersHook) (ID3D11DeviceContext* pContext, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppVertexBuffers, const UINT *pStrides, const UINT *pOffsets);
typedef void(__stdcall *D3D11DrawIndexedHook) (ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
typedef void(__stdcall *D3D11PSSetShaderResourcesHook) (ID3D11DeviceContext* pContext, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews);
typedef void(__stdcall *D3D11CreateQueryHook) (ID3D11Device* pDevice, const D3D11_QUERY_DESC *pQueryDesc, ID3D11Query **ppQuery);

typedef void(__stdcall *D3D11DrawIndexedInstancedHook) (ID3D11DeviceContext* pContext, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
typedef void(__stdcall *D3D11DrawHook) (ID3D11DeviceContext* pContext, UINT VertexCount, UINT StartVertexLocation);
typedef void(__stdcall *D3D11DrawInstancedHook) (ID3D11DeviceContext* pContext, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
typedef void(__stdcall *D3D11DrawInstancedIndirectHook) (ID3D11DeviceContext* pContext, ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs);
typedef void(__stdcall *D3D11DrawIndexedInstancedIndirectHook) (ID3D11DeviceContext* pContext, ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs);
typedef void(__stdcall *D3D11VSSetConstantBuffersHook) (ID3D11DeviceContext* pContext, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers);
typedef void(__stdcall *D3D11PSSetSamplersHook) (ID3D11DeviceContext* pContext, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers);


D3D11PresentHook phookD3D11Present = NULL;
D3D11ResizeBuffersHook phookD3D11ResizeBuffers = NULL;
D3D11IASetVertexBuffersHook phookD3D11IASetVertexBuffers = NULL;
D3D11DrawIndexedHook phookD3D11DrawIndexed = NULL;
D3D11PSSetShaderResourcesHook phookD3D11PSSetShaderResources = NULL;
D3D11CreateQueryHook phookD3D11CreateQuery = NULL;

D3D11DrawIndexedInstancedHook phookD3D11DrawIndexedInstanced = NULL;
D3D11DrawHook phookD3D11Draw = NULL;
D3D11DrawInstancedHook phookD3D11DrawInstanced = NULL;
D3D11DrawInstancedIndirectHook phookD3D11DrawInstancedIndirect = NULL;
D3D11DrawIndexedInstancedIndirectHook phookD3D11DrawIndexedInstancedIndirect = NULL;
D3D11VSSetConstantBuffersHook phookD3D11VSSetConstantBuffers = NULL;
D3D11PSSetSamplersHook phookD3D11PSSetSamplers = NULL;


ID3D11Device *pDevice = NULL;
ID3D11DeviceContext *pContext = NULL;

DWORD_PTR* pSwapChainVtable = NULL;
DWORD_PTR* pContextVTable = NULL;
DWORD_PTR* pDeviceVTable = NULL;

IFW1Factory *pFW1Factory = NULL;
IFW1FontWrapper *pFontWrapper = NULL;

#include "main.h" //helper funcs
#include "renderer.h" //renderer funcs
std::unique_ptr<Renderer> renderer;

//==========================================================================================================================

HRESULT __stdcall hookD3D11ResizeBuffers(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	if (RenderTargetView != NULL)
		SAFE_RELEASE(RenderTargetView);

	return phookD3D11ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

//==========================================================================================================================

HRESULT __stdcall hookD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (firstTime)
	{
		firstTime = false;

		//PlaySoundA(GetDirectoryFile("something.wav"), 0, SND_FILENAME | SND_ASYNC | SND_NOSTOP | SND_NODEFAULT);

		//get device
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void **)&pDevice)))
		{
			pSwapChain->GetDevice(__uuidof(pDevice), (void**)&pDevice);
			pDevice->GetImmediateContext(&pContext);
		}

		//create font
		hr = FW1CreateFactory(FW1_VERSION, &pFW1Factory);
		if (FAILED(hr)) { Log("Failed FW1CreateFactory"); return hr; }
		hr = pFW1Factory->CreateFontWrapper(pDevice, L"Tahoma", &pFontWrapper);
		if (FAILED(hr)) { Log("Failed CreateFontWrapper"); return hr; }
		pFW1Factory->Release();

		//sample state
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory(&sampDesc, sizeof(sampDesc));
		sampDesc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC; 
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		sampDesc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sampDesc.BorderColor[0] = 0.0f;
		sampDesc.BorderColor[1] = 255.0f;
		sampDesc.BorderColor[2] = 0.0f;
		sampDesc.BorderColor[3] = 0.0f;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		pDevice->CreateSamplerState(&sampDesc, &pSamplerState);

		//create green texture
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //format
		static const uint32_t s_pixel = 0xff00ff00; //0xffffffff white, 0xff00ff00 green, 0xffff0000 blue, 0xff0000ff red, 0x00000000
		D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };
		D3D11_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.Width = desc.Height = desc.MipLevels = desc.ArraySize = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;// D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;// D3D11_BIND_SHADER_RESOURCE;
		pDevice->CreateTexture2D(&desc, &initData, &texGreen);

		//create red texture
		static const uint32_t s_pixelr = 0xff0000ff; //0xffffffff white, 0xff00ff00 green, 0xffff0000 blue, 0xff0000ff red, 0x00000000
		D3D11_SUBRESOURCE_DATA initDatar = { &s_pixelr, sizeof(uint32_t), 0 };
		D3D11_TEXTURE2D_DESC descr;
		memset(&descr, 0, sizeof(descr));
		descr.Width = descr.Height = descr.MipLevels = descr.ArraySize = 1;
		descr.Format = format;
		descr.SampleDesc.Count = 1;
		descr.Usage = D3D11_USAGE_DEFAULT;// D3D11_USAGE_IMMUTABLE;
		descr.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;// D3D11_BIND_SHADER_RESOURCE;
		pDevice->CreateTexture2D(&descr, &initDatar, &texRed);

		//create green shaderresourceview
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		memset(&SRVDesc, 0, sizeof(SRVDesc));
		SRVDesc.Format = format;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;
		pDevice->CreateShaderResourceView(texGreen, &SRVDesc, &texSRVg);
		texGreen->Release();

		//create red shaderresourceview
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDescr;
		memset(&SRVDescr, 0, sizeof(SRVDescr));
		SRVDescr.Format = format;
		SRVDescr.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDescr.Texture2D.MipLevels = 1;
		pDevice->CreateShaderResourceView(texRed, &SRVDescr, &texSRVr);
		texRed->Release();

		//renderer
		renderer = std::make_unique<Renderer>(pDevice);

		//load cfg settings
		LoadCfg();
	}


	//create rendertarget
	if (RenderTargetView == NULL)
	{
		//Log("called");

		//viewport
		pContext->RSGetViewports(&vps, &viewport);
		ScreenCenterX = viewport.Width / 2.0f;
		ScreenCenterY = viewport.Height / 2.0f;

		ID3D11Texture2D* backbuffer = NULL;
		hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backbuffer);
		if (FAILED(hr)) {
			Log("Failed to get BackBuffer");
			pContext->OMGetRenderTargets(1, &RenderTargetView, NULL);
			pContext->OMSetRenderTargets(1, &RenderTargetView, NULL);
			return hr;
		}
		
		hr = pDevice->CreateRenderTargetView(backbuffer, NULL, &RenderTargetView);
		backbuffer->Release();
		if (FAILED(hr)) {
			Log("Failed to get RenderTarget");
			return hr;
		}
	}
	else //call before you draw
		pContext->OMSetRenderTargets(1, &RenderTargetView, NULL);

	//draw
	//if (pFontWrapper)
	//pFontWrapper->DrawString(pContext, L"D3D11 Hook", 14, 16.0f, 16.0f, 0xffff1612, FW1_RESTORESTATE | FW1_ALIASED);

	if(performance_loss && pFontWrapper)
		pFontWrapper->DrawString(pContext, L"Performance loss, need better model rec", 14, 16.0f, 16.0f, 0xffff1612, FW1_RESTORESTATE | FW1_ALIASED);

	static DWORD lastTime = timeGetTime();
	DWORD timePassed = timeGetTime() - lastTime;
	if (timePassed>2000)
	{
		performance_loss = false;
		lastTime = timeGetTime();
	}

	//shaders
	if (!psRed)
		GenerateShader(pDevice, &psRed, 1.0f, 0.0f, 0.0f);

	if (!psGreen)
		GenerateShader(pDevice, &psGreen, 0.0f, 1.0f, 0.0f);

	
	//menu background
	renderer->begin();
	if (Visible)
		renderer->drawOutlinedRect(Vec4(100, 100, 230, 196), 1.f, Color{ 0.9f, 0.9f, 0.15f, 0.95f }, Color{ 0.f , 1.f, 1.f, 0.5f }); //182 = 10 entries, 196 = 11
	renderer->draw();
	renderer->end();

	//menu
	if (IsReady() == false)
	{
		Init_Menu(pContext, L"D3D11 Menu", 100, 100);
		Do_Menu();
		Color_Font = 0xFFFFFFFF;//white
		Color_Off = 0xFFFF0000;//red
		Color_On = 0xFF00FF00;//green
		Color_Folder = 0xFF2F4F4F;//grey
		Color_Current = 0xFF00BFFF;//orange
	}
	Draw_Menu();
	Navigation();

	renderer->begin();
	//dot crosshair
	if(sOptions[9].Function == 1)
	renderer->drawOutlinedRect(Vec4(viewport.Width/2, viewport.Height/2, 3, 2), 1.f, Color{ 1.0f, 1.0f, 1.0f, 1.0f }, Color{ 1.f , 1.f, 1.f, 1.2f });

	if (sOptions[2].Function == 1) //if esp is enabled in menu
	if (AimEspInfo.size() != NULL)
	{
		for (unsigned int i = 0; i < AimEspInfo.size(); i++)
		{
			if (AimEspInfo[i].vOutX > 1 && AimEspInfo[i].vOutY > 1 && AimEspInfo[i].vOutX < viewport.Width && AimEspInfo[i].vOutY < viewport.Height)
			{
				//draw box
				renderer->drawOutlinedRect(Vec4(AimEspInfo[i].vOutX - 7, AimEspInfo[i].vOutY, 12, 12), 1.f, Color{ 0.9f, 0.9f, 0.15f, 0.95f }, Color{ 0.f , 0.f, 0.f, 0.2f });
				//renderer->drawOutlinedRect(Vec4(AimEspInfo[i].vOutX - 25.0f, AimEspInfo[i].vOutY, 50, 50), 1.f, Color{ 0.9f, 0.9f, 0.15f, 0.95f }, Color{ 0.f , 0.f, 0.f, 0.2f });

				//draw text
				if (pFontWrapper)
				pFontWrapper->DrawString(pContext, L"*", 14, AimEspInfo[i].vOutX, AimEspInfo[i].vOutY, 0xFF00FF00, FW1_RESTORESTATE| FW1_CENTER | FW1_ALIASED);

				//distance esp
				wchar_t reportValueS[256];
				swprintf_s(reportValueS, L"%d", (int)AimEspInfo[i].vOutZ);
				if (pFontWrapper)
					pFontWrapper->DrawString(pContext, reportValueS, 14.0f, AimEspInfo[i].vOutX, AimEspInfo[i].vOutY+viewport.Height/50.0f, 0xffffffff, FW1_RESTORESTATE | FW1_CENTER | FW1_ALIASED);
			}
		}
	}
	renderer->draw();
	renderer->end();

	//RMouse|LMouse|Shift|Ctrl|Alt|Space|X|C
	if (sOptions[4].Function == 0) Daimkey = VK_RBUTTON;
	if (sOptions[4].Function == 1) Daimkey = VK_LBUTTON;
	if (sOptions[4].Function == 2) Daimkey = VK_SHIFT;
	if (sOptions[4].Function == 3) Daimkey = VK_CONTROL;
	if (sOptions[4].Function == 4) Daimkey = VK_MENU;
	if (sOptions[4].Function == 5) Daimkey = VK_SPACE;
	if (sOptions[4].Function == 6) Daimkey = 0x58; //X
	if (sOptions[4].Function == 7) Daimkey = 0x43; //C
	aimheight = sOptions[7].Function;//aimheight
	preaim = sOptions[10].Function;//preaim
	
	//aimbot
	if(sOptions[3].Function == 1) //if aimbot is enabled in menu
	//if (AimEspInfo.size() != NULL && GetAsyncKeyState(Daimkey) & 0x8000)//warning: GetAsyncKeyState here would cause aimbot not to work for a few people
	if (AimEspInfo.size() != NULL)
	{
		UINT BestTarget = -1;
		DOUBLE fClosestPos = 99999;

		for (unsigned int i = 0; i < AimEspInfo.size(); i++)
		{
			//aimfov
			float radiusx = (sOptions[6].Function*10.0f) * (ScreenCenterX / 100.0f);
			float radiusy = (sOptions[6].Function*10.0f) * (ScreenCenterY / 100.0f);

			//get crosshairdistance
			AimEspInfo[i].CrosshairDst = GetDst(AimEspInfo[i].vOutX, AimEspInfo[i].vOutY, ScreenCenterX, ScreenCenterY);

			//aim at team 1 or 2 (not needed)
			//if (aimbot == AimEspInfo[i].iTeam)

			//if in fov
			if (AimEspInfo[i].vOutX >= ScreenCenterX - radiusx && AimEspInfo[i].vOutX <= ScreenCenterX + radiusx && AimEspInfo[i].vOutY >= ScreenCenterY - radiusy && AimEspInfo[i].vOutY <= ScreenCenterY + radiusy)

				//get closest/nearest target to crosshair
				if (AimEspInfo[i].CrosshairDst < fClosestPos)
				{
					fClosestPos = AimEspInfo[i].CrosshairDst;
					BestTarget = i;
				}
		}

		//if nearest target to crosshair
		if (BestTarget != -1)
		{
			double DistX = AimEspInfo[BestTarget].vOutX - ScreenCenterX;
			double DistY = AimEspInfo[BestTarget].vOutY - ScreenCenterY;

			//aimsens
			DistX /= (1.0f + (sOptions[5].Function*0.5f));
			DistY /= (1.0f + (sOptions[5].Function*0.5f));

			//aim
			if(GetAsyncKeyState(Daimkey) & 0x8000)
			mouse_event(MOUSEEVENTF_MOVE, (float)DistX, (float)DistY, 0, NULL);

			//autoshoot on
			//if ((!GetAsyncKeyState(VK_LBUTTON) && (*sOptions[8].Function))) //
			if ((!GetAsyncKeyState(VK_LBUTTON) && (sOptions[8].Function==1) && (GetAsyncKeyState(Daimkey) & 0x8000)))
			{
				if (!IsPressed)
				{
					IsPressed = true;
					mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
				}
			}
		}
	}
	AimEspInfo.clear();

	//autoshoot off
	if (sOptions[8].Function==1 && IsPressed)
	{
		if (timeGetTime() - astime >= asdelay)//sOptions[x].Function*100
		{
			IsPressed = false;
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			astime = timeGetTime();
		}
	}


	//logger
	if (logger && pFontWrapper) //&& countnum >= 0)
	{
		wchar_t reportValue[256];
		swprintf_s(reportValue, L"(Keys:-O P+ I=Log) countnum = %d", countnum);
		pFontWrapper->DrawString(pContext, reportValue, 16.0f, 320.0f, 100.0f, 0xff00ff00, FW1_RESTORESTATE);

		wchar_t reportValueA[256];
		swprintf_s(reportValueA, L"(Keys:-Z U+) WorldViewCBnum = %d", WorldViewCBnum);
		pFontWrapper->DrawString(pContext, reportValueA, 16.0f, 320.0f, 120.0f, 0xffffffff, FW1_RESTORESTATE);

		wchar_t reportValueB[256];
		swprintf_s(reportValueB, L"(Keys:-H J+) ProjCBnum = %d", ProjCBnum);
		pFontWrapper->DrawString(pContext, reportValueB, 16.0f, 320.0f, 140.0f, 0xffffffff, FW1_RESTORESTATE);

		wchar_t reportValueC[256];
		swprintf_s(reportValueC, L"(Keys:-N M+) matProjnum = %d", matProjnum);
		pFontWrapper->DrawString(pContext, reportValueC, 16.0f, 320.0f, 160.0f, 0xffffffff, FW1_RESTORESTATE);

		pFontWrapper->DrawString(pContext, L"F9 = log drawfunc", 16.0f, 320.0f, 180.0f, 0xffffffff, FW1_RESTORESTATE);
	}
	
	return phookD3D11Present(pSwapChain, SyncInterval, Flags);
}

//==========================================================================================================================

void __stdcall hookD3D11IASetVertexBuffers(ID3D11DeviceContext* pContext, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppVertexBuffers, const UINT *pStrides, const UINT *pOffsets)
{
	//Stride = *pStrides;

	return phookD3D11IASetVertexBuffers(pContext, StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}

//==========================================================================================================================

void __stdcall hookD3D11DrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	//if (GetAsyncKeyState(VK_F9) & 1)
		//Log("DrawIndexed called");

	if (firstTime2)
	{
		firstTime2 = false;

		//create depthstencilstate
		hr = pDevice->CreateDepthStencilState(&Desc, &pHackedDepthStencilState);
		if (FAILED(hr)) { Log("Failed to CreateDepthStencilState pHackedDepthStencilState"); }

		////////////////////////////////////////////////////////////////////////
		// DEPTH TRUE
		D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
		//
		// Depth state:
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
		//
		// Stencil state:
		depthStencilDesc.StencilEnable = true;
		depthStencilDesc.StencilReadMask = 0xFF;
		depthStencilDesc.StencilWriteMask = 0xFF;
		//
		// Stencil operations if pixel is front-facing:
		depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		//
		// Stencil operations if pixel is back-facing:
		depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		//
		pDevice->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
		////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////
		// DEPTH FALSE
		D3D11_DEPTH_STENCIL_DESC depthStencilDescfalse = {};
		//
		// Depth state:
		depthStencilDescfalse.DepthEnable = false;
		depthStencilDescfalse.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthStencilDescfalse.DepthFunc = D3D11_COMPARISON_LESS;
		//
		// Stencil state:
		depthStencilDescfalse.StencilEnable = true;
		depthStencilDescfalse.StencilReadMask = 0xFF;
		depthStencilDescfalse.StencilWriteMask = 0xFF;
		//
		// Stencil operations if pixel is front-facing:								//default:					test:
		depthStencilDescfalse.FrontFace.StencilFailOp = D3D11_STENCIL_OP_DECR;		//D3D11_STENCIL_OP_KEEP;	<-
		depthStencilDescfalse.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;	//D3D11_STENCIL_OP_INCR;	<-
		depthStencilDescfalse.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR;		//D3D11_STENCIL_OP_KEEP;	<-
		depthStencilDescfalse.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		//
		// Stencil operations if pixel is back-facing:
		depthStencilDescfalse.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;		//D3D11_STENCIL_OP_KEEP;	<-
		depthStencilDescfalse.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;	//D3D11_STENCIL_OP_DECR;	<-
		depthStencilDescfalse.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;		//D3D11_STENCIL_OP_KEEP;	<-
		depthStencilDescfalse.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		//
		pDevice->CreateDepthStencilState(&depthStencilDescfalse, &depthStencilStatefalse);
		////////////////////////////////////////////////////////////////////////

		//wireframe
		D3D11_RASTERIZER_DESC rwDesc;
		pContext->RSGetState(&rwState); // retrieve the current state
		if (rwState != NULL) {
			rwState->GetDesc(&rwDesc);
			rwDesc.FillMode = D3D11_FILL_WIREFRAME;
			rwDesc.CullMode = D3D11_CULL_NONE;
		}
		else { Log("No RS state set, defaults are in use"); }
		// create a whole new rasterizer state
		hr = pDevice->CreateRasterizerState(&rwDesc, &rwState);
		if (FAILED(hr)) { Log("Failed to CreateRasterizerState rwDesc"); }

		//solid
		D3D11_RASTERIZER_DESC rsDesc;
		pContext->RSGetState(&rsState); // retrieve the current state
		if (rsState != NULL) {
		rsState->GetDesc(&rsDesc);    // get the desc of the state
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_BACK;
		}
		else { Log("No RS state set, defaults are in use"); }
		// create a whole new rasterizer state
		pDevice->CreateRasterizerState(&rsDesc, &rsState);
		if (FAILED(hr)) { Log("Failed to CreateRasterizerState rsDesc"); }
	}

	//get stride & vedesc.ByteWidth
	pContext->IAGetVertexBuffers(0, 1, &veBuffer, &Stride, &veBufferOffset);
	if (veBuffer != NULL)
		veBuffer->GetDesc(&vedesc);
	if (veBuffer != NULL) { veBuffer->Release(); veBuffer = NULL; }
	
	//get indesc.ByteWidth
	pContext->IAGetIndexBuffer(&inBuffer, &inFormat, &inOffset);
	if (inBuffer != NULL)
		inBuffer->GetDesc(&indesc);
	if (inBuffer != NULL) { inBuffer->Release(); inBuffer = NULL; }
	
	//get pscdesc.ByteWidth
	pContext->PSGetConstantBuffers(pscStartSlot, 1, &pscBuffer);
	if (pscBuffer != NULL)
		pscBuffer->GetDesc(&pscdesc);
	if (pscBuffer != NULL) { pscBuffer->Release(); pscBuffer = NULL; }

	//get vsdesc.ByteWidth
	//pContext->VSGetConstantBuffers(vsStartSlot, 1, &vsBuffer);
	//if (vsBuffer!=NULL)
		//vsBuffer->GetDesc(&vsdesc);
	//if (vsBuffer != NULL) { vsBuffer->Release(); vsBuffer = NULL; }

	
	//wallhack/chams
	if (sOptions[0].Function == 1 || sOptions[1].Function == 1) //if wallhack/chams option is enabled in menu
	if (countnum == Stride)
		{
			pContext->OMGetDepthStencilState(&pDepthStencilState, &stencilRef);
			if (pDepthStencilState != NULL)
			{
				pDepthStencilState->GetDesc(&Desc);
				if (Desc.DepthEnable == TRUE)// && Desc.DepthFunc == D3D11_COMPARISON_GREATER_EQUAL)
				{
					//disable depth
					pContext->OMSetDepthStencilState(depthStencilStatefalse, 1);

					//pssetshader chams
					if (sOptions[1].Function == 1)
					pContext->PSSetShader(psRed, NULL, NULL);

					//red texture chams (for games where shader turns transparent)
					//if (sOptions[1].Function == 1)
					//{
						//for (int x1 = 0; x1 <= 10; x1++)
						//{
							//pContext->PSSetShaderResources(x1, 1, &texSRVr);
						//}
						//pContext->PSSetSamplers(0, 1, &pSamplerState);
					//}
					
					//SAFE_RELEASE(depthStencilStatefalse);//bad: breaks chams, good: wallhack goes through
				}
			}

			phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);

			//if (pDepthStencilState != NULL && Desc.DepthEnable == FALSE)
			//{
			// restore depth stencil state.
			pContext->OMSetDepthStencilState(pDepthStencilState, stencilRef);

			if (sOptions[1].Function == 1)
			pContext->PSSetShader(psGreen, NULL, NULL);
			
			//green texture chams (only use in games where shader turns transparent)
			//if (sOptions[1].Function == 1)
			//{
				//for (int y1 = 0; y1 <= 10; y1++)
				//{
					//pContext->PSSetShaderResources(y1, 1, &texSRVg);
				//}
				//pContext->PSSetSamplers(0, 1, &pSamplerState);
			//}
			
			//}
			SAFE_RELEASE(pDepthStencilState);
		}
		

	//esp/aimbot 
	if ((sOptions[2].Function==1) || (sOptions[3].Function==1)) //if esp/aimbot option is enabled in menu
		if (countnum == Stride)
		/*
		if ( //unreal
			(Stride == 32 && IndexCount == 10155 && indesc.ByteWidth == 24264) ||
			(Stride == 44 && IndexCount == 11097 && indesc.ByteWidth == 24420) ||
			(Stride == 40 && IndexCount == 11412 && indesc.ByteWidth == 24366) ||
			(Stride == 40 && IndexCount == 11487 && indesc.ByteWidth == 24762) ||
			(Stride == 44 && IndexCount == 83262 && indesc.ByteWidth == 197496) ||
			(Stride == 40 && IndexCount == 23283 && indesc.ByteWidth == 49518)
			)
		*/
	{
		//if (GetAsyncKeyState(VK_F9) & 1) //log pscdesc.ByteWidth
		//Log("Stride == %d && IndexCount == %d && indesc.ByteWidth == %d && vedesc.ByteWidth == %d && pscdesc.ByteWidth == %d", Stride, IndexCount, indesc.ByteWidth, vedesc.ByteWidth, pscdesc.ByteWidth);

		pContext->OMGetDepthStencilState(&pDepthStencilState, &stencilRef);
		if (pDepthStencilState != NULL)
		{
			D3D11_DEPTH_STENCIL_DESC Desc;
			pDepthStencilState->GetDesc(&Desc);

			//if (GetAsyncKeyState(VK_F10) & 1) //log Desc.StencilWriteMask
			//Log("Stride == %d && IndexCount == %d && indesc.ByteWidth == %d && vedesc.ByteWidth == %d && Desc.StencilWriteMask == %d", Stride, IndexCount, indesc.ByteWidth, vedesc.ByteWidth, Desc.StencilWriteMask);

		//	if (Desc.StencilWriteMask == 0xFF)//for better performance, WARNING: this can be 0x0 
		//	{
				AddModel(pContext);//w2s
				//pContext->Flush();
		//	}
		}
		SAFE_RELEASE(pDepthStencilState);
	}

	
	//removal
	//if (sOptions[xx].Function == 1)
	//if (Stride == xx)
		//return;


	//small bruteforce logger
	//ALT + CTRL + L toggles logger
	if (logger)
	{
		//if ((Stride == 44 ||Stride == 40) && (GetAsyncKeyState(VK_F10) & 1))//press f10 to log most models on screen
			//Log("Stride == %d && IndexCount == %d && indesc.ByteWidth == %d && vedesc.ByteWidth == %d && Descr.Format == %d && pscdesc.ByteWidth == %d && Descr.Buffer.NumElements == %d && vsStartSlot == %d && psStartSlot == %d && vsdesc.ByteWidth == %d && texdesc.Format == %d && texdesc.Height == %d && texdesc.Width == %d", Stride, IndexCount, indesc.ByteWidth, vedesc.ByteWidth, Descr.Format, pscdesc.ByteWidth, Descr.Buffer.NumElements, vsStartSlot, psStartSlot, vsdesc.ByteWidth, texdesc.Format, texdesc.Height, texdesc.Width);

		if (countnum == Stride)//IndexCount/100)
			if (GetAsyncKeyState('I') & 1)
				Log("Stride == %d && IndexCount == %d && indesc.ByteWidth == %d && vedesc.ByteWidth == %d && Descr.Format == %d && pscdesc.ByteWidth == %d && Descr.Buffer.NumElements == %d && vsStartSlot == %d && psStartSlot == %d && vsdesc.ByteWidth == %d && texdesc.Format == %d && texdesc.Height == %d && texdesc.Width == %d", Stride, IndexCount, indesc.ByteWidth, vedesc.ByteWidth, Descr.Format, pscdesc.ByteWidth, Descr.Buffer.NumElements, vsStartSlot, psStartSlot, vsdesc.ByteWidth, texdesc.Format, texdesc.Height, texdesc.Width);

		if (countnum == Stride)//IndexCount / 100)
		{
			//delete texture
			return;
		}
	}

    return phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
}

//==========================================================================================================================

void __stdcall hookD3D11PSSetShaderResources(ID3D11DeviceContext* pContext, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
	pssrStartSlot = StartSlot;

	/*
	//texture stuff usually not needed
	for (UINT j = 0; j < NumViews; j++)
	{
		//resources loop
		ID3D11ShaderResourceView* pShaderResView = ppShaderResourceViews[j];
		if (pShaderResView)
		{
			pShaderResView->GetDesc(&Descr);

			//unstable in some games
			//ID3D11Resource *Resource;
			//pShaderResView->GetResource(&Resource);
			//ID3D11Texture2D *Texture = (ID3D11Texture2D *)Resource;
			//Texture->GetDesc(&texdesc);
			
			//if (texdesc.Usage == D3D11_USAGE::D3D11_USAGE_DYNAMIC) // COLORBUFFER
			//{
		
			//}
			//else if (texdesc.Usage == D3D11_USAGE::D3D11_USAGE_DEFAULT) // RENDERTARGETS
			//{
				
			//}
			
			//SAFE_RELEASE(Resource);
			//SAFE_RELEASE(Texture);
		}
	}
	*/

	//logger
	if (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(0x4C) & 1) //ALT + CTRL + L toggles logger
		logger = !logger;
	if (logger && pFontWrapper) //&& countnum >= 0)
	{
		//hold down P key until a texture is wallhacked, press I to log values of those textures
		if (GetAsyncKeyState('O') & 1) //-
			countnum--;
		if (GetAsyncKeyState('P') & 1) //+
			countnum++;
		if (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState('9') & 1) //reset, set to -1
			countnum = -1;

		//bruteforce WorldViewCBnum
		if (GetAsyncKeyState('Z') & 1) //-
			WorldViewCBnum--;
		if (GetAsyncKeyState('U') & 1) //+
			WorldViewCBnum++;
		if (GetAsyncKeyState(0x37) & 1) //7 reset, set to 0
			WorldViewCBnum = 0;
		if (WorldViewCBnum < 0)
			WorldViewCBnum = 0;

		//bruteforce ProjCBnum
		if (GetAsyncKeyState('H') & 1) //-
			ProjCBnum--;
		if (GetAsyncKeyState('J') & 1) //+
			ProjCBnum++;
		if (GetAsyncKeyState(0x38) & 1) //8 reset, set to 0
			ProjCBnum = 0;
		if (ProjCBnum < 0)
			ProjCBnum = 0;

		//bruteforce matProjnum
		if (GetAsyncKeyState('N') & 1) //-
			matProjnum--;
		if (GetAsyncKeyState('M') & 1) //+
			matProjnum++;
		if (GetAsyncKeyState(0x39) & 1) //9 reset, set to 0
			matProjnum = 0;
		if (matProjnum < 0)
			matProjnum = 0;
	}

	/*
	//alternative wallhack example for f'up games, only use this if no draw function works for wallhack
	if (pssrStartSlot == 1) //if black screen, find correct pssrStartSlot
		pContext->OMSetDepthStencilState(depthStencilStatefalse, 1);
	if (pscdesc.ByteWidth == 224 && Descr.Format == 71) //models in Tom Clancys Rainbow Six Siege old version
	{
		SetDepthStencilState(DISABLED);
		//pContext->PSSetShader(psRed, NULL, NULL);
	}
	else if (pssrStartSlot == 1) //if black screen, find correct pssrStartSlot
		pContext->OMSetDepthStencilState(depthStencilState, 1);
	*/

	return phookD3D11PSSetShaderResources(pContext, StartSlot, NumViews, ppShaderResourceViews);
}

//==========================================================================================================================

void __stdcall hookD3D11CreateQuery(ID3D11Device* pDevice, const D3D11_QUERY_DESC *pQueryDesc, ID3D11Query **ppQuery)
{
	/*
	//this is required in some games for better wallhack (REDUCES FPS, not recommended)
	//disables Occlusion which prevents rendering player models through certain objects (used by wallhack to see models through walls at all distances)
	if (pQueryDesc->Query == D3D11_QUERY_OCCLUSION)
	{
		D3D11_QUERY_DESC oqueryDesc = CD3D11_QUERY_DESC();
		(&oqueryDesc)->MiscFlags = pQueryDesc->MiscFlags;
		(&oqueryDesc)->Query = D3D11_QUERY_TIMESTAMP;

		return phookD3D11CreateQuery(pDevice, &oqueryDesc, ppQuery);
	}
	*/
	return phookD3D11CreateQuery(pDevice, pQueryDesc, ppQuery);
}

//==========================================================================================================================

void __stdcall hookD3D11DrawIndexedInstanced(ID3D11DeviceContext* pContext, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{
	if (GetAsyncKeyState(VK_F9) & 1)
		Log("DrawIndexedInstanced called");
	
	return phookD3D11DrawIndexedInstanced(pContext, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

//==========================================================================================================================

void __stdcall hookD3D11Draw(ID3D11DeviceContext* pContext, UINT VertexCount, UINT StartVertexLocation)
{
	if (GetAsyncKeyState(VK_F9) & 1)
		Log("Draw called");

	return phookD3D11Draw(pContext, VertexCount, StartVertexLocation);
}

//==========================================================================================================================

void __stdcall hookD3D11DrawInstanced(ID3D11DeviceContext* pContext, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
{
	if (GetAsyncKeyState(VK_F9) & 1)
		Log("DrawInstanced called");

	return phookD3D11DrawInstanced(pContext, VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

//==========================================================================================================================

void __stdcall hookD3D11DrawInstancedIndirect(ID3D11DeviceContext* pContext, ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
	if (GetAsyncKeyState(VK_F9) & 1)
		Log("DrawInstancedIndirect called");

	return phookD3D11DrawInstancedIndirect(pContext, pBufferForArgs, AlignedByteOffsetForArgs);
}

//==========================================================================================================================

void __stdcall hookD3D11DrawIndexedInstancedIndirect(ID3D11DeviceContext* pContext, ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
	if (GetAsyncKeyState(VK_F9) & 1)
		Log("DrawIndexedInstancedIndirect called");

	return phookD3D11DrawIndexedInstancedIndirect(pContext, pBufferForArgs, AlignedByteOffsetForArgs);
}

//==========================================================================================================================

void __stdcall hookD3D11VSSetConstantBuffers(ID3D11DeviceContext* pContext, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
{
		vsStartSlot = StartSlot;

	return phookD3D11VSSetConstantBuffers(pContext, StartSlot, NumBuffers, ppConstantBuffers);
}

//==========================================================================================================================

void __stdcall hookD3D11PSSetSamplers(ID3D11DeviceContext* pContext, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
{
		psStartSlot = StartSlot;

	return phookD3D11PSSetSamplers(pContext, StartSlot, NumSamplers, ppSamplers);
}

//==========================================================================================================================

const int MultisampleCount = 1; // Set to 1 to disable multisampling
LRESULT CALLBACK DXGIMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){ return DefWindowProc(hwnd, uMsg, wParam, lParam); }
DWORD __stdcall InitializeHook(LPVOID)
{
	HMODULE hDXGIDLL = 0;
	do
	{
		hDXGIDLL = GetModuleHandle("dxgi.dll");
		Sleep(4000);
	} while (!hDXGIDLL);
	Sleep(100);

    IDXGISwapChain* pSwapChain;

	WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DXGIMsgProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
	RegisterClassExA(&wc);
	HWND hWnd = CreateWindowA("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);

	D3D_FEATURE_LEVEL requestedLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
	D3D_FEATURE_LEVEL obtainedLevel;
	ID3D11Device* d3dDevice = nullptr;
	ID3D11DeviceContext* d3dContext = nullptr;

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(scd));
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	scd.OutputWindow = hWnd;
	scd.SampleDesc.Count = MultisampleCount;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scd.Windowed = ((GetWindowLongPtr(hWnd, GWL_STYLE) & WS_POPUP) != 0) ? false : true;

	// LibOVR 0.4.3 requires that the width and height for the backbuffer is set even if
	// you use windowed mode, despite being optional according to the D3D11 documentation.
	scd.BufferDesc.Width = 1;
	scd.BufferDesc.Height = 1;
	scd.BufferDesc.RefreshRate.Numerator = 0;
	scd.BufferDesc.RefreshRate.Denominator = 1;

	UINT createFlags = 0;
#ifdef _DEBUG
	// This flag gives you some quite wonderful debug text. Not wonderful for performance, though!
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	IDXGISwapChain* d3dSwapChain = 0;

	if (FAILED(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createFlags,
		requestedLevels,
		sizeof(requestedLevels) / sizeof(D3D_FEATURE_LEVEL),
		D3D11_SDK_VERSION,
		&scd,
		&pSwapChain,
		&pDevice,
		&obtainedLevel,
		&pContext)))
	{
		MessageBox(hWnd, "Failed to create directX device and swapchain!", "Error", MB_ICONERROR);
		return NULL;
	}


    pSwapChainVtable = (DWORD_PTR*)pSwapChain;
    pSwapChainVtable = (DWORD_PTR*)pSwapChainVtable[0];

    pContextVTable = (DWORD_PTR*)pContext;
    pContextVTable = (DWORD_PTR*)pContextVTable[0];

	pDeviceVTable = (DWORD_PTR*)pDevice;
	pDeviceVTable = (DWORD_PTR*)pDeviceVTable[0];

	if (MH_Initialize() != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pSwapChainVtable[8], hookD3D11Present, reinterpret_cast<void**>(&phookD3D11Present)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pSwapChainVtable[8]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pSwapChainVtable[13], hookD3D11ResizeBuffers, reinterpret_cast<void**>(&phookD3D11ResizeBuffers)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pSwapChainVtable[13]) != MH_OK) { return 1; }

	//if (MH_CreateHook((DWORD_PTR*)pContextVTable[18], hookD3D11IASetVertexBuffers, reinterpret_cast<void**>(&phookD3D11IASetVertexBuffers)) != MH_OK) { return 1; }
	//if (MH_EnableHook((DWORD_PTR*)pContextVTable[18]) != MH_OK) { return 1; }
	
	if (MH_CreateHook((DWORD_PTR*)pContextVTable[12], hookD3D11DrawIndexed, reinterpret_cast<void**>(&phookD3D11DrawIndexed)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pContextVTable[12]) != MH_OK) { return 1; }
	
	if (MH_CreateHook((DWORD_PTR*)pContextVTable[8], hookD3D11PSSetShaderResources, reinterpret_cast<void**>(&phookD3D11PSSetShaderResources)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pContextVTable[8]) != MH_OK) { return 1; }
	
	if (MH_CreateHook((DWORD_PTR*)pDeviceVTable[24], hookD3D11CreateQuery, reinterpret_cast<void**>(&phookD3D11CreateQuery)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pDeviceVTable[24]) != MH_OK) { return 1; }

	if (MH_CreateHook((DWORD_PTR*)pContextVTable[20], hookD3D11DrawIndexedInstanced, reinterpret_cast<void**>(&phookD3D11DrawIndexedInstanced)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pContextVTable[20]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pContextVTable[13], hookD3D11Draw, reinterpret_cast<void**>(&phookD3D11Draw)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pContextVTable[13]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pContextVTable[21], hookD3D11DrawInstanced, reinterpret_cast<void**>(&phookD3D11DrawInstanced)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pContextVTable[21]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pContextVTable[40], hookD3D11DrawInstancedIndirect, reinterpret_cast<void**>(&phookD3D11DrawInstancedIndirect)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pContextVTable[40]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pContextVTable[39], hookD3D11DrawIndexedInstancedIndirect, reinterpret_cast<void**>(&phookD3D11DrawIndexedInstancedIndirect)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pContextVTable[39]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pContextVTable[7], hookD3D11VSSetConstantBuffers, reinterpret_cast<void**>(&phookD3D11VSSetConstantBuffers)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pContextVTable[7]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)pContextVTable[10], hookD3D11PSSetSamplers, reinterpret_cast<void**>(&phookD3D11PSSetSamplers)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pContextVTable[10]) != MH_OK) { return 1; }
	
    DWORD dwOld;
    VirtualProtect(phookD3D11Present, 2, PAGE_EXECUTE_READWRITE, &dwOld);

	while (true) {
		Sleep(10);
	}

	pDevice->Release();
	pContext->Release();
	pSwapChain->Release();

    return NULL;
}

//==========================================================================================================================

BOOL __stdcall DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{ 
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH: // A process is loading the DLL.
		DisableThreadLibraryCalls(hModule);
		GetModuleFileName(hModule, dlldir, 512);
		for (size_t i = strlen(dlldir); i > 0; i--) { if (dlldir[i] == '\\') { dlldir[i + 1] = 0; break; } }
		CreateThread(NULL, 0, InitializeHook, NULL, 0, NULL);
		break;

	case DLL_PROCESS_DETACH: // A process unloads the DLL.
		if (MH_Uninitialize() != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pSwapChainVtable[8]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pSwapChainVtable[13]) != MH_OK) { return 1; }
		//if (MH_DisableHook((DWORD_PTR*)pContextVTable[18]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pContextVTable[12]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pContextVTable[8]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pDeviceVTable[24]) != MH_OK) { return 1; }

		if (MH_DisableHook((DWORD_PTR*)pContextVTable[20]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pContextVTable[13]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pContextVTable[21]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pContextVTable[40]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pContextVTable[39]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pContextVTable[7]) != MH_OK) { return 1; }
		if (MH_DisableHook((DWORD_PTR*)pContextVTable[10]) != MH_OK) { return 1; }
		break;
	}
	return TRUE;
}

/*
// from d3d11.h. Simply walk the inheritance. In C++ the order of methods in a .h file is the order in the vtable, after the vtable of inherited
// types, so it's easy to determine what the location is without silly loggers.
// IUnknown
0	virtual HRESULT STDMETHODCALLTYPE QueryInterface
1	virtual ULONG STDMETHODCALLTYPE AddRef
2	virtual ULONG STDMETHODCALLTYPE Release
// ID3D11Device
3	virtual HRESULT STDMETHODCALLTYPE CreateBuffer
4	virtual HRESULT STDMETHODCALLTYPE CreateTexture1D
5	virtual HRESULT STDMETHODCALLTYPE CreateTexture2D
6	virtual HRESULT STDMETHODCALLTYPE CreateTexture3D
7	virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView
8	virtual HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView
9	virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView
10	virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilView
11	virtual HRESULT STDMETHODCALLTYPE CreateInputLayout
12	virtual HRESULT STDMETHODCALLTYPE CreateVertexShader
13	virtual HRESULT STDMETHODCALLTYPE CreateGeometryShader
14	virtual HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput
15	virtual HRESULT STDMETHODCALLTYPE CreatePixelShader
16	virtual HRESULT STDMETHODCALLTYPE CreateHullShader
17	virtual HRESULT STDMETHODCALLTYPE CreateDomainShader
18	virtual HRESULT STDMETHODCALLTYPE CreateComputeShader
19	virtual HRESULT STDMETHODCALLTYPE CreateClassLinkage
20	virtual HRESULT STDMETHODCALLTYPE CreateBlendState
21	virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilState
22	virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState
23	virtual HRESULT STDMETHODCALLTYPE CreateSamplerState
24	virtual HRESULT STDMETHODCALLTYPE CreateQuery
25	virtual HRESULT STDMETHODCALLTYPE CreatePredicate
26	virtual HRESULT STDMETHODCALLTYPE CreateCounter
27	virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext
28	virtual HRESULT STDMETHODCALLTYPE OpenSharedResource
29	virtual HRESULT STDMETHODCALLTYPE CheckFormatSupport
30	virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels
31	virtual void STDMETHODCALLTYPE CheckCounterInfo
32	virtual HRESULT STDMETHODCALLTYPE CheckCounter
33	virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport
34	virtual HRESULT STDMETHODCALLTYPE GetPrivateData
35	virtual HRESULT STDMETHODCALLTYPE SetPrivateData
36	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface
37	virtual D3D_FEATURE_LEVEL STDMETHODCALLTYPE GetFeatureLevel
38	virtual UINT STDMETHODCALLTYPE GetCreationFlags
39	virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason
40	virtual void STDMETHODCALLTYPE GetImmediateContext
41	virtual HRESULT STDMETHODCALLTYPE SetExceptionMode
42	virtual UINT STDMETHODCALLTYPE GetExceptionMode
---------------------------------------------------------------------------
// IUnknown
0	virtual HRESULT STDMETHODCALLTYPE QueryInterface
1	virtual ULONG STDMETHODCALLTYPE AddRef
2	virtual ULONG STDMETHODCALLTYPE Release
// ID3D11DeviceChild
3	virtual void STDMETHODCALLTYPE GetDevice
4	virtual HRESULT STDMETHODCALLTYPE GetPrivateData
5	virtual HRESULT STDMETHODCALLTYPE SetPrivateData
6	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface
// ID3D11DeviceContext
7	virtual void STDMETHODCALLTYPE VSSetConstantBuffers
8	virtual void STDMETHODCALLTYPE PSSetShaderResources
9	virtual void STDMETHODCALLTYPE PSSetShader
10	virtual void STDMETHODCALLTYPE PSSetSamplers
11	virtual void STDMETHODCALLTYPE VSSetShader
12	virtual void STDMETHODCALLTYPE DrawIndexed
13	virtual void STDMETHODCALLTYPE Draw
14	virtual HRESULT STDMETHODCALLTYPE Map
15	virtual void STDMETHODCALLTYPE Unmap
16	virtual void STDMETHODCALLTYPE PSSetConstantBuffers
17	virtual void STDMETHODCALLTYPE IASetInputLayout
18	virtual void STDMETHODCALLTYPE IASetVertexBuffers
19	virtual void STDMETHODCALLTYPE IASetIndexBuffer
20	virtual void STDMETHODCALLTYPE DrawIndexedInstanced
21	virtual void STDMETHODCALLTYPE DrawInstanced
22	virtual void STDMETHODCALLTYPE GSSetConstantBuffers
23	virtual void STDMETHODCALLTYPE GSSetShader
24	virtual void STDMETHODCALLTYPE IASetPrimitiveTopology
25	virtual void STDMETHODCALLTYPE VSSetShaderResources
26	virtual void STDMETHODCALLTYPE VSSetSamplers
27	virtual void STDMETHODCALLTYPE Begin
28	virtual void STDMETHODCALLTYPE End
29	virtual HRESULT STDMETHODCALLTYPE GetData
30	virtual void STDMETHODCALLTYPE SetPredication
31	virtual void STDMETHODCALLTYPE GSSetShaderResources
32	virtual void STDMETHODCALLTYPE GSSetSamplers
33	virtual void STDMETHODCALLTYPE OMSetRenderTargets
34	virtual void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews
35	virtual void STDMETHODCALLTYPE OMSetBlendState
36	virtual void STDMETHODCALLTYPE OMSetDepthStencilState
37	virtual void STDMETHODCALLTYPE SOSetTargets
38	virtual void STDMETHODCALLTYPE DrawAuto
39	virtual void STDMETHODCALLTYPE DrawIndexedInstancedIndirect
40	virtual void STDMETHODCALLTYPE DrawInstancedIndirect
41	virtual void STDMETHODCALLTYPE Dispatch
42	virtual void STDMETHODCALLTYPE DispatchIndirect
43	virtual void STDMETHODCALLTYPE RSSetState
44	virtual void STDMETHODCALLTYPE RSSetViewports
45	virtual void STDMETHODCALLTYPE RSSetScissorRects
46	virtual void STDMETHODCALLTYPE CopySubresourceRegion
47	virtual void STDMETHODCALLTYPE CopyResource
48	virtual void STDMETHODCALLTYPE UpdateSubresource
49	virtual void STDMETHODCALLTYPE CopyStructureCount
50	virtual void STDMETHODCALLTYPE ClearRenderTargetView
51	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint
52	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat
53	virtual void STDMETHODCALLTYPE ClearDepthStencilView
54	virtual void STDMETHODCALLTYPE GenerateMips
55	virtual void STDMETHODCALLTYPE SetResourceMinLOD
56	virtual FLOAT STDMETHODCALLTYPE GetResourceMinLOD
57	virtual void STDMETHODCALLTYPE ResolveSubresource
58	virtual void STDMETHODCALLTYPE ExecuteCommandList
59	virtual void STDMETHODCALLTYPE HSSetShaderResources
60	virtual void STDMETHODCALLTYPE HSSetShader
61	virtual void STDMETHODCALLTYPE HSSetSamplers
62	virtual void STDMETHODCALLTYPE HSSetConstantBuffers
63	virtual void STDMETHODCALLTYPE DSSetShaderResources
64	virtual void STDMETHODCALLTYPE DSSetShader
65	virtual void STDMETHODCALLTYPE DSSetSamplers
66	virtual void STDMETHODCALLTYPE DSSetConstantBuffers
67	virtual void STDMETHODCALLTYPE CSSetShaderResources
68	virtual void STDMETHODCALLTYPE CSSetUnorderedAccessViews
69	virtual void STDMETHODCALLTYPE CSSetShader
70	virtual void STDMETHODCALLTYPE CSSetSamplers
71	virtual void STDMETHODCALLTYPE CSSetConstantBuffers
72	virtual void STDMETHODCALLTYPE VSGetConstantBuffers
73	virtual void STDMETHODCALLTYPE PSGetShaderResources
74	virtual void STDMETHODCALLTYPE PSGetShader
75	virtual void STDMETHODCALLTYPE PSGetSamplers
76	virtual void STDMETHODCALLTYPE VSGetShader
77	virtual void STDMETHODCALLTYPE PSGetConstantBuffers
78	virtual void STDMETHODCALLTYPE IAGetInputLayout
79	virtual void STDMETHODCALLTYPE IAGetVertexBuffers
80	virtual void STDMETHODCALLTYPE IAGetIndexBuffer
81	virtual void STDMETHODCALLTYPE GSGetConstantBuffers
82	virtual void STDMETHODCALLTYPE GSGetShader
83	virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology
84	virtual void STDMETHODCALLTYPE VSGetShaderResources
85	virtual void STDMETHODCALLTYPE VSGetSamplers
86	virtual void STDMETHODCALLTYPE GetPredication
87	virtual void STDMETHODCALLTYPE GSGetShaderResources
88	virtual void STDMETHODCALLTYPE GSGetSamplers
89	virtual void STDMETHODCALLTYPE OMGetRenderTargets
90	virtual void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews
91	virtual void STDMETHODCALLTYPE OMGetBlendState
92	virtual void STDMETHODCALLTYPE OMGetDepthStencilState
93	virtual void STDMETHODCALLTYPE SOGetTargets
94	virtual void STDMETHODCALLTYPE RSGetState
95	virtual void STDMETHODCALLTYPE RSGetViewports
96	virtual void STDMETHODCALLTYPE RSGetScissorRects
97	virtual void STDMETHODCALLTYPE HSGetShaderResources
98	virtual void STDMETHODCALLTYPE HSGetShader
99	virtual void STDMETHODCALLTYPE HSGetSamplers
100	virtual void STDMETHODCALLTYPE HSGetConstantBuffers
101	virtual void STDMETHODCALLTYPE DSGetShaderResources
102	virtual void STDMETHODCALLTYPE DSGetShader
103	virtual void STDMETHODCALLTYPE DSGetSamplers
104	virtual void STDMETHODCALLTYPE DSGetConstantBuffers
105	virtual void STDMETHODCALLTYPE CSGetShaderResources
106	virtual void STDMETHODCALLTYPE CSGetUnorderedAccessViews
107	virtual void STDMETHODCALLTYPE CSGetShader
108	virtual void STDMETHODCALLTYPE CSGetSamplers
109	virtual void STDMETHODCALLTYPE CSGetConstantBuffers
110	virtual void STDMETHODCALLTYPE ClearState
111	virtual void STDMETHODCALLTYPE Flush
112	virtual D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType
113	virtual UINT STDMETHODCALLTYPE GetContextFlags
114	virtual HRESULT STDMETHODCALLTYPE FinishCommandList
-----------------------------------------------------------------------
// IUnknown
0	virtual HRESULT STDMETHODCALLTYPE QueryInterface
1	virtual ULONG STDMETHODCALLTYPE AddRef
2	virtual ULONG STDMETHODCALLTYPE Release
// IDXGIObject
3	virtual HRESULT STDMETHODCALLTYPE SetPrivateData
4	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface
5	virtual HRESULT STDMETHODCALLTYPE GetPrivateData
6	virtual HRESULT STDMETHODCALLTYPE GetParent
// IDXGIDeviceSubObject
7   virtual HRESULT STDMETHODCALLTYPE GetDevice
// IDXGISwapChain
8	virtual HRESULT STDMETHODCALLTYPE Present
9	virtual HRESULT STDMETHODCALLTYPE GetBuffer
10	virtual HRESULT STDMETHODCALLTYPE SetFullscreenState
11	virtual HRESULT STDMETHODCALLTYPE GetFullscreenState
12	virtual HRESULT STDMETHODCALLTYPE GetDesc
13	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers
14	virtual HRESULT STDMETHODCALLTYPE ResizeTarget
15	virtual HRESULT STDMETHODCALLTYPE GetContainingOutput
16	virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics
17	virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount
*/
