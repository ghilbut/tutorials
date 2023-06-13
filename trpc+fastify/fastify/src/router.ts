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
