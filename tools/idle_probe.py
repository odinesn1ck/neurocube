"""Observe production idle and physical wake over USB; never inject, reset or write."""
import argparse
import datetime
import json
import time
from pathlib import Path
import neurocube as bridge


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds',type=float,default=420)
    parser.add_argument('--output',type=Path,default=bridge.ROOT/'artifacts/idle-probe.json')
    parser.add_argument('--serial',default=bridge.DEFAULT_SERIAL)
    args=parser.parse_args()
    report={'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'source':'production USB telemetry; no synthetic input',
            'physical_visual_confirmation':'NOT_TESTED','events':[]}
    start=time.monotonic()
    with bridge.Connection(args.serial) as c:
        report['hello']=c.hello()
        if report['hello'].get('debug_input') is not False:
            raise bridge.BridgeError('Production firmware required')
        report['records_before']=c.request('records')
        previous=None
        dark=None
        while time.monotonic()-start<args.seconds:
            status=c.request('status');display=c.request('display')
            key=(status['screen'],display['backlight_requested'])
            row={'seconds':round(time.monotonic()-start,2),'screen':status['screen'],
                 'frames':status['frames'],'touches':status['touches'],
                 'button_events':status['button_events'],'i2c_errors':status['i2c_errors'],
                 'display':display}
            if key!=previous:
                report['events'].append(row)
                print(json.dumps(row),flush=True)
                previous=key
            if display['idle'] and dark is None:
                dark=row;report['dark_power']=c.request('power')
            elif dark is not None and not display['idle']:
                time.sleep(.3)
                after=c.request('status')
                report.setdefault('wake_observations',[]).append({'before':dark,'after':after,
                    'pwm_restored':display['pwm_level']==256,
                    'returned_to_menu':after['screen']==0,
                    'touch_event_seen':after['touches']>dark['touches'],
                    'button_event_seen':after['button_events']>dark['button_events']})
                dark=None
            args.output.parent.mkdir(parents=True,exist_ok=True)
            args.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
            if len(report.get('wake_observations',[]))>=2:break
            time.sleep(.5)
        report['records_after']=c.request('records')
        report['records_preserved']=report['records_before']==report['records_after']
        report['final_power']=c.request('power')
    args.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'report':str(args.output),'wake_observations':len(report.get('wake_observations',[])),
                      'records_preserved':report['records_preserved']}),flush=True)


if __name__=='__main__':main()
