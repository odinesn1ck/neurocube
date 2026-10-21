"""Read-only production USB checks. No synthetic input, reset, flash or record writes."""
import argparse
import datetime
import json
import time
from pathlib import Path
import neurocube as bridge


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial', default=bridge.DEFAULT_SERIAL)
    parser.add_argument('--baseline', type=Path, help='JSON records response captured before update')
    parser.add_argument('--output', type=Path, default=bridge.ROOT/'artifacts/polish-usb-smoke.json')
    args = parser.parse_args()
    report = {'timestamp_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'level': 'VERIFIED_ON_HARDWARE', 'physical_controls': 'NOT_TESTED',
              'battery_autonomy': 'NOT_TESTED', 'checks': {}}
    with bridge.Connection(args.serial) as c:
        report['hello'] = c.hello()
        if report['hello'].get('debug_input') is not False:
            raise bridge.BridgeError('Expected production firmware with debug input disabled')
        report['before'] = c.request('records')
        report['power'] = c.request('power')
        report['settings'] = c.request('settings')
        report['selftest'] = c.request('selftest')
        before = c.request('status')
        time.sleep(1)
        after = c.request('status')
        report['status'] = after
        report['after'] = c.request('records')
        report['checks']['frame_progress'] = after['frames'] > before['frames']
        report['checks']['no_new_i2c_errors'] = after['i2c_errors'] == before['i2c_errors']
        report['checks']['records_unchanged_during_readonly_test'] = report['before'] == report['after']
        report['checks']['latch_high'] = report['power'].get('bat_en') is True
        report['checks']['electronic_selftest'] = all(report['selftest'].get(k) is True for k in
            ('imu_identity', 'imu_sample', 'touch_identity', 'lcd_spi_sent'))
        if args.baseline:
            baseline = json.loads(args.baseline.read_text(encoding='utf-8'))
            report['checks']['legacy_scores_preserved'] = baseline['best'] == report['after']['best']
            if 'maze_best' in baseline:
                report['checks']['maze_scores_preserved'] = baseline['maze_best'] == report['after'].get('maze_best')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'passed': all(report['checks'].values()), 'report': str(args.output)}))
    return 0 if all(report['checks'].values()) else 1


if __name__ == '__main__':
    raise SystemExit(main())
