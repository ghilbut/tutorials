tRPC + Next.js 13 + Fastify
========

* tRPC
  * [Home](https://trpc.io/)
* Fastify
  * [Home](https://www.fastify.io/) 

---

# A. Initialize project

## A01. Create fastify server

**[NOTE]** [Fastify / Documentation / Typescript](https://www.fastify.io/docs/latest/Reference/TypeScript/)

### initialize environment

```shell
$ mkdir fastify && cd "$_"
$ yarn init -y
$ yarn add fastify
$ yarn add -D typescript @types/node ts-node
$ yarn tsc --init \
  --sourceMap true \
  --target es2017 \
  --rootDir src \
  --outDir  build \
  --strict true \
  --esModuleInterop true
```

### create `index.ts` file

```typescript
import fastify from 'fastify'


const server = fastify()

server.get('/healthz', async (request, reply) => {
    return 'OK'
})

server.listen({ port: 8080 }, (err, address) => {
    if (err) {
        console.error(err)
        process.exit(1)
    }
    console.log(`Server listening at ${address}`)
})
```

### run server

add below block in [pakcage.json](fastify/package.json) file

```json5
{
  // ...
  
  "scripts": {
    "dev": "ts-node ./src/index.ts",
    "build": "tsc",
    "start": "node ./build/index.js"
  },
  
  // ...
}
```

#### run develop mode

```shell
$ yarn dev
```

#### run production mode

``shell
$ yarn build
$ yarn start
``

## A02. Create next.js client

### initialize environment

```shell
$ mkdir next.js && cd "$_"
$ yarn create next-app \
       --ts \
       --tailwind \
       --eslint \
       --no-src-dir \
       --import-alias "\~/*" \
       --app \
       .
```

### run server

#### run develop mode

```shell
$ yarn dev
```

#### run production mode

```shell
$ yarn build
$ yarn start
```

---

# B. Apply tRPC protocol

## B01. tRPC query

### fastify server

```shell
$ cd ${TUTORIAL_ROOT}/fastify
$ yarn add @trpc/server zod
```

* **fastify/router.ts**

```typescript
import { initTRPC } from '@trpc/server';
import { z } from 'zod';


export const t = initTRPC.create();

export const appRouter = t.router({
  hello: t.procedure.query(async (opts) => {
    await new Promise(r => setTimeout(r, 1000));
    return 'Hello, World';
  }),
});

// export type definition of API
export type AppRouter = typeof appRouter;
```

* **fastify/index.ts**

```typescript
import { fastifyTRPCPlugin } from '@trpc/server/adapters/fastify';
import fastify from 'fastify';
import { appRouter } from './router';


const server = fastify({
    maxParamLength: 4096,
});

server.get('/healthz', async (request, reply) => {
    return 'OK'
})

server.register(fastifyTRPCPlugin, {
    prefix: '/api/trpc',
    trpcOptions: { router: appRouter },
});

(async () => {
    try {
        await server.listen({ port: 8080 });
    } catch (err) {
        server.log.error(err);
        process.exit(1);
    }
})();
```

### next.js client

#### add packages

```shell
$ cd ${TUTORIAL_ROOT}/next.js
$ yarn add @trpc/server @trpc/client @trpc/react-query @trpc/next @tanstack/react-query
```

#### set proxy to fastify server

* **next.js/next.config.js**

```typescript
/** @type {import('next').NextConfig} */
const nextConfig = {
    async rewrites() {
        return [
            {
                source: "/api/trpc/:path*",
                destination: "http://localhost:8080/api/trpc/:path*",
            },
        ];
    },
}

module.exports = nextConfig
```

* **next.js/trpc/index.ts**

```typescript
import { httpBatchLink, wsLink, createWSClient } from '@trpc/client';
import { createTRPCNext } from "@trpc/next";
import type { AppRouter } from "../../fastify/src/router";


function getBaseUrl() {
    if (typeof window !== 'undefined')
        // browser should use relative path
        return '';
    if (process.env.VERCEL_URL)
        // reference for vercel.com
        return `https://${process.env.VERCEL_URL}`;
    if (process.env.RENDER_INTERNAL_HOSTNAME)
        // reference for render.com
        return `http://${process.env.RENDER_INTERNAL_HOSTNAME}:${process.env.PORT}`;
    // assume localhost
    return `http://localhost:${process.env.PORT ?? 8080}`;
}

const trpc = createTRPCNext<AppRouter>({
    config(opts) {
        const { ctx } = opts;
        if (typeof window !== 'undefined') {
            // during client requests
            return {
                links: [
                    httpBatchLink({
                        url: '/api/trpc',
                    }),
                ],
            };
        }
        return {
            links: [
                httpBatchLink({
                    // The server needs to know your app's full url
                    url: `${getBaseUrl()}/api/trpc`,
                    /**
                     * Set custom request headers on every request from tRPC
                     * @link https://trpc.io/docs/v10/header
                     */
                    headers() {
                        if (!ctx?.req?.headers) {
                            return {};
                        }
                        // To use SSR properly, you need to forward the client's headers to the server
                        // This is so you can pass through things like cookies when we're server-side rendering
                        const {
                            // If you're using Node 18 before 18.15.0, omit the "connection" header
                            connection: _connection,
                            ...headers
                        } = ctx.req.headers;
                        return headers;
                    },
                }),
            ],
        };
    },
    ssr: true,
});

export default trpc;
```

***next.js/app/page.tsx***

```tsx
'use client'

import Image from 'next/image'
import trpc from '~/trpc'


export default trpc.withTRPC(function Home() {
    let { data, isLoading, isFetching } = trpc.hello.useQuery();

    return (
        // ...
        
        <div>
            {isLoading ? 'loading...' : isFetching ? data : <b>{data}</b>}
        </div>
        
        // ...
    )
});
```

## B02. tRPC mutation

## B03. tRPC subscription
