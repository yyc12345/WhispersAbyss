import BmmoProto
import OutputHelper
import MessagePatcher, CppHelper
import threading, io, socket, time, struct, traceback, collections

class BmmoClientParam:
    mLocalHost: str
    mLocalPort: int
    mRemoteUrl: str
    def __init__(self, local_host: str, local_port: int, remote_url: str):
        self.mLocalHost = local_host
        self.mLocalPort = local_port
        self.mRemoteUrl = remote_url

class BmmoClient:
    __mStateMachine: CppHelper.StateMachine
    __mOutput: OutputHelper.OutputHelper
    __mConn: socket.socket

    __mConnParam: BmmoClientParam

    __mRecvMsgMutex: threading.Lock
    __mSendMsgMutex: threading.Lock
    __mRecvMsg: collections.deque[BmmoProto.BpMessage]
    __mSendMsg: collections.deque[BmmoProto.BpMessage]
    __mTdSend: CppHelper.JThread
    __mTdRecv: CppHelper.JThread

    def __init__(self, output: OutputHelper.OutputHelper, client_param: BmmoClientParam):
        self.__mStateMachine = CppHelper.StateMachine()
        self.__mOutput = output
        self.__mConn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        self.__mConnParam = client_param

        self.__mRecvMsgMutex = threading.Lock()
        self.__mSendMsgMutex = threading.Lock()
        self.__mSendMsg = collections.deque()
        self.__mRecvMsg = collections.deque()

        self.__mConn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        #self.__mConn.settimeout(3)

        self.__mTdSend = None
        self.__mTdRecv = None

        self.__mOutput.Print("[BmmoClient] Created.")

    # ============= Export Interface =============
    def __StartWorker(self) -> bool:
        try:
            # start socket
            self.__mConn.connect((self.__mConnParam.mLocalHost, self.__mConnParam.mLocalPort))
        except Exception as e:
            # order stop
            self.__mOutput.Print(f"[BmmoClient] Fail to start.\n\tReason: {e}")
            self.__mConn.close()
            return False

        # start send recv threads
        self.__mTdSender = CppHelper.JThread(self.__SendWorker)
        self.__mTdSender.start()
        self.__mTdRecver = CppHelper.JThread(self.__RecvWorker)
        self.__mTdRecver.start()
        self.__mOutput.Print("[BmmoClient] Started.")

        return True
    def Start(self):
        self.__mStateMachine.InitTransition(self.__StartWorker)

    def __StopWorker(self):
        # stop socket
        self.__mOutput.Print("[BmmoClient] Closing connection...")
        self.__mConn.close()
        # stop thread
        self.__mOutput.Print("[BmmoClient] Stopping threads...")
        if self.__mTdSend is not None and self.__mTdSend.is_alive():
            self.__mTdSend.request_stop()
            self.__mTdSend.join()
            self.__mTdSend = None
        if self.__mTdRecv is not None and self.__mTdRecv.is_alive():
            self.__mTdRecv.request_stop()
            self.__mTdRecv.join()
            self.__mTdRecv = None

        # set status
        self.__mOutput.Print("[BmmoClient] Stopped.")
    def Stop(self):
        self.__mStateMachine.StopTransition(self.__StopWorker)

    def GetStateMachine(self) -> CppHelper.StateMachine:
        return self.__mStateMachine

    def Send(self, msg: BmmoProto.BpMessage):
        if self.__mStateMachine.IsInState(CppHelper.State.Running):
            with self.__mSendMsgMutex:
                self.__mSendMsg.append(msg)
    def Recv(self, ls: collections.deque[BmmoProto.BpMessage]):
        if self.__mStateMachine.IsInState(CppHelper.State.Running):
            with self.__mRecvMsgMutex:
                ls.extend(self.__mRecvMsg)
                self.__mRecvMsg.clear()

    # ============= Send / Recv Helper =============
    def __SocketSendHelper(self, msg: bytes) -> bool:
        msglen = len(msg)
        totalsent = 0
        try:
            while totalsent < msglen:
                sent = self.__mConn.send(msg[totalsent:])
                if sent == 0:
                    raise RuntimeError("socket connection broken")
                totalsent = totalsent + sent
        except Exception as e:
            self.__mOutput.Print(f"[BmmoClient] Error when sending: {e}")
            self.Stop()
            return False
        return True
    def __SocketRecvHelper(self, msglen: int) -> tuple[int, bytes]:
        chunks: bytes = b''
        bytes_recd = 0
        try:
            while bytes_recd < msglen:
                chunk = self.__mConn.recv(min(msglen - bytes_recd, 2048))
                if chunk == b'':
                    raise RuntimeError("socket connection broken")
                chunks += chunk
                bytes_recd = bytes_recd + len(chunk)
        except Exception as e:
            self.__mOutput.Print(f"[BmmoClient] Error when recving: {e}")
            self.Stop()
            return (False, b'')
        return (True, chunks)
    
    # ============= Send / Recv IMPL =============
    __sFmtHeader: struct.Struct = struct.Struct("=IB")
    __sFmtHeaderData: struct.Struct = struct.Struct("=B")
    __sFmtHeaderCmd: struct.Struct = struct.Struct("=I")
    def __SendWorker(self, stop_token: CppHelper.StopToken):
        ss: io.BytesIO = io.BytesIO()
        has_sent_url: bool = False
        send_messages: collections.deque[BmmoProto.BpMessage] = collections.deque()

        msg_patcher: MessagePatcher.MessagePatcher = MessagePatcher.MessagePatcher()

        while not stop_token.stop_requested():
            if not self.__mStateMachine.IsInState(CppHelper.State.Running):
                time.sleep(CppHelper.SPIN_INTERVAL)
                continue
            
            # if we still don't send url, send it first
            if not has_sent_url:
                # get essential data
                raw_data = self.__mConnParam.mRemoteUrl.encode('utf-8', errors='ignore')
                raw_data_len = len(raw_data)

                # send header
                if not self.__SocketSendHelper(BmmoClient.__sFmtHeader.pack(raw_data_len + 1 + 4, 1) + BmmoClient.__sFmtHeaderCmd.pack(raw_data_len)):
                    # `+ 1 + 4` for 2 extra fields. `1` mean this msg is command msg
                    return
                # send body
                if not self.__SocketSendHelper(raw_data):
                    return
                
                # mark has sent
                has_sent_url = True
            
            # get send message list
            with self.__mSendMsgMutex:
                send_messages.extend(self.__mSendMsg)
                self.__mSendMsg.clear()
            
            # send all messages
            for msg in send_messages:
                # clean ss
                ss.seek(0, io.SEEK_SET)
                ss.truncate(0)

                # serialize
                try:
                    BmmoProto.UniformSerialize(msg, ss)
                except Exception as e:
                    self.__mOutput.Print(f"[BmmoClient] Error when deserializing msg.\n\tReason: {e}\n\tMessage payload: {ss.getvalue()}")
                    traceback.print_exc()
                    continue

                # patch serialized msg
                ss.seek(0, io.SEEK_SET)
                msg_patcher.PatchSend(ss)

                # get essential data
                raw_data = ss.getvalue()
                print('send: ' + raw_data.__repr__())
                raw_data_len = len(raw_data)
                is_reliable: int = 1 if msg.IsReliable() else 0

                # send header
                if not self.__SocketSendHelper(BmmoClient.__sFmtHeader.pack(raw_data_len + 1 + 1, 0) + BmmoClient.__sFmtHeaderData.pack(is_reliable)):
                    # `+ 1 + 1` for 2 extra fields. `0` mean this msg is not command msg
                    return
                # send body
                if not self.__SocketSendHelper(raw_data):
                    return
                
            # clear cache
            send_messages.clear()

    def __RecvWorker(self, stop_token: CppHelper.StopToken):
        ss: io.BytesIO = io.BytesIO()

        msg_filter: MessagePatcher.MessageFilter = MessagePatcher.MessageFilter()
        msg_patcher: MessagePatcher.MessagePatcher = MessagePatcher.MessagePatcher()

        while not stop_token.stop_requested():
            if not self.__mStateMachine.IsInState(CppHelper.State.Running):
                time.sleep(CppHelper.SPIN_INTERVAL)
                continue
            
            # read header
            (ec, header) = self.__SocketRecvHelper(BmmoClient.__sFmtHeader.size + BmmoClient.__sFmtHeaderData.size)
            print('header: ' + header.__repr__())
            if not ec:
                return
            (raw_data_len, is_command) = BmmoClient.__sFmtHeader.unpack(header[:BmmoClient.__sFmtHeader.size])
            (is_reliable, ) = BmmoClient.__sFmtHeaderData.unpack(header[BmmoClient.__sFmtHeader.size:])
            
            # read body and write into clean ss
            (ec, body) = self.__SocketRecvHelper(raw_data_len - 1 - 1)   # `- 1 - 1` for 2 extra fields.
            if not ec:
                return
            ss.seek(0, io.SEEK_SET)
            ss.truncate(0)
            ss.write(body)
            print('recv: ' + body.__repr__())

            # patch msg, and filter it, then deserialize it
            msg: BmmoProto.BpMessage = None
            try:
                ss.seek(0, io.SEEK_SET)
                msg_patcher.PatchRecv(ss)
                ss.seek(0, io.SEEK_SET)
                msg = msg_filter.UniversalDeserialize(ss)
            except Exception as e:
                self.__mOutput.Print(f"[BmmoClient] Error when deserializing msg.\n\tReason: {e}\n\tMessage payload: {ss.getvalue()}")
                traceback.print_exc()
                continue

            # msg fail to parse or blocked by filter
            if msg is None:
                continue

            # add into msg deque
            with self.__mRecvMsgMutex:
                self.__mRecvMsg.append(msg)
