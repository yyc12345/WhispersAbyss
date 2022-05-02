using System;
using System.Collections.Generic;
using System.Text;

namespace ShadowWalker.Kernel {
    public class OutputHelper {

        Queue<string> mPendingMessage = new Queue<string>();
        bool mDirectOutput = true;
        object mOutputHelperLock = new object();

        private void DebugOutput(string msg) {
            string real_msg = $"{DateTime.Now.ToString("[HH:mm:ss.FFFF]", System.Globalization.DateTimeFormatInfo.InvariantInfo)} {msg}";
            lock (mOutputHelperLock) {
                if (mDirectOutput)
                    Console.WriteLine(real_msg);
                else {
                    if (mPendingMessage.Count < 100) {
                        mPendingMessage.Enqueue(real_msg);
                    }
                }
            }
        }

        public void Printf(string format, params object?[] args) {
            DebugOutput(string.Format(format, args));
        }
        public void FatalError(string format, params object?[] args) {
            DebugOutput(string.Format(format, args));
            Environment.Exit(1);
        }

        public string GetCommand() {
            lock (mOutputHelperLock) {
                mDirectOutput = false;
            }

            Console.Write("> ");
            string cmd = Console.ReadLine();

            lock (mOutputHelperLock) {
                // print all pending message
                while (mPendingMessage.Count != 9) {
                    Console.WriteLine(mPendingMessage.Dequeue());
                }
                mDirectOutput = true;
            }

            return cmd;
        }

    }
}
