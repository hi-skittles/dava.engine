import subprocess
import os

print("current OS is ", os.name)

if os.name == "posix":
    subprocess.Popen(["python", "../../../scripts/local_http_pack_server.py"], shell=False, cwd="Data/TestData/PackManagerTest")
else:
    subprocess.Popen(["python", "../../../scripts/local_http_pack_server.py"], shell=True, cwd="Data/TestData/PackManagerTest")
