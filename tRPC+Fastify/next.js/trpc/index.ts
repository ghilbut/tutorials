import { createWSClient, httpBatchLink, splitLink, wsLink } from '@trpc/client';
import { createTRPCNext } from "@trpc/next";
import type { AppRouter } from "../../fastify/src/router";


const endpoint = 'localhost:8080/api/trpc';

const trpc = createTRPCNext<AppRouter>({
    config(opts) {
        const { ctx } = opts;
        return {
            links: [
                splitLink({
                    condition: (op) => {
                        return op.type === 'subscription';
                    },
                    true: wsLink({
                        client: createWSClient({
                            url: `ws://${endpoint}`,
                        })
                    }),
                    false: httpBatchLink({
                        url: `http://${endpoint}`,
                    }),
                }),
            ],
        };
    },
    ssr: true,
});

export default trpc;
