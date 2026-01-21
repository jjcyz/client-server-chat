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
  private currentUrl: string = ''

  connect(url?: string) {
    // Use environment variable in production, fallback to localhost for development
    const wsUrl = url || process.env.NEXT_PUBLIC_WS_URL || 'ws://localhost:8080'
    this.currentUrl = wsUrl
    
    if (this.ws?.readyState === WebSocket.OPEN) {
      return
    }

    // Check if we're trying to connect to localhost in production
    const isLocalhost = typeof window !== 'undefined' && 
      (wsUrl.includes('localhost') || wsUrl.includes('127.0.0.1'))
    const isProduction = typeof window !== 'undefined' && 
      window.location.hostname !== 'localhost' && window.location.hostname !== '127.0.0.1'
    
    if (isLocalhost && isProduction) {
      // Don't attempt connection if misconfigured in production (trying to connect to localhost when deployed)
      console.warn('WebSocket URL not configured. Please set NEXT_PUBLIC_WS_URL environment variable.')
      this.connectionHandlers.forEach(handler => handler(false))
      return
    }

    try {
      this.ws = new WebSocket(wsUrl)

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
        // Only attempt reconnect if not misconfigured
        if (!(isLocalhost && isProduction)) {
          this.attemptReconnect()
        }
      }
    } catch (error) {
      console.error('Failed to create WebSocket:', error)
      this.connectionHandlers.forEach(handler => handler(false))
    }
  }

  private attemptReconnect() {
    if (this.reconnectAttempts < this.maxReconnectAttempts) {
      this.reconnectAttempts++
      setTimeout(() => {
        console.log(`Reconnecting... (attempt ${this.reconnectAttempts})`)
        this.connect(this.currentUrl)
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
