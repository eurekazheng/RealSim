import subprocess
import random

cmd = "D:\studies\CIS-660\Authoring Tool\RealSim\RealSimCL\Debug\RealSimCL.exe"
inputStr = ""
for i in range(25000):
  inputStr += str(random.random()) + "\n"
p = subprocess.Popen([cmd], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
output = p.communicate(input=inputStr)
print(output.splitlines())
