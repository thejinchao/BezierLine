#include "bl_helper.h"
#include "bl_line.h"

const char* BlineHelper::m_shaderText =
"cbuffer cbChangesEveryFrame : register(b0)\n"
"{\n"
"	matrix WorldViewProj;\n"
"	matrix World;\n"
"};\n"
"\n"
"struct VS_INPUT\n"
"{\n"
"	float4 Pos : POSITION;\n"
"	float4 Diffuse : COLOR0;\n"
"};\n"
"\n"
"struct PS_INPUT\n"
"{\n"
"	float4 Pos : SV_POSITION;\n"
"	float4 Diffuse : COLOR0;\n"
"};\n"
"\n"
"PS_INPUT VS(VS_INPUT input)\n"
"{\n"
"	PS_INPUT output = (PS_INPUT)0;\n"
"	output.Pos = mul(input.Pos, WorldViewProj);\n"
"	output.Diffuse = input.Diffuse;\n"
"	return output;\n"
"}\n"
"\n"
"float4 PS(PS_INPUT input) : SV_Target\n"
"{\n"
"	return input.Diffuse;\n"
"}\n";


//--------------------------------------------------------------------------------------
BlineHelper::BlineHelper()
	: m_pD3DDevice(nullptr)
	, m_pVertexShader(nullptr)
	, m_pPixelShader(nullptr)
	, m_pVertexLayout(nullptr)
	, m_pCBChangesEveryFrame(nullptr)
	, m_pBounderVertexBuffer(nullptr)
	, m_pBounderIndexBuffer(nullptr)
	, m_pSegmentVertexBuffer(nullptr)
	, m_pLegendVertexBuffer(nullptr)
	, m_bRenderKey(true)
	, m_bRenderSegment(true)
	, m_bRenderLine(true)
	, m_bRenderTangent(true)
	, m_bRenderBounder(true)
	, m_tangentLength(3.0f)
	, m_legendCounts(50)
	, m_linePointVertexBuffer(nullptr)
	, m_linePointCounts(100)
{

}

//--------------------------------------------------------------------------------------
BlineHelper::~BlineHelper()
{

}

//--------------------------------------------------------------------------------------
bool BlineHelper::rebuild(Bline* bline)
{
	//rebuild boundervertex buffer
	SAFE_RELEASE(m_pBounderVertexBuffer);
	if (FAILED(_buildBounderVertexBuffer(m_pD3DDevice, bline))) {
		return false;
	}

	//rebuild segment vertex buffer
	SAFE_RELEASE(m_pSegmentVertexBuffer);
	if (FAILED(_buildSegmentVertexBuffer(m_pD3DDevice, bline))) {
		return false;
	}

	//rebuild line point vertex buffer
	SAFE_RELEASE(m_linePointVertexBuffer);
	if (FAILED(_buildLinePointVertexBuffer(m_pD3DDevice, bline))) {
		return false;
	}

	//rebuild legend vertex buffer
	SAFE_RELEASE(m_pLegendVertexBuffer);
	if(FAILED(_buildLegendVertexBuffer(m_pD3DDevice, bline))) {
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------
HRESULT BlineHelper::OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, 
	void* pUserContext)
{
	HRESULT hr = S_OK;

	m_pD3DDevice = pd3dDevice;
	auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr;
	V_RETURN(D3DCompile(m_shaderText, strlen(m_shaderText)+1, nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &pVSBlob, 0));

	// Create the vertex shader
	hr = pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader);
	if (FAILED(hr))
	{
		SAFE_RELEASE(pVSBlob);
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &m_pVertexLayout);
	SAFE_RELEASE(pVSBlob);
	if (FAILED(hr))
		return hr;

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	V_RETURN(D3DCompile(m_shaderText, strlen(m_shaderText) + 1, nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &pPSBlob, 0));

	// Create the pixel shader
	hr = pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	SAFE_RELEASE(pPSBlob);
	if (FAILED(hr))
		return hr;

	// Create index buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));

	DWORD indices[] =
	{
		0,1,
		1,2,
		2,3,
		3,0,
		0,4,
		1,5,
		2,6,
		3,7,
		4,5,
		5,6,
		6,7,
		7,4,
	};

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(DWORD) * 24;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	InitData.pSysMem = indices;
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &m_pBounderIndexBuffer));

	// Create the constant buffers
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = sizeof(CBChangesEveryFrame);
	V_RETURN(pd3dDevice->CreateBuffer(&bd, nullptr, &m_pCBChangesEveryFrame));

	return S_OK;
}

//--------------------------------------------------------------------------------------
HRESULT BlineHelper::_buildBounderVertexBuffer(ID3D11Device* pd3dDevice, const Bline* bline)
{
	HRESULT hr;

	XMFLOAT4 bounderColor = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);

	Bline::Point min, max;
	bline->getBounder(min, max);

	XMFLOAT3 bounderMin;

	// Create vertex buffer
	LineVertex vertices[] =
	{
		{ XMFLOAT3((float)min.x, (float)min.y, (float)max.z), bounderColor },
		{ XMFLOAT3((float)max.x, (float)min.y, (float)max.z), bounderColor },
		{ XMFLOAT3((float)max.x, (float)min.y, (float)min.z), bounderColor },
		{ XMFLOAT3((float)min.x, (float)min.y, (float)min.z), bounderColor },

		{ XMFLOAT3((float)min.x, (float)max.y, (float)max.z), bounderColor },
		{ XMFLOAT3((float)max.x, (float)max.y, (float)max.z), bounderColor },
		{ XMFLOAT3((float)max.x, (float)max.y, (float)min.z), bounderColor },
		{ XMFLOAT3((float)min.x, (float)max.y, (float)min.z), bounderColor },
	};

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(LineVertex) * 8;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &m_pBounderVertexBuffer));

	return S_OK;
}

//--------------------------------------------------------------------------------------
HRESULT BlineHelper::_buildSegmentVertexBuffer(ID3D11Device* pd3dDevice, const Bline* bline)
{
	HRESULT hr;

	m_keyCounts = bline->getKeyCounts();
	const Bline::Point* pt = bline->getKeys();

	LineVertex* keys = new LineVertex[m_keyCounts];
	for (size_t i = 0; i < m_keyCounts; i++) {
		keys[i].Pos.x = (float)pt->x;
		keys[i].Pos.y = (float)pt->y;
		keys[i].Pos.z = (float)pt->z;

		keys[i].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		pt++;
	}
	
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(LineVertex) * m_keyCounts;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = keys;
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &m_pSegmentVertexBuffer));

	delete[] keys;
	return S_OK;
}

//--------------------------------------------------------------------------------------
HRESULT BlineHelper::_buildLinePointVertexBuffer(ID3D11Device* pd3dDevice, const Bline* bline)
{
	HRESULT hr;

	LineVertex* pts = new LineVertex[m_linePointCounts];
	for (size_t i = 0; i < m_linePointCounts; i++) {
		Bline::Real t = (Bline::Real)i / (Bline::Real)(m_linePointCounts - 1);
		Bline::Point pt, ta;
		bline->getPoint(t, pt, ta);

		pts[i].Pos.x = (float)pt.x;
		pts[i].Pos.y = (float)pt.y;
		pts[i].Pos.z = (float)pt.z;
		pts[i].Color = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);

	}
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(LineVertex) * m_linePointCounts;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = pts;
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &m_linePointVertexBuffer));

	delete[] pts;

	return S_OK;
}

//--------------------------------------------------------------------------------------
HRESULT BlineHelper::_buildLegendVertexBuffer(ID3D11Device* pd3dDevice, const Bline* bline)
{
	HRESULT hr;

	LineVertex* pts = new LineVertex[m_legendCounts*2];
	for (size_t i = 0; i < m_legendCounts; i++) {
		Bline::Real t = (Bline::Real)i / (Bline::Real)(m_legendCounts - 1);
		Bline::Point pt, ta;
		bline->getPoint(t, pt, ta);

		pts[i*2].Pos.x = (float)pt.x;
		pts[i*2].Pos.y = (float)pt.y;
		pts[i*2].Pos.z = (float)pt.z;
		pts[i*2].Color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

		pts[i * 2+1].Pos.x = (float)pt.x+(float)ta.x*m_tangentLength;
		pts[i * 2+1].Pos.y = (float)pt.y+(float)ta.y*m_tangentLength;
		pts[i * 2+1].Pos.z = (float)pt.z+(float)ta.z*m_tangentLength;
		pts[i * 2+1].Color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	}


	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(LineVertex) * m_legendCounts*2;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = pts;
	V_RETURN(pd3dDevice->CreateBuffer(&bd, &InitData, &m_pLegendVertexBuffer));

	delete[] pts;

	return S_OK;
}

//--------------------------------------------------------------------------------------
void BlineHelper::OnD3D11DestroyDevice(void* pUserContext)
{
	SAFE_RELEASE(m_pBounderVertexBuffer);
	SAFE_RELEASE(m_pBounderIndexBuffer);

	SAFE_RELEASE(m_pSegmentVertexBuffer);
	SAFE_RELEASE(m_pLegendVertexBuffer);
	SAFE_RELEASE(m_linePointVertexBuffer);

	SAFE_RELEASE(m_pVertexLayout);
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pCBChangesEveryFrame);
}

//--------------------------------------------------------------------------------------
void BlineHelper::Draw(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
	double fTime, float fElapsedTime, void* pUserContext,
	const XMMATRIX& view, const XMMATRIX& proj)
{
	XMMATRIX world = XMMatrixIdentity();

	XMMATRIX mWorldViewProjection = world * view * proj;

	// Update constant buffer that changes once per frame
	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	V(pd3dImmediateContext->Map(m_pCBChangesEveryFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	auto pCB = reinterpret_cast<CBChangesEveryFrame*>(MappedResource.pData);
	XMStoreFloat4x4(&pCB->mWorldViewProj, XMMatrixTranspose(mWorldViewProjection));
	XMStoreFloat4x4(&pCB->mWorld, XMMatrixTranspose(world));
	pd3dImmediateContext->Unmap(m_pCBChangesEveryFrame, 0);

	pd3dImmediateContext->IASetInputLayout(m_pVertexLayout);
	pd3dImmediateContext->VSSetShader(m_pVertexShader, nullptr, 0);
	pd3dImmediateContext->VSSetConstantBuffers(0, 1, &m_pCBChangesEveryFrame);
	pd3dImmediateContext->PSSetShader(m_pPixelShader, nullptr, 0);
	pd3dImmediateContext->PSSetConstantBuffers(0, 1, &m_pCBChangesEveryFrame);

	//draw segment
	if (m_bRenderSegment && m_pSegmentVertexBuffer != nullptr) {
		UINT stride = sizeof(LineVertex);
		UINT offset = 0;
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pSegmentVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);

		pd3dImmediateContext->Draw(m_keyCounts, 0);
	}

	//draw line
	if (m_bRenderLine &&m_linePointVertexBuffer != nullptr) {
		UINT stride = sizeof(LineVertex);
		UINT offset = 0;
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_linePointVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);

		pd3dImmediateContext->Draw(m_linePointCounts, 0);

	}

	//draw tangent
	if (m_bRenderTangent &&m_pLegendVertexBuffer != nullptr) {
		UINT stride = sizeof(LineVertex);
		UINT offset = 0;
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pLegendVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

		pd3dImmediateContext->Draw(m_legendCounts * 2, 0);

	}

	//draw bounder
	if (m_bRenderBounder && m_pBounderVertexBuffer != nullptr) {
		UINT stride = sizeof(LineVertex);
		UINT offset = 0;
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pBounderVertexBuffer, &stride, &offset);
		pd3dImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		pd3dImmediateContext->IASetIndexBuffer(m_pBounderIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		pd3dImmediateContext->DrawIndexed(24, 0, 0);
	}
	
}
