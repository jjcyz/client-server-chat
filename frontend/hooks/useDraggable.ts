'use client'

import { useState, useEffect, useCallback, useRef } from 'react'

interface Position {
  x: number
  y: number
}

interface UseDraggableOptions {
  initialPosition?: Position
  onPositionChange?: (position: Position) => void
}

interface UseDraggableReturn {
  position: Position
  isDragging: boolean
  onDragStart: (e: React.MouseEvent) => void
  setPosition: (position: Position) => void
}

export function useDraggable({
  initialPosition = { x: 100, y: 100 },
  onPositionChange
}: UseDraggableOptions = {}): UseDraggableReturn {
  const [position, setPosition] = useState<Position>(initialPosition)
  const [isDragging, setIsDragging] = useState(false)
  const startMouseRef = useRef<Position>({ x: 0, y: 0 })
  const startPositionRef = useRef<Position>({ x: 0, y: 0 })

  const handleMouseMove = useCallback((e: MouseEvent) => {
    const deltaX = e.clientX - startMouseRef.current.x
    const deltaY = e.clientY - startMouseRef.current.y

    const newPosition = {
      x: Math.max(0, startPositionRef.current.x + deltaX),
      y: Math.max(0, startPositionRef.current.y + deltaY)
    }

    setPosition(newPosition)
    onPositionChange?.(newPosition)
  }, [onPositionChange])

  const handleMouseUp = useCallback(() => {
    setIsDragging(false)
    document.body.style.cursor = ''
    document.body.style.userSelect = ''
  }, [])

  useEffect(() => {
    if (isDragging) {
      document.addEventListener('mousemove', handleMouseMove)
      document.addEventListener('mouseup', handleMouseUp)
      document.body.style.cursor = 'grabbing'
      document.body.style.userSelect = 'none'
    }

    return () => {
      document.removeEventListener('mousemove', handleMouseMove)
      document.removeEventListener('mouseup', handleMouseUp)
    }
  }, [isDragging, handleMouseMove, handleMouseUp])

  const onDragStart = useCallback((e: React.MouseEvent) => {
    e.preventDefault()
    startMouseRef.current = { x: e.clientX, y: e.clientY }
    startPositionRef.current = position
    setIsDragging(true)
  }, [position])

  return { position, isDragging, onDragStart, setPosition }
}
