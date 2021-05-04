from subprocess import Popen, PIPE
import os

os.environ['PYTHONHOME'] = 'D:/studies/CIS-660/Authoring Tool/RealSim/mesh2pc/python'
os.environ['PYTHONPATH'] = 'D:/studies/CIS-660/Authoring Tool/RealSim/mesh2pc/python'

p1 = Popen(['D:/studies/CIS-660/Authoring Tool/RealSim/mesh2pc/python/python',
            'D:/studies/CIS-660/Authoring Tool/RealSim/mesh2pc/mesh2pc.py'],
           stdin=PIPE, stdout=PIPE, stderr=PIPE)
# p2 = Popen([cmd], stdin=p1.stdout, stdout=PIPE)
output, err = p1.communicate()

print(err)
print(output)
