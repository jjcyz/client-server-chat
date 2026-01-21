import React from 'react'
import { Message } from './websocket'

/**
 * Demo mode detection
 * Can be explicitly enabled via NEXT_PUBLIC_DEMO_MODE environment variable
 * Otherwise activates when deployed to production without NEXT_PUBLIC_WS_URL configured
 */
export const isDemoMode = (): boolean => {
  // Check for explicit demo mode setting
  const explicitDemoMode = process.env.NEXT_PUBLIC_DEMO_MODE
  if (explicitDemoMode === 'true') return true
  if (explicitDemoMode === 'false') return false

  // Fallback to automatic detection
  if (typeof window === 'undefined') return false
  const isProduction = window.location.hostname !== 'localhost' && window.location.hostname !== '127.0.0.1'
  const hasWsUrl = !!process.env.NEXT_PUBLIC_WS_URL
  return isProduction && !hasWsUrl
}

/**
 * Mock messages for demo mode
 */
export const getMockMessages = (): Message[] => [
  {
    type: 'system',
    content: '[14:23:15] Welcome, DemoUser!',
    timestamp: new Date(Date.now() - 300000),
    sender: 'System'
  },
  {
    type: 'message',
    content: '[14:23:20] DemoUser: Hello everyone! This is a demo of the chat terminal.',
    timestamp: new Date(Date.now() - 280000),
    sender: 'DemoUser'
  },
  {
    type: 'message',
    content: '[14:23:45] Alice: Hi DemoUser! Nice to see you here.',
    timestamp: new Date(Date.now() - 250000),
    sender: 'Alice'
  },
  {
    type: 'message',
    content: '[14:24:10] Bob: This chat system looks great!',
    timestamp: new Date(Date.now() - 220000),
    sender: 'Bob'
  },
  {
    type: 'message',
    content: '[14:24:35] DemoUser: Thanks! It supports real-time messaging, private messages, and admin commands.',
    timestamp: new Date(Date.now() - 190000),
    sender: 'DemoUser'
  },
  {
    type: 'system',
    content: '[14:25:00] Active users:\nDemoUser\nAlice\nBob\nCharlie',
    timestamp: new Date(Date.now() - 160000),
    sender: 'System'
  }
]

/**
 * Mock active users for demo mode
 */
export const getMockUsers = (): string[] => ['DemoUser', 'Alice', 'Bob', 'Charlie']

/**
 * Mock server statistics for demo mode
 */
export const getMockStats = () => ({
  totalMessages: '247',
  activeConnections: '4',
  uptime: '2d 14h 23m'
})

/**
 * Format time for demo messages
 */
const formatTime = (date: Date): string => {
  return date.toLocaleTimeString('en-US', {
    hour12: false,
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit'
  })
}

/**
 * Demo mode message handler
 */
export interface DemoMessageHandler {
  handleMessage: (text: string, username: string, activeUsers: string[], setMessages: React.Dispatch<React.SetStateAction<Message[]>>, privateMessageTarget?: string | null) => void
  handleCommand: (command: string, activeUsers: string[], stats: any, username: string, setMessages: React.Dispatch<React.SetStateAction<Message[]>>) => void
  handleRemoveUser: (targetUser: string, setActiveUsers: React.Dispatch<React.SetStateAction<string[]>>, setMessages: React.Dispatch<React.SetStateAction<Message[]>>) => void
  handlePrivateMessage: (targetUser: string, message: string, username: string, onResponse: (msg: Message) => void) => void
}

export const createDemoMessageHandler = (): DemoMessageHandler => {
  const handleMessage = (
    text: string,
    username: string,
    activeUsers: string[],
    setMessages: React.Dispatch<React.SetStateAction<Message[]>>,
    privateMessageTarget?: string | null
  ) => {
    const now = new Date()
    const timeStr = formatTime(now)

    if (privateMessageTarget && !text.startsWith('/')) {
      // Private message in demo mode
      const userMessage: Message = {
        type: 'message',
        content: `[${timeStr}] (private to ${privateMessageTarget}) ${text}`,
        timestamp: now,
        sender: username || 'DemoUser',
      }
      setMessages(prev => [...prev, userMessage])

      // Simulate a response after a delay
      setTimeout(() => {
        const response: Message = {
          type: 'message',
          content: `[${formatTime(new Date())}] (private from ${privateMessageTarget}) Got it!`,
          timestamp: new Date(),
          sender: privateMessageTarget,
        }
        setMessages(prev => [...prev, response])
      }, 1000)
      return
    }

    if (text.startsWith('/')) {
      // Commands are handled separately
      return
    }

    // Regular message in demo mode
    const userMessage: Message = {
      type: 'message',
      content: `[${timeStr}] ${text}`,
      timestamp: now,
      sender: username || 'DemoUser',
    }
    setMessages(prev => [...prev, userMessage])

    // Simulate a response from another user after a delay
    setTimeout(() => {
      const responses = [
        'That\'s interesting!',
        'I see what you mean.',
        'Thanks for sharing!',
        'Great point!',
        'Agreed!'
      ]
      const randomResponse = responses[Math.floor(Math.random() * responses.length)]
      const otherUsers = activeUsers.filter(u => u !== username)
      const randomUser = otherUsers[Math.floor(Math.random() * otherUsers.length)]

      if (randomUser) {
        const response: Message = {
          type: 'message',
          content: `[${formatTime(new Date())}] ${randomUser}: ${randomResponse}`,
          timestamp: new Date(),
          sender: randomUser,
        }
        setMessages(prev => [...prev, response])
      }
    }, 1500)
  }

  const handleCommand = (
    command: string,
    activeUsers: string[],
    stats: any,
    username: string,
    setMessages: React.Dispatch<React.SetStateAction<Message[]>>
  ) => {
    const now = new Date()
    const timeStr = formatTime(now)

    if (command === '/list') {
      const response: Message = {
        type: 'system',
        content: `[${timeStr}] Active users:\n${activeUsers.join('\n')}`,
        timestamp: now,
        sender: 'System'
      }
      setMessages(prev => [...prev, response])
    } else if (command === '/stats') {
      const response: Message = {
        type: 'system',
        content: `[${timeStr}] Server Statistics:\nTotal Messages: ${stats?.totalMessages || '247'}\nCurrent Connections: ${stats?.activeConnections || '4'}\nUptime: ${stats?.uptime || '2d 14h 23m'}`,
        timestamp: now,
        sender: 'System'
      }
      setMessages(prev => [...prev, response])
    } else if (command.startsWith('/msg ')) {
      // Private message command
      const parts = command.split(' ')
      if (parts.length >= 3) {
        const target = parts[1]
        const message = parts.slice(2).join(' ')
        const userMessage: Message = {
          type: 'message',
          content: `[${timeStr}] (private to ${target}) ${message}`,
          timestamp: now,
          sender: username || 'DemoUser',
        }
        setMessages(prev => [...prev, userMessage])
      }
    } else {
      // Unknown command
      const response: Message = {
        type: 'error',
        content: `[${timeStr}] Unknown command: ${command}`,
        timestamp: now,
        sender: 'System'
      }
      setMessages(prev => [...prev, response])
    }
  }

  const handleRemoveUser = (
    targetUser: string,
    setActiveUsers: React.Dispatch<React.SetStateAction<string[]>>,
    setMessages: React.Dispatch<React.SetStateAction<Message[]>>
  ) => {
    // Demo mode: simulate user removal
    setActiveUsers(prev => prev.filter(u => u !== targetUser))
    const now = new Date()
    const timeStr = formatTime(now)
    const response: Message = {
      type: 'system',
      content: `[${timeStr}] User ${targetUser} removed successfully`,
      timestamp: now,
      sender: 'System'
    }
    setMessages(prev => [...prev, response])
  }

  const handlePrivateMessage = (
    targetUser: string,
    _message: string,
    username: string,
    onResponse: (msg: Message) => void
  ) => {
    // Simulate a response after a delay
    const responses = [
      'Got it!',
      'Thanks for the message!',
      'I understand.',
      'Okay, sounds good!',
      'Sure thing!'
    ]
    const randomResponse = responses[Math.floor(Math.random() * responses.length)]

    setTimeout(() => {
      const response: Message = {
        type: 'message',
        content: randomResponse,
        timestamp: new Date(),
        sender: targetUser,
        isPrivate: true,
        privateWith: username
      }
      onResponse(response)
    }, 800 + Math.random() * 700)
  }

  return {
    handleMessage,
    handleCommand,
    handleRemoveUser,
    handlePrivateMessage
  }
}
