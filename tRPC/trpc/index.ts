import { httpBatchLink } from '@trpc/client';
import { createTRPCNext } from "@trpc/next";
import type { AppRouter } from "./router";


function getBaseUrl() {
    if (typeof window !== 'undefined')
        // browser should use relative path
        return '';
    if (process.env.VERCEL_URL)
        // reference for vercel.com
        return `https://${process.env.VERCEL_URL}`;
    if (process.env.RENDER_INTERNAL_HOSTNAME)
        // reference for render.com
        return `http://${process.env.RENDER_INTERNAL_HOSTNAME}:${process.env.PORT}`;
    // assume localhost
    return `http://localhost:${process.env.PORT ?? 3000}`;
}

const trpc = createTRPCNext<AppRouter>({
    config(opts) {
        const { ctx } = opts;
        if (typeof window !== 'undefined') {
            // during client requests
            return {
                links: [
                    httpBatchLink({
                        url: '/api/trpc',
                    }),
                ],
            };
        }
        return {
            links: [
                httpBatchLink({
                    // The server needs to know your app's full url
                    url: `${getBaseUrl()}/api/trpc`,
                    /**
                     * Set custom request headers on every request from tRPC
                     * @link https://trpc.io/docs/v10/header
                     */
                    headers() {
                        if (!ctx?.req?.headers) {
                            return {};
                        }
                        // To use SSR properly, you need to forward the client's headers to the server
                        // This is so you can pass through things like cookies when we're server-side rendering
                        const {
                            // If you're using Node 18 before 18.15.0, omit the "connection" header
                            connection: _connection,
                            ...headers
                        } = ctx.req.headers;
                        return headers;
                    },
                }),
            ],
        };
    },
    ssr: true,
});

export default trpc;
