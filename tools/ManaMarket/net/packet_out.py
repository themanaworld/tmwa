from packet import *
from protocol import *

def emote(emoteId):
    emote_packet = PacketOut(CMSG_PLAYER_EMOTE)
    emote_packet.write_int8(emoteId)
    return str(emote_packet)

def whisper(nick, message):
    whisp_packet = PacketOut(CMSG_CHAT_WHISPER)
    whisp_packet.write_int16(len(message) + 28)
    whisp_packet.write_string(nick, 24)
    whisp_packet.write_string(message, len(message))
    return str(whisp_packet)

def chat(text):
    chat_packet = PacketOut(CMSG_CHAT_MESSAGE)
    mes = player_node.name + " : " + text
    chat_packet.write_int16(len(mes) + 4 + 1)
    chat_packet.write_string(mes, len(mes) + 1)
    return str(chat_packet)

def sit(val):
    sit_packet = PacketOut(CMSG_PLAYER_CHANGE_ACT)
    sit_packet.write_int32(0)
    if val == True:
        sit_packet.write_int8(2)
    else:
        sit_packet.write_int8(3)
    return str(sit_packet)

def trade_request(being_id):
    trade_req_packet = PacketOut(CMSG_TRADE_REQUEST)
    trade_req_packet.write_int32(being_id)
    return str(trade_req_packet)

def trade_respond(accept):
    trade_respond_packet = PacketOut(CMSG_TRADE_RESPONSE)
    if accept == True:
        trade_respond_packet.write_int8(3)
    elif accept == False:
        trade_respond_packet.write_int8(4)
    return str(trade_respond_packet)

def trade_add_item(item_index, amount):
    trade_add_packet = PacketOut(CMSG_TRADE_ITEM_ADD_REQUEST)
    trade_add_packet.write_int16(item_index + inventory_offset)
    trade_add_packet.write_int32(amount)
    return str(trade_add_packet)

