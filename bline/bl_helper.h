#pragma once
#include "DXUT.h"
#include "SDKmisc.h"

using namespace DirectX;

class Bline;

class BlineHelper
{
public:
	bool rebuild(Bline* bline);
	
	bool isRenderControlKey(void) const { return m_bRenderKey; }
	void renderControlKey(bool r) { m_bRenderKey = r; }

	bool isRenderSegment(void) const { return m_bRenderSegment; }
	void renderSegment(bool r) { m_bRenderSegment = r; }

	bool isRenderLine(void) const { return m_bRenderLine; }
	void renderLine(bool r) { m_bRenderLine = r; }

	bool isRenderTargent(void) const { return m_bRenderTangent; }
	void renderTargent(bool r) { m_bRenderTangent = r; }

	bool isRenderBounder(void) const { return m_bRenderBounder; }
	void renderBounder(bool r) { m_bRenderBounder = r; }

public:
	HRESULT OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, 
		void* pUserContext);
	void OnD3D11DestroyDevice(void* pUserContext);

	void Draw(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
		double fTime, float fElapsedTime, void* pUserContext,
		const XMMATRIX& view, const XMMATRIX& proj);

private:
	HRESULT _buildBounderVertexBuffer(ID3D11Device* pd3dDevice, const Bline* bline);
	HRESULT _buildSegmentVertexBuffer(ID3D11Device* pd3dDevice, const Bline* bline);
	HRESULT _buildLinePointVertexBuffer(ID3D11Device* pd3dDevice, const Bline* bline);
	HRESULT _buildLegendVertexBuffer(ID3D11Device* pd3dDevice, const Bline* bline);

private:
	struct LineVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT4 Color;
	};
	
	struct CBChangesEveryFrame
	{
		XMFLOAT4X4 mWorldViewProj;
		XMFLOAT4X4 mWorld;
	};

	static const char*			m_shaderText;

	ID3D11Device*				m_pD3DDevice;
	ID3D11VertexShader*         m_pVertexShader;
	ID3D11PixelShader*          m_pPixelShader;
	ID3D11InputLayout*          m_pVertexLayout;
	ID3D11Buffer*               m_pCBChangesEveryFrame;

	ID3D11Buffer*               m_pBounderVertexBuffer;
	ID3D11Buffer*               m_pBounderIndexBuffer;

	ID3D11Buffer*				m_pSegmentVertexBuffer;
	size_t						m_keyCounts;

	ID3D11Buffer*				m_pLegendVertexBuffer;
	float						m_tangentLength;
	size_t						m_legendCounts;

	ID3D11Buffer*				m_linePointVertexBuffer;
	size_t						m_linePointCounts;

	bool						m_bRenderKey;
	bool						m_bRenderSegment;
	bool						m_bRenderLine;
	bool						m_bRenderTangent;
	bool						m_bRenderBounder;


public:
	BlineHelper();
	~BlineHelper();
};