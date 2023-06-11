# tRPC + Next.js 13 tutorial

* [Home](https://trpc.io/)
* [Next.js Integration](https://trpc.io/docs/client/nextjs)

## A. Initialize project

### A01. create Next.js application

```shell
$ cd ${TUTORIALS_ROOT}
$ yarn create next-app \
       --ts \
       --tailwind \
       --eslint \
       --no-src-dir \
       --import-alias "\~/*" \
       --app \
       trpc
$ mv trpc tRPC
```

### A02. add default packages

**[NOTE]** [Set up with Next.js](https://trpc.io/docs/client/nextjs/setup)

```shell
$ cd ${TUTORIALS_ROOT}/tRPC
$ yarn add \
       @trpc/server \
       @trpc/client \
       @trpc/react-query \
       @trpc/next \
       @tanstack/react-query \
       zod
```

## B. Apply tRPC

**[NOTE]** [Setup tRPC Server and Client in Next.js 13 App Directory](https://codevoweb.com/setup-trpc-server-and-client-in-nextjs-13-app-directory/)

### B01. define tRPC callback handler

***trcp/trpc.ts***

```typescript
import { initTRPC } from '@trpc/server';


const t = initTRPC.create();

export const middleware = t.middleware;
export const procedure = t.procedure;
export const router = t.router;
```

***trpc/router.ts***

```typescript
import { middleware, procedure, router } from './trpc'


export const appRouter = router({
    hello: procedure.query(async ({ctx}) => {
        await new Promise(r => setTimeout(r, 1000));
        return "Hello, World";
    }),
});

export type AppRouter = typeof appRouter;
```

### B02. define server side HTTP handler

***app/api/trpc/[trpc]/route.ts***

```typescript
'use server';

import {
    FetchCreateContextFnOptions,
    fetchRequestHandler,
} from "@trpc/server/adapters/fetch";
import { appRouter } from "~/trpc/router";


const handler = (request: Request) => {
    console.log(`incoming request ${request.url}`);
    return fetchRequestHandler({
        endpoint: "/api/trpc",
        req: request,
        router: appRouter,
        createContext: function (
            opts: FetchCreateContextFnOptions
        ): object | Promise<object> {
            return {};
        },
    });
};

export { handler as GET, handler as POST };
```

### B03. query for page rendering

***trcp/index.ts***

```typescript
import { httpBatchLink } from '@trpc/client';
import { createTRPCNext } from "@trpc/next";
import type { AppRouter } from "./router";


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
    return `http://localhost:${process.env.PORT ?? 3000}`;
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

***app/page.tsx***

```tsx
'use client';

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
