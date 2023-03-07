import Head from 'next/head'
import Link from "next/link";
import { Inter } from 'next/font/google'
import { useEffect, useState } from "react";
import styles from '@/styles/Home.module.css'
import ApmConfigurations from "@/components/apm-configurations";

const inter = Inter({ subsets: ['latin'] })

export default function Test() {

  const [serverData, setServerData] = useState({});
  const [agentData,  setAgentData]  = useState({});

  useEffect(() => {
    fetch('http://localhost:8000/')
        .then((res) => res.text())
        .then((data) => console.log(data))
        .catch((err) => console.log(err))
    fetch('http://localhost:8000/exception/')
        .then((res) => res.text())
        .then((data) => console.log(data))
        .catch((err) => console.log(err))
  }, [])

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
          <ApmConfigurations server={serverData} agent={agentData}></ApmConfigurations>
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
          <Link className={styles.description} href="/test/ssr" passHref legacyBehavior>
            <a className={styles.card} rel="noopener noreferrer">
              <h2 className={inter.className}>
                Test SSR <span>-&gt;</span>
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
