import { WebSocketServer, WebSocket } from 'ws'
import { Packet } from './packet.ts'

class MCBackend {
    wss: WebSocketServer
    clients: Map<bigint, WebSocket> = new Map()
    listen: Record<string, (packet: Packet, from?: bigint) => void> = {}
    constructor(public port: number) {
        this.wss = new WebSocketServer({ port })
        this.wss.on('error', console.error)

        this.wss.on('connection', ws => {
            const id = 10000n + BigInt(Math.ceil(Math.random() * 50000))
            this.clients.set(id, ws)
            server.listen['ws_connect']?.(Packet.build(id, 'ws_connect', Buffer.alloc(0)))

            ws.on('close', () => {
                server.listen['ws_disconnect']?.(Packet.build(id, 'ws_disconnect', Buffer.alloc(0)))
                this.clients.delete(id)
            })

            ws.on('message', (raw, binary) => {
                const packet = new Packet(raw as Buffer<ArrayBuffer>)
                const out = Packet.build(id, packet.type, packet.data).buf
                
                this.listen['ws_message']?.(packet, id)
                this.listen[packet.type]?.(packet, id)

                if(packet.guid == 0n)
                    this.clients.forEach((cli, id2) => (cli.readyState == 1 && id != id2) && cli.send(out, { binary }))
                else this.clients.get(packet.guid)?.send(out, { binary })
            })
        })
        console.log(`ok! ${port}`)
    }

    pollServers(client: WebSocket): void {
        const list = ['main', 'dummy']
        const packet = Packet.build(0n, 'server_list', Buffer.from(list.join('\n'), 'utf-8'))
        client.send(packet.buf, { binary: true })
    }
}

const server = new MCBackend(19132)
server.listen['ws_message'] = ({ guid, data }) => console.log(`[recv ${guid.toString(16)}] ${data.toString('hex')}`)
server.listen['ws_connect'] = ({ guid }) => console.log(`[ws_connect] ${guid.toString(16)}`)
server.listen['ws_disconnect'] = ({ guid }) => console.log(`[ws_disconnect] ${guid.toString(16)}`)
server.listen['login'] = (packet) => console.log(`[login] ${packet.decode()!.name}`)