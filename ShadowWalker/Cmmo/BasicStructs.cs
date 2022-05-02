using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ShadowWalker.Cmmo {

    public enum OpCode : UInt32 {
		None,
		CS_OrderChat,
		SC_Chat,

		CS_OrderGlobalCheat,

		CS_OrderClientList,
		SC_ClientList,

		SC_BallState,

		SC_ClientConnected,
		SC_ClientDisconnected,
		SC_CheatState,
		SC_GlobalCheat,
		SC_LevelFinish
	}

	public struct PlayerEntity {
		public UInt32 mPlayerId;
		public string mNickname;
		public byte mCheated;
    }

    public struct VxVector {
        public float x, y, z;
    }
    public struct VxQuaternion {
        public float x, y, z, w;
    }

}
