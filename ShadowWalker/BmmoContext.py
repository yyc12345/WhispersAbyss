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
    def __init__(self, local_host: str, local_port: int, remote_url: str, username: str, uuid: tuple[int]):
        self.mLocalHost = local_host
        self.mLocalPort = local_port
        self.mRemoteUrl = remote_url
        self.mUsername = username
        self.mUUID = uuid[:]

class BmmoFmt:

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

class BmmoUserProfile:
    mUsername: str
    mGnsId: int
    def __init__(self, username: str, gnsid: int):
        self.mUsername = username
        self.mGnsId = gnsid
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
                self.__mGnsid2Profile[gnsid]
    def GetUsernameFromGnsUid(self, uid: int) -> str:
        with self.__mMutex:
            if uid == 0: return "[Server]"
            else:
                profile = self.__mGnsid2Profile.get(uid, None)
                if profile is None: return "[Unknow]"
                else: return profile.mUsername
    def GetUserSheet(self) -> str:
        table = []

        with self.__mMutex:
            for profile in self.__mGnsid2Profile.values():
                row = (
                    profile.mUsername,
                    BmmoFmt.FormatUuid(profile.mUUID)
                )
                table.append(row)

        return tabulate.tabulate(table, headers = ("Username", "Gns UUID", ), tablefmt="grid")

class BmmoContext:
    __mStateMachine: CppHelper.StateMachine
    __mOutput: OutputHelper.OutputHelper

    __mClient: BmmoClient.BmmoClient
    __mUserManager: BmmoUserManager
    __mTdCtx: CppHelper.JThread

    __mCtxParam: BmmoContextParam

    __sCurrentVer: BmmoProto.version_t = BmmoProto.version_t()
    __sCurrentVer.major = 3
    __sCurrentVer.minor = 4
    __sCurrentVer.subminor = 8
    __sCurrentVer.stage = BmmoProto.stage_t.Beta.value
    __sCurrentVer.build = 13
    def __init__(self, output: OutputHelper.OutputHelper, ctx_param: BmmoContextParam):
        self.__mStateMachine = CppHelper.StateMachine()
        self.__mOutput = output

        self.__mCtxParam = ctx_param

        self.__mClient = BmmoClient.BmmoClient(
            output,
            BmmoClient.BmmoClientParam(
                self.__mCtxParam.mLocalHost,
                self.__mCtxParam.mLocalPort,
                self.__mCtxParam.mRemoteUrl
            )
        )
        self.__mTdCtx = None

        self.__mUserManager = BmmoUserManager()

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
            self.__mOutput.Print(self.__mUserManager.GetUserSheet())

    # ============= Context Worker =============
    def __CtxWorker(self, stop_token: CppHelper.StopToken):
        # send login request message
        login_req: BmmoProto.login_request_v3_msg = BmmoProto.login_request_v3_msg()
        login_req.nickname = self.__mCtxParam.mUsername
        login_req.version = BmmoContext.__sCurrentVer
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
                        profile: BmmoUserProfile = BmmoUserProfile(entity.name, entity.player_id)
                        self.__mUserManager.AddUser(profile)

                # chat
                elif opcode == BmmoProto.OpCode.chat_msg:
                    lintmsg: BmmoProto.chat_msg = msg
                    self.__mOutput.Print(f"[Chat] <{lintmsg.player_id}, {self.__mUserManager.GetUsernameFromGnsUid(lintmsg.player_id)}> {lintmsg.chat_content}", "green")

                # other players login logout
                elif opcode == BmmoProto.OpCode.player_connected_v2_msg:
                    lintmsg: BmmoProto.player_connected_v2_msg = msg
                    self.__mOutput.Print("[User] {} joined the server.".format(lintmsg.name), "yellow")
                    # update clients data
                    profile: BmmoUserProfile = BmmoUserProfile(lintmsg.name, lintmsg.connection_id)
                    self.__mUserManager.AddUser(profile)
                elif opcode == BmmoProto.OpCode.player_disconnected_msg:
                    lintmsg: BmmoProto.player_disconnected_msg = msg
                    self.__mOutput.Print(f"[User] {self.__mUserManager.GetUsernameFromGnsUid(lintmsg.connection_id)} left the server.", "yellow")
                    # update clients data
                    self.__mUserManager.DeleteUser(lintmsg.connection_id)
                
            # if this turn no msg, sleep a while
            if len(recv_msg) == 0:
                time.sleep(CppHelper.SPIN_INTERVAL)
                continue
            recv_msg.clear()

            # end of core loop

