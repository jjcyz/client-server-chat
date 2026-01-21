'use client'

import { useState, useEffect, useRef, KeyboardEvent } from 'react'
import { Message } from '@/lib/websocket'
import { useDraggable } from '@/hooks/useDraggable'

interface PrivateChatWindowProps {
  targetUser: string
  messages: Message[]
  position: { x: number; y: number }
  zIndex: number
  onSend: (message: string) => void
  onClose: () => void
  onFocus: () => void
  onPositionChange: (position: { x: number; y: number }) => void
}

export default function PrivateChatWindow({
  targetUser,
  messages,
  position: initialPosition,
  zIndex,
  onSend,
  onClose,
  onFocus,
  onPositionChange
}: PrivateChatWindowProps) {
  const [message, setMessage] = useState('')
  const inputRef = useRef<HTMLInputElement>(null)
  const messagesEndRef = useRef<HTMLDivElement>(null)
  const lastClickRef = useRef<number>(0)

  const { position, isDragging, onDragStart, setPosition } = useDraggable({
    initialPosition,
    onPositionChange
  })

  // Sync position when initialPosition changes (e.g., from external state)
  useEffect(() => {
    setPosition(initialPosition)
  }, [initialPosition, setPosition])

  // Scroll to bottom when new messages arrive
  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' })
  }, [messages])

  // Focus input when window opens
  useEffect(() => {
    inputRef.current?.focus()
  }, [])

  // Handle ESC key
  useEffect(() => {
    const handleKeyDown = (e: globalThis.KeyboardEvent) => {
      if (e.key === 'Escape') {
        onClose()
      }
    }

    document.addEventListener('keydown', handleKeyDown)
    return () => document.removeEventListener('keydown', handleKeyDown)
  }, [onClose])

  const handleSubmit = () => {
    if (!message.trim()) return
    onSend(message.trim())
    setMessage('')
  }

  const handleKeyDown = (e: KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault()
      handleSubmit()
    }
  }

  const handleHeaderClick = () => {
    onFocus()
  }

  const handleHeaderDoubleClick = () => {
    const now = Date.now()
    if (now - lastClickRef.current < 300) {
      onClose()
    }
    lastClickRef.current = now
  }

  const handleMouseDown = (e: React.MouseEvent) => {
    onFocus()
    onDragStart(e)
  }

  const formatTime = (date: Date) => {
    return date.toLocaleTimeString('en-US', {
      hour12: false,
      hour: '2-digit',
      minute: '2-digit'
    })
  }

  return (
    <div
      className="fixed bg-bloomberg-bg-secondary border bloomberg-border rounded-lg shadow-lg overflow-hidden"
      style={{
        left: `${position.x}px`,
        top: `${position.y}px`,
        width: '320px',
        height: '400px',
        zIndex,
        cursor: isDragging ? 'grabbing' : 'default'
      }}
      onClick={onFocus}
    >
      {/* Header - draggable, double-click to close */}
      <div
        className="flex items-center justify-between px-2 py-1.5 bloomberg-border-b bg-bloomberg-bg-tertiary cursor-grab select-none"
        onMouseDown={handleMouseDown}
        onDoubleClick={handleHeaderDoubleClick}
        onClick={handleHeaderClick}
      >
        <div className="flex items-center gap-1">
          <span className="text-bloomberg-accent text-xs font-bold">@</span>
          <span className="text-bloomberg-text text-xs font-mono">{targetUser}</span>
        </div>
        <button
          onClick={(e) => {
            e.stopPropagation()
            onClose()
          }}
          className="flex items-center justify-center text-bloomberg-text-dim hover:text-bloomberg-error transition-colors p-0.5"
          aria-label="Close"
        >
          <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
          </svg>
        </button>
      </div>

      {/* Messages area */}
      <div className="flex-1 overflow-y-auto p-3 scrollbar-thin" style={{ height: 'calc(100% - 120px)' }}>
        {messages.length === 0 ? (
          <div className="text-xs text-bloomberg-text-dim text-center py-4">
            Start a conversation with {targetUser}
          </div>
        ) : (
          <div className="space-y-2">
            {messages.map((msg, index) => {
              const isOwn = msg.sender !== targetUser
              return (
                <div
                  key={index}
                  className={`text-xs font-mono ${isOwn ? 'text-right' : 'text-left'}`}
                >
                  <div
                    className={`inline-block px-2 py-1 rounded max-w-[85%] ${
                      isOwn
                        ? 'bg-bloomberg-accent bg-opacity-20 text-bloomberg-accent'
                        : 'bg-bloomberg-bg-tertiary text-bloomberg-text'
                    }`}
                  >
                    <div className="break-words">{msg.content}</div>
                    <div className="text-[10px] text-bloomberg-text-dim mt-0.5">
                      {formatTime(msg.timestamp)}
                    </div>
                  </div>
                </div>
              )
            })}
            <div ref={messagesEndRef} />
          </div>
        )}
      </div>

      {/* Input area */}
      <div className="p-3 bloomberg-border-t bg-bloomberg-bg-tertiary">
        <div className="flex items-center gap-2">
          <input
            ref={inputRef}
            type="text"
            value={message}
            onChange={(e) => setMessage(e.target.value)}
            onKeyDown={handleKeyDown}
            placeholder={`Message ${targetUser}...`}
            className="flex-1 bg-bloomberg-bg border bloomberg-border rounded px-3 py-2 text-bloomberg-text font-mono text-xs focus:outline-none focus:ring-2 focus:ring-bloomberg-accent focus:border-transparent"
          />
          <button
            onClick={handleSubmit}
            disabled={!message.trim()}
            className="px-3 py-2 bg-bloomberg-accent text-bloomberg-bg rounded font-mono text-xs hover:bg-opacity-90 disabled:opacity-50 disabled:cursor-not-allowed transition-colors"
          >
            SEND
          </button>
        </div>
        <div className="mt-1 text-[10px] text-bloomberg-text-dim">
          Double-click header to close | ESC to close
        </div>
      </div>
    </div>
  )
}
