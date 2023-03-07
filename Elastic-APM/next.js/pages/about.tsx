import Head from 'next/head'
import Link from "next/link";
import { Inter } from 'next/font/google'
import styles from '@/styles/Home.module.css'

const inter = Inter({ subsets: ['latin'] })

export default function About() {
  return (
    <>
      <Head>
        <title>About</title>
        <meta name="description" content="about page" />
        <meta name="viewport" content="width=device-width, initial-scale=1" />
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <main className={styles.main}>
        <div className={styles.description}>
        </div>

        <div className={styles.center}>
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
                Go to CSR test page
              </p>
            </a>
          </Link>
          <Link className={styles.description} href="/test/ssr" passHref legacyBehavior>
            <a className={styles.card} rel="noopener noreferrer">
              <h2 className={inter.className}>
                <span>&lt;-</span> Test SSR
              </h2>
              <p className={inter.className}>
                Go to SSR test page
              </p>
            </a>
          </Link>
        </div>
      </main>
    </>
  )
}
