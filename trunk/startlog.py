import M5e
r = M5e.M5e(readPwr=3000, compact=False, portSTR='/dev/robot/rfidreader')
r.LoopQuery()
