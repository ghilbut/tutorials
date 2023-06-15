import { initTRPC } from '@trpc/server';
import { observable } from '@trpc/server/observable';
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
    }),
    onChanged: t.procedure.subscription(() => {
        return observable<{ name: string|null }>((emit) => {

            let dirty:string|null = name;

            const timer = setInterval(() => {
                if (name !== dirty) {
                    dirty = name;
                    emit.next({ name });
                }
            }, 1000);

            return () => {
                clearInterval(timer);
            };
        });
    }),
});

// export type definition of API
export type AppRouter = typeof appRouter;
