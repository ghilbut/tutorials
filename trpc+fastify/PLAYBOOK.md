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

**run develop mode**

```shell
$ yarn dev
```

**run production mode**

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

**run develop mode**

```shell
$ yarn dev
```

**run production mode**

```shell
$ yarn build
$ yarn start
```

---
