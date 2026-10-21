
#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "audio_pio.pio.h"
#include "es8311.h"
#include "DEV_Config.h"
static bool io_ok=true;
void DEV_Delay_Ms(uint32_t ms){sleep_ms(ms);}
void DEV_I2C_Write_Byte(i2c_inst_t *port,uint8_t addr,uint8_t reg,uint8_t value){
 uint8_t data[]={reg,value};if(i2c_write_timeout_us(port,addr,data,2,false,20000)!=2)io_ok=false;
}
uint8_t DEV_I2C_Read_Byte(i2c_inst_t *port,uint8_t addr,uint8_t reg){
 uint8_t v=0;if(i2c_write_timeout_us(port,addr,&reg,1,true,20000)!=1 || i2c_read_timeout_us(port,addr,&v,1,false,20000)!=1)io_ok=false;return v;
}
static void restore(void){
 gpio_put(0,false);
 watchdog_reboot(0,0,100);
 while(true)tight_loop_contents();
}
int main(void){
 gpio_init(28);gpio_put(28,true);gpio_set_dir(28,GPIO_OUT);
 gpio_init(0);gpio_put(0,false);gpio_set_dir(0,GPIO_OUT);
 stdio_init_all();
 // Even an I2S fault returns to the installed image; never write Flash.
 watchdog_enable(8000,true);
 i2c_init(i2c1,400000);gpio_set_function(6,GPIO_FUNC_I2C);gpio_set_function(7,GPIO_FUNC_I2C);gpio_pull_up(6);gpio_pull_up(7);
 sleep_ms(3000);
 uint16_t id=Es8311_Read_Id();
 printf("AUDIO_PROBE RAM ONLY codec=%04x io_ok=%d\n",id,io_ok);
 if(!io_ok || (id!=0x8311 && id!=0x1183)){sleep_ms(500);restore();}
 Es8311_Init(pico_audio);
 Es8311_Voice_Mute(true);Es8311_Voice_Volume_Set(60);
 if(!io_ok)restore();
 uint off=pio_add_program(pio1,&mclk_pio_program);
 mclk_pio_program_init(pio1,2,off,3);
 pio_sm_set_clkdiv(pio1,2,(float)clock_get_hz(clk_sys)/(24000.f*256.f*5.f));
 pio_sm_set_enabled(pio1,2,true);
 off=pio_add_program(pio2,&audio_pio_program);
 audio_pio_program_init(pio2,0,off,1,5);pio_sm_set_enabled(pio2,0,true);
 // Prime I2S with zeroes while muted; no microphone recording/loopback.
 for(int n=0;n<16;++n)pio_sm_put_blocking(pio2,0,0);
 Es8311_Voice_Mute(false);if(!io_ok)restore();
 printf("AUDIO_PROBE tone 1000 Hz, 300 ms, codec volume 60, PCM peak 8191\n");
 gpio_put(0,true);
 for(int n=0;n<7200;++n){
  float env=fminf(1.f,fminf(n/120.f,(7199-n)/120.f));
  int16_t sample=(int16_t)(8191.f*env*sinf(6.283185307f*1000.f*n/24000.f));
  uint32_t bits=(uint16_t)sample;pio_sm_put_blocking(pio2,0,(bits<<16)|bits);
 }
 for(int n=0;n<8;++n)pio_sm_put_blocking(pio2,0,0);
 gpio_put(0,false);Es8311_Voice_Mute(true);
 printf("AUDIO_PROBE completed; returning to NeuroCube\n");sleep_ms(100);restore();
}
