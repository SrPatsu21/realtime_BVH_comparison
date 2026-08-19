#pragma once

#include <cstdint>

namespace Config
{
    enum class RenderMode : uint32_t
    {
        Forward = 0,
        GeometryGBuffer = 1
    };


    enum RenderBits : uint64_t
    {
        // Geometry / Deferred
        UseGbuffer = 1ull << 0,

        // Optional geometry features
        UseGeometryShader = 1ull << 1
    };


    enum LightingBits : uint64_t
    {
        Shadows = 1ull << 0,
        SSAO    = 1ull << 1,
        RayTracing = 1ull << 2
    };


    enum PostBits : uint64_t
    {
        Bloom   = 1ull << 0,
        Tonemap = 1ull << 1,
        TAA     = 1ull << 2
    };


    struct RenderConfig
    {
        RenderMode mode = RenderMode::Forward;

        uint64_t flags = 0;

        bool has(RenderBits bit) const
        {
            return (flags & uint64_t(bit)) != 0;
        }
    };


    struct LightingConfig
    {
        uint64_t flags = 0;

        bool has(LightingBits bit) const
        {
            return (flags & uint64_t(bit)) != 0;
        }
    };


    struct PostConfig
    {
        uint64_t flags = 0;

        bool has(PostBits bit) const
        {
            return (flags & uint64_t(bit)) != 0;
        }
    };


    class ConfigTable
    {
    public:

        RenderConfig render;
        LightingConfig lighting;
        PostConfig post;

    public:

        static constexpr uint64_t Bit(RenderBits bit)
        {
            return uint64_t(bit);
        }

        static constexpr uint64_t Bit(LightingBits bit)
        {
            return uint64_t(bit);
        }

        static constexpr uint64_t Bit(PostBits bit)
        {
            return uint64_t(bit);
        }

        void normalize()
        {
            switch (render.mode)
            {
                case RenderMode::Forward:
                {
                    render.flags &= ~Bit(RenderBits::UseGbuffer);
                    break;
                }

                case RenderMode::GeometryGBuffer:
                {
                    render.flags |= Bit(RenderBits::UseGbuffer);
                    break;
                }
            }
        }
    };
}