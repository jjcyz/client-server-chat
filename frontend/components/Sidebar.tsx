'use client'

interface SidebarProps {
  activeUsers: string[]
  stats: any
  onRefreshUsers: () => void
  onRefreshStats: () => void
}

export default function Sidebar({ activeUsers, stats, onRefreshUsers, onRefreshStats }: SidebarProps) {
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
              {activeUsers.map((user, index) => (
                <div
                  key={index}
                  className="text-xs font-mono text-bloomberg-text hover:text-bloomberg-accent transition-colors cursor-pointer py-1"
                >
                  <span className="text-bloomberg-success">●</span> {user}
                </div>
              ))}
            </div>
          )}
        </div>
      </div>
    </div>
  )
}
