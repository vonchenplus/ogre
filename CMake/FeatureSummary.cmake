#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# This file prints a summary of the selected build features.

set(_features "\n")
set(_features "${_features}----------------------------------------------------------------------------\n")
set(_features "${_features}  FEATURE SUMMARY\n")
set(_features "${_features}----------------------------------------------------------------------------\n\n")

#summarise components
if (OGRE_BUILD_COMPONENT_PAGING)
	set(_components "${_components}  + Paging\n")
endif ()
if (OGRE_BUILD_COMPONENT_MESHLODGENERATOR)
	set(_components "${_components}  + MeshLodGenerator\n")
endif ()
if (OGRE_BUILD_COMPONENT_PROPERTY)
	set(_components "${_components}  + Property\n")
endif ()
if (OGRE_BUILD_COMPONENT_TERRAIN)
	set(_components "${_components}  + Terrain\n")
endif ()
if (OGRE_BUILD_COMPONENT_RTSHADERSYSTEM)
	set(_components "${_components}  + RTShader System\n")

	if (OGRE_BUILD_RTSHADERSYSTEM_CORE_SHADERS)
		set(_components "${_components}  + RTShader System Core Shaders\n")
	endif ()

	if (OGRE_BUILD_RTSHADERSYSTEM_EXT_SHADERS)
		set(_components "${_components}  + RTShader System Extensions Shaders\n")
	endif ()

endif ()
if (OGRE_BUILD_COMPONENT_VOLUME)
	set(_components "${_components}  + Volume\n")
endif ()
if (OGRE_BUILD_COMPONENT_OVERLAY)
	set(_components "${_components}  + Overlay\n")
endif ()
if (OGRE_BUILD_COMPONENT_HLMS)
	set(_components "${_components}  + HLMS\n")
endif ()

if (DEFINED _components)
	set(_features "${_features}Building components:\n${_components}")
endif ()

# summarise plugins
if (OGRE_BUILD_PLUGIN_BSP)
	set(_plugins "${_plugins}  + BSP scene manager\n")
endif ()
if (OGRE_BUILD_PLUGIN_CG)
	set(_plugins "${_plugins}  + Cg program manager\n")
endif ()
if (OGRE_BUILD_PLUGIN_OCTREE)
	set(_plugins "${_plugins}  + Octree scene manager\n")
endif ()
if (OGRE_BUILD_PLUGIN_PCZ)
	set(_plugins "${_plugins}  + Portal connected zone scene manager\n")
endif ()
if (OGRE_BUILD_PLUGIN_PFX)
	set(_plugins "${_plugins}  + Particle FX\n")
endif ()

if (DEFINED _plugins)
	set(_features "${_features}Building plugins:\n${_plugins}")
endif ()

# summarise rendersystems
if (OGRE_BUILD_RENDERSYSTEM_D3D9)
	set(_rendersystems "${_rendersystems}  + Direct3D 9\n")
endif ()
if (OGRE_BUILD_RENDERSYSTEM_D3D11)
	set(_rendersystems "${_rendersystems}  + Direct3D 11\n")
endif ()
if (OGRE_BUILD_RENDERSYSTEM_GL)
	set(_rendersystems "${_rendersystems}  + OpenGL\n")
endif ()
if (OGRE_BUILD_RENDERSYSTEM_GL3PLUS)
	set(_rendersystems "${_rendersystems}  + OpenGL 3+ (EXPERIMENTAL)\n")
endif ()
if (OGRE_BUILD_RENDERSYSTEM_GLES)
	set(_rendersystems "${_rendersystems}  + OpenGL ES 1.x\n")
endif ()
if (OGRE_BUILD_RENDERSYSTEM_GLES2)
	set(_rendersystems "${_rendersystems}  + OpenGL ES 2.x\n")
endif ()

if (DEFINED _rendersystems)
	set(_features "${_features}Building rendersystems:\n${_rendersystems}")
endif ()

# summarise programs
if (OGRE_BUILD_SAMPLES)
	set(_programs "${_programs}  + Samples\n")
endif ()
if (OGRE_BUILD_TESTS)
	set(_programs "${_programs}  + Tests\n")
endif ()
if (OGRE_BUILD_TOOLS)
	set(_programs "${_programs}  + Tools\n")
endif ()

if (DEFINED _programs)
	set(_features "${_features}Building executables:\n${_programs}")
endif ()

# summarise core features
if (OGRE_CONFIG_ENABLE_MESHLOD)
	set(_core "${_core}  + Mesh Lod\n")
endif ()
if (OGRE_CONFIG_ENABLE_DDS)
	set(_core "${_core}  + DDS image codec\n")
endif ()
if (OGRE_CONFIG_ENABLE_PVRTC)
	set(_core "${_core}  + PVRTC image codec\n")
endif ()
if (OGRE_CONFIG_ENABLE_ETC)
	set(_core "${_core}  + ETC image codec\n")
endif ()
if (OGRE_CONFIG_ENABLE_FREEIMAGE)
	set(_core "${_core}  + FreeImage codec\n")
endif ()
if (OGRE_CONFIG_ENABLE_STBI)
	set(_core "${_core}  + STBI codec\n")
endif ()
if (OGRE_CONFIG_ENABLE_ZIP)
	set(_core "${_core}  + ZIP archives\n")
endif ()
if (OGRE_CONFIG_ENABLE_VIEWPORT_ORIENTATIONMODE)
	set(_core "${_core}  + Viewport orientation mode support\n")
endif ()
if (OGRE_CONFIG_ENABLE_GLES2_GLSL_OPTIMISER)
	set(_core "${_core}  + GLSL Optimiser for OpenGL ES 2.0\n")
endif ()
if (OGRE_CONFIG_ENABLE_GLES2_CG_SUPPORT)
	set(_core "${_core}  + Cg for OpenGL ES 2.0\n")
endif ()
if (OGRE_CONFIG_ENABLE_GLES2_VAO_SUPPORT)
	set(_core "${_core}  + VertexArrayObjects for OpenGL ES 2.0\n")
endif ()
if (OGRE_CONFIG_ENABLE_GL_STATE_CACHE_SUPPORT)
	set(_core "${_core}  + StateCacheManager for OpenGL\n")
endif ()
if (OGRE_CONFIG_ENABLE_GLES3_SUPPORT)
	set(_core "${_core}  + OpenGL ES 3.0 Support (EXPERIMENTAL)\n")
endif ()
if (OGRE_CONFIG_ENABLE_QUAD_BUFFER_STEREO)
	set(_core "${_core}  + Quad Buffer Stereo Technology (EXPERIMENTAL)\n")
endif ()
if (DEFINED _core)
	set(_features "${_features}Building core features:\n${_core}")
endif ()


set(_features "${_features}\n")


# miscellaneous
macro(var_to_string VAR STR)
	if (${VAR})
		set(${STR} "enabled")
	else ()
		set(${STR} "disabled")
	endif ()
endmacro ()

# allocator settings
if (OGRE_CONFIG_ALLOCATOR EQUAL 1)
	set(_allocator "standard")
elseif (OGRE_CONFIG_ALLOCATOR EQUAL 2)
	set(_allocator "nedmalloc")
elseif (OGRE_CONFIG_ALLOCATOR EQUAL 3)
	set(_allocator "user")
else ()
    set(_allocator "nedmalloc (pooling)")
endif()
# assert settings
if (OGRE_ASSERT_MODE EQUAL 0)
	set(_assert "standard")
elseif (OGRE_ASSERT_MODE EQUAL 1)
	set(_assert "release exceptions")
else ()
    set(_assert "exceptions")
endif()
# various true/false settings
var_to_string(OGRE_CONFIG_CONTAINERS_USE_CUSTOM_ALLOCATOR _containers)
var_to_string(OGRE_CONFIG_DOUBLE _double)
var_to_string(OGRE_CONFIG_NODE_INHERIT_TRANSFORM _inherit_transform)
var_to_string(OGRE_CONFIG_MEMTRACK_DEBUG _memtrack_debug)
var_to_string(OGRE_CONFIG_MEMTRACK_RELEASE _memtrack_release)
var_to_string(OGRE_CONFIG_STRING_USE_CUSTOM_ALLOCATOR _string)
var_to_string(OGRE_USE_BOOST _boost)
# threading settings
if (OGRE_CONFIG_THREADS EQUAL 0)
	set(_threads "none")
elseif (OGRE_CONFIG_THREADS EQUAL 1)
	set(_threads "full (${OGRE_CONFIG_THREAD_PROVIDER})")
else ()
	set(_threads "background (${OGRE_CONFIG_THREAD_PROVIDER})")
endif ()
# build type
if (OGRE_STATIC)
	set(_buildtype "static")
else ()
	set(_buildtype "dynamic")
endif ()

set(_features "${_features}Build type:                      ${_buildtype}\n")
set(_features "${_features}Threading support:               ${_threads}\n")
set(_features "${_features}Use double precision:            ${_double}\n")
set(_features "${_features}Nodes inherit transform:         ${_inherit_transform}\n")
set(_features "${_features}Assert mode:                     ${_assert}\n")
set(_features "${_features}Allocator type:                  ${_allocator}\n")
set(_features "${_features}STL containers use allocator:    ${_containers}\n")
set(_features "${_features}Strings use allocator:           ${_string}\n")
set(_features "${_features}Memory tracker (debug):          ${_memtrack_debug}\n")
set(_features "${_features}Memory tracker (release):        ${_memtrack_release}\n")
set(_features "${_features}Use Boost:                       ${_boost}\n")


set(_features "${_features}\n----------------------------------------------------------------------------\n")
message(STATUS ${_features})
