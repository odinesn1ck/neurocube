#include "neurocube/framebuffer.hpp"
#include "font_data.hpp"
#include <algorithm>
#include <cmath>
namespace nc {
void Framebuffer::clear(uint16_t c){pixels.fill(c);}
void Framebuffer::rect(int x,int y,int w,int h,uint16_t c){for(int yy=std::max(0,y);yy<std::min(Height,y+h);yy++)for(int xx=std::max(0,x);xx<std::min(Width,x+w);xx++)pixels[yy*Width+xx]=c;}
void Framebuffer::circle(int x,int y,int radius,uint16_t c){for(int yy=-radius;yy<=radius;yy++){int w=int(std::sqrt(float(radius*radius-yy*yy)));rect(x-w,y+yy,2*w+1,1,c);}}
static uint32_t utf8(const char*&p){uint8_t c=uint8_t(*p++);if(c<128)return c;int n=c<0xE0?1:c<0xF0?2:3;uint32_t v=c&((1<<(6-n))-1);for(int i=0;i<n;i++){uint8_t b=uint8_t(*p);if(!b||(b&0xC0)!=0x80)return '?';v=(v<<6)|(b&63);++p;}return v;}
void Framebuffer::text(int x,int y,const char*s,uint16_t c,int scale){int origin=x;scale=std::clamp(scale,1,3);while(*s){uint32_t cp=utf8(s);if(cp=='\n'||x+10*scale>Width-5){x=origin;y+=18*scale;if(cp=='\n')continue;}if(y+16*scale>Height)return;size_t g=0;while(g<font_count&&font_codes[g]!=cp)g++;if(g==font_count){g=0;}for(int row=0;row<16;row++)for(int col=0;col<10;col++)if(font_rows[g][row]&(1u<<(9-col)))rect(x+col*scale,y+row*scale,scale,scale,c);x+=10*scale;}}
}
