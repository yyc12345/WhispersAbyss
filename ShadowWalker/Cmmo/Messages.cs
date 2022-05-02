using ShadowWalker.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShadowWalker.Cmmo.Messages {

    public abstract class IMessage {
        public OpCode GetOpCode() {
            return mInternalOpCode;
        }
        public virtual bool Deserialize(ShadowWalker.Kernel.StringStream data) {
            OpCode gotten_code = OpCode.None;
            if (!data.ReadOpCode(ref gotten_code)) return false;
            if (gotten_code != mInternalOpCode) return false;
            return true;
        }
        public virtual bool Serialize(ShadowWalker.Kernel.StringStream data) {
            if (!data.WriteOpCode(ref mInternalOpCode)) return false;
            return true;
        }

        protected OpCode mInternalOpCode = OpCode.None;
    }

    //---------------------------------------
    // 
    //---------------------------------------

    public class OrderChat : IMessage {
        public string mChatContent;
        public OrderChat() { mInternalOpCode = OpCode.CS_OrderChat; }
        public override bool Deserialize(StringStream data) {
            if (!base.Deserialize(data)) return false;
            if (!data.ReadString(ref mChatContent)) return false;
            return true;
        }
        public override bool Serialize(StringStream data) {
            if (!base.Serialize(data)) return false;
            if (!data.WriteString(ref mChatContent)) return false;
            return true;
        }
    }

    public class Chat : OrderChat {
        public UInt32 mPlayerId;
        public Chat() { mInternalOpCode = OpCode.SC_Chat; }
        public override bool Deserialize(StringStream data) {
            if (!base.Deserialize(data)) return false;
            if (!data.ReadUInt32(ref mPlayerId)) return false;
            return true;
        }
        public override bool Serialize(StringStream data) {
            if (!base.Serialize(data)) return false;
            if (!data.WriteUInt32(ref mPlayerId)) return false;
            return true;
        }
    }

    //---------------------------------------
    // 
    //---------------------------------------

    public class OrderGlobalCheat : IMessage {
        public byte mCheated;
        public OrderGlobalCheat() { mInternalOpCode = OpCode.CS_OrderGlobalCheat; }
        public override bool Deserialize(StringStream data) {
            if (!base.Deserialize(data)) return false;
            if (!data.ReadUInt8(ref mCheated)) return false;
            return true;
        }
        public override bool Serialize(StringStream data) {
            if (!base.Serialize(data)) return false;
            if (!data.WriteUInt8(ref mCheated)) return false;
            return true;
        }
    }

    public class GlobalCheat : OrderGlobalCheat {
        public UInt32 mPlayerId;
        public GlobalCheat() { mInternalOpCode = OpCode.SC_GlobalCheat; }
        public override bool Deserialize(StringStream data) {
            if (!base.Deserialize(data)) return false;
            if (!data.ReadUInt32(ref mPlayerId)) return false;
            return true;
        }
        public override bool Serialize(StringStream data) {
            if (!base.Serialize(data)) return false;
            if (!data.WriteUInt32(ref mPlayerId)) return false;
            return true;
        }
    }

    //---------------------------------------
    // 
    //---------------------------------------

    public class OrderClientList : IMessage {
        public OrderClientList() { mInternalOpCode = OpCode.CS_OrderClientList; }
        public override bool Deserialize(StringStream data) {
            if (!base.Deserialize(data)) return false;
            return true;
        }
        public override bool Serialize(StringStream data) {
            if (!base.Serialize(data)) return false;
            return true;
        }
    }

    public class ClientList : OrderClientList {
        public List<PlayerEntity> mOnlinePlayers = new List<PlayerEntity>();
        public ClientList() { mInternalOpCode = OpCode.SC_ClientList; }
        public override bool Deserialize(StringStream data) {
            if (!base.Deserialize(data)) return false;

            UInt32 len = 0;
            if (!data.ReadUInt32(ref len)) return false;
            for (int i = 0; i < len; i++) {
                var item = new PlayerEntity();
                if (!data.ReadUInt32(ref item.mPlayerId)) return false;
                if (!data.ReadString(ref item.mNickname)) return false;
                if (!data.ReadUInt8(ref item.mCheated)) return false;
                mOnlinePlayers.Add(item);
            }

            return true;
        }
        public override bool Serialize(StringStream data) {
            if (!base.Serialize(data)) return false;

            UInt32 len = (UInt32)mOnlinePlayers.Count;
            if (!data.WriteUInt32(ref len)) return false;
            for (int i = 0; i < len; i++) {
                var item = mOnlinePlayers[i];
                if (!data.WriteUInt32(ref item.mPlayerId)) return false;
                if (!data.WriteString(ref item.mNickname)) return false;
                if (!data.WriteUInt8(ref item.mCheated)) return false;
            }

            return true;
        }
    }


    //---------------------------------------
    // 
    // ==================================================================
    // 
    //---------------------------------------


    public class BallState : IMessage {
        public UInt32 mPlayerId;
        public UInt32 mType;
        public VxVector mPosition = new VxVector();
        public VxQuaternion mRotation = new VxQuaternion();
        public BallState() { mInternalOpCode = OpCode.SC_BallState; }
        public override bool Deserialize(StringStream data) {
            if (!base.Deserialize(data)) return false;

            if (!data.ReadUInt32(ref mPlayerId)) return false;
            if (!data.ReadUInt32(ref mType)) return false;

            if (!data.ReadFloat(ref mPosition.x)) return false;
            if (!data.ReadFloat(ref mPosition.y)) return false;
            if (!data.ReadFloat(ref mPosition.z)) return false;

            if (!data.ReadFloat(ref mRotation.x)) return false;
            if (!data.ReadFloat(ref mRotation.y)) return false;
            if (!data.ReadFloat(ref mRotation.z)) return false;
            if (!data.ReadFloat(ref mRotation.w)) return false;

            return true;
        }
        public override bool Serialize(StringStream data) {
            if (!base.Serialize(data)) return false;

            if (!data.WriteUInt32(ref mPlayerId)) return false;
            if (!data.WriteUInt32(ref mType)) return false;

            if (!data.WriteFloat(ref mPosition.x)) return false;
            if (!data.WriteFloat(ref mPosition.y)) return false;
            if (!data.WriteFloat(ref mPosition.z)) return false;

            if (!data.WriteFloat(ref mRotation.x)) return false;
            if (!data.WriteFloat(ref mRotation.y)) return false;
            if (!data.WriteFloat(ref mRotation.z)) return false;
            if (!data.WriteFloat(ref mRotation.w)) return false;

            return true;
        }
    }

    //---------------------------------------
    // 
    //---------------------------------------

    public class ClientConnected : IMessage {
        public PlayerEntity mPlayer = new PlayerEntity();
        public ClientConnected() { mInternalOpCode = OpCode.SC_ClientConnected; }
        public override bool Deserialize(StringStream data) {
            if (!base.Deserialize(data)) return false;

            if (!data.ReadUInt32(ref mPlayer.mPlayerId)) return false;
            if (!data.ReadString(ref mPlayer.mNickname)) return false;
            if (!data.ReadUInt8(ref mPlayer.mCheated)) return false;

            return true;
        }
        public override bool Serialize(StringStream data) {
            if (!base.Serialize(data)) return false;

            if (!data.WriteUInt32(ref mPlayer.mPlayerId)) return false;
            if (!data.WriteString(ref mPlayer.mNickname)) return false;
            if (!data.WriteUInt8(ref mPlayer.mCheated)) return false;

            return true;
        }
    }

    //---------------------------------------
    // 
    //---------------------------------------

    public class ClientDisconnected : IMessage {
        public UInt32 mPlayerId;
        public ClientDisconnected() { mInternalOpCode = OpCode.SC_ClientDisconnected; }
        public override bool Deserialize(StringStream data) {
            if (!base.Deserialize(data)) return false;
            if (!data.ReadUInt32(ref mPlayerId)) return false;
            return true;
        }
        public override bool Serialize(StringStream data) {
            if (!base.Serialize(data)) return false;
            if (!data.WriteUInt32(ref mPlayerId)) return false;
            return true;
        }
    }

    //---------------------------------------
    // 
    //---------------------------------------

    public class CheatState : IMessage {
        public UInt32 mPlayerId;
        public byte mCheated;
        public CheatState() { mInternalOpCode = OpCode.SC_CheatState; }
        public override bool Deserialize(StringStream data) {
            if (!base.Deserialize(data)) return false;

            if (!data.ReadUInt32(ref mPlayerId)) return false;
            if (!data.ReadUInt8(ref mCheated)) return false;

            return true;
        }
        public override bool Serialize(StringStream data) {
            if (!base.Serialize(data)) return false;

            if (!data.WriteUInt32(ref mPlayerId)) return false;
            if (!data.WriteUInt8(ref mCheated)) return false;

            return true;
        }
    }

    //---------------------------------------
    // 
    //---------------------------------------

    public class LevelFinish : IMessage {
        public UInt32 mPlayerId;
        public int mPoints, mLifes, mLifeBouns, mLevelBouns;
        public float mTimeElapsed;
        public int mStartPoints, mCurrentLevel;
        public byte mCheated;
        public LevelFinish() { mInternalOpCode = OpCode.SC_LevelFinish; }
        public override bool Deserialize(StringStream data) {
            if (!base.Deserialize(data)) return false;

            if (!data.ReadUInt32(ref mPlayerId)) return false;
            if (!data.ReadInt32(ref mPoints)) return false;
            if (!data.ReadInt32(ref mLifes)) return false;
            if (!data.ReadInt32(ref mLifeBouns)) return false;
            if (!data.ReadInt32(ref mLevelBouns)) return false;
            if (!data.ReadFloat(ref mTimeElapsed)) return false;
            if (!data.ReadInt32(ref mStartPoints)) return false;
            if (!data.ReadInt32(ref mCurrentLevel)) return false;
            if (!data.ReadUInt8(ref mCheated)) return false;

            return true;
        }
        public override bool Serialize(StringStream data) {
            if (!base.Serialize(data)) return false;

            if (!data.WriteUInt32(ref mPlayerId)) return false;
            if (!data.WriteInt32(ref mPoints)) return false;
            if (!data.WriteInt32(ref mLifes)) return false;
            if (!data.WriteInt32(ref mLifeBouns)) return false;
            if (!data.WriteInt32(ref mLevelBouns)) return false;
            if (!data.WriteFloat(ref mTimeElapsed)) return false;
            if (!data.WriteInt32(ref mStartPoints)) return false;
            if (!data.WriteInt32(ref mCurrentLevel)) return false;
            if (!data.WriteUInt8(ref mCheated)) return false;

            return true;
        }
    }

}
