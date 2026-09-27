#include "../src/TextHud.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
int main(int argc,char** argv){
 require(argc==2,"output bitmap path required");
 WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"ActionNitroHudTest";RegisterClassW(&wc);
 HWND w=CreateWindowW(wc.lpszClassName,L"ActionNitro HUD test",WS_OVERLAPPED,0,0,1280,720,nullptr,nullptr,wc.hInstance,nullptr);require(w!=nullptr,"hidden window");
 IDirect3D9* api=Direct3DCreate9(D3D_SDK_VERSION);require(api!=nullptr,"D3D9");IDirect3DDevice9* d=nullptr;
 D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.BackBufferWidth=1280;pp.BackBufferHeight=720;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.hDeviceWindow=w;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
 require(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,w,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&d)),"hidden device");
 d->Clear(0,nullptr,D3DCLEAR_TARGET,D3DCOLOR_XRGB(20,25,34),1,0);
 D3DVIEWPORT9 vp{20,30,900,600,.1f,.9f};RECT rect{25,35,400,500};d->SetViewport(&vp);d->SetScissorRect(&rect);d->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_LIGHTING,TRUE);
 IDirect3DVertexBuffer9* stream=nullptr;require(SUCCEEDED(d->CreateVertexBuffer(256,0,0,D3DPOOL_DEFAULT,&stream,nullptr)),"test stream");d->SetStreamSource(0,stream,16,32);
 an::TextBitmap bitmap;auto pixels=bitmap.render(L"ニアミス + 100\n対向車線 + 125\n対向車ニアミス + 200\nジャンプ + 100\n滞空 + 80\nドリフト + 150\nスリップストリーム + 120\n連続ニアミス + 50\nトップスピード + 30\nドリフト継続 + 100\n");
 size_t ink=0;for(auto p:pixels)if(p>>24)++ink;require(ink>1000,"Unicode glyph rasterization");
 an::HudConfig cfg;cfg.width=512;require(an::drawText(d,pixels,cfg,1),"D3D draw");
 D3DVIEWPORT9 after{};RECT afterRect{};DWORD alpha=99,lighting=99,scissor=99;d->GetViewport(&after);d->GetScissorRect(&afterRect);d->GetRenderState(D3DRS_ALPHABLENDENABLE,&alpha);d->GetRenderState(D3DRS_LIGHTING,&lighting);d->GetRenderState(D3DRS_SCISSORTESTENABLE,&scissor);
 require(!memcmp(&vp,&after,sizeof(vp))&&!memcmp(&rect,&afterRect,sizeof(rect))&&!alpha&&lighting&&scissor,"viewport/scissor/render-state restored");
 IDirect3DVertexBuffer9* afterStream=nullptr;UINT offset=0,stride=0;d->GetStreamSource(0,&afterStream,&offset,&stride);require(afterStream==stream&&offset==16&&stride==32,"DrawPrimitiveUP stream restored");afterStream->Release();
 IDirect3DSurface9* back=nullptr,*read=nullptr;d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back);require(SUCCEEDED(d->CreateOffscreenPlainSurface(1280,720,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr)),"readback surface");require(SUCCEEDED(d->GetRenderTargetData(back,read)),"readback");
 D3DLOCKED_RECT lock{};require(SUCCEEDED(read->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"readback lock");
 FILE* f=nullptr;fopen_s(&f,argv[1],"wb");require(f!=nullptr,"bitmap output");BITMAPFILEHEADER fh{};BITMAPINFOHEADER ih{};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(ih);fh.bfSize=fh.bfOffBits+1280*720*4;ih.biSize=sizeof(ih);ih.biWidth=1280;ih.biHeight=-720;ih.biPlanes=1;ih.biBitCount=32;fwrite(&fh,sizeof(fh),1,f);fwrite(&ih,sizeof(ih),1,f);for(int y=0;y<720;y++)fwrite(static_cast<char*>(lock.pBits)+y*lock.Pitch,1280*4,1,f);fclose(f);read->UnlockRect();read->Release();back->Release();
 // Nested scenes must be rejected without taking ownership of the outer scene.
 require(SUCCEEDED(d->BeginScene()),"outer BeginScene");require(!an::drawText(d,pixels,cfg,1),"nested scene rejected");require(SUCCEEDED(d->EndScene()),"outer scene retained");
 stream->Release();d->Release();api->Release();DestroyWindow(w);UnregisterClassW(wc.lpszClassName,wc.hInstance);
 std::puts("PASS HUD: Unicode raster, D3D draw/readback, viewport/scissor/stream/render-state restoration, nested scene; window closed");
}
