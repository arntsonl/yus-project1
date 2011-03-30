import math

ratio = 360.0/256.0

f = open('out.inc', 'w')

for i in xrange(0, 16):
	str = ".db "
	for j in xrange(0, 16):
		str += "$%02X, "%((math.sin(((j+i*16)*ratio)*(math.pi/180.0))+1.0)*19.0)
	str += "\n"
	f.write(str)

f.close()