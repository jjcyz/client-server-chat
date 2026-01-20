'use client'

import { useState, useEffect, useRef } from 'react'
import ChatTerminal from '@/components/ChatTerminal'

export default function Home() {
  return (
    <main className="h-screen w-screen overflow-hidden">
      <ChatTerminal />
    </main>
  )
}
