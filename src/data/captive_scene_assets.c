#include "captive_scene_assets.h"
#include <stdlib.h>

const char *const captive_view_source_hashes[CAPTIVE_VIEW_SOURCE_COUNT] = {
    "47ad15b4a593c37880d0306b6a0f51b7a9f20615cf6a188f23716d5b48315524",
    "43833e4a8df622f84d53698a76c6d18f910c1cca79c6b89cbfacc563f695356c",
    "8b7301fc6c302fd673a81d23e7a99d715aa02d5b404c1e1edea19ceccccc9681",
    "519d3ef4494f0e868479a90c8a47249b840598e382c7ba3272f417ce3daf5936",
    "7edb8ee856a91e835ea86dda00af49fda3dae730d694bd7234b7fa96d711e296",
    "4a2bc840d184ff07657f56e630b0293f1d5d7cdbf1d00e4505a1a69dcf721667",
    "2c8db6bfbec2b463856ab4cd9a313f9fbf20be408a2e278d94d498653562f754",
    "0b0d6ee225493c92b534b50e893d9c27e423ce0a4298e1789682b8cf222b7adc",
    "1f1b89e7692dc7c01f9d649677c820e79076304e8bc79835683e14484d68bb5b",
    "fed16e510697e17123d474c08687de548076b26a55f08f1d00fd17e3fcdf9410",
    "63ffa6901b59d463b050088065503d386ca2f3813ed91d8e0833320f9df2fe11",
    "48df42e6906bfd167981f19e89149aa4c5791297b6e92f3a87470b59e8d0f1f3",
    "303c540f9e88ca9a8e736541b3c6f9a9cb9817b8640b5133ca7721e6db667e1d",
    "4edc60eb7d530ed6a7b11673d26831eb6701f131df3f9e291d882f5b78c2de25",
    "978d18857d5ffcf6fb7b91fb22c02b85079db0171caeac3d290a69b276cf098f",
    "dec7143f063c98459ab2f267ed135204cdee1b521eda9810b219e8c10e05c7e8",
    "d7338db4df839f0b1090234f6b3e30db1ab43c936be5479d007f865a0175cc32",
    "21db7daf64cff3b0cae19c3e7eb2057762df9110055e7253175024ecb146fb6b",
    /* Remaining original surfaces may provide UI overlays, animated work
       buffers or exterior/interior state variants.  Keep them available to
       the compositor rather than guessing their role from archive names. */
    "dfca77f0e219962242226f11f9697f580f92e8ad24786296a5b2571b20c2b707",
    "ce00ba2bc78f160b934486fe101a90264163356e02e7acbea2a41cf5d125b017",
    "4f05bad2a2b5d7474b0dc3735bbec4b5650110b5e79d6902c5536a556f8c27f5",
    "6eb09e17fdd6b97bdef223b3ebab5c94f12010c8cba615536fd40bf1509299c0",
    "70e0b9bfbaa5dfd12643b50cbe10d0b664de2fb1106d8ff0f2fde1ce6f443bbe",
};

bool captive_scene_assets_available(const DataVFS *vfs) {
    if (!vfs || !vfs->initialized) return false;
    for (int i = 0; i < CAPTIVE_VIEW_SOURCE_COUNT; ++i) {
        size_t size = 0;
        uint8_t *data = vfs_find_sha256(vfs, captive_view_source_hashes[i], &size);
        if (!data) return false;
        free(data);
    }
    return true;
}
