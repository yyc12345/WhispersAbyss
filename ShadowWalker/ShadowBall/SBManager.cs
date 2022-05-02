using System;
using System.Collections.Generic;
using System.Text;

namespace ShadowWalker.ShadowBall {

    public enum DNFReason {
        None,
        Cheat,
        Logout,
        Timeout
    }
    public struct PlayerRank {
        public UInt32 PlayerId;
        public string PlayerNickname;
        public UInt32 PlayerIndex;  // 0 is dnf
        public DNFReason PlayerDNFReason;
    }

    public class SBManager {

        object mIsRecordingLock = new object();
        bool mIsRecording = false;
        Dictionary<UInt32, Cmmo.PlayerEntity> mPlayersDict = new Dictionary<uint, Cmmo.PlayerEntity>();
        Dictionary<UInt32, SBRecorder> mRecorderDict = new Dictionary<uint, SBRecorder>();
        Queue<PlayerRank> mPlayerRankQueue = new Queue<PlayerRank>();
        UInt32 mRankCounter = 1;

        public event Action<PlayerRank[]> NewRank;

        public void StartRecord(IEnumerable<Cmmo.PlayerEntity> players) {
            if (mIsRecording) StopRecord();

            // init counter and player dict
            mRankCounter = 1;
            foreach(var item in players) {
                mPlayersDict.Add(item.mPlayerId, item);
            }

            // todo: init recorder

            lock (mIsRecordingLock) {
                mIsRecording = true;
            }
        }

        public void IncomeBallState(Cmmo.Messages.BallState msg) {
            // todo: add ballstate to recorder
        }

        void TerminatePlayerWith(UInt32 playerId, UInt32 rank, DNFReason dnfReason) {
            if (mPlayersDict.TryGetValue(playerId, out Cmmo.PlayerEntity value)) {
                mPlayerRankQueue.Enqueue(new PlayerRank() {
                    PlayerId = value.mPlayerId,
                    PlayerNickname = value.mNickname,
                    PlayerIndex = rank,
                    PlayerDNFReason = dnfReason
                });
                mPlayersDict.Remove(playerId);
            }
            // no matched, forget it

            // todo: check recorder

            // check stop requirements
            if (mPlayersDict.Count == 0 && mRecorderDict.Count == 0) {
                StopRecord();
            }
        }
        public void IncomeCheat(Cmmo.Messages.CheatState msg) {
            if (msg.mCheated != 0)
                TerminatePlayerWith(msg.mPlayerId, 0, DNFReason.Cheat);
        }
        public void IncomeDisconnect(Cmmo.Messages.ClientDisconnected msg) {
            TerminatePlayerWith(msg.mPlayerId, 0, DNFReason.Logout);
        }
        public void InconmeLevelFinish(Cmmo.Messages.LevelFinish msg) {
            TerminatePlayerWith(msg.mPlayerId, mRankCounter++, DNFReason.None);
        }

        public void StopRecord() {
            lock (mIsRecordingLock) {
                if (!mIsRecording) return;
                mIsRecording = false;
            }

            // proc remain player
            foreach (var item in mPlayersDict) {
                mPlayerRankQueue.Enqueue(new PlayerRank() {
                    PlayerId = item.Value.mPlayerId,
                    PlayerNickname = item.Value.mNickname,
                    PlayerIndex = 0,
                    PlayerDNFReason = DNFReason.Timeout
                });
            }
            mPlayersDict.Clear();

            // todo: stop remain recorder


            NewRank?.Invoke(mPlayerRankQueue.ToArray());
            mPlayerRankQueue.Clear();

        }

    }





}
