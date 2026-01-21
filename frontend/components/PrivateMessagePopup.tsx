'use client'

import { useState, useEffect, useRef, KeyboardEvent } from 'react'

interface PrivateMessagePopupProps {
  targetUser: string
  onSend: (message: string) => void
  onClose: () => void
}

export default function PrivateMessagePopup({
  targetUser,
  onSend,
  onClose
}: PrivateMessagePopupProps) {
  const [message, setMessage] = useState('')
  const inputRef = useRef<HTMLInputElement>(null)

  useEffect(() => {
    inputRef.current?.focus()
  }, [])

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

  return (
    <div className="fixed bottom-4 right-4 left-4 md:left-auto md:w-80 lg:w-96 bg-bloomberg-bg-secondary border bloomberg-border rounded-lg shadow-lg animate-slide-up z-50">
      {/* Header */}
      <div className="flex items-center justify-between px-4 py-3 bloomberg-border-b">
        <div className="flex items-center gap-2">
          <span className="text-bloomberg-accent text-sm font-bold">@</span>
          <span className="text-bloomberg-text text-sm font-mono">{targetUser}</span>
        </div>
        <button
          onClick={onClose}
          className="touch-target flex items-center justify-center text-bloomberg-text-dim hover:text-bloomberg-error transition-colors"
          aria-label="Close"
        >
          <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
          </svg>
        </button>
      </div>

      {/* Input */}
      <div className="p-4">
        <div className="flex items-center gap-2">
          <input
            ref={inputRef}
            type="text"
            value={message}
            onChange={(e) => setMessage(e.target.value)}
            onKeyDown={handleKeyDown}
            placeholder={`Private message to ${targetUser}...`}
            className="flex-1 bg-bloomberg-bg-tertiary border bloomberg-border rounded px-3 py-2 text-bloomberg-text font-mono text-sm focus:outline-none focus:ring-2 focus:ring-bloomberg-accent focus:border-transparent"
          />
          <button
            onClick={handleSubmit}
            disabled={!message.trim()}
            className="touch-target px-4 py-2 bg-bloomberg-accent text-bloomberg-bg rounded font-mono text-sm hover:bg-opacity-90 disabled:opacity-50 disabled:cursor-not-allowed transition-colors"
          >
            SEND
          </button>
        </div>
        <div className="mt-2 text-xs text-bloomberg-text-dim">
          Press ESC to close
        </div>
      </div>
    </div>
  )
}
