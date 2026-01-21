'use client'

import { useState, useEffect, useCallback, useRef } from 'react'

interface UseResizableOptions {
  minWidth?: number
  maxWidth?: number
  defaultWidth?: number
  storageKey?: string
}

interface UseResizableReturn {
  width: number
  isResizing: boolean
  onResizeStart: (e: React.MouseEvent) => void
}

export function useResizable({
  minWidth = 200,
  maxWidth = 500,
  defaultWidth = 320,
  storageKey = 'sidebar-width'
}: UseResizableOptions = {}): UseResizableReturn {
  const [width, setWidth] = useState(defaultWidth)
  const [isResizing, setIsResizing] = useState(false)
  const startXRef = useRef(0)
  const startWidthRef = useRef(0)

  // SSR-safe initialization from localStorage
  useEffect(() => {
    if (typeof window !== 'undefined') {
      const stored = localStorage.getItem(storageKey)
      if (stored) {
        const parsed = parseInt(stored, 10)
        if (!isNaN(parsed) && parsed >= minWidth && parsed <= maxWidth) {
          setWidth(parsed)
        }
      }
    }
  }, [storageKey, minWidth, maxWidth])

  // Persist width to localStorage
  useEffect(() => {
    if (typeof window !== 'undefined' && !isResizing) {
      localStorage.setItem(storageKey, String(width))
    }
  }, [width, isResizing, storageKey])

  const handleMouseMove = useCallback((e: MouseEvent) => {
    const delta = startXRef.current - e.clientX
    const newWidth = Math.min(maxWidth, Math.max(minWidth, startWidthRef.current + delta))
    setWidth(newWidth)
  }, [minWidth, maxWidth])

  const handleMouseUp = useCallback(() => {
    setIsResizing(false)
    document.body.style.cursor = ''
    document.body.style.userSelect = ''
  }, [])

  useEffect(() => {
    if (isResizing) {
      document.addEventListener('mousemove', handleMouseMove)
      document.addEventListener('mouseup', handleMouseUp)
      document.body.style.cursor = 'ew-resize'
      document.body.style.userSelect = 'none'
    }

    return () => {
      document.removeEventListener('mousemove', handleMouseMove)
      document.removeEventListener('mouseup', handleMouseUp)
    }
  }, [isResizing, handleMouseMove, handleMouseUp])

  const onResizeStart = useCallback((e: React.MouseEvent) => {
    e.preventDefault()
    startXRef.current = e.clientX
    startWidthRef.current = width
    setIsResizing(true)
  }, [width])

  return { width, isResizing, onResizeStart }
}
