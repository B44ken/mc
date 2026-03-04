import { WebSocketServer, WebSocket } from 'ws'

class WSNetServer {
    wss: WebSocketServer
    clients = new Map<number, WebSocket>()
    nextClientId = 1

    constructor(public port: number) {
        this.wss = new WebSocketServer({ port })
        this.wss.on('error', err => console.error('ws server error', err))

        this.wss.on('connection', ws => {
            const id = this.nextClientId++
            this.clients.set(id, ws)
            console.log(`client connected: ${id}`)

            ws.on('close', () => {
                this.clients.delete(id)
                console.log(`client disconnected: ${id}`)
            })

            ws.on('message', (raw, binary) => {
                const packet = Buffer.isBuffer(raw)
                    ? raw : Array.isArray(raw)? Buffer.concat(raw): Buffer.from(raw)

                const wrapped = Buffer.allocUnsafe(8 + packet.length)
                wrapped.writeBigUInt64LE(BigInt(id), 0)
                packet.copy(wrapped, 8)

                for (const [otherId, client] of this.clients) {
                    if (otherId !== id && client.readyState === WebSocket.OPEN) client.send(wrapped, { binary })
                }
            })
        })

        console.log(`ok! ${port}`)
    }
}

new WSNetServer(6767)