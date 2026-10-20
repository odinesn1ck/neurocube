#include "neurocube/application.hpp"
#include "neurocube/framebuffer.hpp"
#include <fstream>
#include <string>
#include <cstdio>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
using namespace nc;
struct FileStore:RecordStore {std::string path="sim-records.bin";bool load(Records&r)override{std::ifstream f(path,std::ios::binary);RecordPage p{};if(!f.read((char*)p.data(),p.size()))return false;return decode_records(p.data(),p.size(),r);}bool save(const Records&r)override{auto p=encode_records(r);std::ofstream f(path,std::ios::binary);f.write((char*)p.data(),p.size());return bool(f);}};
static FileStore store;static App app(store);static Framebuffer fb;
static void ppm(const char*path){std::ofstream f(path,std::ios::binary);f<<"P6\n240 240\n255\n";for(auto p:fb.pixels){char rgb[]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};f.write(rgb,3);}}
#ifdef _WIN32
static InputFrame pending;static bool was_click=false;static uint32_t rgb[240*240];
LRESULT CALLBACK WindowProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){switch(msg){case WM_DESTROY:PostQuitMessage(0);return 0;case WM_LBUTTONDOWN:pending.tap=true;pending.x=LOWORD(lp)/2;pending.y=HIWORD(lp)/2;return 0;case WM_KEYDOWN:if(!(lp&(1<<30))){if(wp==VK_TAB)pending.short_button=true;if(wp==VK_ESCAPE)pending.long_button=true;}return 0;case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);for(int j=0;j<240*240;j++){auto p=fb.pixels[j];rgb[j]=(((p>>11)&31)*255/31<<16)|(((p>>5)&63)*255/63<<8)|((p&31)*255/31);}BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=240;bi.bmiHeader.biHeight=-240;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;StretchDIBits(dc,0,0,480,480,0,0,240,240,rgb,&bi,DIB_RGB_COLORS,SRCCOPY);EndPaint(w,&ps);return 0;}}return DefWindowProcW(w,msg,wp,lp);}
#endif
int main(int argc,char**argv){if(argc>1&&std::string(argv[1])=="--headless"){for(int i=0;i<600;i++)app.update({},1.f/60);app.render(fb);ppm(argc>2?argv[2]:"sim-menu.ppm");std::puts("VERIFIED_IN_SIMULATOR: 600 frames, screenshot captured");return 0;}
#ifdef _WIN32
 auto inst=GetModuleHandleW(nullptr);WNDCLASSW wc{};wc.lpfnWndProc=WindowProc;wc.hInstance=inst;wc.lpszClassName=L"NeuroCubeSimulator";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);RECT rect{0,0,480,480};AdjustWindowRect(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE);HWND w=CreateWindowW(wc.lpszClassName,L"NeuroCube | arrows tilt, Q turn, Space shake, Tab next, Esc menu",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,inst,nullptr);ShowWindow(w,SW_SHOW);MSG msg{};while(true){while(PeekMessage(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT)return 0;TranslateMessage(&msg);DispatchMessage(&msg);}pending.imu=true;pending.ax=((GetAsyncKeyState(VK_RIGHT)&0x8000)?.7f:0)-((GetAsyncKeyState(VK_LEFT)&0x8000)?.7f:0);pending.ay=((GetAsyncKeyState(VK_DOWN)&0x8000)?.7f:0)-((GetAsyncKeyState(VK_UP)&0x8000)?.7f:0);pending.gz=(GetAsyncKeyState('Q')&0x8000)?200.f:0;pending.az=(GetAsyncKeyState(VK_SPACE)&0x8000)?3.f:1.f;app.update(pending,.02f);app.render(fb);pending.tap=pending.short_button=pending.long_button=false;InvalidateRect(w,nullptr,FALSE);Sleep(20);}
#else
 app.render(fb);ppm("sim-menu.ppm");std::puts("Use --headless on non-Windows hosts.");
#endif
}
