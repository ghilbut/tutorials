import { TRPCError } from '@trpc/server';
import { observable } from '@trpc/server/observable';
import { middleware, procedure, router } from './trpc';
import { z } from 'zod';


interface User{
    name: string
    role: string
}

let user: User | null = null;

export const appRouter = router({
    hello: procedure.query(async () => {
        await new Promise(r => setTimeout(r, 1000));
        return 'Hello, World';
    }),

    login: procedure.input(z.object({
        username: z.string(),
        password: z.string(),
    })).mutation(async (opts) => {
        const { username, password } = opts.input;

        if (username != 'ghilbut' || password != 'ghilbutpw') {
            throw new TRPCError({
                code: 'BAD_REQUEST',
                message: 'invalid username or password',
            });
        }

        user = {
            name: username,
            role: 'admin',
        };
        return user;
    }),

    logout: procedure.mutation(async (opts) => {
        user = null;
    }),

    onTimer: procedure.subscription(() => {
        return observable<string>((emit) => {
            return () => {
            };
        });
    }),
});

export type AppRouter = typeof appRouter;
