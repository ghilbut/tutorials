import { initTRPC } from '@trpc/server';


const t = initTRPC.create();

export const middleware = t.middleware;
export const procedure = t.procedure;
export const router = t.router;
