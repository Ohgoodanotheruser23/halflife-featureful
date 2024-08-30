
// Some quick project-wide macros to customize a build.
// There are still other important ones in other files such as PLAYER_ALWAYSHASLONGJUMP in dlls/player.h.

// The commonly referred to "CLIENT_WEAPONS" is defined in the visual studio projects as constants
// (cl_dlls/hl project, properties -> C/C++ -> Preprocessor -> Preprocessor definitions).
// Others such as "_DEBUG" and "CLIENT_DLL" are intuitive: _DEBUG for the "Debug" configuration for
// either project, "CLIENT_DLL" for cl_dlls looking at a file (useful for shared files that need to 
// do somthing different per client/serverside).

#define BUILD_INFO_TITLE "FUCKED-Up"
// shortened?
//#define BUILD_INFO_TITLE "Half-Life: AZ - Dev Build"

#define BUILD_INFO_DEBUG "DEBUG"
#define BUILD_INFO_RELEASE "RELEASE"
