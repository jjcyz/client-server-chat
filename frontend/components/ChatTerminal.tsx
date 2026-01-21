'use client'

import { useState, useEffect, useMemo } from 'react'
import { wsClient, Message, PrivateConversation } from '@/lib/websocket'
import {
  isDemoMode,
  getMockMessages,
  getMockUsers,
  getMockStats,
  createDemoMessageHandler
} from '@/lib/demo-mode'
import MessageList from './MessageList'
import InputArea from '@/components/InputArea'
import Sidebar from './Sidebar'
import AuthModal from './AuthModal'
import MobileMenuButton from './MobileMenuButton'
import PrivateChatWindow from './PrivateChatWindow'
import { useResizable } from '@/hooks/useResizable'

export default function ChatTerminal() {
  const [messages, setMessages] = useState<Message[]>([])
  const [isConnected, setIsConnected] = useState(false)
  const [isAuthenticated, setIsAuthenticated] = useState(false)
  const [username, setUsername] = useState('')
  const [showAuth, setShowAuth] = useState(true)
  const [activeUsers, setActiveUsers] = useState<string[]>([])
  const [stats, setStats] = useState<any>(null)
  const [isAdmin, setIsAdmin] = useState(false)
  const [privateConversations, setPrivateConversations] = useState<Map<string, PrivateConversation>>(new Map())
  const [highestZIndex, setHighestZIndex] = useState(100)
  const [demoMode, setDemoMode] = useState(false)
  const [isMobileSidebarOpen, setIsMobileSidebarOpen] = useState(false)

  // Resizable sidebar hook
  const { width: sidebarWidth, isResizing, onResizeStart } = useResizable({
    minWidth: 200,
    maxWidth: 500,
    defaultWidth: 320,
    storageKey: 'chat-sidebar-width'
  })

  // Create demo message handler
  const demoHandler = useMemo(() => createDemoMessageHandler(), [])

  useEffect(() => {
    const demo = isDemoMode()
    setDemoMode(demo)

    if (demo) {
      // Demo mode: initialize with mock data
      setMessages(getMockMessages())
      setActiveUsers(getMockUsers())
      setStats(getMockStats())
      setIsConnected(true)
      setIsAuthenticated(true)
      setShowAuth(false)
      setUsername('DemoUser')
      return () => {
        // No cleanup needed for demo mode
      }
    }

    // Real mode: connect to WebSocket
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

      // Check for admin permission denial (means user is not admin)
      if (content.includes('Permission denied') && content.includes('admin')) {
        setIsAdmin(false)
      }

      // Check for successful user removal (means user is admin)
      if (content.includes('removed successfully')) {
        setIsAdmin(true)
        // Refresh user list after removal
        setTimeout(() => wsClient.send('/list'), 100)
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
    if (!text.trim()) return

    // Demo mode: use demo handler
    if (demoMode) {
      if (text.startsWith('/')) {
        // Handle commands in demo mode
        handleCommand(text)
        return
      }
      demoHandler.handleMessage(text, username, activeUsers, setMessages, null)
      return
    }

    // Real mode: send to WebSocket
    if (!isConnected) return

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

  // Open or focus a private chat window
  const handleOpenPrivateChat = (targetUser: string) => {
    setIsMobileSidebarOpen(false)

    setPrivateConversations(prev => {
      const newMap = new Map(prev)
      if (newMap.has(targetUser)) {
        // Window exists, bring to front
        const conv = newMap.get(targetUser)!
        newMap.set(targetUser, { ...conv, zIndex: highestZIndex + 1 })
        setHighestZIndex(z => z + 1)
      } else {
        // Create new window with offset position based on number of open windows
        const offset = newMap.size * 30
        newMap.set(targetUser, {
          id: targetUser,
          messages: [],
          position: { x: 100 + offset, y: 100 + offset },
          zIndex: highestZIndex + 1
        })
        setHighestZIndex(z => z + 1)
      }
      return newMap
    })
  }

  // Close a private chat window
  const handleClosePrivateChat = (targetUser: string) => {
    setPrivateConversations(prev => {
      const newMap = new Map(prev)
      newMap.delete(targetUser)
      return newMap
    })
  }

  // Bring a window to front
  const handleFocusPrivateChat = (targetUser: string) => {
    setPrivateConversations(prev => {
      const newMap = new Map(prev)
      const conv = newMap.get(targetUser)
      if (conv) {
        newMap.set(targetUser, { ...conv, zIndex: highestZIndex + 1 })
        setHighestZIndex(z => z + 1)
      }
      return newMap
    })
  }

  // Update window position
  const handleUpdatePrivateChatPosition = (targetUser: string, position: { x: number; y: number }) => {
    setPrivateConversations(prev => {
      const newMap = new Map(prev)
      const conv = newMap.get(targetUser)
      if (conv) {
        newMap.set(targetUser, { ...conv, position })
      }
      return newMap
    })
  }

  // Send a private message
  const handleSendPrivateMessage = (targetUser: string, message: string) => {
    if (!message.trim()) return

    const userMessage: Message = {
      type: 'message',
      content: message,
      timestamp: new Date(),
      sender: username || 'You',
      isPrivate: true,
      privateWith: targetUser
    }

    // Add message to conversation
    setPrivateConversations(prev => {
      const newMap = new Map(prev)
      const conv = newMap.get(targetUser)
      if (conv) {
        newMap.set(targetUser, {
          ...conv,
          messages: [...conv.messages, userMessage]
        })
      }
      return newMap
    })

    if (demoMode) {
      // Simulate a response in demo mode
      demoHandler.handlePrivateMessage(targetUser, message, username, (responseMsg: Message) => {
        setPrivateConversations(prev => {
          const newMap = new Map(prev)
          const conv = newMap.get(targetUser)
          if (conv) {
            newMap.set(targetUser, {
              ...conv,
              messages: [...conv.messages, responseMsg]
            })
          }
          return newMap
        })
      })
      return
    }

    if (!isConnected) return

    const privateCommand = `/msg ${targetUser} ${message}`
    wsClient.send(privateCommand)
  }

  const handleRemoveUser = (targetUser: string) => {
    if (demoMode) {
      demoHandler.handleRemoveUser(targetUser, setActiveUsers, setMessages)
      return
    }
    wsClient.send(`/removeuser ${targetUser}`)
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
    if (demoMode) {
      demoHandler.handleCommand(command, activeUsers, stats, username, setMessages)
      return
    }

    // Real mode: send to WebSocket
    if (command === '/list') {
      wsClient.send('/list')
    } else if (command === '/stats') {
      wsClient.send('/stats')
    }
  }

  const toggleMobileSidebar = () => {
    setIsMobileSidebarOpen(prev => !prev)
  }

  const closeMobileSidebar = () => {
    setIsMobileSidebarOpen(false)
  }

  return (
    <div className="flex h-screen w-screen bg-bloomberg-bg">
      {/* Main Chat Area */}
      <div className="flex-1 flex flex-col min-w-0">
        {/* Header */}
        <div className="bg-bloomberg-bg-secondary bloomberg-border-b px-4 py-2 flex items-center justify-between">
          <div className="flex items-center gap-4">
            <h1 className="text-bloomberg-accent text-lg font-bold bloomberg-text-shadow">
              CHAT TERMINAL
            </h1>
            <div className="flex items-center gap-2">
              <div className={`w-2 h-2 rounded-full ${isConnected ? 'bg-bloomberg-success' : 'bg-bloomberg-error'}`} />
              <span className="text-xs text-bloomberg-text-dim">
                {demoMode ? 'DEMO MODE' : isConnected ? 'CONNECTED' : 'DISCONNECTED'}
              </span>
            </div>
          </div>
          <div className="flex items-center gap-4">
            {isAuthenticated && (
              <>
                {demoMode && (
                  <span className="text-xs px-2 py-1 bg-bloomberg-warning bg-opacity-20 text-bloomberg-warning rounded border border-bloomberg-warning">
                    DEMO
                  </span>
                )}
                <div className="text-sm text-bloomberg-text-dim hidden sm:block">
                  User: <span className="text-bloomberg-accent">{username}</span>
                </div>
              </>
            )}
            <MobileMenuButton onClick={toggleMobileSidebar} isOpen={isMobileSidebarOpen} />
          </div>
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
            disabled={demoMode ? false : (!isConnected || !isAuthenticated)}
            placeholder={demoMode ? 'Type message or command (demo mode)...' : (!isAuthenticated ? 'Please authenticate to chat...' : 'Type message or command...')}
          />
        </div>
      </div>

      {/* Desktop Sidebar */}
      <div className="hidden md:block h-full">
        <Sidebar
          activeUsers={activeUsers}
          stats={stats}
          currentUser={username}
          isAdmin={isAdmin}
          onRefreshUsers={() => handleCommand('/list')}
          onRefreshStats={() => handleCommand('/stats')}
          onPrivateMessage={handleOpenPrivateChat}
          onRemoveUser={handleRemoveUser}
          width={sidebarWidth}
          onResizeStart={onResizeStart}
          isResizing={isResizing}
        />
      </div>

      {/* Mobile Sidebar Overlay */}
      {isMobileSidebarOpen && (
        <div
          className="fixed inset-0 bg-black bg-opacity-50 z-40 md:hidden"
          onClick={closeMobileSidebar}
        />
      )}

      {/* Mobile Sidebar Drawer */}
      <div
        className={`fixed top-0 right-0 h-full w-80 max-w-[85vw] z-50 transform transition-transform duration-300 ease-in-out md:hidden ${
          isMobileSidebarOpen ? 'translate-x-0' : 'translate-x-full'
        }`}
      >
        <Sidebar
          activeUsers={activeUsers}
          stats={stats}
          currentUser={username}
          isAdmin={isAdmin}
          onRefreshUsers={() => handleCommand('/list')}
          onRefreshStats={() => handleCommand('/stats')}
          onPrivateMessage={handleOpenPrivateChat}
          onRemoveUser={handleRemoveUser}
          isMobile={true}
          onClose={closeMobileSidebar}
        />
      </div>

      {/* Private Chat Windows */}
      {Array.from(privateConversations.values()).map((conversation) => (
        <PrivateChatWindow
          key={conversation.id}
          targetUser={conversation.id}
          messages={conversation.messages}
          position={conversation.position}
          zIndex={conversation.zIndex}
          onSend={(msg) => handleSendPrivateMessage(conversation.id, msg)}
          onClose={() => handleClosePrivateChat(conversation.id)}
          onFocus={() => handleFocusPrivateChat(conversation.id)}
          onPositionChange={(pos) => handleUpdatePrivateChatPosition(conversation.id, pos)}
        />
      ))}

      {/* Auth Modal - Don't show in demo mode */}
      {showAuth && !demoMode && (
        <AuthModal
          onAuthSuccess={handleAuthSuccess}
          isConnected={isConnected}
        />
      )}
    </div>
  )
}
