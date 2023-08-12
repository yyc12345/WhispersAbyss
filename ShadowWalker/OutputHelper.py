import threading
import termcolor
import sys, os

class OutputHelper:
    _mMAX_MSG_SIZE = 100

    def __init__(self):
        # enable terminal output first
        self.__EnableTerminalColor()
        # setup msg list
        self._mMsgMutex = threading.Lock()
        self._mIsInputing = False
        self._mMsgList = []
        self._mMsgListCount = 0

    def __EnableTerminalColor(self):
        if sys.platform == "win32" or sys.platform == "cygwin":
            import ctypes, msvcrt
            import ctypes.wintypes

            #if (os.isatty(sys.stdout.fileno())):
            h_output = ctypes.wintypes.HANDLE(msvcrt.get_osfhandle(sys.stdout.fileno()))
            dw_mode = ctypes.wintypes.DWORD(0)

            kernel32 = ctypes.windll.kernel32
            kernel32.GetConsoleMode.restype = ctypes.wintypes.BOOL
            kernel32.GetConsoleMode.argtypes = [ctypes.wintypes.HANDLE, ctypes.wintypes.LPDWORD]
            ret_bool = kernel32.GetConsoleMode(h_output, ctypes.byref(dw_mode))
            if not bool(ret_bool):
                return
        
            ENABLE_VIRTUAL_TERMINAL_PROCESSING = ctypes.wintypes.DWORD(0x0004)
            dw_mode = ctypes.wintypes.DWORD(dw_mode.value | ENABLE_VIRTUAL_TERMINAL_PROCESSING.value)

            kernel32.SetConsoleMode.restype = ctypes.wintypes.BOOL
            kernel32.SetConsoleMode.argtypes = [ctypes.wintypes.HANDLE, ctypes.wintypes.DWORD]
            ret_bool = kernel32.SetConsoleMode(h_output, dw_mode)
            if not bool(ret_bool):
                return

    def Print(self, strl: str, col: str = "white"):
        # process str first
        if col != 'white':
            strl = termcolor.colored(strl, col, attrs=["bold"])
        
        # msg list oper
        self._mMsgMutex.acquire()
        if (self._mIsInputing):
            if (self._mMsgListCount >= OutputHelper._mMAX_MSG_SIZE):
                self._mMsgList.pop(0)
                self._mMsgListCount -= 1
            self._mMsgList.append(strl)
            self._mMsgListCount += 1
        else:
            print(strl)
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
