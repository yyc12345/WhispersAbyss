import threading

class OutputHelper:
    _mMAX_MSG_SIZE = 100

    def __init__(self):
        self._mMsgMutex = threading.Lock()
        self._mIsInputing = False
        self._mMsgList = []
        self._mMsgListCount = 0

    def Print(self, strl: str):
        self._mMsgMutex.acquire()
        if (self._mIsInputing):
            if (self._mMsgListCount >= OutputHelper._mMAX_MSG_SIZE):
                self._mMsgList.pop(0)
                self._mMsgListCount -= 1
            self._mMsgList.append(strl)
            self._mMsgListCount += 1
        else:
            print(strl);
        self._mMsgMutex.release()

    def StartInput(self):
        self._mMsgMutex.acquire()
        self._mIsInputing = True
        self._mMsgMutex.release()

    def StopInput(self):
        self._mMsgMutex.acquire()

        self._mIsInputing = False
        for strl in self._mMsgList:
            print(strl)
        self._mMsgList.clear()
        self._mMsgListCount = 0

        self._mMsgMutex.release()
