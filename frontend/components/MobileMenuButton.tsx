'use client'

interface MobileMenuButtonProps {
  onClick: () => void
  isOpen: boolean
}

export default function MobileMenuButton({ onClick, isOpen }: MobileMenuButtonProps) {
  return (
    <button
      onClick={onClick}
      className="md:hidden touch-target flex items-center justify-center p-2 text-bloomberg-text-dim hover:text-bloomberg-accent transition-colors"
      aria-label={isOpen ? 'Close menu' : 'Open menu'}
    >
      <svg
        className="w-6 h-6"
        fill="none"
        stroke="currentColor"
        viewBox="0 0 24 24"
      >
        {isOpen ? (
          <path
            strokeLinecap="round"
            strokeLinejoin="round"
            strokeWidth={2}
            d="M6 18L18 6M6 6l12 12"
          />
        ) : (
          <path
            strokeLinecap="round"
            strokeLinejoin="round"
            strokeWidth={2}
            d="M4 6h16M4 12h16M4 18h16"
          />
        )}
      </svg>
    </button>
  )
}
