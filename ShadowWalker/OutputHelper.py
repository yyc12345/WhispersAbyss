import threading
import termcolor
import sys, os, collections

class OutputHelper:
    __cMAX_MSG_SIZE: int = 100
    __mMutex: threading.Lock
    __mIsInputting: bool
    __mMsgDeque: collections.deque[str]

    _mMAX_MSG_SIZE = 100

    def __init__(self):
        # enable terminal output first
        self.__EnableTerminalColor()
        # setup msg list
        self.__mMutex = threading.Lock()
        self.__mIsInputting = False
        self.__mMsgDeque = collections.deque()

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
        with self.__mMutex:
            if self.__mIsInputting:
                if len(self.__mMsgDeque) >= OutputHelper.__cMAX_MSG_SIZE:
                    self.__mMsgDeque.popleft()
                self.__mMsgDeque.append(strl)
            else:
                print(strl)

    def StartInput(self):
        with self.__mMutex:
            self.__mIsInputting = True

    def StopInput(self):
        with self.__mMutex:
            self.__mIsInputting = False
        
            for strl in self.__mMsgDeque:
                print(strl)
            self.__mMsgDeque.clear()
