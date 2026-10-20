#include "neurocube/application.hpp"
#include <algorithm>
#include <cstdio>
namespace nc {
App::App(RecordStore&s,GameRegistry r):registry_(r),store_(s){store_.load(records_);}
void App::launch(int i,bool rules){if(i<0||i>=GameCount)return;game().stop();selected_=i;gestures_.reset();game().start(difficulty_,++seed_);screen_=rules?Screen::Rules:Screen::Playing;}
void App::restart(){launch(selected_,false);}
void App::pause(){if(screen_==Screen::Playing){game().pause();screen_=Screen::Paused;}}
void App::resume(){if(screen_==Screen::Paused){gestures_.reset();game().resume();screen_=Screen::Playing;}}
void App::finish(){auto result=game().result();if(selected_<3&&result.score>records_.best[selected_][int(difficulty_)]){records_.best[selected_][int(difficulty_)]=result.score;dirty_=true;}flush();}
void App::menu(){if(screen_==Screen::Playing||screen_==Screen::Paused||screen_==Screen::Result)finish();game().stop();screen_=Screen::Menu;gestures_.reset();}
void App::next(){finish();launch((selected_+1)%GameCount);}
bool App::flush(){if(!dirty_)return save_ok_;auto copy=records_;copy.sequence++;save_ok_=store_.save(copy);if(save_ok_){records_=copy;dirty_=false;}return save_ok_;}
void App::update(const InputFrame&i,float dt){
    dt=std::clamp(dt,0.f,.1f);clock_+=dt;auto a=gestures_.update(i,dt);
    if(i.long_button){menu();return;}if(i.short_button){next();return;}
    if(screen_==Screen::Menu){if(i.tap){if(i.y>=210){screen_=Screen::Settings;return;}int n=(i.y-43)/39;if(i.y>=43&&n<GameCount)launch(n);}return;}
    if(screen_==Screen::Settings){if(i.tap){if(i.y<90)difficulty_=Difficulty((int(difficulty_)+1)%3);else if(i.y<137)gestures_.sensitivity=gestures_.sensitivity<1.3f?1.5f:.8f;else if(i.y<183&&i.imu)gestures_.calibrate(i.ax,i.ay);else screen_=Screen::Menu;}return;}
    // Global touch toolbar consumes input before game actions and shake.
    if(i.tap&&i.y>=208){int cmd=std::clamp(i.x/60,0,3);if(cmd==0){if(screen_==Screen::Paused)resume();else pause();}if(cmd==1)restart();if(cmd==2)next();if(cmd==3)menu();return;}
    if(screen_==Screen::Rules){if(i.tap){screen_=Screen::Playing;gestures_.reset();}return;}
    if(screen_==Screen::Paused)return;
    if(screen_==Screen::Result){if(i.tap)restart();return;}
    if(a.shake&&selected_!=3){restart();return;}
    if(selected_==3&&a.tap&&a.y<177)a.tap=false;
    game().update(a,dt);
    if(game().result().finished){finish();screen_=Screen::Result;}
}
static void button(Renderer&r,int x,int y,int w,int h,const char*label,uint16_t c=Accent){r.rect(x,y,w,h,c);r.text(x+6,y+5,label,Bg);}
void App::render(Renderer&r)const{
    r.clear(Bg);char b[96];
    if(screen_==Screen::Menu){r.text(15,10,"NeuroCube",Accent,2);for(int j=0;j<GameCount;j++){button(r,10,43+j*39,220,34,registry_.games[j]->metadata().title,j==3?Gold:Accent);}r.text(12,212,"Настройки",Muted);return;}
    if(screen_==Screen::Settings){r.text(10,10,"Настройки",Accent,2);const char*levels[]={"Легко","Обычно","Сложно"};std::snprintf(b,sizeof b,"Уровень: %s",levels[int(difficulty_)]);button(r,8,48,224,36,b);button(r,8,94,224,36,"Чувствительность");button(r,8,140,224,36,"Калибровка");button(r,8,186,224,36,"В меню",Gold);return;}
    r.text(10,4,registry_.games[selected_]->metadata().title,Accent);
    if(screen_==Screen::Rules){r.text(12,40,"Как играть",Gold);r.text(12,69,registry_.games[selected_]->metadata().rule,Ink);if(selected_<3){std::snprintf(b,sizeof b,"Рекорд: %lu",(unsigned long)records_.best[selected_][int(difficulty_)]);r.text(12,140,b,Gold);}r.text(12,174,"Коснись для старта",Accent);}
    else if(screen_==Screen::Paused){r.text(35,75,"Пауза",Gold,2);r.text(12,125,"Отдохни и продолжай",Ink);}
    else if(screen_==Screen::Result){r.text(12,54,"Готово!",Gold,2);std::snprintf(b,sizeof b,"Счёт: %lu",(unsigned long)registry_.games[selected_]->result().score);r.text(12,100,b,Ink);r.text(12,145,"Коснись: ещё попытка",Accent);if(!save_ok_)r.text(12,173,"Ошибка сохранения",Red);}
    else registry_.games[selected_]->render(r);
    const char* labels[]={screen_==Screen::Paused?"Играть":"Пауза","Заново","Далее","Меню"};for(int j=0;j<4;j++){r.rect(j*60,208,59,32,0x294A);r.text(j*60+2,216,labels[j],Ink);}
}
}
