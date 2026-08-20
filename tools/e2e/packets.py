#!/usr/bin/env python3
# packets.py - packet ids, lengths, and pack/unpack helpers for the TMWA
# eAthena-derived client protocol.
#
# The length table below is generated from src/proto2/client-enum.hpp and
# src/proto2/client-packet-info.cpp (the authoritative description of what a
# supported client sends and receives). -1 means variable length: a u16
# little-endian total length follows the packet id.

import struct

VAR = -1

# packet id -> (length, name)
PACKET_LENGTHS = {
    0x0061: 50,    # CMSG_CHAR_PASSWORD_CHANGE
    0x0062: 3,     # SMSG_CHAR_PASSWORD_RESPONSE
    0x0063: VAR,   # SMSG_UPDATE_HOST
    0x0064: 55,    # CMSG_LOGIN_REGISTER
    0x0065: 17,    # CMSG_CHAR_SERVER_CONNECT
    0x0066: 3,     # CMSG_CHAR_SELECT
    0x0067: 37,    # CMSG_CHAR_CREATE
    0x0068: 46,    # CMSG_CHAR_DELETE
    0x0069: VAR,   # SMSG_LOGIN_DATA
    0x006a: 23,    # SMSG_LOGIN_ERROR
    0x006b: VAR,   # SMSG_CHAR_LOGIN
    0x006c: 3,     # SMSG_CHAR_LOGIN_ERROR
    0x006d: 108,   # SMSG_CHAR_CREATE_SUCCEEDED
    0x006e: 3,     # SMSG_CHAR_CREATE_FAILED
    0x006f: 2,     # SMSG_CHAR_DELETE_SUCCEEDED
    0x0070: 3,     # SMSG_CHAR_DELETE_FAILED
    0x0071: 28,    # SMSG_CHAR_MAP_INFO
    0x0072: 19,    # CMSG_MAP_SERVER_CONNECT
    0x0073: 11,    # SMSG_MAP_LOGIN_SUCCESS
    0x0078: 54,    # SMSG_BEING_VISIBLE
    0x007b: 60,    # SMSG_BEING_MOVE
    0x007c: 41,    # SMSG_BEING_SPAWN
    0x007d: 2,     # CMSG_MAP_LOADED
    0x007e: 6,     # CMSG_MAP_PING
    0x007f: 6,     # SMSG_SERVER_PING
    0x0080: 7,     # SMSG_BEING_REMOVE
    0x0081: 3,     # SMSG_CONNECTION_PROBLEM
    0x0085: 5,     # CMSG_PLAYER_CHANGE_DEST
    0x0087: 12,    # SMSG_WALK_RESPONSE
    0x0088: 10,    # SMSG_PLAYER_STOP
    0x0089: 7,     # CMSG_PLAYER_CHANGE_ACT
    0x008a: 29,    # SMSG_BEING_ACTION
    0x008c: VAR,   # CMSG_CHAT_MESSAGE
    0x008d: VAR,   # SMSG_BEING_CHAT
    0x008e: VAR,   # SMSG_PLAYER_CHAT
    0x0090: 7,     # CMSG_NPC_TALK
    0x0091: 22,    # SMSG_PLAYER_WARP
    0x0092: 28,    # SMSG_CHANGE_MAP_SERVER
    0x0094: 6,     # CMSG_NAME_REQUEST
    0x0095: 30,    # SMSG_BEING_NAME_RESPONSE
    0x0096: VAR,   # CMSG_CHAT_WHISPER
    0x0097: VAR,   # SMSG_WHISPER
    0x0098: 3,     # SMSG_WHISPER_RESPONSE
    0x009a: VAR,   # SMSG_GM_CHAT
    0x009b: 5,     # CMSG_PLAYER_CHANGE_DIR
    0x009c: 9,     # SMSG_BEING_CHANGE_DIRECTION
    0x009d: 17,    # SMSG_ITEM_VISIBLE
    0x009e: 17,    # SMSG_ITEM_DROPPED
    0x009f: 6,     # CMSG_ITEM_PICKUP
    0x00a0: 23,    # SMSG_PLAYER_INVENTORY_ADD
    0x00a1: 6,     # SMSG_ITEM_REMOVE
    0x00a2: 6,     # CMSG_PLAYER_INVENTORY_DROP
    0x00a4: VAR,   # SMSG_PLAYER_EQUIPMENT
    0x00a6: VAR,   # SMSG_PLAYER_STORAGE_EQUIP
    0x00a7: 8,     # CMSG_PLAYER_INVENTORY_USE
    0x00a8: 7,     # SMSG_ITEM_USE_RESPONSE
    0x00a9: 6,     # CMSG_PLAYER_EQUIP
    0x00aa: 7,     # SMSG_PLAYER_EQUIP
    0x00ab: 4,     # CMSG_PLAYER_UNEQUIP
    0x00ac: 7,     # SMSG_PLAYER_UNEQUIP
    0x00af: 6,     # SMSG_PLAYER_INVENTORY_REMOVE
    0x00b0: 8,     # SMSG_PLAYER_STAT_UPDATE_1
    0x00b1: 8,     # SMSG_PLAYER_STAT_UPDATE_2
    0x00b2: 3,     # CMSG_PLAYER_REBOOT
    0x00b3: 3,     # SMSG_CHAR_SWITCH_RESPONSE
    0x00b4: VAR,   # SMSG_NPC_MESSAGE
    0x00b5: 6,     # SMSG_NPC_NEXT
    0x00b6: 6,     # SMSG_NPC_CLOSE
    0x00b7: VAR,   # SMSG_NPC_CHOICE
    0x00b8: 7,     # CMSG_NPC_LIST_CHOICE
    0x00b9: 6,     # CMSG_NPC_NEXT_REQUEST
    0x00bb: 5,     # CMSG_STAT_UPDATE_REQUEST
    0x00bc: 6,     # SMSG_PLAYER_STAT_UPDATE_4
    0x00bd: 44,    # SMSG_PLAYER_STAT_UPDATE_5
    0x00be: 5,     # SMSG_PLAYER_STAT_UPDATE_6
    0x00bf: 3,     # CMSG_PLAYER_EMOTE
    0x00c0: 7,     # SMSG_BEING_EMOTION
    0x00c4: 6,     # SMSG_NPC_BUY_SELL_CHOICE
    0x00c5: 7,     # CMSG_NPC_BUY_SELL_REQUEST
    0x00c6: VAR,   # SMSG_NPC_BUY
    0x00c7: VAR,   # SMSG_NPC_SELL
    0x00c8: VAR,   # CMSG_NPC_BUY_REQUEST
    0x00c9: VAR,   # CMSG_NPC_SELL_REQUEST
    0x00ca: 3,     # SMSG_NPC_BUY_RESPONSE
    0x00cb: 3,     # SMSG_NPC_SELL_RESPONSE
    0x00cd: 6,     # SMSG_ADMIN_KICK_ACK
    0x00e4: 6,     # CMSG_TRADE_REQUEST
    0x00e5: 26,    # SMSG_TRADE_REQUEST
    0x00e6: 3,     # CMSG_TRADE_RESPONSE
    0x00e7: 3,     # SMSG_TRADE_RESPONSE
    0x00e8: 8,     # CMSG_TRADE_ITEM_ADD_REQUEST
    0x00e9: 19,    # SMSG_TRADE_ITEM_ADD
    0x00eb: 2,     # CMSG_TRADE_ADD_COMPLETE
    0x00ec: 3,     # SMSG_TRADE_OK
    0x00ed: 2,     # CMSG_TRADE_CANCEL_REQUEST
    0x00ee: 2,     # SMSG_TRADE_CANCEL
    0x00ef: 2,     # CMSG_TRADE_OK
    0x00f0: 3,     # SMSG_TRADE_COMPLETE
    0x00f2: 6,     # SMSG_PLAYER_STORAGE_STATUS
    0x00f3: 8,     # CMSG_MOVE_TO_STORAGE
    0x00f4: 21,    # SMSG_PLAYER_STORAGE_ADD
    0x00f5: 8,     # CMSG_MOVE_FROM_STORAGE
    0x00f6: 8,     # SMSG_PLAYER_STORAGE_REMOVE
    0x00f7: 2,     # CMSG_CLOSE_STORAGE
    0x00f8: 2,     # SMSG_PLAYER_STORAGE_CLOSE
    0x00f9: 26,    # CMSG_PARTY_CREATE
    0x00fa: 3,     # SMSG_PARTY_CREATE
    0x00fb: VAR,   # SMSG_PARTY_INFO
    0x00fc: 6,     # CMSG_PARTY_INVITE
    0x00fd: 27,    # SMSG_PARTY_INVITE_RESPONSE
    0x00fe: 30,    # SMSG_PARTY_INVITED
    0x00ff: 10,    # CMSG_PARTY_INVITED
    0x0100: 2,     # CMSG_PARTY_LEAVE
    0x0101: 6,     # SMSG_PARTY_SETTINGS
    0x0102: 6,     # CMSG_PARTY_SETTINGS
    0x0103: 30,    # CMSG_PARTY_KICK
    0x0105: 31,    # SMSG_PARTY_LEAVE
    0x0106: 10,    # SMSG_PARTY_UPDATE_HP
    0x0107: 10,    # SMSG_PARTY_UPDATE_COORDS
    0x0108: VAR,   # CMSG_PARTY_MESSAGE
    0x0109: VAR,   # SMSG_PARTY_MESSAGE
    0x010e: 11,    # SMSG_PLAYER_SKILL_UP
    0x010f: VAR,   # SMSG_PLAYER_SKILLS
    0x0110: 10,    # SMSG_SKILL_FAILED
    0x0112: 4,     # CMSG_SKILL_LEVELUP_REQUEST
    0x0118: 2,     # CMSG_PLAYER_STOP_ATTACK
    0x0119: 13,    # SMSG_PLAYER_STATUS_CHANGE
    0x0139: 16,    # SMSG_PLAYER_MOVE_TO_ATTACK
    0x013a: 4,     # SMSG_PLAYER_ATTACK_RANGE
    0x013b: 4,     # SMSG_PLAYER_ARROW_MESSAGE
    0x013c: 4,     # SMSG_PLAYER_ARROW_EQUIP
    0x0141: 14,    # SMSG_PLAYER_STAT_UPDATE_3
    0x0142: 6,     # SMSG_NPC_INT_INPUT
    0x0143: 10,    # CMSG_NPC_INT_RESPONSE
    0x0146: 6,     # CMSG_NPC_CLOSE
    0x0148: 8,     # SMSG_BEING_RESURRECT
    0x018a: 4,     # CMSG_CLIENT_QUIT
    0x018b: 4,     # SMSG_MAP_QUIT_RESPONSE
    0x0195: 102,   # SMSG_PLAYER_GUILD_PARTY_INFO
    0x0196: 9,     # SMSG_BEING_STATUS_CHANGE
    0x0199: 4,     # SMSG_PVP_MAP_MODE
    0x019a: 14,    # SMSG_PVP_SET
    0x019b: 10,    # SMSG_BEING_SELFEFFECT
    0x01b1: 7,     # SMSG_TRADE_ITEM_ADD_RESPONSE
    0x01c8: 13,    # SMSG_PLAYER_INVENTORY_USE
    0x01d4: 6,     # SMSG_NPC_STR_INPUT
    0x01d5: VAR,   # CMSG_NPC_STR_RESPONSE
    0x01d7: 11,    # SMSG_BEING_CHANGE_LOOKS2
    0x01d8: 54,    # SMSG_PLAYER_UPDATE_1
    0x01d9: 53,    # SMSG_PLAYER_UPDATE_2
    0x01da: 60,    # SMSG_PLAYER_MOVE
    0x01de: 33,    # SMSG_SKILL_DAMAGE
    0x01ee: VAR,   # SMSG_PLAYER_INVENTORY
    0x01f0: VAR,   # SMSG_PLAYER_STORAGE_ITEMS
    0x020c: 10,    # SMSG_BEING_IP_RESPONSE
    0x0210: 2,     # CMSG_ONLINE_LIST
    0x0211: VAR,   # SMSG_ONLINE_LIST
    0x0212: 16,    # SMSG_NPC_COMMAND
    0x0214: 8,     # SMSG_QUEST_SET_VAR
    0x0215: VAR,   # SMSG_QUEST_PLAYER_VARS
    0x0225: VAR,   # SMSG_BEING_MOVE3
    0x0226: 10,    # SMSG_MAP_MASK
    0x0227: VAR,   # SMSG_MAP_MUSIC
    0x0228: VAR,   # SMSG_NPC_CHANGETITLE
    0x0229: VAR,   # SMSG_SCRIPT_MESSAGE
    0x0230: VAR,   # SMSG_PLAYER_CLIENT_COMMAND
    0x0231: 34,    # SMSG_MAP_SET_TILES_TYPE
    0x0232: 10,    # SMSG_PLAYER_HP
    0x0233: 14,    # SMSG_PLAYER_HP_FULL
    0x7530: 2,     # CMSG_SERVER_VERSION_REQUEST
    0x7531: 10,    # SMSG_SERVER_VERSION_RESPONSE
    0x7532: 2,     # CMSG_CLIENT_DISCONNECT
    # The char and map servers send this 4-byte marker right after accepting
    # a connection (Packet_Payload<0x8000>, the modern stand-in for the old
    # raw account-id echo). Skip it.
    0x8000: 4,
}

# SP ids (db/params.txt / clif.t.hpp)
SP_STR = 13
SP_ZENY = 20


def fixed_string(s, length):
    b = s.encode('utf-8') if isinstance(s, str) else s
    if len(b) > length:
        raise ValueError('string too long for field: %r' % (s,))
    return b.ljust(length, b'\0')


def read_fixed_string(data):
    return data.split(b'\0', 1)[0].decode('utf-8', 'replace')


def pack_pos1(x, y, direction=0):
    b0 = (x >> 2) & 0xff
    b1 = ((x << 6) | ((y >> 4) & 0x3f)) & 0xff
    b2 = ((y << 4) | (direction & 0xf)) & 0xff
    return bytes([b0, b1, b2])


def unpack_pos1(data):
    b0, b1, b2 = data[0], data[1], data[2]
    x = (b0 << 2) | (b1 >> 6)
    y = ((b1 & 0x3f) << 4) | (b2 >> 4)
    direction = b2 & 0xf
    return x, y, direction


class Packet(object):
    """One received packet: id plus raw payload (including the 2-byte id)."""

    def __init__(self, packet_id, data):
        self.id = packet_id
        self.data = data

    def u8(self, off):
        return self.data[off]

    def u16(self, off):
        return struct.unpack_from('<H', self.data, off)[0]

    def u32(self, off):
        return struct.unpack_from('<I', self.data, off)[0]

    def i32(self, off):
        return struct.unpack_from('<i', self.data, off)[0]

    def string(self, off, length):
        return read_fixed_string(self.data[off:off + length])

    def tail(self, off):
        return self.data[off:]

    def pos1(self, off):
        return unpack_pos1(self.data[off:off + 3])

    def __repr__(self):
        return 'Packet(0x%04x, %d bytes)' % (self.id, len(self.data))
