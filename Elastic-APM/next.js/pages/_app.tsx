import '@/styles/globals.css'
import type { AppProps } from 'next/app'
import apm from '@/modules/apm'

// NOTE(ghilbut): Don't remove this. This is for activate apm module when react start
console.log(`Elastic-APM RUM\n    enabled: ${apm.isEnabled()}\n    activated: ${apm.isActive()}`)

export default function App({ Component, pageProps }: AppProps) {
  return <Component {...pageProps} />
}
