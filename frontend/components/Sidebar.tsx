'use client'

import { useState } from 'react'

interface SidebarProps {
  activeUsers: string[]
  stats: any
  currentUser: string
  isAdmin: boolean
  onRefreshUsers: () => void
  onRefreshStats: () => void
  onPrivateMessage: (targetUser: string) => void
  onRemoveUser: (targetUser: string) => void
}

export default function Sidebar({
  activeUsers,
  stats,
  currentUser,
  isAdmin,
  onRefreshUsers,
  onRefreshStats,
  onPrivateMessage,
  onRemoveUser
}: SidebarProps) {
  const [confirmRemove, setConfirmRemove] = useState<string | null>(null)

  const handleRemoveClick = (user: string) => {
    if (confirmRemove === user) {
      onRemoveUser(user)
      setConfirmRemove(null)
    } else {
      setConfirmRemove(user)
      // Auto-clear confirmation after 3 seconds
      setTimeout(() => setConfirmRemove(null), 3000)
    }
  }

  return (
    <div className="w-80 bg-bloomberg-bg-secondary bloomberg-border-l flex flex-col">
      {/* Stats Panel */}
      <div className="bloomberg-border-b p-4">
        <div className="flex items-center justify-between mb-3">
          <h2 className="text-bloomberg-accent text-sm font-bold uppercase">Server Stats</h2>
          <button
            onClick={onRefreshStats}
            className="text-xs text-bloomberg-text-dim hover:text-bloomberg-accent transition-colors"
          >
            REFRESH
          </button>
        </div>
        {stats ? (
          <div className="space-y-2 text-xs font-mono">
            <div className="flex justify-between">
              <span className="text-bloomberg-text-dim">Messages:</span>
              <span className="text-bloomberg-success">{stats.totalMessages || 'N/A'}</span>
            </div>
            <div className="flex justify-between">
              <span className="text-bloomberg-text-dim">Connections:</span>
              <span className="text-bloomberg-success">{stats.activeConnections || 'N/A'}</span>
            </div>
            {stats.uptime && (
              <div className="flex justify-between">
                <span className="text-bloomberg-text-dim">Uptime:</span>
                <span className="text-bloomberg-text">{stats.uptime}</span>
              </div>
            )}
          </div>
        ) : (
          <div className="text-xs text-bloomberg-text-dim">No stats available</div>
        )}
      </div>

      {/* Active Users Panel */}
      <div className="flex-1 overflow-hidden flex flex-col">
        <div className="p-4 bloomberg-border-b">
          <div className="flex items-center justify-between">
            <h2 className="text-bloomberg-accent text-sm font-bold uppercase">
              Active Users ({activeUsers.length})
            </h2>
            <button
              onClick={onRefreshUsers}
              className="text-xs text-bloomberg-text-dim hover:text-bloomberg-accent transition-colors"
            >
              REFRESH
            </button>
          </div>
        </div>
        <div className="flex-1 overflow-y-auto scrollbar-thin p-4">
          {activeUsers.length === 0 ? (
            <div className="text-xs text-bloomberg-text-dim">No active users</div>
          ) : (
            <div className="space-y-1">
              {activeUsers.map((user, index) => {
                const isCurrentUser = user === currentUser
                return (
                  <div
                    key={index}
                    className="flex items-center justify-between group py-1"
                  >
                    <div
                      onClick={() => !isCurrentUser && onPrivateMessage(user)}
                      className={`flex-1 text-xs font-mono transition-colors ${
                        isCurrentUser
                          ? 'text-bloomberg-accent cursor-default'
                          : 'text-bloomberg-text hover:text-bloomberg-accent cursor-pointer'
                      }`}
                      title={isCurrentUser ? 'You' : `Click to send private message to ${user}`}
                    >
                      <span className="text-bloomberg-success">●</span> {user}
                      {isCurrentUser && <span className="text-bloomberg-text-dim ml-1">(you)</span>}
                    </div>
                    {!isCurrentUser && (
                      <div className="flex items-center gap-1 opacity-0 group-hover:opacity-100 transition-opacity">
                        <button
                          onClick={() => onPrivateMessage(user)}
                          className="text-xs px-2 py-0.5 text-bloomberg-text-dim hover:text-bloomberg-accent hover:bg-bloomberg-bg-tertiary rounded transition-colors"
                          title="Send private message"
                        >
                          MSG
                        </button>
                        <button
                          onClick={() => handleRemoveClick(user)}
                          className={`text-xs px-2 py-0.5 rounded transition-colors ${
                            confirmRemove === user
                              ? 'bg-bloomberg-error text-white'
                              : 'text-bloomberg-text-dim hover:text-bloomberg-error hover:bg-bloomberg-bg-tertiary'
                          }`}
                          title={confirmRemove === user ? 'Click again to confirm removal' : 'Remove user (admin only)'}
                        >
                          {confirmRemove === user ? 'CONFIRM' : 'DEL'}
                        </button>
                      </div>
                    )}
                  </div>
                )
              })}
            </div>
          )}
        </div>
        <div className="p-2 bloomberg-border-t">
          <div className="text-xs text-bloomberg-text-dim text-center">
            {isAdmin ? (
              <span className="text-bloomberg-accent">Admin Mode</span>
            ) : (
              <span>Click user to message | DEL for admins</span>
            )}
          </div>
        </div>
      </div>
    </div>
  )
}
