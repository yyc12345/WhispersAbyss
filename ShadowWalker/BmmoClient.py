import BmmoProto
import OutputHelper
import threading, io, socket, time, struct, traceback

class State:
    Ready = 0
    Running = 1
    Stopped = 2
class StateMachine:
    __mState: State
    __mIsInTransition: bool
    __mHasRunStopping: bool
    __mMutex: threading.Lock

    def __init__(self, func_init):
        pass

    def Stop(self, func_stop):
        pass

class BmmoClient:
    def __init__(self, output: OutputHelper.OutputHelper):
        self._mRecvMsgMutex = threading.Lock()
        self._mSendMsgMutex = threading.Lock()
        self._mSendMsg = []
        self._mRecvMsg = []

        #self._mStopCtx = False
        #self._mStopCtxMutex = threading.Lock()
        self._mStatus = ModuleStatus.Ready
        self._mStatusMutex = threading.Lock()

        self._mConn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        #self._mConn.settimeout(3)

        self._mTdSender = None
        self._mTdRecver = None

        self._mOutput = output
        self._mOutput.Print("[BmmoClient] Initialized.")

    def Start(self, host: str, port: int):
        # check requirements and change status
        self._mStatusMutex.acquire()
        need_return = self._mStatus != ModuleStatus.Ready
        if not need_return:
            self._mStatus = ModuleStatus.Initializing
        self._mStatusMutex.release()
        if need_return:
            return

        try:
            # start socket and thread
            self._mConn.connect((host, port))
            self._mTdSender = threading.Thread(target = self._SendWorker)
            self._mTdSender.start()
            self._mTdRecver = threading.Thread(target = self._RecvWorker)
            self._mTdRecver.start()
        except Exception as e:
            # call stop
            self._mOutput.Print("[BmmoClient] Fail to start.\nReason: {}".format(e))
            self.Stop()
        else:
            self._mStatusMutex.acquire()
            self._mStatus = ModuleStatus.Running
            self._mStatusMutex.release()
            self._mOutput.Print("[BmmoClient] Started.")

    def Stop(self):
        # check requirements and change status
        self._mStatusMutex.acquire()
        need_return = self._mStatus != ModuleStatus.Running and self._mStatus != ModuleStatus.Initializing
        if not need_return:
            self._mStatus = ModuleStatus.Stopping
        self._mStatusMutex.release()
        if need_return:
            return

        # use a thread to stop everything
        td_stop_worker = threading.Thread(target = self._StopWorker)
        td_stop_worker.start()

    def GetStatus(self):
        self._mStatusMutex.acquire()
        cache = self._mStatus
        self._mStatusMutex.release()
        return cache

    def Send(self, msg):
        self._mSendMsgMutex.acquire()
        self._mSendMsg.append(msg)
        self._mSendMsgMutex.release()

    def Recv(self) -> list:
        self._mRecvMsgMutex.acquire()
        if len(self._mRecvMsg) != 0:
            cache = self._mRecvMsg;
            self._mRecvMsg = []
        else:
            cache = []
        self._mRecvMsgMutex.release()
        return cache

    def _StopWorker(self):
        # if _RecverWorker call self.Stop()
        # it will cause self join self error
        # so we need to open a new thread to join all the threads
        # and set status

        # stop socket
        self._mOutput.Print("[BmmoClient] Closing connection...")
        self._mConn.close();
        # stop thread
        self._mOutput.Print("[BmmoClient] Stopping threads...")
        if self._mTdSender is not None:
            self._mTdSender.join()
            self._mTdSender = None
        if self._mTdRecver is not None:
            self._mTdRecver.join()
            self._mTdRecver = None

        # set status
        self._mStatusMutex.acquire()
        self._mStatus = ModuleStatus.Stopped
        self._mStatusMutex.release()
        self._mOutput.Print("[BmmoClient] Stopped.")

    def _SocketSendHelper(self, msg) -> bool:
        msglen = len(msg)
        totalsent = 0

        try:
            while totalsent < msglen:
                sent = self._mConn.send(msg[totalsent:])
                if sent == 0:
                    raise RuntimeError("socket connection broken")
                totalsent = totalsent + sent
        except:
            return False

        return True

    def _SocketRecvHelper(self, msglen: int) -> tuple:
        chunks = []
        bytes_recd = 0

        try:
            while bytes_recd < msglen:
                chunk = self._mConn.recv(min(msglen - bytes_recd, 2048))
                if chunk == b'':
                    raise RuntimeError("socket connection broken")
                chunks.append(chunk)
                bytes_recd = bytes_recd + len(chunk)
        except:
            return (False, b'')

        return (True, b''.join(chunks))

    def _SendWorker(self):
        ss = io.BytesIO()

        while True:
            # check exit requirement
            self._mStatusMutex.acquire()
            cache_status = self._mStatus
            self._mStatusMutex.release()
            if cache_status == ModuleStatus.Stopping:
                self._mOutput.Print("[BmmoClient] Sender detected stop status.")
                break
            if cache_status != ModuleStatus.Running:
                time.sleep(0.01)
                continue

            # get send message list
            self._mSendMsgMutex.acquire()
            send_messages = self._mSendMsg
            self._mSendMsg = []
            self._mSendMsgMutex.release()
            
            # send all messages
            if len(send_messages) == 0:
                time.sleep(0.01)
                continue
            for msg in send_messages:
                # clean ss
                ss.seek(io.SEEK_SET, 0)
                ss.truncate(0)
                # serialize
                try:
                    msg.Serialize(ss)
                except Exception as e:
                    self._mOutput.Print("[BmmoClient] Error when serializing msg.\nReason: {}\nUnfinished message payload: {}".format(e, ss.getvalue()))
                    traceback.print_exc()
                    continue

                # get essential data
                raw_data = ss.getvalue()
                raw_data_len = ss.tell()
                is_reliable = 1 if msg.GetIsReliable() else 0

                # send header
                ec = self._SocketSendHelper(struct.pack("II", raw_data_len, is_reliable))
                if not ec:
                    self._mOutput.Print("[BmmoClient] Error when sending msg header.")
                    self.Stop()
                    break
                # send body
                ec = self._SocketSendHelper(raw_data)
                if not ec:
                    self._mOutput.Print("[BmmoClient] Error when sending msg body.")
                    self.Stop()
                    break

    def _RecvWorker(self):
        ss = io.BytesIO()

        while True:
            # check exit requirement
            self._mStatusMutex.acquire()
            cache_status = self._mStatus
            self._mStatusMutex.release()
            if cache_status == ModuleStatus.Stopping:
                self._mOutput.Print("[BmmoClient] Recver detected stop status.")
                break
            if cache_status != ModuleStatus.Running:
                time.sleep(0.01)
                continue

            # read header
            (ec, header) = self._SocketRecvHelper(8)
            if not ec:
                self._mOutput.Print("[BmmoClient] Error when receving msg header.")
                self.Stop()
                break

            (raw_data_len, raw_is_reliable) = struct.unpack("II", header)
            is_reliable = raw_is_reliable != 0

            # read body and write into clean ss
            (ec, body) = self._SocketRecvHelper(raw_data_len)
            if not ec:
                self._mOutput.Print("[BmmoClient] Error when receving msg body.")
                self.Stop()
                break

            ss.seek(io.SEEK_SET, 0)
            ss.truncate(0)
            ss.write(body)

            # deserialize msg
            try:
                ss.seek(io.SEEK_SET, 0)
                msg = BmmoProto._UniformDeserialize(ss)
            except Exception as e:
                self._mOutput.Print("[BmmoClient] Error when deserializing msg.\nReason: {}\nError message payload: {}".format(e, ss.getvalue()))
                traceback.print_exc()
                continue
            if msg is None:
                self._mOutput.Print("[BmmoClient] Unknow msg opcode.")
                continue

            self._mRecvMsgMutex.acquire()
            self._mRecvMsg.append(msg)
            self._mRecvMsgMutex.release()

