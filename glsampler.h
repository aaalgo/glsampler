#pragma once

namespace glsampler {

    class Sampler {
    public:
        static constexpr int CUBE_SIZE = 64;
        static constexpr int VOLUME_SIZE = 512;
        static constexpr int VIEW_SIZE = 512;
        static constexpr int VIEW_PIXELS = VIEW_SIZE * VIEW_SIZE;
        Sampler () {}
        virtual ~Sampler () {}
        virtual void load (uint8_t const *) = 0;
        virtual void sample (float x, float y, float z, float phi, float theta, float kappa, float scale, uint8_t *) = 0;
        static Sampler *get (bool gl=true);
        static void cleanup ();
    };

}
