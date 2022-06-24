import BmmoContext
import OutputHelper
import GetchHelper
import time

mOutputHelper = OutputHelper.OutputHelper()
mOutputHelper.Print("ShadowWalker")
mOutputHelper.Print("==========")
mOutputHelper.Print("")

# start context
mBmmoCtx = BmmoContext.BmmoContext(mOutputHelper, "127.0.0.1", 6172, "SwungMoe")
while True:
    inc = GetchHelper.getch()

    if inc == b'q':
        mBmmoCtx.SetCommand(BmmoContext.ContextCommandType.ExitCmd, None)
        break
    elif inc == b'p':
        mBmmoCtx.SetCommand(BmmoContext.ContextCommandType.ProfileCmd, None)
    elif inc == b'\t':
        mOutputHelper.StartInput()
        msg_content = input("> ")
        mOutputHelper.StopInput()

        mBmmoCtx.SetCommand(BmmoContext.ContextCommandType.ChatCmd, (msg_content, ))
    else:
        if inc != b'h':
            mOutputHelper.Print("Unknow command!")
        mOutputHelper.Print("Command help:\nq: quit, p: show profile, tab: send message, h: show help.")

# wait exit
while True:
    if mBmmoCtx.IsDead():
        break
    time.sleep(0.01)

