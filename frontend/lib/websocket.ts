export interface Message {
  type: 'message' | 'command' | 'system' | 'error'
  content: string
  timestamp: Date
  sender?: string
}

class WebSocketClient {
  private ws: WebSocket | null = null
  private reconnectAttempts = 0
  private maxReconnectAttempts = 10
  private reconnectDelay = 1000
  private messageHandlers: Set<(message: Message) => void> = new Set()
  private connectionHandlers: Set<(connected: boolean) => void> = new Set()

  connect(url: string = 'ws://localhost:8080') {
    if (this.ws?.readyState === WebSocket.OPEN) {
      return
    }

    try {
      this.ws = new WebSocket(url)

      this.ws.onopen = () => {
        console.log('WebSocket connected')
        this.reconnectAttempts = 0
        this.connectionHandlers.forEach(handler => handler(true))
      }

      this.ws.onmessage = (event) => {
        const message: Message = {
          type: 'message',
          content: event.data,
          timestamp: new Date(),
        }
        this.messageHandlers.forEach(handler => handler(message))
      }

      this.ws.onerror = (error) => {
        console.error('WebSocket error:', error)
        const errorMessage: Message = {
          type: 'error',
          content: 'Connection error occurred',
          timestamp: new Date(),
        }
        this.messageHandlers.forEach(handler => handler(errorMessage))
      }

      this.ws.onclose = () => {
        console.log('WebSocket disconnected')
        this.connectionHandlers.forEach(handler => handler(false))
        this.attemptReconnect(url)
      }
    } catch (error) {
      console.error('Failed to create WebSocket:', error)
    }
  }

  private attemptReconnect(url: string) {
    if (this.reconnectAttempts < this.maxReconnectAttempts) {
      this.reconnectAttempts++
      setTimeout(() => {
        console.log(`Reconnecting... (attempt ${this.reconnectAttempts})`)
        this.connect(url)
      }, this.reconnectDelay * this.reconnectAttempts)
    } else {
      console.error('Max reconnection attempts reached')
    }
  }

  send(message: string) {
    if (this.ws?.readyState === WebSocket.OPEN) {
      this.ws.send(message)
      return true
    }
    console.warn('WebSocket is not connected')
    return false
  }

  onMessage(handler: (message: Message) => void) {
    this.messageHandlers.add(handler)
    return () => this.messageHandlers.delete(handler)
  }

  onConnectionChange(handler: (connected: boolean) => void) {
    this.connectionHandlers.add(handler)
    return () => this.connectionHandlers.delete(handler)
  }

  disconnect() {
    if (this.ws) {
      this.ws.close()
      this.ws = null
    }
  }

  isConnected(): boolean {
    return this.ws?.readyState === WebSocket.OPEN
  }
}

export const wsClient = new WebSocketClient()
