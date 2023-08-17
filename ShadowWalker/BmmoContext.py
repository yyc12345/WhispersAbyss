import BmmoClient, BmmoProto
import OutputHelper
import CppHelper
import collections, threading, typing, time, tabulate

class BmmoContextParam:
    mLocalHost: str
    mLocalPort: int
    mRemoteUrl: str
    mUsername: str
    mUUID: tuple[int]
    mEnableDebug: bool
    def __init__(self, local_host: str, local_port: int, remote_url: str, username: str, uuid: tuple[int], enable_debug: bool):
        self.mLocalHost = local_host
        self.mLocalPort = local_port
        self.mRemoteUrl = remote_url
        self.mUsername = username
        self.mUUID = uuid[:]
        self.mEnableDebug = enable_debug

class BmmoCvt:
    @staticmethod
    def Int2Bool(is_cheat: int) -> bool: return is_cheat != 0

class BmmoFmt:
    @staticmethod
    def FormatVersionT(version: BmmoProto.version_t) -> str:
        s = f"{version.major}.{version.minor}.{version.subminor}"
        if version.stage == BmmoProto.stage_t.Alpha.value: s += f'-alpha{version.build}'
        elif version.stage == BmmoProto.stage_t.Beta.value: s += f'-beta{version.build}'
        elif version.stage == BmmoProto.stage_t.RC.value: s += f'-rc{version.build}'
        elif version.stage == BmmoProto.stage_t.Release.value: pass
        else: raise Exception("Invalid version_t::stage.")
        return s

    @staticmethod
    def FormatCheatState(state: bool) -> str: return "ON" if state else "OFF"

    @staticmethod
    def FormatMd5(md5: tuple) -> str:
        return ''.join('{:0>2x}'.format(item) for item in md5)
    
    @staticmethod
    def FormatUuid(uuid: tuple) -> str:
        slash = (3, 5, 7, 9)
        return ''.join(('{:0>2x}-' if idx in slash else '{:0>2x}').format(item) for idx, item in enumerate(uuid))
    
    @staticmethod
    def FormatGnsUuid(uuid: int) -> str:
        return '{:0>8x}'.format(uuid)

class BmmoMd5:
    mMd5: bytes
    def __init__(self, val: typing.Union[str, tuple, list]):
        if isinstance(val, str):
            # https://stackoverflow.com/questions/5649407/how-to-convert-hexadecimal-string-to-bytes-in-python
            self.mMd5 = bytes.fromhex(val)
        elif isinstance(val, list) or isinstance(val, tuple):
            if len(val) != 16: raise Exception("Invalid Md5 tuple.")
            # https://stackoverflow.com/questions/51865919/convert-between-int-array-or-tuple-vs-bytes-correctly
            self.mMd5 = bytes(val)
        else:
            raise Exception("Invalid val type.")
    def __eq__(self, other):
        return self.mMd5 == other.mMd5
    def __hash__(self):
        return hash(self.mMd5)

class BmmoMapManager:
    __mMd5Map2Name: dict[BmmoMd5, str]
    __mMutex: threading.Lock
    def __init__(self):
        self.__mMd5Map2Name = {}
        self.__mMutex = threading.Lock()
    def AddMap(self, md5: BmmoMd5, name: str):
        with self.__mMutex:
            if md5 in self.__mMd5Map2Name: return
            self.__mMd5Map2Name[md5] = name
    def GetDisplayName(self, md5: BmmoMd5) -> str:
        with self.__mMutex:
            name = self.__mMd5Map2Name.get(md5, None)
            if name is None: name = "[Unknown]"
            return name

class BmmoUserProfile:
    mUsername: str
    mGnsId: int
    mIsCheated: bool
    mMap: BmmoMd5
    mSector: int
    def __init__(self, username: str, gnsid: int, is_cheated: bool, inmap: BmmoMd5, sector: int):
        self.mUsername = username
        self.mGnsId = gnsid
        self.mIsCheated = is_cheated
        self.mMap = inmap
        self.mSector = sector
class BmmoUserManager:
    __mGnsid2Profile: dict[int, BmmoUserProfile]
    __mMutex: threading.Lock
    def __init__(self):
        self.__mGnsid2Profile = {}
        self.__mMutex = threading.Lock()
    def Clear(self):
        with self.__mMutex:
            self.__mGnsid2Profile.clear()
    def AddUser(self, profile: BmmoUserProfile):
        with self.__mMutex:
            if profile.mGnsId in self.__mGnsid2Profile: return
            self.__mGnsid2Profile[profile.mGnsId] = profile
    def DeleteUser(self, gnsid: int):
        with self.__mMutex:
            if gnsid in self.__mGnsid2Profile:
                del self.__mGnsid2Profile[gnsid]
    def UpdateCheat(self, gnsid: int, is_cheat: bool):
        with self.__mMutex:
            profile = self.__mGnsid2Profile.get(gnsid, None)
            if profile:
                profile.mIsCheated = is_cheat
    def UpdateMap(self, gnsid: int, inmap: BmmoMd5, sector: int):
        with self.__mMutex:
            profile = self.__mGnsid2Profile.get(gnsid, None)
            if profile:
                profile.mMap = inmap
                profile.mSector = sector
    def UpdateSector(self, gnsid: int, sector: int):
        with self.__mMutex:
            profile = self.__mGnsid2Profile.get(gnsid, None)
            if profile:
                profile.mSector = sector
    def GetUsernameFromGnsUid(self, uid: int) -> str:
        with self.__mMutex:
            if uid == 0: return "[Server]"
            else:
                profile = self.__mGnsid2Profile.get(uid, None)
                if profile is None: return "[Unknown]"
                else: return profile.mUsername
    def GetUserSheet(self, mapmgr: BmmoMapManager) -> str:
        table = []

        with self.__mMutex:
            for profile in self.__mGnsid2Profile.values():
                row = (
                    profile.mUsername,
                    BmmoFmt.FormatGnsUuid(profile.mGnsId),
                    BmmoFmt.FormatCheatState(profile.mIsCheated),
                    mapmgr.GetDisplayName(profile.mMap),
                    str(profile.mSector),
                )
                table.append(row)

        return tabulate.tabulate(table, headers = ("Username", "Gns ID", "Cheat", "Map", "Sector", ), tablefmt="grid")

class BmmoContext:
    __mStateMachine: CppHelper.StateMachine
    __mOutput: OutputHelper.OutputHelper

    __mClient: BmmoClient.BmmoClient
    __mTdCtx: CppHelper.JThread

    __mUserManager: BmmoUserManager
    __mMapManager: BmmoMapManager

    __mCtxParam: BmmoContextParam

    __cCurrentVer: BmmoProto.version_t = BmmoProto.version_t()
    __cCurrentVer.major = 3
    __cCurrentVer.minor = 4
    __cCurrentVer.subminor = 8
    __cCurrentVer.stage = BmmoProto.stage_t.Beta.value
    __cCurrentVer.build = 13

    __cFakeMods: dict[str, str] = {
        "BallanceMMOClient": BmmoFmt.FormatVersionT(__cCurrentVer),
        "ImproperResetDetector": "3.4.4",
    }

    __cBlankMap: str = "00000000000000000000000000000000"
    __cOriginalLevel: tuple[str] = (
        "a364b408fffaab4344806b427e37f1a7",
        "e90b2f535c8bf881e9cb83129fba241d",
        "22f895812942f954bab73a2924181d0d",
        "478faf2e028a7f352694fb2ab7326fec",
        "5797e3a9489a1cd6213c38c7ffcfb02a",
        "0dde7ec92927563bb2f34b8799b49e4c",
        "3473005097612bd0c0e9de5c4ea2e5de",
        "8b81694d53e6c5c87a6c7a5fa2e39a8d",
        "21283cde62e0f6d3847de85ae5abd147",
        "d80f54ffaa19be193a455908f8ff6e1d",
        "47f936f45540d67a0a1865eac334d2db",
        "2a1d29359b9802d4c6501dd2088884db",
        "9b5be1ca6a92ce56683fa208dd3453b4",
    )

    def __init__(self, output: OutputHelper.OutputHelper, ctx_param: BmmoContextParam):
        self.__mStateMachine = CppHelper.StateMachine()
        self.__mOutput = output

        self.__mCtxParam = ctx_param

        self.__mClient = BmmoClient.BmmoClient(
            output,
            BmmoClient.BmmoClientParam(
                self.__mCtxParam.mLocalHost,
                self.__mCtxParam.mLocalPort,
                self.__mCtxParam.mRemoteUrl,
                self.__mCtxParam.mEnableDebug
            )
        )
        self.__mTdCtx = None

        # init user manager and map manager
        self.__mUserManager = BmmoUserManager()
        self.__mMapManager = BmmoMapManager()
        for idx, val in enumerate(BmmoContext.__cOriginalLevel):
            self.__mMapManager.AddMap(BmmoMd5(val), f"Level {idx + 1}")

        self.__mOutput.Print("[BmmoContext] Created.")

    # ============= Export Interface =============
    def __StartWorker(self) -> bool:
        # start client
        self.__mClient.Start()
        reporter = self.__mClient.GetStateMachine()
        reporter.SpinUntil(CppHelper.State.Running | CppHelper.State.Stopped)

        if reporter.IsInState(CppHelper.State.Stopped):
            # fail to start, go back
            return False
        
        # start ctx worker
        self.__mTdCtx = CppHelper.JThread(self.__CtxWorker)
        self.__mTdCtx.start()

        # return
        self.__mOutput.Print("[BmmoContext] Started.")
        return True
    def Start(self):
        self.__mStateMachine.InitTransition(self.__StartWorker)

    def __StopWorker(self):
        # stop ctx worker
        if (self.__mTdCtx is not None) and (self.__mTdCtx.is_alive()):
            self.__mTdCtx.request_stop()
            self.__mTdCtx.join()

        # stop client
        self.__mClient.Stop()
        self.__mClient.GetStateMachine().SpinUntil(CppHelper.State.Stopped)

        # clear game data
        self.__mUserManager.Clear()

        self.__mOutput.Print("[BmmoContext] Stopped.")
    def Stop(self):
        self.__mStateMachine.StopTransition(self.__StopWorker)
    
    def GetStateMachine(self) -> CppHelper.StateMachine:
        return self.__mStateMachine

    def Chat(self, content: str):
        if self.__mStateMachine.IsInState(CppHelper.State.Running):
            # construct msg data
            msg: BmmoProto.chat_msg = BmmoProto.chat_msg()
            msg.chat_content = content
            # send it
            self.__mClient.Send(msg)

    def Profile(self):
        if self.__mStateMachine.IsInState(CppHelper.State.Running):
            self.__mOutput.Print(self.__mUserManager.GetUserSheet(self.__mMapManager))

    # ============= Context Worker =============
    def __CtxWorker(self, stop_token: CppHelper.StopToken):
        # send login request message
        login_req: BmmoProto.login_request_v3_msg = BmmoProto.login_request_v3_msg()
        login_req.nickname = self.__mCtxParam.mUsername
        login_req.version = BmmoContext.__cCurrentVer
        login_req.cheated = 0
        login_req.uuid = self.__mCtxParam.mUUID[:]
        self.__mClient.Send(login_req)
        
        # core loop
        recv_msg: collections.deque[BmmoProto.BpMessage] = collections.deque()
        while not stop_token.stop_requested():
            # wait until it can work
            if not self.__mStateMachine.IsInState(CppHelper.State.Running):
                time.sleep(CppHelper.SPIN_INTERVAL)
                continue

            # if it can not work, stop
            if self.__mClient.GetStateMachine().IsInState(CppHelper.State.Stopped):
                self.Stop()
                return

            # process message
            self.__mClient.Recv(recv_msg)
            for msg in recv_msg:
                opcode = msg.GetOpCode()

                # self login logout
                if opcode == BmmoProto.OpCode.simple_action_msg:
                    lintmsg: BmmoProto.simple_action_msg = msg
                    if lintmsg.action == BmmoProto.action_type.LoginDenied:
                        ## login failed. actively exit
                        self.__mOutput.Print("[User] Login failed.", "yellow")
                        self.Stop()
                        return
                    
                elif opcode == BmmoProto.OpCode.login_accepted_v3_msg:
                    lintmsg: BmmoProto.login_accepted_v3_msg = msg
                    self.__mOutput.Print(f"[User] Login successfully. Online player count: {len(lintmsg.online_players)}", "yellow")

                    # update clients data
                    self.__mUserManager.Clear()
                    for entity in lintmsg.online_players:
                        profile: BmmoUserProfile = BmmoUserProfile(
                            entity.name, 
                            entity.player_id, 
                            BmmoCvt.Int2Bool(entity.cheated),
                            BmmoMd5(entity.in_map.md5),
                            entity.sector
                            )
                        self.__mUserManager.AddUser(profile)

                    # simulate fake mod info data
                    fake_mods: BmmoProto.mod_list_msg = BmmoProto.mod_list_msg()
                    for modname, modver in BmmoContext.__cFakeMods.items():
                        pair: BmmoProto.mod_pair = BmmoProto.mod_pair()
                        pair.mod_name = modname
                        pair.mod_version = modver
                        fake_mods.mods.append(pair)
                    self.__mClient.Send(fake_mods)

                # other players login logout
                elif opcode == BmmoProto.OpCode.player_connected_v2_msg:
                    lintmsg: BmmoProto.player_connected_v2_msg = msg
                    self.__mOutput.Print("[User] {} joined the server.".format(lintmsg.name), "yellow")
                    # update clients data
                    profile: BmmoUserProfile = BmmoUserProfile(
                        lintmsg.name, 
                        lintmsg.connection_id,
                        lintmsg.cheated,
                        BmmoMd5(BmmoContext.__cBlankMap),
                        0
                        )
                    self.__mUserManager.AddUser(profile)
                elif opcode == BmmoProto.OpCode.player_disconnected_msg:
                    lintmsg: BmmoProto.player_disconnected_msg = msg
                    self.__mOutput.Print(f"[User] {self.__mUserManager.GetUsernameFromGnsUid(lintmsg.connection_id)} left the server.", "yellow")
                    # update clients data
                    self.__mUserManager.DeleteUser(lintmsg.connection_id)
                elif opcode == BmmoProto.OpCode.player_kicked_msg:
                    lintmsg: BmmoProto.player_kicked_msg = msg

                    words = f"[User] {lintmsg.kicked_player_name} was kicked by"
                    if len(lintmsg.executor_name) == 0: 
                        words += " [Server]"
                    else: 
                        words += f" {lintmsg.executor_name}"
                    if len(lintmsg.reason) != 0: 
                        words += f' ({lintmsg.reason})'
                    if BmmoCvt.Int2Bool(lintmsg.crashed):
                        words += ' and crashed subsequently'

                    words += '.'
                    self.__mOutput.Print(words, "yellow")

                
                # chat
                elif opcode == BmmoProto.OpCode.chat_msg:
                    lintmsg: BmmoProto.chat_msg = msg
                    self.__mOutput.Print(f"[Chat] <{BmmoFmt.FormatGnsUuid(lintmsg.player_id)}, {self.__mUserManager.GetUsernameFromGnsUid(lintmsg.player_id)}> {lintmsg.chat_content}", "light_green")
                elif opcode == BmmoProto.OpCode.private_chat_msg:
                    lintmsg: BmmoProto.private_chat_msg = msg
                    self.__mOutput.Print(f"[Whisper] <{BmmoFmt.FormatGnsUuid(lintmsg.opposite_player_id)}, {self.__mUserManager.GetUsernameFromGnsUid(lintmsg.opposite_player_id)}> {lintmsg.chat_content}", "light_green")
                elif opcode == BmmoProto.OpCode.important_notification_msg:
                    lintmsg: BmmoProto.important_notification_msg = msg
                    self.__mOutput.Print(f"[Announcement] <{BmmoFmt.FormatGnsUuid(lintmsg.opposite_player_id)}, {self.__mUserManager.GetUsernameFromGnsUid(lintmsg.opposite_player_id)}> {lintmsg.chat_content}", "green")
                elif opcode == BmmoProto.OpCode.permanent_notification_msg:
                    lintmsg: BmmoProto.permanent_notification_msg = msg
                    self.__mOutput.Print(f"[Bulletin] {lintmsg.title}: {lintmsg.text_content}", "green")

                # cheat
                elif opcode == BmmoProto.OpCode.owned_cheat_state_msg:
                    lintmsg: BmmoProto.owned_cheat_state_msg = msg
                    self.__mOutput.Print(f"[Cheat] {self.__mUserManager.GetUsernameFromGnsUid(lintmsg.player_id)} turned cheat {BmmoFmt.FormatCheatState(BmmoCvt.Int2Bool(lintmsg.state.cheated))}.", "light_magenta")
                    # update cheat state
                    self.__mUserManager.UpdateCheat(lintmsg.player_id, BmmoCvt.Int2Bool(lintmsg.state.cheated))
                elif opcode == BmmoProto.OpCode.cheat_toggle_msg:
                    lintmsg: BmmoProto.cheat_toggle_msg = msg
                    self.__mOutput.Print(f"[Cheat] [Server] toggled cheat {BmmoFmt.FormatCheatState(BmmoCvt.Int2Bool(lintmsg.data.cheated))} globally!", "light_magenta")
                elif opcode == BmmoProto.OpCode.owned_cheat_toggle_msg:
                    lintmsg: BmmoProto.owned_cheat_toggle_msg = msg
                    self.__mOutput.Print(f"[Cheat] {self.__mUserManager.GetUsernameFromGnsUid(lintmsg.player_id)} toggled cheat {BmmoFmt.FormatCheatState(BmmoCvt.Int2Bool(lintmsg.state.cheated))} globally!", "light_magenta")

                # map + sector
                elif opcode == BmmoProto.OpCode.map_names_msg:
                    lintmsg: BmmoProto.map_names_msg = msg
                    for mapv in lintmsg.maps:
                        self.__mMapManager.AddMap(BmmoMd5(mapv.md5), mapv.name)
                elif opcode == BmmoProto.OpCode.current_map_msg:
                    lintmsg: BmmoProto.current_map_msg = msg
                    self.__mUserManager.UpdateMap(lintmsg.player_id, BmmoMd5(lintmsg.in_map.md5), lintmsg.sector)
                    # show map change
                    self.__mOutput.Print(f'[Map] {self.__mUserManager.GetUsernameFromGnsUid(lintmsg.player_id)} is at the sector {lintmsg.sector} of "{self.__mMapManager.GetDisplayName(BmmoMd5(lintmsg.in_map.md5))}".', "light_grey")
                elif opcode == BmmoProto.OpCode.current_sector_msg:
                    lintmsg: BmmoProto.current_sector_msg = msg
                    self.__mUserManager.UpdateSector(lintmsg.player_id, lintmsg.sector)

                # game context
                elif opcode == BmmoProto.OpCode.player_ready_msg:
                    lintmsg: BmmoProto.player_ready_msg = msg

                    words: str = f"[Game] {self.__mUserManager.GetUsernameFromGnsUid(lintmsg.player_id)} is"
                    if BmmoCvt.Int2Bool(lintmsg.ready):
                        words += " ready"
                    else:
                        words += " not ready"
                    words += f" to start ({lintmsg.count} player ready)."
                    
                    self.__mOutput.Print(words, "light_blue")
                elif opcode == BmmoProto.OpCode.countdown_msg:
                    lintmsg: BmmoProto.countdown_msg = msg

                    words: str = f"[Game] <{BmmoFmt.FormatGnsUuid(lintmsg.sender)}, {self.__mUserManager.GetUsernameFromGnsUid(lintmsg.sender)}> {self.__mMapManager.GetDisplayName(BmmoMd5(lintmsg.for_map.md5))} - "
                    cdt = lintmsg.cd_type
                    if cdt == BmmoProto.countdown_type.Go:
                        words += "Go!"
                    elif cdt == BmmoProto.countdown_type.Countdown_1:
                        words += "1"
                    elif cdt == BmmoProto.countdown_type.Countdown_2:
                        words += "2"
                    elif cdt == BmmoProto.countdown_type.Countdown_3:
                        words += "3"
                    elif cdt == BmmoProto.countdown_type.Ready:
                        words += "Get ready."
                    elif cdt == BmmoProto.countdown_type.ConfirmReady:
                        words += 'Please use "/mmo ready" to confirm if you are ready'

                    self.__mOutput.Print(words, "light_blue")
                elif opcode == BmmoProto.OpCode.did_not_finish_msg:
                    lintmsg: BmmoProto.did_not_finish_msg = msg
                    self.__mOutput.Print(f"{self.__mUserManager.GetUsernameFromGnsUid(lintmsg.player_id)} did not finish {self.__mMapManager.GetDisplayName(BmmoMd5(lintmsg.in_map.md5))} (aborted at sector {lintmsg.sector}).", "light_blue")
                elif opcode == BmmoProto.OpCode.level_finish_v2_msg:
                    lintmsg: BmmoProto.level_finish_v2_msg = msg
                    # collect some data
                    score: int = lintmsg.levelBonus + lintmsg.points + lintmsg.lives * lintmsg.lifeBonus;
                    timetotal: int = int(lintmsg.timeElapsed)
                    timemin: int = timetotal // 60
                    timesec: int = timetotal % 60
                    timehr: int = timemin // 60
                    timemin = timemin % 60
                    timems: int = int((lintmsg.timeElapsed - float(timetotal)) * 1000)
                    self.__mOutput.Print(f"[Game] {self.__mUserManager.GetUsernameFromGnsUid(lintmsg.player_id)} finished {self.__mMapManager.GetDisplayName(BmmoMd5(lintmsg.bmap.md5))} in {lintmsg.rank}st place (score: {score}; real time: {timehr:02d}:{timemin:02d}:{timesec:02d}.{timems:03d}).", "light_blue")


            # if this turn no msg, sleep a while
            if len(recv_msg) == 0:
                time.sleep(CppHelper.SPIN_INTERVAL)
                continue
            recv_msg.clear()

            # end of core loop

