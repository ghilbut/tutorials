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

### Initialize environment

```shell
$ cd ${TUTORIAL_ROOT}
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

### Create `index.ts` file

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

### Run server

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

#### run development mode

```shell
$ yarn dev
```

#### run production mode

```shell
$ yarn build
$ yarn start
````

## A02. Create next.js client

### Initialize environment

```shell
$ cd ${TUTORIAL_ROOT}
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

### Run server

#### run development mode

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

### Fastify server

```shell
$ cd ${TUTORIAL_ROOT}/fastify
$ yarn add @trpc/server zod
```

* **fastify/router.ts**

```typescript
import { initTRPC } from '@trpc/server';


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
import fastify from 'fastify';
import cors from '@fastify/cors';
import { fastifyTRPCPlugin } from '@trpc/server/adapters/fastify';
import { appRouter } from './router';


const server = fastify({
    maxParamLength: 4096,
});

server.register(cors);
server.register(fastifyTRPCPlugin, {
  prefix: '/api/trpc',
  trpcOptions: { router: appRouter },
});

server.get('/healthz', async (request, reply) => {
    return 'OK'
})

(async () => {
    try {
        await server.listen({ port: 8080 });
        console.log('listening on port', 8080);
    } catch (err) {
        server.log.error(err);
        process.exit(1);
    }
})();
```

### Next.js client

#### add packages

```shell
$ cd ${TUTORIAL_ROOT}/next.js
$ yarn add @trpc/server @trpc/client @trpc/react-query @trpc/next @tanstack/react-query
```

#### create trpc client

* **next.js/trpc/index.ts**

```typescript
import { httpBatchLink } from '@trpc/client';
import { createTRPCNext } from "@trpc/next";
import type { AppRouter } from "../../fastify/src/router";


const trpc = createTRPCNext<AppRouter>({
    config(opts) {
        return {
            links: [
                httpBatchLink({
                    url: `http://localhost:8080/api/trpc`,
                }),
            ],
        };
    },
    ssr: true,
});

export default trpc;
```

* **next.js/app/page.tsx**

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

### Fastify server

* **src/router.ts**

```typescript
import { initTRPC } from '@trpc/server';
import { z } from 'zod';


export const t = initTRPC.create();

let name: string|null = null;

export const appRouter = t.router({
    hello: t.procedure.query(async (opts) => {
        await new Promise(r => setTimeout(r, 1000));
        return `Hello, ${name || 'World'}`;
    }),
    setName: t.procedure.input(z.string()).mutation(async ({ input }) => {
        name = input;
    }),
    resetName: t.procedure.mutation(async () => {
        name = null;
    })
});

// export type definition of API
export type AppRouter = typeof appRouter;
```

### Next.js client

* **app/page.tsx**

```tsx
'use client'

import Image from 'next/image'
import trpc from '~/trpc'


export default trpc.withTRPC(function Home() {
  let { data, isLoading, isFetching } = trpc.hello.useQuery();

  const setName = trpc.setName.useMutation();
  const resetName = trpc.resetName.useMutation();

  async function onClickSetName() {
    await setName.mutate('ghilbut');
  }

  async function onClickResetName() {
    await resetName.mutate();
  }

  return (
          // ...

          <div>
            {isLoading ? 'loading...' : isFetching ? data : <b>{data}</b>}
          </div>

          // ...
  )
});
```

## B03. tRPC subscription

### Fastify server

```shell
$ yarn add ws @fastify/websocket
$ yarn add --dev @types/ws
```

### Next.js client
