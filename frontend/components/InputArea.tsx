'use client'

import { useState, KeyboardEvent, useRef, useEffect } from 'react'

interface InputAreaProps {
  onSend: (message: string) => void
  onCommand?: (command: string) => void
  disabled?: boolean
  placeholder?: string
}

export default function InputArea({
  onSend,
  onCommand,
  disabled,
  placeholder
}: InputAreaProps) {
  const [input, setInput] = useState('')
  const inputRef = useRef<HTMLInputElement>(null)

  useEffect(() => {
    if (!disabled) {
      inputRef.current?.focus()
    }
  }, [disabled])

  const handleSubmit = () => {
    if (!input.trim() || disabled) return

    const trimmed = input.trim()

    // Handle commands
    if (trimmed.startsWith('/')) {
      if (onCommand) {
        onCommand(trimmed)
      }
      onSend(trimmed)
    } else {
      onSend(trimmed)
    }

    setInput('')
  }

  const handleKeyDown = (e: KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault()
      handleSubmit()
    }
  }

  return (
    <div className="p-4">
      <div className="flex items-center gap-2">
        <span className="text-bloomberg-text-dim text-sm">$</span>
        <input
          ref={inputRef}
          type="text"
          value={input}
          onChange={(e) => setInput(e.target.value)}
          onKeyDown={handleKeyDown}
          disabled={disabled}
          placeholder={placeholder || 'Type message...'}
          className="flex-1 bg-bloomberg-bg-tertiary border bloomberg-border rounded px-3 py-2 text-bloomberg-text font-mono text-sm focus:outline-none focus:ring-2 focus:ring-bloomberg-accent focus:border-transparent disabled:opacity-50 disabled:cursor-not-allowed"
        />
        <button
          onClick={handleSubmit}
          disabled={disabled || !input.trim()}
          className="px-4 py-2 bg-bloomberg-accent text-bloomberg-bg rounded font-mono text-sm hover:bg-opacity-90 disabled:opacity-50 disabled:cursor-not-allowed transition-colors"
        >
          SEND
        </button>
      </div>
      <div className="mt-2 text-xs text-bloomberg-text-dim">
        Commands: /stats, /list, /msg &lt;user&gt; &lt;message&gt;
      </div>
    </div>
  )
}
