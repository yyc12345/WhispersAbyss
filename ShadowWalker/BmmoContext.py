import BmmoClient
import BmmoProto
import OutputHelper
import threading
import time

# ============= Basic Instance =============
mOutputHelper = OutputHelper.OutputHelper()
mBmmoClient = BmmoClient.BmmoClient(mOutputHelper)

# ============= Useful Funcs =============
def GetUsernameFromGnsUid(maps: dict, uid: int) -> str:
    if uid == 0:
        return "[Server]"
    else:
        player_entity = maps.get(msg.player_id, None)
        if player_entity is None:
            return "[Unknow]"
        else:
            return player_entity.nickname

def CountdownTypeToString(cdtype: int) -> str:
    if cdtype == BmmoProto.countdown_type.CountdownType_Go:
        return "Go!"
    elif cdtype == BmmoProto.countdown_type.CountdownType_1:
        return "1"
    elif cdtype == BmmoProto.countdown_type.CountdownType_2:
        return "2"
    elif cdtype == BmmoProto.countdown_type.CountdownType_3:
        return "3"
    else:
        return "Unknow"

def MapTypeToString(mtp: int) -> str:
    if mtp == BmmoProto.map_type.UnknowType:
        return "Unknow"
    elif mtp == BmmoProto.map_type.UnknowType:
        return "Original"
    elif mtp == BmmoProto.map_type.UnknowType:
        return "Custom"
    else:
        return "Unknow"

def FormatMd5(md5: tuple):
    return ''.join('{:0>2x}'.format(item) for item in md5)

def FormatUuid(uuid: tuple):
    slash = (3, 5, 7, 9)
    return ''.join(('{:0>2x}-' if idx in slash else '{:0>2x}').format(item) for idx, item in enumerate(uuid))

def IntToBoolean(num: int) -> bool:
    return num != 0

# ============= Context Worker =============
class ContextCommandType:
    NoneCmd = 0
    ExitCmd = 1
    ChatCmd = 2
    ProfileCmd = 3


mCtxCmd = ContextCommandType.NoneCmd
mCtxCmdArgs = None
mCtxCmdMutex = threading.Lock()
mCtxDead = False
mCtxDeadMutex = threading.Lock()
def ContextWorker():
    # preparing useful data
    mClientsMap = {}

    mClientVersion = BmmoProto.bmmo_version()
    mClientVersion.major = 3
    mClientVersion.minor = 2
    mClientVersion.subminor = 5
    mClientVersion.stage = BmmoProto.bmmo_stage.Beta
    mClientVersion.build = 7

    # start client and send request message
    mBmmoClient.Start("127.0.0.1", 6172)
    req_msg = BmmoProto.login_request_v3_msg()
    req_msg.nickname = "SwungMoe"
    req_msg.version = mClientVersion
    req_msg.cheated = 1
    for i in range(16):
        req_msg.uuid.append(114)

    # loop
    while True:
        time.sleep(0.01)

        # analyse command
        mCtxCmdMutex.acquire()
        cache_cmd = mCtxCmd
        cache_args = mCtxCmdArgs
        mCtxCmd = ContextCommandType.NoneCmd
        mCtxCmdArgs = None
        mCtxCmdMutex.release()

        sent_msg = None
        if cache_cmd == ContextCommandType.NoneCmd:
            pass
        elif cache_cmd == ContextCommandType.ExitCmd:
            break
        elif cache_cmd == ContextCommandType.ChatCmd:
            sent_msg = BmmoProto.chat_msg()
            sent_msg.player_id = 0
            sent_msg.chat_content = cache_args[0]
        elif cache_cmd == ContextCommandType.ProfileCmd:
            mOutputHelper.Print("[ContextWorker] This is profile")
        else:
            mOutputHelper.Print("[ContextWorker] Unknow command")

        client_status = mBmmoClient.GetStatus()
        # check client alive
        if client_status == BmmoClient.ModuleStatus.Stopped:
            break

        # process message ony when client started
        if client_status == BmmoClient.ModuleStatus.Running:
            # send
            if sent_msg is not None:
                mBmmoClient.Send(sent_msg)

            # recv
            msg_list = mBmmoClient.Recv()
            for msg in msg_list:
                opcode = msg.get_opcode()

                # self login logout
                if opcode == BmmoProto.opcode.login_denied_msg:
                    mOutputHelper.Print("[User] Login failed.")
                    break
                elif opcode == BmmoProto.opcode.login_accepted_v2_msg:
                    mOutputHelper.Print("[User] Login successfully.")

                    # update clients data
                    mClientsMap.clear()
                    for entity in msg.data:
                        mClientsMap[entity.uuid] = entity

                # chat
                elif opcode == BmmoProto.opcode.chat_msg:
                    username = GetUsernameFromGnsUid(mClientsMap, msg.player_id)
                    mOutputHelper.Print("[Chat] <{}, {}> {}".format(msg.player_id, username, msg.chat_content))

                # other players login logout
                elif opcode == BmmoProto.opcode.player_connected_v2_msg:
                    mOutputHelper.Print("[User] {} join the server.".format(msg.data.nickname))
                    # update clients data
                    mClientsMap[msg.data.uuid] = msg.data
                elif opcode == BmmoProto.opcode.player_connected_v2_msg:
                    mOutputHelper.Print("[User] {} leave the server.".format(
                        GetUsernameFromGnsUid(mClientsMap, msg.player_id)
                    ))
                    # update clients data
                    del mClientsMap[msg.player_id]
                elif opcode == BmmoProto.opcode.player_kicked_msg:
                    mOutputHelper.Print("[User] {} kicked by {}. Reason: {}".format(
                        msg.kicked_player_name,
                        msg.executor_name,
                        msg.reason
                    ))
                    if IntToBoolean(msg.crashed):
                        mOutputHelper.Print("[User] {} client is crashed.".format(msg.kicked_player_name))

                # cheat update
                elif opcode == BmmoProto.opcode.owned_cheat_state_msg:
                    if IntToBoolean(msg.state.notify):
                        mOutputHelper.Print("[Cheat] {} now in cheat status: {}.".format(
                            GetUsernameFromGnsUid(mClientsMap, msg.player_id),
                            IntToBoolean(msg.state.cheated)
                        ))

                    # update cheat status
                    mClientsMap[msg.player_id].cheated = msg.state.cheated
                elif opcode == BmmoProto.opcode.owned_cheat_toggle_msg:
                    if IntToBoolean(msg.state.notify):
                        mOutputHelper.Print("[Cheat] {} order everyone go into cheat status: {}.".format(
                            GetUsernameFromGnsUid(mClientsMap, msg.player_id),
                            IntToBoolean(msg.state.cheated)
                        ))

                # level status
                elif opcode == BmmoProto.opcode.level_finish_v2_msg:
                    mOutputHelper.Print("""[Level] {} finish level.\n
Cheated: {}, Rank: {}\n
Map Profile\nType: {}, Name: {}, Md5: {}, Level: {}
Statistica:\nPoints: {}, Lifes: {}, LifeBouns: {}, LevelBouns: {}, TimeElapsed: {}, StartPoints: {}
""".format(
                        GetUsernameFromGnsUid(mClientsMap, msg.player_id),
                        IntToBoolean(msg.cheated), msg.rank,

                        MapTypeToString(msg.bmap.bmap_type),
                        msg.bmap.map_name,
                        FormatMd5(msg.bmap.map_md5),
                        msg.bmap.map_level,

                        msg.points, msg.lifes, msg.lifeBouns, msg.levelBouns, msg.timeElapsed, msg.startPoints
                    ))
                elif opcode == BmmoProto.opcode.countdown_msg:
                    mOutputHelper.Print("[Countdown] {}\nSender: {}\nMap: {}\nRestart: {}\nForce restart: {}".format(
                        CountdownTypeToString(msg.cd_type),
                        GetUsernameFromGnsUid(mClientsMap, msg.sender),
                        FormatMd5(msg.for_map),
                        IntToBoolean(msg.restart_level),
                        IntToBoolean(msg.force_restart)
                    ))


    # exit safely
    mBmmoClient.Stop()
    while mBmmoClient.GetStatus() != BmmoClient.ModuleStatus.Stopped:
        time.sleep(0.01)
