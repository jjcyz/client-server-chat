'use client'

import { useEffect, useRef } from 'react'
import { Message } from '@/lib/websocket'

interface MessageListProps {
  messages: Message[]
}

export default function MessageList({ messages }: MessageListProps) {
  const messagesEndRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' })
  }, [messages])

  const formatTime = (date: Date) => {
    return date.toLocaleTimeString('en-US', { 
      hour12: false, 
      hour: '2-digit', 
      minute: '2-digit', 
      second: '2-digit' 
    })
  }

  const formatMessage = (content: string) => {
    // Parse timestamp from server messages [HH:MM:SS]
    const timestampMatch = content.match(/^\[(\d{2}:\d{2}:\d{2})\]\s*(.*)$/)
    if (timestampMatch) {
      return {
        timestamp: timestampMatch[1],
        content: timestampMatch[2],
      }
    }
    return { timestamp: null, content }
  }

  return (
    <div className="h-full overflow-y-auto scrollbar-thin p-4 space-y-1">
      {messages.length === 0 && (
        <div className="text-center text-bloomberg-text-dim mt-8">
          <div className="text-sm">No messages yet. Start chatting!</div>
        </div>
      )}
      {messages.map((message, index) => {
        const { timestamp, content } = formatMessage(message.content)
        const isError = message.type === 'error' || content.toLowerCase().includes('error')
        const isSystem = message.type === 'system' || 
          content.includes('Login') || 
          content.includes('Registration') ||
          content.includes('Welcome') ||
          content.includes('Active users') ||
          content.includes('Server Statistics')

        return (
          <div
            key={index}
            className={`text-sm font-mono ${
              isError
                ? 'text-bloomberg-error'
                : isSystem
                ? 'text-bloomberg-warning'
                : 'text-bloomberg-text'
            }`}
          >
            <span className="text-bloomberg-text-dim">
              {timestamp || formatTime(message.timestamp)}
            </span>
            {' '}
            <span className={message.sender ? 'text-bloomberg-accent' : ''}>
              {message.sender ? `${message.sender}: ` : ''}
            </span>
            <span className={isSystem ? 'italic' : ''}>{content}</span>
          </div>
        )
      })}
      <div ref={messagesEndRef} />
    </div>
  )
}
