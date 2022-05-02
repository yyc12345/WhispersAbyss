using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;

namespace ShadowWalker.Kernel {
    public class DataProcessor : IDisposable {

        public DataProcessor(CelestiaClient celestiaClient, OutputHelper outputHelper) {
            mCelestiaClient = celestiaClient;
            mOutputHelper = outputHelper;
            mSBManager = new ShadowBall.SBManager();
            mSBManager.NewRank += Proc_SBManager_NewRank;
            mPlayersDict = new Dictionary<uint, Cmmo.PlayerEntity>();

            mRunningWorker = true;
            mRunningWorkerLock = new object();
            mTdMainWorker = new Thread(MainWorker);
            mTdMainWorker.Start();
        }


        public void Dispose() {
            lock (mRunningWorkerLock) {
                mRunningWorker = false;
            }
            mTdMainWorker.Join();
        }


        CelestiaClient mCelestiaClient;
        OutputHelper mOutputHelper;
        ShadowBall.SBManager mSBManager;
        Dictionary<UInt32, Cmmo.PlayerEntity> mPlayersDict;
        Thread mTdMainWorker;

        bool mRunningWorker;
        object mRunningWorkerLock;

        void MainWorker() {
            Queue<Cmmo.Messages.IMessage> msg_list = new Queue<Cmmo.Messages.IMessage>();

            // start server
            mCelestiaClient.Start();

            while (true) {
                lock (mRunningWorkerLock) {
                    if (!mRunningWorker) break;
                }
                Thread.Sleep(10);

                mCelestiaClient.Recv(msg_list);
                while (msg_list.Count != 0) {
                    var msg = msg_list.Dequeue();
                    switch (msg.GetOpCode()) {
                        case Cmmo.OpCode.None:
                            break;
                        case Cmmo.OpCode.CS_OrderChat:
                            break;
                        case Cmmo.OpCode.SC_Chat:
                            break;
                        case Cmmo.OpCode.CS_OrderGlobalCheat:
                            break;
                        case Cmmo.OpCode.CS_OrderClientList:
                            break;
                        case Cmmo.OpCode.SC_ClientList:
                            break;
                        case Cmmo.OpCode.SC_BallState:
                            mSBManager.IncomeBallState((Cmmo.Messages.BallState)msg);
                            break;
                        case Cmmo.OpCode.SC_ClientConnected:
                            break;
                        case Cmmo.OpCode.SC_ClientDisconnected:
                            mSBManager.IncomeDisconnect((Cmmo.Messages.ClientDisconnected)msg);
                            break;
                        case Cmmo.OpCode.SC_CheatState:
                            mSBManager.IncomeCheat((Cmmo.Messages.CheatState)msg);
                            break;
                        case Cmmo.OpCode.SC_GlobalCheat:
                            break;
                        case Cmmo.OpCode.SC_LevelFinish:
                            mSBManager.InconmeLevelFinish((Cmmo.Messages.LevelFinish)msg);
                            break;
                        default:
                            break;
                    }
                }

            }

            // stop server
            mCelestiaClient.Stop();
        }

        private void Proc_SBManager_NewRank(ShadowBall.PlayerRank[] obj) {
            Queue<Cmmo.Messages.IMessage> msg_list = new Queue<Cmmo.Messages.IMessage>();
            foreach (var item in obj) {
                if (item.PlayerIndex == 0) {
                    msg_list.Enqueue(GenerateServerChat($"{item.PlayerNickname}: {item.PlayerIndex}"));
                } else {
                    msg_list.Enqueue(GenerateServerChat($"{item.PlayerNickname}: DNF. Reason: {item.PlayerDNFReason}"));
                }
            }
            if (msg_list.Count != 0)
                mCelestiaClient.Send(msg_list);
        }

        #region command utils

        enum CommandType {
            ShadowWalker,
            Bmmo,
            Chat
        }

        Cmmo.Messages.Chat GenerateServerChat(string msg) {
            var cmmo_msg = new Cmmo.Messages.Chat();
            cmmo_msg.mChatContent = msg;
            return cmmo_msg;
        }
        CommandType SplitCommand(string command, ref Queue<string> cmd_queue) {
            // split command
            var splited_command = command.Split(' ');
            cmd_queue.Clear();
            foreach (var item in splited_command) {
                if (item != string.Empty) cmd_queue.Enqueue(item);
            }

            // analyse command
            var header = cmd_queue.Peek();
            if (header == "!sw") {
                cmd_queue.Dequeue();
                return CommandType.ShadowWalker;
            } else if (header == "/mmo") {
                cmd_queue.Dequeue();
                return CommandType.Bmmo;
            } else {
                cmd_queue.Clear();
                return CommandType.Chat;
            }
        }
        void ProcChatCommand(string command) {
            Queue<Cmmo.Messages.IMessage> msg_list = new Queue<Cmmo.Messages.IMessage>();
            Queue<string> cmd_list = new Queue<string>();

            var type = SplitCommand(command, ref cmd_list);
            if (type == CommandType.ShadowWalker) {
                // only proc shadow walker command for server chat order
                if (cmd_list.Count == 0) {
                    msg_list.Enqueue(GenerateServerChat("Invalid command. Type `!sw help` for help."));
                } else {
                    var order = cmd_list.Dequeue();
                    switch (order) {
                        case "st": {
                                break;
                            }
                        case "lst": {
                                break;
                            }
                        case "fin": {
                                break;
                            }
                        case "help":
                        default:
                            msg_list.Enqueue(GenerateServerChat("!sw st: Auto send 3,2,1,GO! And start recording."));
                            msg_list.Enqueue(GenerateServerChat("!sw lst: Auto send Ready,3,2,1,GO! And start recording."));
                            msg_list.Enqueue(GenerateServerChat("!sw fin: Stop record forcely."));
                            break;
                    }
                }
            }

            if (msg_list.Count != 0)
                mCelestiaClient.Send(msg_list);
        }
        public void ProcServerCommand(string command, out bool quit) {
            Queue<Cmmo.Messages.IMessage> msg_list = new Queue<Cmmo.Messages.IMessage>();
            Queue<string> cmd_list = new Queue<string>();
            quit = false;

            var type = SplitCommand(command, ref cmd_list);
            if (type == CommandType.ShadowWalker) {
                if (cmd_list.Count == 0) {
                    mOutputHelper.Printf("Invalid command. Type `!sw help` for help.");
                } else {
                    var order = cmd_list.Dequeue();
                    switch (order) {
                        case "st": {
                                break;
                            }
                        case "lst": {
                                break;
                            }
                        case "fin": {
                                break;
                            }
                        case "stop": {
                                quit = true;
                                break;
                            }
                        case "help":
                        default:
                            mOutputHelper.Printf("!sw st: Auto send 3,2,1,GO! And start recording.");
                            mOutputHelper.Printf("!sw lst: Auto send Ready,3,2,1,GO! And start recording.");
                            mOutputHelper.Printf("!sw fin: Stop record forcely.");
                            break;
                    }
                }
            } else if (type == CommandType.Bmmo) {
                if (cmd_list.Count == 0) {
                    mOutputHelper.Printf("Invalid command. Type `/mmo help` for help.");
                } else {
                    var order = cmd_list.Dequeue();
                    switch (order) {
                        case "kick": {
                                break;
                            }
                        case "cheat": {
                                break;
                            }
                        case "help":
                        default:
                            break;
                    }
                }
            } else {
                msg_list.Enqueue(GenerateServerChat(command));
            }

            if (msg_list.Count != 0)
                mCelestiaClient.Send(msg_list);
        }


        #endregion

    }
}
