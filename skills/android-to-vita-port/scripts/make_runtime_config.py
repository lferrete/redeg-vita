"""Create an auditable XML config experiment without changing the source file."""
import argparse
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--set', dest='settings', action='append', required=True,
                        metavar='GROUP.PROP=VALUE')
    args = parser.parse_args()
    if args.source.resolve() == args.output.resolve():
        parser.error('output must differ from source; preserve the baseline')
    if args.output.exists():
        parser.error('output already exists; choose a new experiment file')
    source = args.source.read_bytes()
    root = ET.fromstring(source)
    changes, seen = [], set()
    for setting in args.settings:
        key, equals, value = setting.partition('=')
        group, dot, prop = key.partition('.')
        if not equals or not dot or not group or not prop or not value or key in seen:
            parser.error(f'invalid or repeated setting: {setting}')
        matches = [p for g in root if g.tag == 'group' and g.get('name') == group
                   for p in g if p.tag == 'prop' and p.get('name') == prop]
        if len(matches) != 1:
            parser.error(f'{key}: expected one property, found {len(matches)}')
        seen.add(key)
        changes.append({'property': key, 'before': matches[0].get('value'), 'after': value})
        matches[0].set('value', value)
    ET.indent(root, space='  ')
    output = (ET.tostring(root, encoding='unicode') + '\n').encode('utf-8')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(output)
    print(json.dumps({'source': str(args.source), 'source_sha256': hashlib.sha256(source).hexdigest(),
                      'output': str(args.output), 'output_sha256': hashlib.sha256(output).hexdigest(),
                      'changes': changes}, indent=2))


if __name__ == '__main__':
    main()
