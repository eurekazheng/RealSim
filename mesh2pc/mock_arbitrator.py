from subprocess import Popen, PIPE

cmd = "D:\studies\CIS-660\Authoring Tool\RealSim\RealSimCL\Debug\RealSimCL.exe"
p1 = Popen(['python', 'mesh2pc.py'], stdout=PIPE)
p2 = Popen([cmd], stdin=p1.stdout, stdout=PIPE)
output = p2.communicate()[0]

print(output)
