using System;
using System.Collections.Generic;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace ShadowWalker.Kernel {
    public class CelestiaClient {

        static int WARNING_MSG_CAPACITY = 2048;
        static int NUKE_MSG_CAPACITY = 3072;

        static int MAX_MSG_SIZE = 2048;

        public CelestiaClient(OutputHelper outputHelper, string server, string port) {
            mOutputHelper = outputHelper;
            mServerAddress = server;
            if (!int.TryParse(port, out mPort)) {
                mOutputHelper.Printf("Fail to parse Celestia Server port. Set 6172 as default.");
                mPort = 6172;
            }

            mSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            mSocket.NoDelay = true;
            mSocket.SendTimeout = 1000;
            mSocket.SendBufferSize = 8192;
            mSocket.ReceiveBufferSize = 8192;
        }

        OutputHelper mOutputHelper;
        string mServerAddress;
        int mPort;
        Socket mSocket;
        Thread mTdConnect, mTdSend, mTdRecv;
        bool mIsRunning = false, mStopWorker = true;
        Queue<Cmmo.Messages.IMessage> mRecvMsg, mSendMsg;

        object mIsRunningLock = new object();
        object mStopWorkerLock = new object();
        object mRecvMsgLock = new object(), mSendMsgLock = new object();

        public void Start() {
            lock (mStopWorkerLock) {
                mStopWorker = false;
            }
            mTdConnect = new Thread(ConnectWorker);
            mTdSend = new Thread(SendWorker);
            mTdRecv = new Thread(RecvWorker);
            mTdConnect.Start();
            mTdSend.Start();
            mTdRecv.Start();

            lock (mIsRunningLock) {
                mIsRunning = true;
            }
        }

        public void Stop() {

            lock (mStopWorkerLock) {
                mStopWorker = true;
            }
            mSocket.Close();

            mTdConnect.Join();
            mTdSend.Join();
            mTdRecv.Join();

            lock (mIsRunningLock) {
                mIsRunning = false;
            }
        }


        public void Send(Queue<Cmmo.Messages.IMessage> msg_list) {
            int capacity = 0;
            lock (mSendMsgLock) {
                while(msg_list.Count != 0) {
                    mSendMsg.Enqueue(msg_list.Dequeue());
                }
                capacity = mSendMsg.Count;
            }

            CheckCapacity(capacity);
        }
        public void Send(Cmmo.Messages.IMessage msg) {
            int capacity = 0;
            lock (mSendMsgLock) {
                mSendMsg.Enqueue(msg);
                capacity = mSendMsg.Count;
            }

            CheckCapacity(capacity);
        }
        public void Recv(Queue<Cmmo.Messages.IMessage> msg_list) {
            lock (mRecvMsgLock) {
                while(mRecvMsg.Count != 0) {
                    msg_list.Enqueue(mRecvMsg.Dequeue());
                }
            }
        }
        void CheckCapacity(int count) {
            if (count >= NUKE_MSG_CAPACITY) {
                mOutputHelper.FatalError("Message in Celestia Server queue is overflow and reach to NUKE LEVEL. App will be nuked!");
            } else if (count >= WARNING_MSG_CAPACITY) {
                mOutputHelper.Printf("Warning! Too much message in Celestia Server queue.");
            }
        }



        Cmmo.Messages.IMessage CreateMessageFromStream(StringStream ss) {
            return null;
        }
        void ConnectWorker() {
            try {
                mOutputHelper.Printf($"Connecting {mServerAddress}:{mPort} ...");
                mSocket.Connect(mServerAddress, mPort);
                mOutputHelper.Printf($"Server {mServerAddress}:{mPort} connected.");

                // order client first
                var player_list_request = new Cmmo.Messages.OrderClientList();
                Send(player_list_request);
            } catch (Exception e) {
                mOutputHelper.FatalError($"Fail to connect. Reason {e.Message}");
            }
        }
        void SendWorker() {
            Queue<Cmmo.Messages.IMessage> internal_list = new Queue<Cmmo.Messages.IMessage>();
            StringStream ss = new StringStream();

            while (true) {
                lock (mStopWorkerLock) {
                    if (mStopWorker) return;
                }
                Thread.Sleep(10);

                // pick all data
                lock (mSendMsgLock) {
                    while(mSendMsg.Count != 0) {
                        internal_list.Enqueue(mSendMsg.Dequeue());
                    }
                }

                while(internal_list.Count != 0) {
                    var msg = internal_list.Dequeue();

                    try {

                        ss.Reset();
                        msg.Serialize(ss);
                        byte[] data = ss.GetByteArray();

                        UInt32 data_length = (UInt32)data.Length;
                        byte[] uint32_buffer = BitConverter.GetBytes(data_length);
                        mSocket.Send(uint32_buffer, 0, 4, SocketFlags.None, out SocketError ec);
                        if (ec != SocketError.Success) {
                            mOutputHelper.Printf("Fail to send message header.");
                            mSocket.Close();
                            return;
                        }
                        mSocket.Send(data, 0, (int)data_length, SocketFlags.None, out ec);
                        if (ec != SocketError.Success) {
                            mOutputHelper.Printf("Fail to send message body.");
                            mSocket.Close();
                            return;
                        }

                    } catch (ObjectDisposedException e) {
                        mOutputHelper.Printf($"Error when operating Socket. Detail: {e.Message}");
                        mSocket.Close();
                        return;
                    }
                }

            }
        }
        void RecvWorker() {
            byte[] uint32_buffer = new byte[4];
            StringStream ss = new StringStream();

            while (mSocket.Connected) {
                lock (mStopWorkerLock) {
                    if (mStopWorker) return;
                }

                try {

                    // read header
                    mSocket.Receive(uint32_buffer, 0, sizeof(UInt32), SocketFlags.None, out SocketError ec);
                    if (ec != SocketError.Success) {
                        mOutputHelper.Printf("Fail to read message header.");
                        mSocket.Close();
                        return;
                    }
                    UInt32 data_length = BitConverter.ToUInt32(uint32_buffer, 0);
                    if (data_length >= MAX_MSG_SIZE) {
                        mOutputHelper.Printf("Header exceed MAX_MSG_SIZE.");
                        mSocket.Close();
                        return;
                    }

                    // ready body
                    byte[] data = new byte[data_length];
                    mSocket.Receive(data, 0, (int)data_length, SocketFlags.None, out ec);
                    if (ec != SocketError.Success) {
                        mOutputHelper.Printf("Fail to read message body.");
                        mSocket.Close();
                        return;
                    }

                    // parse data and push into list
                    ss.Reset(data);
                    var msg = CreateMessageFromStream(ss);
                    if (!(msg is null)) {
                        lock (mRecvMsgLock) {
                            mRecvMsg.Enqueue(msg);
                        }
                    }

                } catch (ObjectDisposedException e) {
                    mOutputHelper.Printf($"Error when operating Socket. Detail: {e.Message}");
                    mSocket.Close();
                    return;
                }

            }
        }

    }
}
