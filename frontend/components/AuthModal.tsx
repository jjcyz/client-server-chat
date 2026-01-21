'use client'

import { useState } from 'react'
import { wsClient } from '@/lib/websocket'

interface AuthModalProps {
  onAuthSuccess: (username: string) => void
  isConnected: boolean
}

export default function AuthModal({ onAuthSuccess, isConnected }: AuthModalProps) {
  const [mode, setMode] = useState<'login' | 'register'>('login')
  const [username, setUsername] = useState('')
  const [password, setPassword] = useState('')
  const [error, setError] = useState('')
  const [loading, setLoading] = useState(false)

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault()
    if (!isConnected) {
      setError('Not connected to server')
      return
    }
    if (!username.trim() || !password.trim()) {
      setError('Username and password are required')
      return
    }

    setLoading(true)
    setError('')

    const command = mode === 'login'
      ? `/login ${username.trim()} ${password.trim()}`
      : `/register ${username.trim()} ${password.trim()}`

    // Set up a one-time message handler to check for auth success
    const unsubscribe = wsClient.onMessage((message) => {
      const content = message.content.trim()

      if (content.includes('Login successful!') || content.includes('Registration successful!')) {
        unsubscribe()
        setLoading(false)
        onAuthSuccess(username.trim())
      } else if (content.toLowerCase().includes('error') ||
                 content.toLowerCase().includes('invalid') ||
                 content.toLowerCase().includes('failed')) {
        unsubscribe()
        setLoading(false)
        setError(content)
      }
    })

    wsClient.send(command)

    // Timeout after 5 seconds
    setTimeout(() => {
      unsubscribe()
      if (loading) {
        setLoading(false)
        setError('Authentication timeout. Please try again.')
      }
    }, 5000)
  }

  return (
    <div className="fixed inset-0 bg-black bg-opacity-75 flex items-center justify-center z-50">
      <div className="bg-bloomberg-bg-secondary bloomberg-border rounded-lg p-8 w-full max-w-md">
        <div className="mb-6">
          <h2 className="text-2xl font-bold text-bloomberg-accent mb-2 bloomberg-text-shadow">
            CHAT TERMINAL
          </h2>
          <p className="text-sm text-bloomberg-text-dim">
            {mode === 'login' ? 'Login to continue' : 'Create a new account'}
          </p>
        </div>

        {!isConnected && (
          <div className="mb-4 p-3 bg-bloomberg-error bg-opacity-20 border border-bloomberg-error rounded text-sm text-bloomberg-error">
            {typeof window !== 'undefined' &&
             window.location.hostname !== 'localhost' &&
             window.location.hostname !== '127.0.0.1'
              ? 'Backend server not configured. Please contact the administrator.'
              : 'Not connected to server. Please wait...'}
          </div>
        )}

        {error && (
          <div className="mb-4 p-3 bg-bloomberg-error bg-opacity-20 border border-bloomberg-error rounded text-sm text-bloomberg-error">
            {error}
          </div>
        )}

        <form onSubmit={handleSubmit} className="space-y-4">
          <div>
            <label className="block text-sm text-bloomberg-text-dim mb-2 font-mono">
              USERNAME
            </label>
            <input
              type="text"
              value={username}
              onChange={(e) => setUsername(e.target.value)}
              disabled={loading || !isConnected}
              className="w-full bg-bloomberg-bg-tertiary border bloomberg-border rounded px-3 py-2 text-bloomberg-text font-mono focus:outline-none focus:ring-2 focus:ring-bloomberg-accent focus:border-transparent disabled:opacity-50"
              autoFocus
            />
          </div>

          <div>
            <label className="block text-sm text-bloomberg-text-dim mb-2 font-mono">
              PASSWORD
            </label>
            <input
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              disabled={loading || !isConnected}
              className="w-full bg-bloomberg-bg-tertiary border bloomberg-border rounded px-3 py-2 text-bloomberg-text font-mono focus:outline-none focus:ring-2 focus:ring-bloomberg-accent focus:border-transparent disabled:opacity-50"
            />
          </div>

          <div className="flex gap-2">
            <button
              type="submit"
              disabled={loading || !isConnected}
              className="flex-1 bg-bloomberg-accent text-bloomberg-bg py-2 px-4 rounded font-mono text-sm hover:bg-opacity-90 disabled:opacity-50 disabled:cursor-not-allowed transition-colors"
            >
              {loading ? 'AUTHENTICATING...' : mode === 'login' ? 'LOGIN' : 'REGISTER'}
            </button>
          </div>

          <div className="text-center">
            <button
              type="button"
              onClick={() => {
                setMode(mode === 'login' ? 'register' : 'login')
                setError('')
              }}
              disabled={loading}
              className="text-sm text-bloomberg-text-dim hover:text-bloomberg-accent transition-colors disabled:opacity-50"
            >
              {mode === 'login'
                ? 'Need an account? Register'
                : 'Already have an account? Login'}
            </button>
          </div>
        </form>
      </div>
    </div>
  )
}
