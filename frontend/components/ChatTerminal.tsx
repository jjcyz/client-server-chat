'use client'

import { useState, useEffect, useRef } from 'react'
import { wsClient, Message } from '@/lib/websocket'
import MessageList from './MessageList'
import InputArea from '@/components/InputArea'
import Sidebar from './Sidebar'
import AuthModal from './AuthModal'

export default function ChatTerminal() {
  const [messages, setMessages] = useState<Message[]>([])
  const [isConnected, setIsConnected] = useState(false)
  const [isAuthenticated, setIsAuthenticated] = useState(false)
  const [username, setUsername] = useState('')
  const [showAuth, setShowAuth] = useState(true)
  const [activeUsers, setActiveUsers] = useState<string[]>([])
  const [stats, setStats] = useState<any>(null)

  useEffect(() => {
    wsClient.connect()

    const unsubscribeMessage = wsClient.onMessage((message) => {
      // Parse server responses
      const content = message.content.trim()

      // Check for authentication success
      if (content.includes('Login successful!') || content.includes('Registration successful!')) {
        setIsAuthenticated(true)
        setShowAuth(false)
        // Extract username from response if possible
        const match = content.match(/Welcome,?\s+(\w+)/i)
        if (match) {
          setUsername(match[1])
        }
      }

      // Check for user list
      if (content.includes('Active users:')) {
        const lines = content.split('\n')
        const users: string[] = []
        let inUserList = false
        for (const line of lines) {
          if (line.includes('Active users:')) {
            inUserList = true
            continue
          }
          if (inUserList && line.trim()) {
            // Backend sends usernames directly, one per line (no "- " prefix)
            const trimmed = line.trim()
            if (trimmed && !trimmed.includes(':')) {
              // Skip lines that look like headers or other info
              users.push(trimmed)
            }
          }
        }
        setActiveUsers(users)
      }

      // Check for stats
      if (content.includes('Server Statistics:')) {
        const statsData: any = {}
        const lines = content.split('\n')
        for (const line of lines) {
          if (line.includes('Total Messages:')) {
            statsData.totalMessages = line.match(/\d+/)?.[0]
          }
          if (line.includes('Current Connections:')) {
            statsData.activeConnections = line.match(/\d+/)?.[0]
          }
          if (line.includes('Uptime:')) {
            statsData.uptime = line.split('Uptime:')[1]?.trim()
          }
        }
        setStats(statsData)
      }

      setMessages(prev => [...prev, message])
    })

    const unsubscribeConnection = wsClient.onConnectionChange((connected) => {
      setIsConnected(connected)
      if (!connected) {
        setIsAuthenticated(false)
        setShowAuth(true)
      }
    })

    return () => {
      unsubscribeMessage()
      unsubscribeConnection()
      wsClient.disconnect()
    }
  }, [])

  const handleSendMessage = (text: string) => {
    if (!isConnected || !text.trim()) return

    wsClient.send(text)

    // Add user's own message to UI immediately
    if (!text.startsWith('/')) {
      const userMessage: Message = {
        type: 'message',
        content: text,
        timestamp: new Date(),
        sender: username || 'You',
      }
      setMessages(prev => [...prev, userMessage])
    }
  }

  const handleAuthSuccess = (user: string) => {
    setUsername(user)
    setIsAuthenticated(true)
    setShowAuth(false)
    // Fetch user list and stats after authentication
    setTimeout(() => {
      wsClient.send('/list')
      wsClient.send('/stats')
    }, 100)
  }

  const handleCommand = (command: string) => {
    if (command === '/list') {
      wsClient.send('/list')
    } else if (command === '/stats') {
      wsClient.send('/stats')
    }
  }

  return (
    <div className="flex h-screen w-screen bg-bloomberg-bg">
      {/* Main Chat Area */}
      <div className="flex-1 flex flex-col">
        {/* Header */}
        <div className="bg-bloomberg-bg-secondary bloomberg-border-b px-4 py-2 flex items-center justify-between">
          <div className="flex items-center gap-4">
            <h1 className="text-bloomberg-accent text-lg font-bold bloomberg-text-shadow">
              CHAT TERMINAL
            </h1>
            <div className="flex items-center gap-2">
              <div className={`w-2 h-2 rounded-full ${isConnected ? 'bg-bloomberg-success' : 'bg-bloomberg-error'}`} />
              <span className="text-xs text-bloomberg-text-dim">
                {isConnected ? 'CONNECTED' : 'DISCONNECTED'}
              </span>
            </div>
          </div>
          {isAuthenticated && (
            <div className="text-sm text-bloomberg-text-dim">
              User: <span className="text-bloomberg-accent">{username}</span>
            </div>
          )}
        </div>

        {/* Messages */}
        <div className="flex-1 overflow-hidden">
          <MessageList messages={messages} />
        </div>

        {/* Input */}
        <div className="bloomberg-border-t bg-bloomberg-bg-secondary">
          <InputArea
            onSend={handleSendMessage}
            onCommand={handleCommand}
            disabled={!isConnected || !isAuthenticated}
            placeholder={!isAuthenticated ? 'Please authenticate to chat...' : 'Type message or command...'}
          />
        </div>
      </div>

      {/* Sidebar */}
      <Sidebar
        activeUsers={activeUsers}
        stats={stats}
        onRefreshUsers={() => handleCommand('/list')}
        onRefreshStats={() => handleCommand('/stats')}
      />

      {/* Auth Modal */}
      {showAuth && (
        <AuthModal
          onAuthSuccess={handleAuthSuccess}
          isConnected={isConnected}
        />
      )}
    </div>
  )
}
