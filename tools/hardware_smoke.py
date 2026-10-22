"""Explicitly synthetic hardware integration; never certifies physical controls."""
from pathlib import Path
import argparse
import datetime
import json
import time
import neurocube as bridge

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--serial',default=bridge.DEFAULT_SERIAL)
parser.add_argument('--allow-synthetic-and-reboot',action='store_true',required=True)
parser.add_argument('--clear-generated-records',action='store_true')
args=parser.parse_args()
report={'level':'VERIFIED_ON_HARDWARE','input_source':'SYNTHETIC_DEBUG_INPUT','physical_controls':'NOT_TESTED','battery':'NOT_TESTED','started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'checks':[]}
out=bridge.ROOT/'artifacts';out.mkdir(exist_ok=True)
def check(name,condition,detail=None):
    report['checks'].append({'name':name,'passed':bool(condition),'detail':detail})
    print(name,condition,flush=True)
    if not condition:raise RuntimeError(name)
def inject(c,tap=0,x=120,y=100,ax=0,ay=0,az=1,gz=0,wait=.18):
    reply=c.request(f'inject {tap} {x} {y} {ax} {ay} {az} {gz}')
    check('inject_ack',reply.get('type')=='injected')
    time.sleep(wait)
def tap(c,x,y):inject(c,1,x,y)
def state(c):return c.request('status')
def menu(c):
    if state(c)['screen']!=0:tap(c,220,220)
    check('menu',state(c)['screen']==0)
def launch(c,index):
    menu(c);tap(c,100,60+index*39);check(f'game_{index}_rules',state(c)['screen']==1)
    tap(c,120,100);check(f'game_{index}_playing',state(c)['screen']==2)

def session():
    deadline=time.monotonic()+15
    while True:
        try:bridge.select_port(args.serial);break
        except bridge.BridgeError:
            if time.monotonic()>deadline:raise
            time.sleep(.3)
    with bridge.Connection(args.serial) as c:
        hello=c.hello();check('debug_identity',hello.get('debug_input') is True,hello)
        before=c.request('records');report['baseline_records']=before
        if args.clear_generated_records:check('safe_empty_baseline',not any(before['best']))
        inject(c);check('electronic_selftest',all(c.request('selftest').get(k) for k in ('imu_identity','imu_sample','touch_identity','lcd_spi_sent')))
        launch(c,0)
        cue=-1
        deadline=time.monotonic()+4
        while time.monotonic()<deadline:
            debug=c.request('debug')
            if debug.get('cue',-1)>=0:cue=debug['cue']
            if not debug.get('playback',True):break
            time.sleep(.08)
        check('sequence_cue',cue>=0,{'cue':cue})
        positions=[(120,80),(180,140),(120,180),(45,140)]
        tap(c,*positions[cue]);check('sequence_round',state(c)['score']==1)
        tap(c,25,220);paused=state(c);check('pause',paused['screen']==3)
        time.sleep(.5);check('paused_score_stable',state(c)['score']==paused['score'])
        tap(c,25,220);check('resume',state(c)['screen']==2)
        menu(c);records=c.request('records');check('sequence_record_written',records['best'][1]==1,records)
        launch(c,1);inject(c,ax=.8,wait=.6);check('maze_running',state(c)['screen']==2)
        inject(c);bridge.screenshot(c,out/'hardware-maze.png');menu(c)
        launch(c,2);inject(c,wait=1.05)
        debug=c.request('debug');expected=debug['command'];check('command_visible',0<=expected<6,debug)
        if expected<4:
            axes=[(0,-.8),(.8,0),(0,.8),(-.8,0)][expected];inject(c,ax=axes[0],ay=axes[1],wait=.25)
        elif expected==4:inject(c,gz=220,wait=.15)
        else:tap(c,120,120)
        check('command_accepted',state(c)['score']>=1)
        inject(c);menu(c)
        launch(c,3);before_answer=c.request('debug')['answer'];tap(c,120,187);after_answer=c.request('debug')['answer']
        check('magic_touch',after_answer>=0 and after_answer!=before_answer)
        inject(c,wait=1.1);inject(c,az=3,wait=.15);inject(c);new_answer=c.request('debug')['answer']
        check('magic_shake_no_restart',state(c)['screen']==2 and new_answer!=after_answer)
        bridge.screenshot(c,out/'hardware-magic.png');menu(c)
        saved=c.request('records');report['saved_records']=saved
        c.request('release_input')
    with bridge.exclusive(args.serial):
        bridge.run([bridge.picotool(),'reboot','--ser',args.serial,'-f'],capture_output=True,text=True)
    time.sleep(2)
    with bridge.Connection(args.serial) as c:
        c.hello();after=c.request('records');report['after_reboot_records']=after
        check('records_survive_real_reboot',after==saved,after)
        if args.clear_generated_records:
            check('clear_synthetic_scores',c.request('clear_test_records')['type']=='test_records_cleared')
    if args.clear_generated_records:
        with bridge.exclusive(args.serial):bridge.run([bridge.picotool(),'reboot','--ser',args.serial,'-f'],capture_output=True,text=True)
        time.sleep(2)
        with bridge.Connection(args.serial) as c:
            c.hello();clean=c.request('records');report['clean_records']=clean
            check('synthetic_scores_removed',not any(clean['best']))
try:
    session();report['passed']=True
except Exception as e:
    report['passed']=False;report['error']=str(e);raise
finally:
    (out/'hardware-synthetic-smoke.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
