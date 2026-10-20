#include "neurocube/application.hpp"
#include "neurocube/board.hpp"
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include <cstdio>
#include <cstring>
#include <cmath>
static nc::Framebuffer frame;
static nc::Board board;
static nc::FlashStore storage;
static char serial_id[2*PICO_UNIQUE_BOARD_ID_SIZE_BYTES+1];
static void reply(const char*type,const char*extra=""){printf("{\"v\":1,\"type\":\"%s\"%s}\n",type,extra);}
static void status(nc::App&app,const nc::InputFrame&i){auto&s=board.status;printf("{\"v\":1,\"type\":\"status\",\"serial\":\"%s\",\"frames\":%lu,\"screen\":%d,\"game\":%d,\"score\":%lu,\"record_sequence\":%lu,\"save_ok\":%s,\"imu\":%s,\"imu_id\":%u,\"touch_id\":%u,\"touches\":%lu,\"button_events\":%lu,\"i2c_errors\":%lu,\"ax\":%.4f,\"ay\":%.4f,\"az\":%.4f,\"gz\":%.2f}\n",serial_id,(unsigned long)s.frames,int(app.screen()),app.selected(),(unsigned long)app.game().result().score,(unsigned long)app.records().sequence,app.save_ok()?"true":"false",i.imu?"true":"false",s.imu_id,s.touch_id,(unsigned long)s.touches,(unsigned long)s.button_events,(unsigned long)s.i2c_errors,i.ax,i.ay,i.az,i.gz);}
static void handle(const char*cmd,nc::App&app,const nc::InputFrame&i){
    if(!strcmp(cmd,"NC1 hello")){printf("{\"v\":1,\"type\":\"hello\",\"product\":\"NeuroCube\",\"firmware\":\"0.1.0\",\"serial\":\"%s\",\"capabilities\":[\"status\",\"imu\",\"selftest\",\"framebuffer\",\"records\"],\"debug_input\":%s}\n",serial_id,
#ifdef NEUROCUBE_DEBUG_INPUT
"true"
#else
"false"
#endif
);}
    else if(!strcmp(cmd,"NC1 status")||!strcmp(cmd,"NC1 imu"))status(app,i);
    else if(!strcmp(cmd,"NC1 records")){printf("{\"v\":1,\"type\":\"records\",\"sequence\":%lu,\"best\":[",(unsigned long)app.records().sequence);for(int g=0;g<3;g++)for(int d=0;d<3;d++)printf("%s%lu",g||d?",":"",(unsigned long)app.records().best[g][d]);printf("]}\n");}
    else if(!strcmp(cmd,"NC1 selftest")){auto&s=board.status;printf("{\"v\":1,\"type\":\"selftest\",\"imu_identity\":%s,\"imu_sample\":%s,\"touch_identity\":%s,\"lcd_spi_sent\":%s,\"display_visual\":\"NOT_TESTED\",\"motion\":\"NOT_TESTED\",\"touch_physical\":\"NOT_TESTED\",\"button_physical\":\"NOT_TESTED\",\"battery\":\"NOT_TESTED\"}\n",s.imu_ok?"true":"false",i.imu&&std::isfinite(i.ax)&&std::isfinite(i.ay)&&std::isfinite(i.az)?"true":"false",s.touch_ok?"true":"false",s.lcd_sent?"true":"false");}
    else if(!strcmp(cmd,"NC1 screenshot")){reply("frame_begin",",\"width\":240,\"height\":240,\"format\":\"rgb565be_hex\"");for(int y=0;y<240;y++){printf("{\"v\":1,\"type\":\"frame_row\",\"y\":%d,\"data\":\"",y);for(int x=0;x<240;x++)printf("%04x",frame.pixels[y*240+x]);printf("\"}\n");}reply("frame_end");}
#ifdef NEUROCUBE_DEBUG_INPUT
    else if(!strncmp(cmd,"NC1 inject ",11)){nc::InputFrame injected;int tap,x,y;float ax,ay,az,gz;if(sscanf(cmd+11,"%d %d %d %f %f %f %f",&tap,&x,&y,&ax,&ay,&az,&gz)==7&&std::isfinite(ax)&&std::isfinite(ay)&&std::isfinite(az)&&std::isfinite(gz)&&x>=0&&x<240&&y>=0&&y<240){injected.imu=true;injected.tap=tap!=0;injected.x=x;injected.y=y;injected.ax=ax;injected.ay=ay;injected.az=az;injected.gz=gz;app.update(injected,.02f);reply("injected");}else reply("error",",\"error\":\"invalid_input\"");}
#endif
    else reply("unsupported");
}
int main(){stdio_init_all();pico_get_unique_board_id_string(serial_id,sizeof serial_id);board.init();nc::App app(storage);uint64_t last=time_us_64();char line[160];size_t pos=0;bool overflow=false;while(true){auto now=time_us_64();float dt=float(now-last)/1e6f;last=now;auto input=board.sample();app.update(input,dt);app.render(frame);board.present(frame);for(int n=0;n<160;n++){int c=getchar_timeout_us(0);if(c==PICO_ERROR_TIMEOUT)break;if(c=='\n'){line[pos]=0;if(overflow)reply("error",",\"error\":\"line_too_long\"");else handle(line,app,input);pos=0;overflow=false;}else if(c!='\r'){if(pos+1<sizeof line)line[pos++]=char(c);else overflow=true;}}sleep_ms(10);}}
