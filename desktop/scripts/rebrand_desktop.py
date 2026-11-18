import re
import sys

src = sys.argv[1]
name = sys.argv[2] if len(sys.argv) > 2 else "Lord Gram"
path = src + "/Telegram/SourceFiles/core/version.h"
text = open(path, encoding="utf-8", newline="").read()
new, count = re.subn(
    r'(constexpr auto AppName = )"[^"]*"(_cs;)',
    lambda m: m.group(1) + '"' + name + '"' + m.group(2),
    text,
)
if count != 1:
    raise SystemExit("AppName line not found in version.h")
open(path, "w", encoding="utf-8", newline="").write(new)
print("AppName ->", name)
