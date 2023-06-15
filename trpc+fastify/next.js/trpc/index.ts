import { httpBatchLink } from '@trpc/client';
import { createTRPCNext } from "@trpc/next";
import type { AppRouter } from "../../fastify/src/router";


const trpc = createTRPCNext<AppRouter>({
    config(opts) {
        const { ctx } = opts;
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
