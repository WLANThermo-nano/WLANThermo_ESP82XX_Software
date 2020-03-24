#! /usr/bin/env python
# NOTE: USE PYTHON 3

from prepare_webui import WebUiPacker
import os

webPackerOptions = {
    "inline": {
        "JS": True,
        "CSS": True,
        "IMG": True
    },
    "remove": {
        "level_empty_lines": 1,
        "comments": True
    },
    "minify": {
        "JS": "None", # "None" | "online_andychilton"
        "CSS": "None", # "None" | "online_andychilton"
        "HTML": "None" # "None" | "online_andychilton"
    },
    "prettify": {
        "HTML": True
    }
}
webPacker = WebUiPacker(webPackerOptions)
webPacker.log = True

webUiFilePath = "webui/index.html"
inlined_webUiFileContent = webPacker.processFile(webUiFilePath)

#print(inlined_webUiFileContent)
pathParts = os.path.splitext(webUiFilePath)
outFilePath = pathParts[0] + ".tmp" + pathParts[1]
with open(outFilePath, "w", encoding="utf8") as handle:
    handle.write(inlined_webUiFileContent)
    print("Wrote out file:", outFilePath)
