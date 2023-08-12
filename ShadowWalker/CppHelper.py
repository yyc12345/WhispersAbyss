import threading, typing, enum, time

class StopToken:
    __mRequestStop: bool
    __mMutex: threading.Lock
    def __init__(self):
        self.__mRequestStop = False
        self.__mMutex = threading.Lock()
    def stop_requested(self) -> bool:
        with self.__mMutex:
            return self.__mRequestStop
    def request_stop(self):
        with self.__mMutex:
            self.__mRequestStop = True

class JThread:
    __mStopToken: StopToken
    __mTd: threading.Thread
    def __init__(self, func: typing.Callable[[StopToken], None]):
        self.__mStopToken = StopToken()
        self.__mTd = threading.Thread(target=func, args=(self.__mStopToken, ))
    def run(self):
        self.__mTd.run()
    def request_stop(self):
        self.__mStopToken.request_stop()
    def is_alive(self) -> bool:
        return self.__mTd.is_alive()
    def join(self):
        self.__mTd.join()

SPIN_INTERVAL: float = 0.01
class State(enum.IntFlag):
    Ready = enum.auto()
    Running = enum.auto()
    Stopped = enum.auto()
class StateMachine:
    TpfInit = typing.Callable[[], bool]     # return True mean no init error
    TpfStop = typing.Callable[[], None]
    __mMutex: threading.Lock
    __mState: State
    __mIsInTransition: bool
    __mHasRun: list[bool]
    def __init__(self):
        self.__mMutex = threading.Lock()
        self.__mState = State.Ready
        self.__mIsInTransition = False
        self.__mHasRun = [False, False]
    
    def __BeginTransition(self, required_state: State, has_run_index: int) -> bool:
        while True:
            self.__mMutex.acquire()

            if self.__mIsInTransition:
                self.__mMutex.release()
                time.sleep(SPIN_INTERVAL)
                continue

            if (self.__mState in required_state) and (not self.__mHasRun[has_run_index]):
                self.__mIsInTransition = True
                self.__mHasRun[has_run_index] = True
                return True
            else:
                return False
    def __StopTrnsition(self, final_state: State):
        with self.__mMutex:
            self.__mIsInTransition = False
            self.__mState = final_state

    def __InitWroker(self, func: TpfInit):
        can_run: bool = self.__BeginTransition(State.Ready, 0)
        if not can_run: return
        ret: bool = func()
        self.__StopTrnsition(State.Running if ret else State.Stopped)
    def InitTransition(self, func: TpfInit):
        td: threading.Thread = threading.Thread(args=(func, ), target=self.__InitWroker)
        td.daemon = True
        td.start()
    
    def __StopWroker(self, func: TpfStop):
        can_run: bool = self.__BeginTransition(State.Ready | State.Running, 1)
        if not can_run: return
        func()
        self.__StopTrnsition(State.Stopped)
    def StopTransition(self, func: TpfStop):
        td: threading.Thread = threading.Thread(args=(func, ), target=self.__StopWroker)
        td.daemon = True
        td.start()
    
    def SpinUntil(self, state: State):
        while True:
            is_in: bool = False
            with self.__mMutex:
                is_in = self.__mState in state
            if is_in: break
            else: time.sleep(SPIN_INTERVAL)
    def IsInState(self, state: State) -> bool:
        with self.__mMutex:
            return self.__mState in state

