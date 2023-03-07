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
            <Link href="/">
              GO TO HOME PAGE
            </Link>
            <Link href="/test/csr">
              GO TO CSR PAGE
            </Link>
            <Link href="/test/ssr">
              GO TO SSR PAGE
            </Link>
        </div>
      </main>
    </>
  )
}
