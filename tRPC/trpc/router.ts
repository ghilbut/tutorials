import { middleware, procedure, router } from './trpc'


export const appRouter = router({
    hello: procedure.query(async ({ctx}) => {
        await new Promise(r => setTimeout(r, 1000));
        return "Hello, World";
    }),
});

export type AppRouter = typeof appRouter;
