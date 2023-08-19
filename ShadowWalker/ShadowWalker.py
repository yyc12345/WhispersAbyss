import BmmoContext, OutputHelper, CppHelper
import GetchHelper
import time, sys, re, argparse

output_helper = OutputHelper.OutputHelper()
output_helper.Print("ShadowWalker")
output_helper.Print("==========")
output_helper.Print("")

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
    if not re.match('^\*?[a-zA-Z0-9_+=.~()\-]+$', strl): raise argparse.ArgumentTypeError("Invalid username.")
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
parser.add_argument('-d', '--debug', action='store_true', dest='enable_debug')
parser.add_argument('-p', '--local-port', required=True, type=regulator_port, action='store', dest='local_port', metavar='6172')
parser.add_argument('-u', '--server-url', required=True, type=regulator_server_url, action='store', dest='server_url', metavar='127.0.0.1:25565')
parser.add_argument('-n', '--username', required=True, type=regulator_username, action='store', dest='username', metavar='ShadowPower')
parser.add_argument('-i', '--uuid', required=True, type=regulator_uuid, action='store', dest='uuid', metavar='11451400-1140-1140-1140-114514114514')

# parse arg
args = parser.parse_args()

# define chat related data
def DispatchChatType(ctx :BmmoContext.BmmoContext, rawchat: str):
    if rawchat.startswith("!"):
        ctx.AnnouncementChat(rawchat[1:])
        return
    if rawchat.startswith("#"):
        ctx.BulletinChat(rawchat[1:])
        return
    
    match = re.match('^@\*?[a-zA-Z0-9_+=.~()\-]+', rawchat)
    if match:
        username = match.group(0)[1:]
        chat_content = rawchat[len(match.group(0)):].lstrip(' ')
        ctx.WhisperChat(username, chat_content)
        return

    # no matched
    # normal chat
    ctx.Chat(rawchat)

# start context
ctx = BmmoContext.BmmoContext(output_helper, BmmoContext.BmmoContextParam(
    "127.0.0.1", args.local_port,
    args.server_url,
    args.username, args.uuid[:],
    args.enable_debug
))
ctx.Start()

# main loop
while True:
    inc = GetchHelper.getch()

    if inc == b'q':
        break
    elif inc == b'p':
        ctx.Profile()
    elif inc == b'\t':
        output_helper.StartInput()
        msg_content = input("> ")
        output_helper.StopInput()
        DispatchChatType(ctx, msg_content)
    else:
        if inc != b'h':
            output_helper.Print("Unknown command!")
        output_helper.Print("""Command help:
\tq: quit, p: show players, tab: chat, h: show help.
\tChat format:
\t\t> chat-content: Chat
\t\t> @username chat-content: Whisper
\t\t> !chat-content: Announcement
\t\t> #chat-content: Bulletin""")

# wait exit
ctx.Stop()
ctx.GetStateMachine().SpinUntil(CppHelper.State.Stopped)
