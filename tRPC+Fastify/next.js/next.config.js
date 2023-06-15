/** @type {import('next').NextConfig} */
const nextConfig = {
    async rewrites() {
        return [
            {
                source: "/api/trpc/:path*",
                destination: "http://localhost:8080/api/trpc/:path*",
            },
        ];
    },
}

module.exports = nextConfig
