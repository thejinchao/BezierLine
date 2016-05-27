#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"

#include "bl_line.h"
#include "bl_helper.h"

#pragma warning( disable : 4100 )

using namespace DirectX;


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
CModelViewerCamera          g_Camera;               // A model viewing camera
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = nullptr;
CDXUTDialog                 g_HUD;                  // dialog for standard controls
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls

Bline g_Bline;
BlineHelper g_BlineHelper;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           2
#define IDC_CHANGEDEVICE        3
#define IDC_TOGGLEWARP          4

#define IDC_BUTTON_REBUILD          5
#define IDC_BUTTON_RESET_CAMERA		6
#define IDC_CHECKBOX_CONTROL_KEY	7
#define IDC_CHECKBOX_SEGMENT		8
#define IDC_CHECKBOX_LINE			9
#define IDC_CHECKBOX_TANGENT		10
#define IDC_CHECKBOX_BOUNDER		11

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
	DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
	return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
	pDeviceSettings->d3d11.sd.SampleDesc.Count = 4;

	//pDeviceSettings->d3d11.
	return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	HRESULT hr = S_OK;

	auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
	V_RETURN(g_DialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));
	V_RETURN(g_SettingsDlg.OnD3D11CreateDevice(pd3dDevice));
	g_pTxtHelper = new CDXUTTextHelper(pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15);

	V_RETURN(g_BlineHelper.OnD3D11CreateDevice(pd3dDevice, pBackBufferSurfaceDesc, pUserContext));

	// Initialize the view matrix
	static const XMVECTORF32 s_Eye = { 0.0f, 60.0f, -120.0f, 0.f };
	static const XMVECTORF32 s_At = { 0.0f, 0.0f, 0.0f, 0.f };
	g_Camera.SetViewParams(s_Eye, s_At);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	HRESULT hr;

	V_RETURN(g_DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));
	V_RETURN(g_SettingsDlg.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));

	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams(XM_PI / 4, fAspectRatio, 0.1f, 1000.0f);
	g_Camera.SetWindow(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
	g_Camera.SetButtonMasks(MOUSE_RIGHT_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON);

	g_HUD.SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
	g_HUD.SetSize(170, 170);
	g_SampleUI.SetLocation(pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300);
	g_SampleUI.SetSize(170, 300);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	g_Camera.FrameMove(fElapsedTime);
}

//--------------------------------------------------------------------------------------
// Render the help and statistics text.
//--------------------------------------------------------------------------------------
void RenderText()
{
	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos(5, 5);
	g_pTxtHelper->SetForegroundColor(Colors::Yellow);
	g_pTxtHelper->DrawTextLine(DXUTGetFrameStats(DXUTIsVsyncEnabled()));
	g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());
	g_pTxtHelper->End();
}

//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
	double fTime, float fElapsedTime, void* pUserContext)
{
	// If the settings dialog is being shown, then render it instead of rendering the app's scene
	if (g_SettingsDlg.IsActive())
	{
		g_SettingsDlg.OnRender(fElapsedTime);
		return;
	}  

	//
	// Clear the back buffer
	//
	auto pRTV = DXUTGetD3D11RenderTargetView();
	pd3dImmediateContext->ClearRenderTargetView(pRTV, Colors::MidnightBlue);

	//
	// Clear the depth stencil
	//
	auto pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

	XMMATRIX mView = g_Camera.GetViewMatrix();
	XMMATRIX mProj = g_Camera.GetProjMatrix();

	g_BlineHelper.Draw(pd3dDevice, pd3dImmediateContext, fTime, fElapsedTime, pUserContext, mView, mProj);

	g_HUD.OnRender(fElapsedTime);
	g_SampleUI.OnRender(fElapsedTime);
	RenderText();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
	g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	g_DialogResourceManager.OnD3D11DestroyDevice();
	g_SettingsDlg.OnD3D11DestroyDevice();
	DXUTGetGlobalResourceCache().OnDestroyDevice();
	SAFE_DELETE(g_pTxtHelper);

	g_BlineHelper.OnD3D11DestroyDevice(pUserContext);
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	bool* pbNoFurtherProcessing, void* pUserContext)
{
	// Pass messages to dialog resource manager calls so GUI state is updated correctly
	*pbNoFurtherProcessing = g_DialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	// Pass messages to settings dialog if its active
	if (g_SettingsDlg.IsActive())
	{
		g_SettingsDlg.MsgProc(hWnd, uMsg, wParam, lParam);
		return 0;
	}

	// Give the dialogs a chance to handle the message first
	*pbNoFurtherProcessing = g_HUD.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;
	*pbNoFurtherProcessing = g_SampleUI.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	// Pass all remaining windows messages to camera so it can respond to user input
	g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);
	return 0;
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	switch (nControlID)
	{
	case IDC_TOGGLEFULLSCREEN:
		DXUTToggleFullScreen();
		break;
	case IDC_TOGGLEREF:
		DXUTToggleREF();
		break;
	case IDC_TOGGLEWARP:
		DXUTToggleWARP();
		break;
	case IDC_CHANGEDEVICE:
		g_SettingsDlg.SetActive(!g_SettingsDlg.IsActive());
		break;

	case IDC_BUTTON_REBUILD:
	{
		const Bline::Real keys[] = {
			(Bline::Real)-48.7134, (Bline::Real) -0.5356, (Bline::Real)0.0000,
			(Bline::Real)0.9593, (Bline::Real)0.7869, (Bline::Real)10.8344,
			(Bline::Real)-0.8523, (Bline::Real) -44.5942, (Bline::Real)28.9085,
			(Bline::Real)52.6940, (Bline::Real) -20.0053, (Bline::Real)46.8857,
			(Bline::Real)66.4766, (Bline::Real)23.0054, (Bline::Real)18.5315,
			(Bline::Real)21.5014, (Bline::Real)57.8555, (Bline::Real)58.5114,
			(Bline::Real)-15.8288, (Bline::Real)42.1165, (Bline::Real)23.2550,
		};
		g_Bline.build(keys, sizeof(keys)/(3*sizeof(keys[0])));

		g_BlineHelper.rebuild(&g_Bline);

		//auto reset camera 
		//break;
	}

	case IDC_BUTTON_RESET_CAMERA:
	{
		Bline::Point min, max;
		g_Bline.getBounder(min, max);

		XMVECTORF32 center = {(float) (min.x + max.x) / 2.0f, (float)(min.y + max.y) / 2.0f, (float)(min.z + max.z) / 2.0f, 0.f };
		XMVECTORF32 sight = { (float)max.x - center[0], (float)max.y - center[1], (float)max.z - center[2], 0.f };

		XMVECTOR eye = center + sight * 2;
		g_Camera.SetViewParams(eye, center);
		break;
	}

	case IDC_CHECKBOX_CONTROL_KEY:
	{
		bool r = g_SampleUI.GetCheckBox(IDC_CHECKBOX_CONTROL_KEY)->GetChecked();
		g_BlineHelper.renderControlKey(r);
		break;
	}

	case IDC_CHECKBOX_SEGMENT:
	{
		bool r = g_SampleUI.GetCheckBox(IDC_CHECKBOX_SEGMENT)->GetChecked();
		g_BlineHelper.renderSegment(r);
		break;
	}

	case IDC_CHECKBOX_LINE:
	{
		bool r = g_SampleUI.GetCheckBox(IDC_CHECKBOX_LINE)->GetChecked();
		g_BlineHelper.renderLine(r);
		break;
	}

	case IDC_CHECKBOX_TANGENT:
	{
		bool r = g_SampleUI.GetCheckBox(IDC_CHECKBOX_TANGENT)->GetChecked();
		g_BlineHelper.renderTargent(r);
		break;
	}

	case IDC_CHECKBOX_BOUNDER:
	{
		bool r = g_SampleUI.GetCheckBox(IDC_CHECKBOX_BOUNDER)->GetChecked();
		g_BlineHelper.renderBounder(r);
		break;
	}

	}
}

//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
	if (bKeyDown)
	{
		switch (nChar)
		{
		case VK_F1: // Change as needed                
			break;
		}
	}
}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved(void* pUserContext)
{
	return true;
}

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
	
	g_SettingsDlg.Init(&g_DialogResourceManager);
	g_HUD.Init(&g_DialogResourceManager);
	g_SampleUI.Init(&g_DialogResourceManager);

	g_HUD.SetCallback(OnGUIEvent); int iY = 10;
	g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 22);

	g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 22, VK_F2);
	g_HUD.AddButton(IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 22, VK_F3);
	g_HUD.AddButton(IDC_TOGGLEWARP, L"Toggle WARP (F4)", 0, iY += 26, 170, 22, VK_F4);
	
	
	g_SampleUI.SetCallback(OnGUIEvent); iY = 10;
	g_SampleUI.AddButton(IDC_BUTTON_REBUILD, L"Rebuild", 0, iY += 26, 170, 22);
	g_SampleUI.AddButton(IDC_BUTTON_RESET_CAMERA, L"ResetCamera", 0, iY += 26, 170, 22);

	//g_SampleUI.AddCheckBox(IDC_CHECKBOX_CONTROL_KEY, L"Control Key", 0, iY += 26, 170, 22, g_BlineHelper.isRenderControlKey());
	g_SampleUI.AddCheckBox(IDC_CHECKBOX_SEGMENT, L"Segment", 0, iY += 26, 170, 22, g_BlineHelper.isRenderSegment());
	g_SampleUI.AddCheckBox(IDC_CHECKBOX_LINE, L"Lines", 0, iY += 26, 170, 22, g_BlineHelper.isRenderLine());
	g_SampleUI.AddCheckBox(IDC_CHECKBOX_TANGENT, L"Tangent", 0, iY += 26, 170, 22, g_BlineHelper.isRenderTargent());
	g_SampleUI.AddCheckBox(IDC_CHECKBOX_BOUNDER, L"Bounder", 0, iY += 26, 170, 22, g_BlineHelper.isRenderBounder());
}

//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	// Enable run-time memory check for debug builds.
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// DXUT will create and use the best device
	// that is available on the system depending on which D3D callbacks are set below

	// Set general DXUT callbacks
	DXUTSetCallbackFrameMove(OnFrameMove);
	DXUTSetCallbackKeyboard(OnKeyboard);
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
	DXUTSetCallbackDeviceRemoved(OnDeviceRemoved);

	// Set the D3D11 DXUT callbacks. Remove these sets if the app doesn't need to support D3D11
	DXUTSetCallbackD3D11DeviceAcceptable(IsD3D11DeviceAcceptable);
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
	DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);

	// Perform any application-level initialization here

	DXUTInit(true, true, nullptr); // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings(true, true); // Show the cursor and clip it when in full screen

	InitApp();
	DXUTCreateWindow(L"Bezeier Line Demo");

	// Only require 10-level hardware or later
	DXUTCreateDevice(D3D_FEATURE_LEVEL_10_0, true, 800, 600);
	DXUTMainLoop(); // Enter into the DXUT render loop

	// Perform any application-level cleanup here
	return DXUTGetExitCode();
}
