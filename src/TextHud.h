#pragma once
#include <windows.h>
#include <d3d9.h>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
namespace an {
struct HudConfig {float right=30,bottom=25,width=430;};
// Rasterize Unicode with the OS font, without touching game language assets.
class TextBitmap {
 std::wstring cached,fontName=L"Yu Gothic UI";std::vector<uint32_t> pixels;
 HDC dc=nullptr;HBITMAP bitmap=nullptr;HFONT font=nullptr;HGDIOBJ oldBitmap=nullptr,oldFont=nullptr;
 HFONT smallFonts[3]{};void* bits=nullptr;unsigned previousRows=0;
 void release(){if(dc){if(oldFont)SelectObject(dc,oldFont);if(oldBitmap)SelectObject(dc,oldBitmap);}if(font)DeleteObject(font);for(auto& f:smallFonts){if(f)DeleteObject(f);f=nullptr;}if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);dc=nullptr;bitmap=nullptr;font=nullptr;bits=nullptr;oldBitmap=oldFont=nullptr;}
 bool create(){
  if(dc)return true;dc=CreateCompatibleDC(nullptr);if(!dc)return false;
  BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
  bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
  auto makeFont=[&](int size){return CreateFontW(-size,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,fontName.c_str());};
  font=makeFont(26);smallFonts[0]=makeFont(22);smallFonts[1]=makeFont(18);smallFonts[2]=makeFont(14);
  if(!bitmap||!font){release();return false;}
  oldBitmap=SelectObject(dc,bitmap);oldFont=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,255,255));return true;
 }
public:
 static constexpr int width=512,height=512;
 TextBitmap()=default;TextBitmap(const TextBitmap&)=delete;TextBitmap& operator=(const TextBitmap&)=delete;
 ~TextBitmap(){release();}
 void setFont(const wchar_t* name){if(fontName==name)return;release();fontName=name;cached.clear();pixels.clear();previousRows=0;}
 const std::vector<uint32_t>& render(const std::wstring& text){
  if(text==cached&&!pixels.empty())return pixels;if(!create())return pixels;
  const unsigned lines=unsigned(std::count(text.begin(),text.end(),L'\n'))+1;
  const unsigned usedRows=std::min(unsigned(height),lines*32+4),clearRows=std::max(usedRows,previousRows);
  std::memset(bits,0,width*clearRows*4);
  size_t begin=0;int y=2;
  while(begin<text.size()&&y<height-30){
   const auto end=text.find(L'\n',begin);const auto n=(end==std::wstring::npos?text.size():end)-begin;
   SelectObject(dc,font);SIZE extent{};GetTextExtentPoint32W(dc,text.data()+begin,int(n),&extent);
   for(auto f:smallFonts){if(extent.cx<=width-8)break;if(f)SelectObject(dc,f);GetTextExtentPoint32W(dc,text.data()+begin,int(n),&extent);}
   RECT line{3,y,width-3,y+32};DrawTextW(dc,text.data()+begin,int(n),&line,DT_LEFT|DT_TOP|DT_SINGLELINE|DT_NOPREFIX);
   y+=32;if(end==std::wstring::npos)break;begin=end+1;
  }
  GdiFlush();pixels.resize(width*height,0x00ffffff);const auto source=static_cast<const uint32_t*>(bits);
  for(size_t i=0;i<size_t(width)*clearRows;i++){const auto p=source[i];const auto a=std::max({p&255,(p>>8)&255,(p>>16)&255});pixels[i]=(a<<24)|0x00ffffff;}
  previousRows=usedRows;cached=text;return pixels;
 }
};
inline unsigned textTextureHeight(unsigned lines){unsigned h=64;const auto used=std::clamp(lines,1u,10u)*32+4;while(h<used)h*=2;return h;}
struct Vertex {float x,y,z,w;DWORD color;float u,v;};
inline bool drawText(IDirect3DDevice9* d,const std::vector<uint32_t>& pixels,const HudConfig& cfg,float alpha,unsigned lines=10){
 if(pixels.size()!=TextBitmap::width*TextBitmap::height||FAILED(d->TestCooperativeLevel()))return false;
 D3DCAPS9 caps{};if(FAILED(d->GetDeviceCaps(&caps))||caps.NumSimultaneousRTs>4||!caps.NumSimultaneousRTs)return false;
 IDirect3DStateBlock9* state=nullptr;if(FAILED(d->CreateStateBlock(D3DSBT_ALL,&state)))return false;
 IDirect3DSurface9* targets[4]{},*depth=nullptr,*back=nullptr;IDirect3DTexture9* texture=nullptr;
 IDirect3DVertexBuffer9* stream=nullptr;UINT offset=0,stride=0;D3DVIEWPORT9 vp{};RECT scissor{};
 d->GetStreamSource(0,&stream,&offset,&stride);d->GetViewport(&vp);d->GetScissorRect(&scissor);
 bool ok=true,scene=false,changed=false;
 for(unsigned i=0;i<caps.NumSimultaneousRTs;i++){auto hr=d->GetRenderTarget(i,&targets[i]);if(FAILED(hr)&&(i==0||hr!=D3DERR_NOTFOUND))ok=false;}
 auto hr=d->GetDepthStencilSurface(&depth);if(FAILED(hr)&&hr!=D3DERR_NOTFOUND)ok=false;
 if(FAILED(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back)))ok=false;
 D3DSURFACE_DESC desc{};if(!back||FAILED(back->GetDesc(&desc)))ok=false;
 const unsigned textureHeight=textTextureHeight(lines);
 if(ok&&FAILED(d->CreateTexture(TextBitmap::width,textureHeight,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&texture,nullptr)))ok=false;
 if(ok){D3DLOCKED_RECT r{};if(FAILED(texture->LockRect(0,&r,nullptr,D3DLOCK_DISCARD)))ok=false;else {for(unsigned y=0;y<textureHeight;y++)std::memcpy(static_cast<char*>(r.pBits)+y*r.Pitch,pixels.data()+y*TextBitmap::width,TextBitmap::width*4);texture->UnlockRect(0);}}
 if(ok){scene=SUCCEEDED(d->BeginScene());ok=scene;}
 if(ok){
  changed=true;d->SetDepthStencilSurface(nullptr);for(unsigned i=1;i<caps.NumSimultaneousRTs;i++)d->SetRenderTarget(i,nullptr);
  ok=SUCCEEDED(d->SetRenderTarget(0,back));D3DVIEWPORT9 full{0,0,desc.Width,desc.Height,0,1};d->SetViewport(&full);
  d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);d->SetTexture(0,texture);
  d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);d->SetRenderState(D3DRS_STENCILENABLE,FALSE);d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);d->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);d->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);d->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
  d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_RESULTARG,D3DTA_CURRENT);d->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);d->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
  d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);d->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE);d->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE);d->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);d->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
  const float usedHeight=float(std::clamp(lines,1u,10u)*32+4),vMax=usedHeight/textureHeight;
  const float scale=float(desc.Height)/1080;const float w=cfg.width*scale,h=w*usedHeight/TextBitmap::width;
  const float x=std::max(0.f,float(desc.Width)-(cfg.right+cfg.width)*scale),y=std::max(0.f,float(desc.Height)-cfg.bottom*scale-h);
  auto quad=[&](float dx,float dy,DWORD color){Vertex q[]={{x+dx-.5f,y+dy-.5f,0,1,color,0,0},{x+w+dx-.5f,y+dy-.5f,0,1,color,1,0},{x+dx-.5f,y+h+dy-.5f,0,1,color,0,vMax},{x+w+dx-.5f,y+h+dy-.5f,0,1,color,1,vMax}};return d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,q,sizeof(Vertex));};
  const DWORD a=DWORD(std::clamp(alpha,0.f,1.f)*255);if(ok){quad(1.5f,1.5f,D3DCOLOR_ARGB(a,0,0,0));ok=SUCCEEDED(quad(0,0,D3DCOLOR_ARGB(a,135,235,255)));}
 }
 if(scene)d->EndScene();
 if(changed){d->SetDepthStencilSurface(nullptr);for(unsigned i=1;i<caps.NumSimultaneousRTs;i++)d->SetRenderTarget(i,nullptr);d->SetRenderTarget(0,targets[0]);for(unsigned i=1;i<caps.NumSimultaneousRTs;i++)d->SetRenderTarget(i,targets[i]);d->SetDepthStencilSurface(depth);}
 state->Apply();d->SetViewport(&vp);d->SetScissorRect(&scissor);d->SetStreamSource(0,stream,offset,stride);
 if(stream)stream->Release();if(texture)texture->Release();if(back)back->Release();if(depth)depth->Release();for(auto t:targets)if(t)t->Release();state->Release();return ok;
}
}
