
#ifndef GAME_EXPORT_H
#define GAME_EXPORT_H

#ifdef GAME_STATIC_DEFINE
#  define GAME_EXPORT
#  define GAME_NO_EXPORT
#else
#  ifndef GAME_EXPORT
#    ifdef game_EXPORTS
        /* We are building this library */
#      define GAME_EXPORT 
#    else
        /* We are using this library */
#      define GAME_EXPORT 
#    endif
#  endif

#  ifndef GAME_NO_EXPORT
#    define GAME_NO_EXPORT 
#  endif
#endif

#ifndef GAME_DEPRECATED
#  define GAME_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef GAME_DEPRECATED_EXPORT
#  define GAME_DEPRECATED_EXPORT GAME_EXPORT GAME_DEPRECATED
#endif

#ifndef GAME_DEPRECATED_NO_EXPORT
#  define GAME_DEPRECATED_NO_EXPORT GAME_NO_EXPORT GAME_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef GAME_NO_DEPRECATED
#    define GAME_NO_DEPRECATED
#  endif
#endif

#endif /* GAME_EXPORT_H */
