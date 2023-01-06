import BmmoClient
import BmmoProto
import OutputHelper
import threading
import time
import tabulate

class ContextCommandType:
    NoneCmd = 0
    ExitCmd = 1
    ChatCmd = 2
    ProfileCmd = 3

class BmmoContext:
    def __init__(self, output: OutputHelper.OutputHelper, host: str, port: int, username: str, uuid: tuple):
        self._mOutputHelper = output
        self._mBmmoClient = BmmoClient.BmmoClient(output)

        self._mCtxCmd = ContextCommandType.NoneCmd
        self._mCtxCmdArgs = None
        self._mCtxCmdMutex = threading.Lock()
        self._mCtxDead = False
        self._mCtxDeadMutex = threading.Lock()

        self._mClientsMap = {}

        self._mClientVersion = BmmoProto.version_t()
        self._mClientVersion.major = 3
        self._mClientVersion.minor = 4
        self._mClientVersion.subminor = 4
        self._mClientVersion.stage = BmmoProto.bmmo_stage.Beta
        self._mClientVersion.build = 8

        self._mCfgHost = host
        self._mCfgPort = port
        self._mCfgUsername = username
        self._mCfgUuid = uuid

        self._mTdCtx = threading.Thread(target = self._ContextWorker)
        self._mTdCtx.start()

    # ============= Export Interface =============
    def IsDead(self) -> bool:
        self._mCtxDeadMutex.acquire()
        cache = self._mCtxDead
        self._mCtxDeadMutex.release()
        return cache

    def WaitExit(self):
        self._mTdCtx.join()

    def SetCommand(self, cmd_type: int, cmd_args: tuple):
        self._mCtxCmdMutex.acquire()
        self._mCtxCmd = cmd_type
        self._mCtxCmdArgs = cmd_args
        self._mCtxCmdMutex.release()

    # ============= Useful Funcs =============
    def _GetUsernameFromGnsUid(self, uid: int) -> str:
        if uid == 0:
            return "[Server]"
        else:
            player_entity = self._mClientsMap.get(uid, None)
            if player_entity is None:
                return "[Unknow]"
            else:
                return player_entity.nickname

    def _CountdownTypeToString(self, cdtype: int) -> str:
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

    def _MapTypeToString(self, mtp: int) -> str:
        if mtp == BmmoProto.map_type.UnknownType:
            return "Unknow"
        elif mtp == BmmoProto.map_type.Original:
            return "Original"
        elif mtp == BmmoProto.map_type.Custom:
            return "Custom"
        else:
            return "Unknow"

    def _FormatMd5(self, md5: tuple):
        return ''.join('{:0>2x}'.format(item) for item in md5)

    def _FormatUuid(self, uuid: tuple):
        slash = (3, 5, 7, 9)
        return ''.join(('{:0>2x}-' if idx in slash else '{:0>2x}').format(item) for idx, item in enumerate(uuid))

    def _FormatGnsUuid(self, uuid: int):
        return '{:0>8x}'.format(uuid)

    def _IntToBoolean(self, num: int) -> bool:
        return num != 0

    def _GenerateUserSheet(self) -> str:
        table = []

        for i in self._mClientsMap.values():
            row = (
                i.nickname, 
                self._FormatGnsUuid(i.uuid), 
                str(self._IntToBoolean(i.cheated))
            )
            table.append(row)

        return tabulate.tabulate(table, headers = ("Username", "Gns UUID", "Is Cheated"), tablefmt="grid")

    # ============= Context Worker =============
    def _ContextWorker(self):
        # start client and send request message
        self._mBmmoClient.Start(self._mCfgHost, self._mCfgPort)
        req_msg = BmmoProto.login_request_v3_msg()
        req_msg.nickname = self._mCfgUsername
        req_msg.version = self._mClientVersion
        req_msg.cheated = 1
        req_msg.uuid = self._mCfgUuid
        self._mBmmoClient.Send(req_msg)
        
        # loop
        while True:
            time.sleep(0.01)

            # analyse command
            self._mCtxCmdMutex.acquire()
            cache_cmd = self._mCtxCmd
            cache_args = self._mCtxCmdArgs
            self._mCtxCmd = ContextCommandType.NoneCmd
            self._mCtxCmdArgs = None
            self._mCtxCmdMutex.release()

            sent_msg = None
            if cache_cmd == ContextCommandType.NoneCmd:
                pass
            elif cache_cmd == ContextCommandType.ExitCmd:
                self._mOutputHelper.Print("[ContextWorker] Actively exit")
                break
            elif cache_cmd == ContextCommandType.ChatCmd:
                sent_msg = BmmoProto.chat_msg()
                sent_msg.data.someone_id = 0
                sent_msg.data.chat_content = cache_args[0]
            elif cache_cmd == ContextCommandType.ProfileCmd:
                self._mOutputHelper.Print("[ContextWorker] User list:\n" + self._GenerateUserSheet())
            else:
                self._mOutputHelper.Print("[ContextWorker] Unknow command")

            client_status = self._mBmmoClient.GetStatus()
            # check client alive
            if client_status == BmmoClient.ModuleStatus.Stopped:
                break

            # process message ony when client started
            if client_status == BmmoClient.ModuleStatus.Running:
                # send
                if sent_msg is not None:
                    self._mBmmoClient.Send(sent_msg)

                # recv
                msg_list = self._mBmmoClient.Recv()
                for msg in msg_list:
                    opcode = msg.GetOpCode()

                    # self login logout
                    if opcode == BmmoProto._OpCode.simple_action_msg:
                        if msg.action == BmmoProto.action_type.LoginDenied:
                            self._mOutputHelper.Print("[User] Login failed.", "yellow")
                        break
                    elif opcode == BmmoProto._OpCode.login_accepted_v3_msg:
                        self._mOutputHelper.Print("[User] Login successfully. Online player count: {}".format(len(msg.online_players)), "yellow")

                        # update clients data
                        self._mClientsMap.clear()
                        for entity in msg.online_players:
                            self._mClientsMap[entity.uuid] = entity

                    # chat
                    elif opcode == BmmoProto._OpCode.chat_msg:
                        username = self._GetUsernameFromGnsUid(msg.data.someone_id)
                        self._mOutputHelper.Print("[Chat] <{}, {}> {}".format(self._FormatGnsUuid(msg.data.someone_id), username, msg.data.chat_content), "green")

                    # other players login logout
                    elif opcode == BmmoProto._OpCode.player_connected_v2_msg:
                        self._mOutputHelper.Print("[User] {} joined the server.".format(msg.player.nickname), "yellow")
                        # update clients data
                        self._mClientsMap[msg.player.uuid] = msg.player
                    elif opcode == BmmoProto._OpCode.player_disconnected_msg:
                        self._mOutputHelper.Print("[User] {} left the server.".format(
                            self._GetUsernameFromGnsUid(msg.connection_id)
                        ), "yellow")
                        # update clients data
                        if msg.connection_id in self._mClientsMap:
                            del self._mClientsMap[msg.connection_id]
                        else:
                            self._mOutputHelper.Print("{} is not existed.".format(self._FormatGnsUuid(msg.connection_id)))
                    elif opcode == BmmoProto._OpCode.player_kicked_msg:
                        self._mOutputHelper.Print("[User] {} kicked by {}. Reason: {}".format(
                            msg.kicked_player_name,
                            msg.executor_name,
                            msg.reason
                        ), "yellow")
                        if self._IntToBoolean(msg.crashed):
                            self._mOutputHelper.Print("[User] {} client is crashed.".format(msg.kicked_player_name), "yellow")

                    # cheat update
                    elif opcode == BmmoProto._OpCode.owned_cheat_state_msg:
                        if self._IntToBoolean(msg.state.notify):
                            self._mOutputHelper.Print("[Cheat] {} now in cheat status: {}.".format(
                                self._GetUsernameFromGnsUid(msg.player_id),
                                self._IntToBoolean(msg.state.cheated)
                            ), "cyan")

                        # update cheat status
                        if msg.player_id in self._mClientsMap:
                            self._mClientsMap[msg.player_id].cheated = msg.state.cheated
                        else:
                            self._mOutputHelper.Print("{} is not existed.".format(self._FormatGnsUuid(msg.player_id)))
                    elif opcode == BmmoProto._OpCode.owned_cheat_toggle_msg:
                        if self._IntToBoolean(msg.state.notify):
                            self._mOutputHelper.Print("[Cheat] {} order everyone go into cheat status: {}.".format(
                                self._GetUsernameFromGnsUid(msg.player_id),
                                self._IntToBoolean(msg.state.cheated)
                            ), "cyan")

                    # level status
                    elif opcode == BmmoProto._OpCode.level_finish_v2_msg:
                        self._mOutputHelper.Print("""[Level] {} finish level.
Cheated: {}, Rank: {}
Map Profile\nType: {}, Name: {}, Md5: {}, Level: {}
Statistica:\nPoints: {}, Lifes: {}, LifeBouns: {}, LevelBouns: {}, TimeElapsed: {}, StartPoints: {}
""".format(
                            self._GetUsernameFromGnsUid(msg.player_id),
                            self._IntToBoolean(msg.cheated), msg.rank,

                            self._MapTypeToString(msg.bmap.bmap_type),
                            msg.bmap.map_name,
                            self._FormatMd5(msg.bmap.map_md5),
                            msg.bmap.map_level,

                            msg.points, msg.lifes, msg.lifeBonus, msg.levelBonus, msg.timeElapsed, msg.startPoints
                        ), "magenta")
                    elif opcode == BmmoProto._OpCode.countdown_msg:
                        self._mOutputHelper.Print("[Countdown] {}\nSender: {}\nMap: {}\nRestart: {}\nForce restart: {}".format(
                            self._CountdownTypeToString(msg.cd_type),
                            self._GetUsernameFromGnsUid(msg.sender),
                            self._FormatMd5(msg.for_map),
                            self._IntToBoolean(msg.restart_level),
                            self._IntToBoolean(msg.force_restart)
                        ), "magenta")


        # exit safely
        self._mOutputHelper.Print("[ContextWorker] Waiting BmmoClient exit...")
        self._mBmmoClient.Stop()
        while self._mBmmoClient.GetStatus() != BmmoClient.ModuleStatus.Stopped:
            time.sleep(0.01)
        self._mOutputHelper.Print("[ContextWorker] BmmoClient exited.")

        # notify dead
        self._mCtxDeadMutex.acquire()
        self._mCtxDead = True
        self._mCtxDeadMutex.release()
