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
          <Link className={styles.description} href="/" passHref legacyBehavior>
            <a className={styles.card} rel="noopener noreferrer">
              <h2 className={inter.className}>
                <span>&lt;-</span> Home
              </h2>
              <p className={inter.className}>
                Go to home page
              </p>
            </a>
          </Link>
          <Link className={styles.description} href="/test/csr" passHref legacyBehavior>
            <a className={styles.card} rel="noopener noreferrer">
              <h2 className={inter.className}>
                <span>&lt;-</span> Test CSR
              </h2>
              <p className={inter.className}>
                Go to SSR test page
              </p>
            </a>
          </Link>
          <Link className={styles.description} href="/about" passHref legacyBehavior>
            <a className={styles.card} rel="noopener noreferrer">
              <h2 className={inter.className}>
                About <span>-&gt;</span>
              </h2>
              <p className={inter.className}>
                Go to about page
              </p>
            </a>
          </Link>
        </div>
      </main>
    </>
  )
}
