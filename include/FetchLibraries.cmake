include(FetchContent)

FetchContent_Declare(
        minhook
        GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
        GIT_TAG 565968b28583221751cc2810e09ea621745fc3a3
)

FetchContent_Declare(
        libhat
        GIT_REPOSITORY https://github.com/BasedInc/libhat.git
        GIT_TAG 41392997ebe2355d0463d28fcfdce63c22c7f641
)

FetchContent_Declare(
        entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG fe8d7d78c4823e8a66a050bf86f5c6318cf76ce7
)

FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
        GIT_TAG 691cc4b2a4bc08ef35528da2af47d434160409d9
)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG f1d748e5e3edfa4b1778edea003bac94781bc7b7
)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 2d4c4b4dd31fde06cfffad7915c2b3006402322f
)

FetchContent_MakeAvailable(
        minhook
        libhat
        entt
        magic_enum
        spdlog
        glm
)
