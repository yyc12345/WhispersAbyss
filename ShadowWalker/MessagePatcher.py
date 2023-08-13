import BmmoProto
import io, os

class MessagePatcher:

    # This OpCode : That OpCode
    # This OpCode is used by yyc12345
    # That OpCode is used by BallanceBug
    __OpCodePair: tuple[tuple[int]] = (
        # (BmmoProto.OpCode.none_msg.value, 0, ),
        # (BmmoProto.OpCode.login_request_msg.value, 1, ),
        # (BmmoProto.OpCode.login_accepted_msg.value, 2, ),
        (BmmoProto.OpCode.simple_action_msg.value, 3, ),
        (BmmoProto.OpCode.player_disconnected_msg.value, 4, ),
        # (BmmoProto.OpCode.player_connected_msg.value, 5, ),
        # (BmmoProto.OpCode.ping_msg.value, 6, ),
        # (BmmoProto.OpCode.ball_state_msg.value, 7, ),
        # (BmmoProto.OpCode.owned_ball_state_msg.value, 8, ),
        # (BmmoProto.OpCode.keyboard_input_msg.value, 9, ),
        (BmmoProto.OpCode.chat_msg.value, 10, ),
        # (BmmoProto.OpCode.level_finish_msg.value, 11, ),
        # (BmmoProto.OpCode.login_request_v2_msg.value, 12, ),
        # (BmmoProto.OpCode.login_accepted_v2_msg.value, 13, ),
        (BmmoProto.OpCode.player_connected_v2_msg.value, 14, ),
        (BmmoProto.OpCode.cheat_state_msg.value, 15, ),
        (BmmoProto.OpCode.owned_cheat_state_msg.value, 16, ),
        (BmmoProto.OpCode.cheat_toggle_msg.value, 17, ),
        (BmmoProto.OpCode.owned_cheat_toggle_msg.value, 18, ),
        (BmmoProto.OpCode.kick_request_msg.value, 19, ),
        (BmmoProto.OpCode.player_kicked_msg.value, 20, ),
        # (BmmoProto.OpCode.owned_ball_state_v2_msg.value, 21, ),
        (BmmoProto.OpCode.login_request_v3_msg.value, 22, ),
        (BmmoProto.OpCode.level_finish_v2_msg.value, 23, ),
        (BmmoProto.OpCode.action_denied_msg.value, 24, ),
        (BmmoProto.OpCode.op_state_msg.value, 25, ),
        (BmmoProto.OpCode.countdown_msg.value, 26, ),
        (BmmoProto.OpCode.did_not_finish_msg.value, 27, ),
        (BmmoProto.OpCode.map_names_msg.value, 28, ),
        (BmmoProto.OpCode.plain_text_msg.value, 29, ),
        (BmmoProto.OpCode.current_map_msg.value, 30, ),
        (BmmoProto.OpCode.hash_data_msg.value, 31, ),
        (BmmoProto.OpCode.timed_ball_state_msg.value, 32, ),
        # (BmmoProto.OpCode.owned_timed_ball_state_msg.value, 33, ),
        (BmmoProto.OpCode.timestamp_msg.value, 34, ),
        (BmmoProto.OpCode.private_chat_msg.value, 35, ),
        (BmmoProto.OpCode.player_ready_msg.value, 36, ),
        (BmmoProto.OpCode.important_notification_msg.value, 37, ),
        (BmmoProto.OpCode.mod_list_msg.value, 38, ),
        (BmmoProto.OpCode.popup_box_msg.value, 39, ),
        (BmmoProto.OpCode.current_sector_msg.value, 40, ),
        (BmmoProto.OpCode.login_accepted_v3_msg.value, 41, ),
        (BmmoProto.OpCode.permanent_notification_msg.value, 42, ),
        (BmmoProto.OpCode.sound_data_msg.value, 43, ),
        (BmmoProto.OpCode.public_notification_msg.value, 44, ),
        (BmmoProto.OpCode.owned_compressed_ball_state_msg.value, 45, ),
    )
    __This2ThatOpCode: dict[int, int] = dict(__OpCodePair)
    __That2ThisOpCode: dict[int, int] = dict(tuple(map(lambda pair: (pair[1], pair[0],), __OpCodePair)))

    def __RecvPatch_owned_compressed_ball_state_msg(self, ss: io.BytesIO):
        pass

    __RecvProcMap: dict[int] = {
        BmmoProto.OpCode.owned_compressed_ball_state_msg.value: __RecvPatch_owned_compressed_ball_state_msg
    }

    def PatchRecv(self, ss: io.BytesIO):
        # patch opcode
        ss.seek(0, io.SEEK_SET)
        this_opcode = self.__That2ThisOpCode[BmmoProto.PeekOpCode(ss)]
        BmmoProto.WriteOpCode(this_opcode, ss)

        # patch data
        func = self.__RecvProcMap.get(this_opcode, None)
        if func: func(self, ss)

    def PatchSend(self, ss: io.BytesIO):
        # patch opcode
        ss.seek(0, io.SEEK_SET)
        that_opcode = self.__This2ThatOpCode[BmmoProto.PeekOpCode(ss)]
        BmmoProto.WriteOpCode(that_opcode, ss)
        # patch data
        # no

class MessageFilter:
    __AllowedOpCode: tuple[int] = (
        BmmoProto.OpCode.login_accepted_v3_msg.value,
        BmmoProto.OpCode.simple_action_msg.value,
        BmmoProto.OpCode.chat_msg.value,
        BmmoProto.OpCode.player_connected_v2_msg.value,
        BmmoProto.OpCode.player_disconnected_msg.value,
    )

    def UniversalDeserialize(self, ss: io.BytesIO) -> BmmoProto.BpMessage:
        opcode = BmmoProto.ReadOpCode(ss)
        if opcode not in self.__AllowedOpCode: return None
        msg = BmmoProto.MessageFactory(opcode)
        msg.Deserialize(ss)
        return msg
