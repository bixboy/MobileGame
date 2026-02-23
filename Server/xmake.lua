set_project("MMO_AuthoritativeServer")
set_version("0.1.0")

add_rules("mode.debug", "mode.release")
set_languages("c++20")
add_rules("plugin.vsxmake.autoupdate")


-- =============/
-- Dépendances
-- =============/
add_requires("entt")
add_requires("flatbuffers")
add_requires("sqlitecpp")
add_requires("libsodium")
add_requires("nlohmann_json")


target("MobileGameServer")
    set_kind("binary")
    
    add_files("src/*.cpp", "src/private/**.cpp", "vendor/enet-csharp/enet.c")
    add_headerfiles("src/public/**.hpp", "src/public/**.h")
    
    add_includedirs("src", "src/public", "src/private", "proto/generated", "vendor/enet-csharp")
    
    add_packages("entt", "flatbuffers", "sqlitecpp", "libsodium", "nlohmann_json")

    add_defines("NOMINMAX")

    if is_plat("windows") then
        add_cxflags("/wd5287", {force = true})
    end

    -- ====================/
    -- Environnement Spec
    -- ====================/
    if is_mode("release") then
        set_optimize("fastest")
        set_strip("all")
        add_defines("NDEBUG")
        
    elseif is_mode("debug") then
        set_symbols("debug")
        add_defines("DEBUG_MODE")
    end

    after_build(function (target)
        os.cp("kingdoms.json", target:targetdir())
    end)