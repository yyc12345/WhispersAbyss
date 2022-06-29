import BmmoContext
import OutputHelper
import GetchHelper
import time
import sys
import re

mOutputHelper = OutputHelper.OutputHelper()
mOutputHelper.Print("ShadowWalker")
mOutputHelper.Print("==========")
mOutputHelper.Print("")

# analyse parameter
if len(sys.argv) != 4:
    print("Wrong arguments.")
    print("Syntax: python3 ShadowWalker.py [port] [username] [uuid]")
    print("Program will exit. See README.md for more detail about commandline arguments.")
    sys.exit(1)

# get str args
argsParsedFailed = False
strPort = sys.argv[1]
strUsername = sys.argv[2]
strUuid = sys.argv[3].lower()

# check port
if not re.match('[0-9]+', strPort):
    argsParsedFailed = True
else:
    strPort = int(strPort)
    if strPort > 65535 or strPort < 0:
        argsParsedFailed = True
    else:
        argsPort = strPort

# check username
if not re.match('^[_0-9a-zA-Z]+$', strUsername):
    argsParsedFailed = True
else:
    argsUsername = strUsername

# check uuid
def hex_ascii_to_int(hexnum):
    idx = ord(hexnum)
    if idx <= ord('9'):
        return idx - ord('0')
    else:
        return idx - ord('a') + 10
if not re.match('^[0-9a-f]{8}\-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$', strUuid):
    argsParsedFailed = True
else:
    strUuid = strUuid.replace('-', '')
    argsUuid = []
    for i in range(16):
        value = hex_ascii_to_int(strUuid[i*2]) << 4
        value += hex_ascii_to_int(strUuid[i*2+1])
        argsUuid.append(value)

if argsParsedFailed:
    print("Wrong arguments. Arguments value is illegal.")
    print("Syntax: python3 ShadowWalker.py [port] [username] [uuid]")
    print("Program will exit. See README.md for more detail about commandline arguments.")
    sys.exit(1)

# start context
mBmmoCtx = BmmoContext.BmmoContext(mOutputHelper, "127.0.0.1", argsPort, argsUsername, tuple(argsUuid))
while True:
    inc = GetchHelper.getch()

    if inc == b'q':
        mBmmoCtx.SetCommand(BmmoContext.ContextCommandType.ExitCmd, None)
        break
    elif inc == b'p':
        mBmmoCtx.SetCommand(BmmoContext.ContextCommandType.ProfileCmd, None)
    elif inc == b'\t':
        mOutputHelper.StartInput()
        msg_content = input("> ")
        mOutputHelper.StopInput()

        mBmmoCtx.SetCommand(BmmoContext.ContextCommandType.ChatCmd, (msg_content, ))
    else:
        if inc != b'h':
            mOutputHelper.Print("Unknow command!")
        mOutputHelper.Print("Command help:\nq: quit, p: show profile, tab: send message, h: show help.")

# wait exit
while True:
    if mBmmoCtx.IsDead():
        break
    time.sleep(0.01)

