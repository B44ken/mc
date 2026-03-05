const typeStart = 0x81
const typeList: string[] = [ 'keep_alive', 'login', 'login_status', 'ready', 'message', 'set_time', 'start_game', 'add_mob', 'add_player', 'remove_player', 'teleport_entity', 'add_entity', 'remove_entity', 'add_item_entity', 'take_item_entity', 'move_entity', 'move_entity_pos', 'move_entity_rot', 'move_entity_pos_rot', 'move_player', 'place_block', 'remove_block', 'update_block', 'add_painting', 'explode', 'level_event', 'tile_event', 'entity_event', 'request_chunk', 'chunk_data', 'player_equipment', 'player_armor_equipment', 'interact', 'use_item', 'player_action', 'update_armor', 'hurt_armor', 'set_entity_data', 'set_entity_motion', 'set_health', 'set_spawn_position', 'animate', 'respawn', 'send_inventory', 'drop_item', 'container_open', 'container_close', 'container_set_slot', 'container_set_data', 'container_set_content', 'container_ack', 'chat', 'sign_update', 'adventure_settings' ]

export class Packet {
    guid: bigint
    type: string
    data: Buffer<ArrayBuffer>
    constructor(public buf: Buffer<ArrayBuffer>) {
        this.guid = this.buf.readBigUInt64BE(0)
        this.type = typeList[this.buf[8] - typeStart] ?? `unknown(${this.buf[8]})`
        this.data = this.buf.subarray(9)
    }

    decode() {
        let read = 9, base = { type: this.type, guid: this.guid }
        const map: Record<string, number | string> = {}
        
        const consume = (...args: [name: string, type: ('u8' | 'i32' | 'str')][]) => {
            for(const [name, type] of args) {
                if(type == 'i32') {
                    map[name] = this.buf.readInt32BE(read)
                    read += 4
                } else if(type == 'u8') {
                    map[name] = this.buf[read]
                    read += 1
                } else if(type == 'str') {
                    const len = this.buf.readInt16BE(read)
                    map[name] = this.buf.toString('utf-8', read + 2, read + 2 + len)
                    read += 2 + len
                }
            }
            return map
        }

        if(this.type == 'login') return consume(['name', 'str'])
    }

    static build(guid: bigint, type: number | string, data: Buffer<ArrayBuffer>) {
        const buf = Buffer.alloc(8 + 1 + data.length)
        buf.writeBigUInt64BE(guid, 0)
        buf[8] = typeof type == 'string' ? typeList.indexOf(type) + typeStart : type
        data.copy(buf, 9)
        return new Packet(buf)
    }
}