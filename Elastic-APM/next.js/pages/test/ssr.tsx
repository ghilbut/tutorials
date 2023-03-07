import Head from 'next/head'
import Link from "next/link";
import { Inter } from 'next/font/google'
import styles from '@/styles/Home.module.css'

import ApmConfigurations from "@/components/apm-configurations";

const inter = Inter({ subsets: ['latin'] })

export async function getServerSideProps() {
  const serverRes = await fetch('http://localhost:8200/')
  const server = await serverRes.json()
  const agentRes = await fetch('http://localhost:8200/config/v1/agents?service.name=tutorial-nextjs-ssr')
  const agent = await agentRes.json()
  let django = await fetch('http://localhost:8000/')
  let txt = django.text()
  django = await fetch('http://localhost:8000/exception/')
  txt = django.text()

  return { props: { server, agent } }
}

export default function Test({ server, agent }: { server: any, agent: any }) {
  return (
    <>
      <Head>
        <title>Test</title>
        <meta name="description" content="test page" />
        <meta name="viewport" content="width=device-width, initial-scale=1" />
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <main className={styles.main}>
        <div className={styles.description}>
        </div>

        <div className={styles.center}>
          <ApmConfigurations server={server} agent={agent}></ApmConfigurations>
        </div>

        <div className={styles.grid}>
          <Link href="/">
            GO TO HOME PAGE
          </Link>
          <Link href="/test/csr">
            GO TO CSR PAGE
          </Link>
          <Link href="/about">
            GO TO ABOUT PAGE
          </Link>
        </div>
      </main>
    </>
  )
}
