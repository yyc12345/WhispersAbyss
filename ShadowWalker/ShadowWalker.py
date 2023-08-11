#import BmmoContext
import OutputHelper
import GetchHelper
import time, sys, re, argparse

mOutputHelper = OutputHelper.OutputHelper()
mOutputHelper.Print("ShadowWalker")
mOutputHelper.Print("==========")
mOutputHelper.Print("")

# analyse parameter
# define some arg regulator
def hex_ascii_to_int(hexnum):
    idx = ord(hexnum)
    if idx <= ord('9'):
        return idx - ord('0')
    else:
        return idx - ord('a') + 10

def regulator_port(strl: str) -> int:
    if not re.match('[0-9]+', strl): raise argparse.ArgumentTypeError("Invalid port.")
    port = int(strl)
    if port > 65535 or port < 0: raise argparse.ArgumentTypeError("Port out of range: (0, 65535).")
    return port
def regulator_server_url(strl: str) -> str:
    urlsp = strl.split(':')
    if len(urlsp) != 2: raise argparse.ArgumentTypeError("Invalid server url.")
    regulator_port(urlsp[1])
    return strl
def regulator_username(strl: str) -> str:
    if not re.match('^[_0-9a-zA-Z]+$', strl): raise argparse.ArgumentTypeError("Invalid username.")
    return strl
def regulator_uuid(strl: str) -> tuple[int]:
    if not re.match('^[0-9a-f]{8}\-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$', strl):
        raise argparse.ArgumentTypeError("Invalid uuid.")
    struuid = strl.replace('-', '')
    uuid: tuple[int] = []
    for i in range(16):
        value = hex_ascii_to_int(struuid[i * 2]) << 4
        value += hex_ascii_to_int(struuid[i * 2 + 1])
        uuid.append(value)
    return tuple(uuid)

# setup parser
parser = argparse.ArgumentParser(description='The Walker of BallanceMMO.')
parser.add_argument('-p', '--local-port', required=True, type=regulator_port, action='store', dest='local_port', metavar='6172')
parser.add_argument('-u', '--server-url', required=True, type=regulator_server_url, action='store', dest='server_url', metavar='127.0.0.1:25565')
parser.add_argument('-n', '--username', required=True, type=regulator_username, action='store', dest='username', metavar='ShadowPower')
parser.add_argument('-i', '--uuid', required=True, type=regulator_uuid, action='store', dest='uuid', metavar='11451400-1140-1140-1140-114514114514')

# parse arg
args = parser.parse_args()
print(args)

"""
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
"""

