/** @type {import('tailwindcss').Config} */
module.exports = {
  content: [
    './pages/**/*.{js,ts,jsx,tsx,mdx}',
    './components/**/*.{js,ts,jsx,tsx,mdx}',
    './app/**/*.{js,ts,jsx,tsx,mdx}',
  ],
  theme: {
    extend: {
      colors: {
        'bloomberg': {
          'bg': '#0d1117',
          'bg-secondary': '#161b22',
          'bg-tertiary': '#21262d',
          'text': '#c9d1d9',
          'text-dim': '#8b949e',
          'accent': '#58a6ff',
          'success': '#3fb950',
          'warning': '#d29922',
          'error': '#f85149',
          'border': '#30363d',
        },
      },
      fontFamily: {
        'mono': ['ui-monospace', 'SFMono-Regular', 'Menlo', 'Monaco', 'Consolas', 'monospace'],
      },
    },
  },
  plugins: [],
}
