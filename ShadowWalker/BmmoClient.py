import BmmoProto
import threading
import io
import socket
import time
import OutputHelper
import struct

class BmmoClient:
    def __init__(self, output: OutputHelper.OutputHelper):
        self._mRecvMsgMutex = threading.Lock()
        self._mSendMsgMutex = threading.Lock()
        self._mSendMsg = []
        self._mRecvMsg = []

        self._mStopCtx = False
        self._mStopCtxMutex = threading.Lock()
        self._mIsRunning = False
        self._mIsRunningMutex = threading.Lock()

        self._mConn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._mConn.settimeout(3)

        self._mTdSender = None
        self._mTdRecver = None

        self._Output = output

    def Start(self, host: str, port: int):
        self._mConn.connect((host, port))
        self._mTdSender = threading.Thread(target = _SendWorker, args = (self, ))
        self._mTdSender.start()
        self._mTdRecver = threading.Thread(target = _RecvWorker, args = (self, ))
        self._mTdRecver.start()

        self._mIsRunningMutex.acquire()
        self._mIsRunning = True
        self._mIsRunningMutex.release()

    def Stop(self):
        self._mStopCtxMutex.acquire()
        self._mStopCtx = True
        self._mStopCtxMutex.release()

        self._mConn.close();

        if self._mTdSender is not None:
            self._mTdSender.join()
        if self._mTdRecver is not None:
            self._mTdRecver.join()

    def IsRunning(self):
        self._mIsRunningMutex.acquire()
        cache = self._mIsRunning
        self._mIsRunningMutex.release()
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

    def _SendWorker(self):
        ss = io.BytesIO()

        while True:
            time.sleep(0.01)

            # check exit requirement
            self._mStopCtxMutex.acquire()
            cache_stop_ctx = self._mStopCtx
            self._mStopCtxMutex.release()
            if cache_stop_ctx:
                break

            # get send message list
            self._mSendMsgMutex.acquire()
            send_messages = self._mSendMsg
            self._mSendMsg = []
            self._mSendMsgMutex.release()
            
            # send all messages
            for msg in send_messages:
                # clean ss
                ss.truncate(0)
                # serialize
                msg.serialize(ss)

                # get essential data
                raw_data = ss.getvalue()
                raw_data_len = ss.tell()
                is_reliable = 1 if msg.get_reliable() else 0

                # send header
                self._mConn.send(struct.pack("II", raw_data_len, is_reliable))
                # send body
                self._mConn.send(raw_data)

    def _RecvWorker(self):
        ss = io.BytesIO()

        while True:
            # check exit requirement
            self._mStopCtxMutex.acquire()
            cache_stop_ctx = self._mStopCtx
            self._mStopCtxMutex.release()
            if cache_stop_ctx:
                break

            # read header
            header = self._mConn.recv(8)
            if header == b'':
                break # todo: notify broken

            (raw_data_len, raw_is_reliable) = struct.unpack("II", header)
            is_reliable = raw_is_reliable != 0

            # read body and write into ss
            body = self._mConn.recv(raw_data_len)
            if header == b'':
                break # todo: notify broken
            ss.truncate(0)
            ss.seek(io.SEEK_SET, 0)
            ss.write(body)

            # deserialize msg
            msg = BmmoProto.uniform_deserialize(ss)
            if msg is None:
                continue # todo: report parse error

            self._mRecvMsgMutex.acquire()
            self._mRecvMsg.append(msg)
            self._mRecvMsgMutex.release()

