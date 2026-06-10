#include <windows.h>
#include <d3d9types.h>
#include <d3dx9.h>
#include <xgraphics.h>

int main()
{
	Direct3D *d = Direct3DCreate9(0);
	d->Release();
	d->CreateDevice( 0, D3DDEVTYPE_HAL, nullptr, D3DCREATE_HARDWARE_VERTEXPROCESSING, nullptr, nullptr );
	D3DDevice *dd = nullptr;
	dd->Release();
	D3DTexture *t = nullptr;
	t->Release();
	t->LockRect(0, nullptr, nullptr, 0);
	t->UnlockRect(0);
	XGCopySurface(nullptr, 0, 0, 0, D3DFMT_UNKNOWN, nullptr, nullptr, 0, D3DFMT_UNKNOWN, nullptr, 0, 0);
	XGGetTextureDesc(nullptr, 0, nullptr);
	D3DXCreateTexture(nullptr, 0, 0, 0, 0, D3DFMT_UNKNOWN, 0, nullptr);
	D3DXSaveTextureToFileA(nullptr, D3DXIFF_BMP, nullptr, nullptr);
	D3DXCreateTextureFromFileA(nullptr, nullptr, nullptr);
}
