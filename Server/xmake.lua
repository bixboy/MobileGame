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
-- add_requires("enet") -- Using native enet-csharp instead


-- ===============/
-- Cible Serveur
-- ===============/
target("MobileGameServer")
    set_kind("binary")
    
    add_files("src/*.cpp", "src/private/**.cpp", "src/private/network/enet.c")
    add_headerfiles("src/public/**.hpp", "src/public/**.h")
    
    add_includedirs("src", "src/public", "src/private", "proto/generated")
    
    add_packages("entt", "flatbuffers", "enet")

    add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN")

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