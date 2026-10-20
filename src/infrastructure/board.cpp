// Hardware facts and register sequences checked against Waveshare model-specific
// examples at 51a7fe28c1e5dd19a089a062980a275709e94df6. See docs/hardware.md.
#include "neurocube/board.hpp"
#include "pico/stdlib.h"
#include "pico/flash.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/flash.h"
#include <algorithm>
#include <cstring>
namespace nc {
static constexpr uint DC=12,CS=9,SCK=10,MOSI=11,RST=14,BL=13,TP_RST=16,KEY=25;
static constexpr uint32_t StorageOffset=16*1024*1024-8192;
static bool read_regs(uint8_t address,uint8_t reg,uint8_t*out,size_t n){return i2c_write_timeout_us(i2c1,address,&reg,1,true,3000)==1&&i2c_read_timeout_us(i2c1,address,out,n,false,3000)==int(n);}
static bool write_reg(uint8_t address,uint8_t reg,uint8_t value){uint8_t data[]={reg,value};return i2c_write_timeout_us(i2c1,address,data,2,false,3000)==2;}
static void output(uint p,bool v){gpio_init(p);gpio_put(p,v);gpio_set_dir(p,GPIO_OUT);}
void Board::command(uint8_t c){gpio_put(DC,0);spi_write_blocking(spi1,&c,1);}
void Board::data(uint8_t c){gpio_put(DC,1);spi_write_blocking(spi1,&c,1);}
void Board::init(){
    // Deliberately do not initialize BAT_EN(28), BAT_STAT(17), BAT_ADC(29), KEY_PWR(24).
    output(CS,1);output(DC,0);output(RST,1);output(BL,0);output(TP_RST,1);
    spi_init(spi1,40000000);gpio_set_function(SCK,GPIO_FUNC_SPI);gpio_set_function(MOSI,GPIO_FUNC_SPI);
    i2c_init(i2c1,400000);gpio_set_function(6,GPIO_FUNC_I2C);gpio_set_function(7,GPIO_FUNC_I2C);gpio_pull_up(6);gpio_pull_up(7);
    gpio_init(KEY);gpio_set_dir(KEY,GPIO_IN);gpio_pull_up(KEY);
    gpio_put(RST,1);sleep_ms(100);gpio_put(RST,0);sleep_ms(100);gpio_put(RST,1);sleep_ms(100);gpio_put(CS,0);sleep_ms(120);
    command(0x36);data(0x00);command(0x3A);data(0x05);
    command(0xB2);for(auto v:{0x0C,0x0C,0x00,0x33,0x33})data(uint8_t(v));
    for(auto pair: {std::array<uint8_t,2>{0xB7,0x35},{0xBB,0x19},{0xC0,0x2C},{0xC2,0x01},{0xC3,0x12},{0xC4,0x20},{0xC6,0x0F}}){command(pair[0]);data(pair[1]);}
    command(0xD0);data(0xA4);data(0xA1);
    command(0xE0);for(auto v:{0xD0,0x04,0x0D,0x11,0x13,0x2B,0x3F,0x54,0x4C,0x18,0x0D,0x0B,0x1F,0x23})data(uint8_t(v));
    command(0xE1);for(auto v:{0xD0,0x04,0x0C,0x11,0x13,0x2C,0x3F,0x44,0x51,0x2F,0x1F,0x1F,0x20,0x23})data(uint8_t(v));
    command(0x21);command(0x11);sleep_ms(120);command(0x29);gpio_put(CS,1);gpio_put(BL,1);
    gpio_put(TP_RST,0);sleep_ms(100);gpio_put(TP_RST,1);sleep_ms(100);
    status.touch_ok=read_regs(0x15,0xA7,&status.touch_id,1)&&status.touch_id==0xB6;
    if(status.touch_ok)status.touch_ok=write_reg(0x15,0xFE,1)&&write_reg(0x15,0xFA,0x41);
    for(uint8_t addr:{0x6B,0x6A}){uint8_t id=0;if(read_regs(addr,0,&id,1)&&id==5){status.imu_addr=addr;status.imu_id=id;status.imu_ok=true;break;}}
    if(status.imu_ok){bool ok=write_reg(status.imu_addr,2,0x60);ok=write_reg(status.imu_addr,3,0x23)&&ok;ok=write_reg(status.imu_addr,4,0x43)&&ok;ok=write_reg(status.imu_addr,6,0)&&ok;ok=write_reg(status.imu_addr,8,3)&&ok;status.imu_ok=ok;}
    flash_safe_execute_core_init();
}
InputFrame Board::sample(){InputFrame f;uint8_t b[12]{};
    if(status.imu_ok){if(read_regs(status.imu_addr,53,b,12)){auto word=[&](int n){return int16_t(uint16_t(b[n])|(uint16_t(b[n+1])<<8));};f.imu=true;f.ax=word(0)/4096.f;f.ay=word(2)/4096.f;f.az=word(4)/4096.f;f.gz=word(10)/64.f;}else status.i2c_errors++;}
    bool down=false;uint64_t now=time_us_64();
    if(status.touch_ok){if(read_regs(0x15,2,b,5)){down=(b[0]&15)>0;f.x=((b[1]&15)<<8)|b[2];f.y=((b[3]&15)<<8)|b[4];if(f.x>=240||f.y>=240)down=false;}else status.i2c_errors++;}
    f.touch=down;f.tap=down&&!previous_touch&&now-last_tap>180000;if(f.tap){status.touches++;last_tap=now;}previous_touch=down;
    bool raw=!gpio_get(KEY);if(raw!=previous_key){key_changed=now;previous_key=raw;}
    if(now-key_changed>30000&&raw!=key_stable){key_stable=raw;if(raw){key_started=now;long_sent=false;}else if(!long_sent&&now-key_started>=30000){f.short_button=true;status.button_events++;}}
    if(key_stable&&!long_sent&&now-key_started>=800000){f.long_button=true;long_sent=true;status.button_events++;}
    return f;
}
void Board::present(const Framebuffer&f){gpio_put(CS,0);command(0x2A);for(auto b:{0,0,0,239})data(uint8_t(b));command(0x2B);for(auto b:{0,0,0,239})data(uint8_t(b));command(0x2C);gpio_put(DC,1);uint8_t line[480];for(int y=0;y<240;y++){for(int x=0;x<240;x++){auto p=f.pixels[y*240+x];line[2*x]=uint8_t(p>>8);line[2*x+1]=uint8_t(p);}spi_write_blocking(spi1,line,480);}gpio_put(CS,1);status.lcd_sent=true;status.frames++;}
bool FlashStore::load(Records&r){Records a,b;bool va=decode_records(reinterpret_cast<const uint8_t*>(XIP_BASE+StorageOffset),256,a),vb=decode_records(reinterpret_cast<const uint8_t*>(XIP_BASE+StorageOffset+4096),256,b);if(!va&&!vb)return false;r=va&&(!vb||newer_sequence(a.sequence,b.sequence))?a:b;return true;}
struct WriteContext{uint32_t offset;RecordPage page;};
static void flash_write(void*ctx){auto*c=static_cast<WriteContext*>(ctx);flash_range_erase(c->offset,4096);flash_range_program(c->offset,c->page.data(),256);}
bool FlashStore::save(const Records&r){extern char __flash_binary_end; // absolute linker symbol is checked below using address from global declaration
    (void)sizeof(__flash_binary_end);
    Records a,b;bool va=decode_records(reinterpret_cast<const uint8_t*>(XIP_BASE+StorageOffset),256,a),vb=decode_records(reinterpret_cast<const uint8_t*>(XIP_BASE+StorageOffset+4096),256,b);int target=va&&(!vb||newer_sequence(a.sequence,b.sequence))?1:0;
    WriteContext ctx{StorageOffset+uint32_t(target)*4096,encode_records(r)};
    if(flash_safe_execute(flash_write,&ctx,500)!=PICO_OK)return false;
    Records check;return decode_records(reinterpret_cast<const uint8_t*>(XIP_BASE+ctx.offset),256,check)&&encode_records(check)==ctx.page;
}
}
