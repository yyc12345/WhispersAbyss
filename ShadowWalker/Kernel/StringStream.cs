using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace ShadowWalker.Kernel {
    public class StringStream : IDisposable {

        MemoryStream mWriteStream = new MemoryStream();
        int mWriteCounter = 0;

        byte[] mReadStream;
        int mReadPtr = 0;

        bool mIsInitialized = false;
        bool mIsReadMode = false;

        public void Dispose() {
            mWriteStream.Close();
            mWriteStream.Dispose();
        }

        public void Reset(byte[] data) {
            mReadStream = data;
            mReadPtr = 0;

            mIsReadMode = true;
            mIsInitialized = true;
        }
        public void Reset() {
            mWriteStream.Seek(0, SeekOrigin.Begin);
            mWriteCounter = 0;

            mIsReadMode = false;
            mIsInitialized = true;
        }


        public byte[] GetByteArray() {
            if (!mIsInitialized || mIsReadMode) throw new InvalidOperationException();

            byte[] data = new byte[mWriteCounter];
            Array.Copy(mWriteStream.GetBuffer(), 0, data, 0, mWriteCounter);
            return data;
        }


        #region read func

        public bool ReadOpCode(ref Cmmo.OpCode data) {
            UInt32 code = 0;
            if (!ReadUInt32(ref code)) return false;
            data = (Cmmo.OpCode)code;
            return true;
        }

        public bool ReadUInt32(ref UInt32 data) {
            if (!mIsInitialized || !mIsReadMode) return false;
            if (mReadPtr + sizeof(UInt32) > mReadStream.Length) return false;

            data = BitConverter.ToUInt32(mReadStream, mReadPtr);
            mReadPtr += sizeof(UInt32);
            return true;
        }

        public bool ReadInt32(ref Int32 data) {
            if (!mIsInitialized || !mIsReadMode) return false;
            if (mReadPtr + sizeof(Int32) > mReadStream.Length) return false;

            data = BitConverter.ToInt32(mReadStream, mReadPtr);
            mReadPtr += sizeof(Int32);
            return true;
        }

        public bool ReadUInt8(ref byte data) {
            if (!mIsInitialized || !mIsReadMode) return false;
            if (mReadPtr + sizeof(byte) > mReadStream.Length) return false;

            data = mReadStream[mReadPtr];
            mReadPtr += sizeof(byte);
            return true;
        }

        public bool ReadFloat(ref float data) {
            if (!mIsInitialized || !mIsReadMode) return false;
            if (mReadPtr + sizeof(float) > mReadStream.Length) return false;

            data = BitConverter.ToSingle(mReadStream, mReadPtr);
            mReadPtr += sizeof(float);
            return true;
        }

        public bool ReadString(ref string data) {
            if (!mIsInitialized || !mIsReadMode) return false;
            if (mReadPtr + sizeof(UInt32) > mReadStream.Length) return false;

            var length = BitConverter.ToUInt32(mReadStream, mReadPtr);
            mReadPtr += sizeof(UInt32);

            if (mReadPtr + length > mReadStream.Length) return false;
            data = Encoding.ASCII.GetString(mReadStream, mReadPtr, (int)length);
            mReadPtr += (int)length;

            return true;
        }

        #endregion

        #region write func

        public bool WriteOpCode(ref Cmmo.OpCode data) {
            UInt32 code = 0;
            if (!WriteUInt32(ref code)) return false;
            data = (Cmmo.OpCode)code;
            return true;
        }

        public bool WriteUInt32(ref UInt32 data) {
            if (!mIsInitialized || mIsReadMode) return false;

            mWriteStream.Write(BitConverter.GetBytes(data), 0, 4);
            mWriteCounter += sizeof(UInt32);
            return true;
        }

        public bool WriteInt32(ref Int32 data) {
            if (!mIsInitialized || mIsReadMode) return false;

            mWriteStream.Write(BitConverter.GetBytes(data), 0, 4);
            mWriteCounter += sizeof(Int32);
            return true;
        }

        public bool WriteUInt8(ref byte data) {
            if (!mIsInitialized || mIsReadMode) return false;

            mWriteStream.WriteByte(data);
            mReadPtr += sizeof(byte);
            return true;
        }

        public bool WriteFloat(ref float data) {
            if (!mIsInitialized || mIsReadMode) return false;

            mWriteStream.Write(BitConverter.GetBytes(data), 0, 4);
            mWriteCounter += sizeof(float);
            return true;
        }

        public bool WriteString(ref string data) {
            if (!mIsInitialized || mIsReadMode) return false;

            mWriteStream.Write(BitConverter.GetBytes((UInt32)data.Length), 0, 4);
            mWriteStream.Write(Encoding.ASCII.GetBytes(data));
            mWriteCounter += sizeof(UInt32) + data.Length;
            return true;
        }


        #endregion

    }
}
