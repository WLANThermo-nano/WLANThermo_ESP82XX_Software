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

    spiff_dir = config.get(environment,"spiff_dir")

    if not os.path.exists(spiff_dir):
        os.mkdir(spiff_dir)
    
    webUiCompressedFilePath = spiff_dir + "index.html.gz"
    with gzip.open(webUiCompressedFilePath,"wb") as f_out:
        f_out.write(inlined_webUiFileContent.encode("utf-8"))
        print("Created ", webUiCompressedFilePath)

print("Current build targets", map(str, BUILD_TARGETS)) 
if("uploadfs" in BUILD_TARGETS or "buildfs" in BUILD_TARGETS):
    before_uploadfs()
else: 
    print("No extraScript for this Target")


#env.AddPreAction("uploadfs",before_buildfs)

