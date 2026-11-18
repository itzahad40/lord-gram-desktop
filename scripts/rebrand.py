#!/usr/bin/env python3
"""Rebrand a patched Telegram checkout into Lord Gram.
usage: rebrand.py <telegram-src-dir> <lordgram.config>
Refuses to run without your own api_id/api_hash (Telegram's official keys must not be reused).
"""
import re, sys, pathlib

def load(cfg):
    d = {}
    for line in pathlib.Path(cfg).read_text().splitlines():
        line = line.strip()
        if line and not line.startswith('#') and '=' in line:
            k, v = line.split('=', 1)
            d[k.strip()] = v.strip()
    return d

def sub(path, pattern, repl, flags=0):
    p = pathlib.Path(path)
    s = p.read_text(encoding='utf-8')
    new, n = re.subn(pattern, repl, s, count=1, flags=flags)
    if n != 1:
        sys.exit(f"rebrand: pattern not found in {path}: {pattern}")
    p.write_text(new, encoding='utf-8')

def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    src, cfg = pathlib.Path(sys.argv[1]), load(sys.argv[2])
    for k in ('LG_PACKAGE', 'LG_APP_NAME', 'LG_API_ID', 'LG_API_HASH'):
        if not cfg.get(k):
            sys.exit(f"rebrand: {k} missing in config")
    if not cfg['LG_API_ID'].isdigit():
        sys.exit("rebrand: LG_API_ID must be a number")
    if not re.fullmatch(r'[a-z][a-z0-9_]*(\.[a-z][a-z0-9_]*)+', cfg['LG_PACKAGE']):
        sys.exit("rebrand: LG_PACKAGE is not a valid Android package name")
    if cfg['LG_PACKAGE'].startswith('org.telegram'):
        sys.exit("rebrand: choose your own package, not org.telegram.*")

    sub(src / 'gradle.properties', r'^APP_PACKAGE=.*$', f"APP_PACKAGE={cfg['LG_PACKAGE']}", re.M)
    bv = src / 'TMessagesProj/src/main/java/org/telegram/messenger/BuildVars.java'
    sub(bv, r'public static int APP_ID = \d+;', f"public static int APP_ID = {cfg['LG_API_ID']};")
    sub(bv, r'public static String APP_HASH = "[^"]*";', f'public static String APP_HASH = "{cfg["LG_API_HASH"]}";')
    sub(src / 'TMessagesProj/src/main/res/values/strings.xml', r'(<string name="AppName">)[^<]*(</string>)', rf"\g<1>{cfg['LG_APP_NAME']}\g<2>")
    print("rebrand ok:", cfg['LG_PACKAGE'], '/', cfg['LG_APP_NAME'])

main()
