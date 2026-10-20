#include "neurocube/games.hpp"
#include "neurocube/application.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace nc {
static const uint16_t colors[]={0x3E7F,0xFEA0,0xFB0C,0x9BFF};
static const char* directions[]={"Вверх","Вправо","Вниз","Влево","Поверни","Коснись"};
static void score(Renderer&r,uint32_t s){char b[48];std::snprintf(b,sizeof b,"Счёт: %lu",(unsigned long)s);r.text(12,28,b,Gold);}
void SequenceGame::start(Difficulty d,uint32_t seed){rng.state=seed?seed:1;length=1;cursor=0;timer=0;shown=-1;showing=true;outcome={};step=.8f-int(d)*.16f;for(auto&v:sequence)v=int(rng.bounded(4));}
void SequenceGame::update(const Actions&a,float dt){
    if(outcome.finished)return;
    if(showing){timer+=dt;int slot=int(timer/step);if(slot>=length){showing=false;timer=0;cursor=0;shown=-1;}else shown=std::fmod(timer,step)<step*.72f?sequence[slot]:-1;return;}
    int dir=a.direction;
    if(a.tap){if(a.y<100)dir=0;else if(a.y>160)dir=2;else dir=a.x>120?1:3;}
    if(dir>=0){shown=dir;timer=.18f;if(dir!=sequence[cursor]){outcome.finished=true;return;}cursor++;if(cursor==length){outcome.score=uint32_t(length);length++;if(length>32){outcome.finished=true;return;}cursor=0;showing=true;timer=-.4f;shown=-1;}}
    if(!showing){timer=std::max(0.f,timer-dt);if(timer==0)shown=-1;}
}
void SequenceGame::render(Renderer&r)const{
    score(r,outcome.score);r.text(12,50,showing?"Смотри и запоминай":"Теперь повтори",Ink);
    const int xs[]={93,155,93,31},ys[]={73,125,163,125};
    for(int i=0;i<4;i++){r.rect(xs[i],ys[i],54,36,i==shown?Ink:colors[i]);r.text(xs[i]+23,ys[i]+8,i==0?"1":i==1?"2":i==2?"3":"4",Bg);}
}
bool circle_hits_wall(float x,float y,float radius,const Wall&w){float cx=std::clamp(x,w.x,w.x+w.w),cy=std::clamp(y,w.y,w.y+w.h);return (x-cx)*(x-cx)+(y-cy)*(y-cy)<radius*radius;}
static constexpr Wall walls[]={{65,48,10,104},{125,104,10,96},{185,48,10,104}};
void MazeGame::start(Difficulty d,uint32_t){x=22;y=65;vx=vy=elapsed=0;level=d;outcome={};}
void MazeGame::update(const Actions&a,float dt){
    if(outcome.finished)return;dt=std::clamp(dt,0.f,.1f);elapsed+=dt;float gain=180+int(level)*70;
    vx=(vx+a.tilt_x*gain*dt)*std::exp(-dt*2);vy=(vy+a.tilt_y*gain*dt)*std::exp(-dt*2);
    // Small substeps prevent crossing thin walls at high velocity.
    for(int n=0;n<4;n++){float nx=std::clamp(x+vx*dt/4,8.f,232.f);bool hit=false;for(auto&w:walls)hit|=circle_hits_wall(nx,y,6,w);if(hit)vx=0;else x=nx;float ny=std::clamp(y+vy*dt/4,55.f,196.f);hit=false;for(auto&w:walls)hit|=circle_hits_wall(x,ny,6,w);if(hit)vy=0;else y=ny;}
    if(x>211&&y>171){outcome={uint32_t(std::max(1.f,1000-elapsed*8)),true};}
    if(elapsed>120-int(level)*20)outcome.finished=true;
}
void MazeGame::render(Renderer&r)const{char b[40];std::snprintf(b,sizeof b,"Время: %d",int(elapsed));r.text(12,28,b,Gold);r.rect(4,48,232,154,0x18E7);for(auto&w:walls)r.rect(int(w.x),int(w.y),int(w.w),int(w.h),Muted);r.rect(210,172,24,28,Accent);r.circle(int(x),int(y),6,Gold);}
void CommandsGame::choose(){expected=int(rng.bounded(6));remaining=limit;}
void CommandsGame::start(Difficulty d,uint32_t seed){rng.state=seed?seed:1;outcome={};limit=3.2f-int(d)*.6f;choose();}
void CommandsGame::update(const Actions&a,float dt){if(outcome.finished)return;remaining-=dt;if(remaining<=0){outcome.finished=true;return;}int action=a.direction;if(a.twist)action=4;if(a.tap)action=5;if(action<0)return;if(action!=expected){outcome.finished=true;return;}outcome.score++;limit=std::max(.75f,limit*.965f);if(outcome.score>=30){outcome.finished=true;return;}choose();}
void CommandsGame::render(Renderer&r)const{score(r,outcome.score);r.circle(120,116,49,colors[expected%4]);r.text(22,105,directions[expected],Bg,2);r.rect(12,181,216,10,0x2128);r.rect(12,181,int(216*std::max(0.f,remaining)/limit),10,Accent);r.text(12,155,expected==4?"Поворот вокруг экрана":"Следуй подсказке",Ink);}
static const char* answers[]={"Да, действуй!","Всё получится","Спроси чуть позже","Попробуй иначе","Удача рядом","Сделай первый шаг","Доверься себе","Нужна пауза","Ответ уже рядом","Сегодня твой день","Лучше не спешить","Позови друга"};
void MagicBall::start(Difficulty,uint32_t seed){rng.state=seed?seed:1;answer=-1;animation=0;time=0;}
void MagicBall::update(const Actions&a,float dt){time+=dt;animation=std::max(0.f,animation-dt);if(a.shake||a.tap){int next=int(rng.bounded(12));if(next==answer)next=(next+1)%12;answer=next;animation=.7f;}}
void MagicBall::render(Renderer&r)const{int radius=60+int(std::sin(time*3)*3);r.circle(120,112,radius,0x39B0);r.circle(120,112,radius-6,0x18CB);r.text(14,101,animation>0?"Думаю...":answer<0?"Задай свой вопрос":answers[answer],Ink);r.rect(12,177,216,25,Accent);r.text(45,180,"Спросить ещё",Bg);}
GameRegistry default_registry(){static SequenceGame s;static MazeGame m;static CommandsGame c;static MagicBall b;return {{{&s,&m,&c,&b}}};}
}
