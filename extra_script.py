print("====Before_Build====")
Import("env")

import gzip
import shutil
import os
import subprocess
try:
    import configparser
except ImportError:
    import ConfigParser as configparser
import pip

def install_package(package):
    subprocess.call(["pip","install","--upgrade",package])

#install_package("html_utils_becothal")
install_package("beautifulsoup4")
install_package("html5lib")

#from html_utils import HTML
from prepare_webui import WebUiPacker


def before_uploadfs():
    print("----uploadfs----")
    environment = "env:esp8285"
    config = configparser.ConfigParser()
    config.read("platformio.ini")
    if config.get(environment,"web_ui_path") == "": 
        print("web_ui_path is not set")
        return

    inlined_webUiFilePath = config.get(environment,"web_ui_path")[:config.get(environment,"web_ui_path").find(".html")] + "_inlined.html"
    spiff_dir = config.get(environment,"spiff_dir")


    # Set all entries in minify to "None" to avoid web access
    webPackerOptions = {
        "minify": {
            "JS": "online_andychilton", # "None" | "online_andychilton"
            "CSS": "online_andychilton", # "None" | "online_andychilton"
            "HTML": "online_andychilton" # "None" | "online_andychilton"
        }
    }
    webPacker = WebUiPacker(webPackerOptions)
    webPacker.log = True

    webUiFilePath = config.get(environment,"web_ui_path")
    inlined_webUiFileContent = webPacker.processFile(webUiFilePath)

    with open(inlined_webUiFilePath, "w", encoding="utf8") as handle:
        handle.write(inlined_webUiFileContent)

    if not os.path.exists(spiff_dir):
        os.mkdir(spiff_dir)

    # Couldn't you write directly to the .gz file instead of using a temporary file? TODO: Check
    with open(inlined_webUiFilePath,"rb") as f_in, gzip.open(spiff_dir + "index.html.gz","wb") as f_out:
        shutil.copyfileobj(f_in,f_out) 
    os.remove(inlined_webUiFilePath)

print("Current build targets", map(str, BUILD_TARGETS)) 
if("uploadfs" in BUILD_TARGETS or "buildfs" in BUILD_TARGETS):
    before_uploadfs()
else: 
    print("No extraScript for this Target")



#env.AddPreAction("uploadfs",before_buildfs)

