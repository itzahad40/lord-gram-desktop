import shutil
import sys

src = sys.argv[1]
logo = sys.argv[2]
target = src + "/Telegram/Resources/art/lordgram_fight.png"
shutil.copyfile(logo, target)

qrc = src + "/Telegram/Resources/qrc/telegram/telegram.qrc"
text = open(qrc, encoding="utf-8", newline="").read()
entry = '<file alias="art/lordgram_fight.png">../../art/lordgram_fight.png</file>'
if entry not in text:
    anchor = '<file alias="art/premium/flecks.png">../../art/premium/flecks.png</file>'
    if anchor not in text:
        raise SystemExit("qrc anchor not found")
    eol = "\r\n" if "\r\n" in text else "\n"
    text = text.replace(anchor, anchor + eol + "    " + entry, 1)
    open(qrc, "w", encoding="utf-8", newline="").write(text)
print("fight logo registered in qrc")
