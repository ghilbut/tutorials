# tRPC tutorial

* [Home](https://trpc.io/)
* [Next.js Integration](https://trpc.io/docs/client/nextjs)

## Initialize project

### create Next.js application

```shell
$ yarn create next-app \
       --ts \
       --tailwind \
       --eslint \
       --no-src-dir \
       --import-alias "\~/*" \
       --app \
       trpc
$ cd trpc
```

### add default packages

**[NOTE]** [Set up with Next.js](https://trpc.io/docs/client/nextjs/setup)

```shell
$ yarn add \
       @trpc/server \
       @trpc/client \
       @trpc/react-query \
       @trpc/next \
       @tanstack/react-query \
       zod
```
