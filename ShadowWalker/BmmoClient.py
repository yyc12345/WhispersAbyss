import BmmoProto
import threading
import io
import socket
import time
import OutputHelper
import struct

class ModuleStatus:
    Ready = 0
    Initializing = 1
    Running = 2
    Stopping = 3
    Stopped = 4

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

        self._Output = output

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
            self._mTdSender = threading.Thread(target = _SendWorker, args = (self, ))
            self._mTdSender.start()
            self._mTdRecver = threading.Thread(target = _RecvWorker, args = (self, ))
            self._mTdRecver.start()
        except:
            # call stop
            self.Stop()
        else:
            self._mStatusMutex.acquire()
            self._mStatus = ModuleStatus.Running
            self._mStatusMutex.release()

    def Stop(self):
        # check requirements and change status
        self._mStatusMutex.acquire()
        need_return = self._mStatus != ModuleStatus.Running and self._mStatus != ModuleStatus.Initializing
        if not need_return:
            self._mStatus = ModuleStatus.Stopping
        self._mStatusMutex.release()
        if need_return:
            return

        # stop socket
        self._mConn.close();
        # stop thread
        if self._mTdSender is not None:
            self._mTdSender.join()
            self._mTdSender = None
        if self._mTdRecver is not None:
            self._mTdRecver.join()
            self._mTdRecver = None

        self._mStatusMutex.acquire()
        self._mStatus = ModuleStatus.Stopped
        self._mStatusMutex.release()

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
            return cache
        self._mRecvMsgMutex.release()

    def _SocketSendHelper(self, msg) -> bool:
        msglen = len(data)
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
            self._mStopCtxMutex.acquire()
            cache_status = self._mStatus
            self._mStopCtxMutex.release()
            if cache_status == ModuleStatus.Stopping:
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
                    msg.serialize(ss)
                except:
                    self._Output.Print("[BmmoClient] Error when serializing msg.")
                    continue

                # get essential data
                raw_data = ss.getvalue()
                raw_data_len = ss.tell()
                is_reliable = 1 if msg.get_reliable() else 0

                # send header
                ec = self._SocketSendHelper(struct.pack("II", raw_data_len, is_reliable))
                if not ec:
                    self._Output.Print("[BmmoClient] Error when sending mesg header.")
                    self.Stop()
                    break
                # send body
                ec = self._SocketSendHelper(raw_data)
                if not ec:
                    self._Output.Print("[BmmoClient] Error when sending mesg body.")
                    self.Stop()
                    break

    def _RecvWorker(self):
        ss = io.BytesIO()

        while True:
            # check exit requirement
            self._mStopCtxMutex.acquire()
            cache_status = self._mStatus
            self._mStopCtxMutex.release()
            if cache_status == ModuleStatus.Stopping:
                break
            if cache_status != ModuleStatus.Running:
                time.sleep(0.01)
                continue

            # read header
            (ec, header) = self._mConn.recv(8)
            if not ec:
                self._Output.Print("[BmmoClient] Error when receving msg header.")
                self.Stop()
                break

            (raw_data_len, raw_is_reliable) = struct.unpack("II", header)
            is_reliable = raw_is_reliable != 0

            # read body and write into clean ss
            (ec, body) = self._mConn.recv(raw_data_len)
            if not ec:
                self._Output.Print("[BmmoClient] Error when receving msg body.")
                self.Stop()
                break

            ss.seek(io.SEEK_SET, 0)
            ss.truncate(0)
            ss.write(body)

            # deserialize msg
            try:
                msg = BmmoProto.uniform_deserialize(ss)
            except:
                self._Output.Print("[BmmoClient] Error when deserializing msg.")
                continue
            if msg is None:
                self._Output.Print("[BmmoClient] Unknow msg opcode.")
                continue

            self._mRecvMsgMutex.acquire()
            self._mRecvMsg.append(msg)
            self._mRecvMsgMutex.release()

