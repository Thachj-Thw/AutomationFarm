import re
import os
import datetime

base = '''const char PROGMEM index_html[] = R"rawliteral(%s)rawliteral";'''

def path(p):
    return os.path.normpath(os.path.join(os.path.dirname(__file__), p))

with open(path("index.html"), "r", encoding="utf-8") as f:
    html = re.sub("\s{4}|\n", "", f.read())

with open(path("css/style.css"), "r", encoding="utf-8") as f:
    css = re.sub("\s{4}|\n", "", f.read())

with open(path("scripts/navbar.js"), "r", encoding="utf-8") as f:
    js = re.sub("(?<!http:)//.*", "", f.read())
    js1 = re.sub("\s{4}|\n", "", js)

with open(path("scripts/index.js"), "r", encoding="utf-8") as f:
    js = re.sub("(?<!http:)//.*", "", f.read())
    js2 = re.sub("\s{4}|\n", "", js)

html = html.replace('<link rel="stylesheet" href="css/style.css">', "<style>"+css+"</style>")
html = html.replace('<script src="scripts/navbar.js"></script>', "<script>"+js1+"</script>")
html = html.replace('<script src="scripts/index.js"></script>', "<script>"+js2+"</script>")

with open(path("../Farm/Html.h"), "w", encoding="utf-8") as f:
    f.write(base % html)

with open(path("test.html"), "w", encoding="utf-8") as f:
    f.write(html)
print(datetime.datetime.now(), "success")